#include <cstring>
#include <vector>

#include "miot.h"
#include "esphome/components/network/util.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/string_ref.h"
#include "esphome/core/util.h"

#ifdef USE_CAPTIVE_PORTAL
#include "esphome/components/captive_portal/captive_portal.h"
#endif

#ifdef USE_OTA
#include "esphome/components/ota/ota_component.h"
#endif

/*
 * TODO
 * wifi state callback and notify mcu
 * add "access" to components? read/write/notify
 * expose actions
 * automations?
 * reboot mcu on ota reboot
 */

namespace esphome {
namespace miot {

static const char *const TAG = "miot";
static const int RECEIVE_TIMEOUT = 300;

void Miot::setup() {
  command_queue_.push("down MIIO_mcu_version_req");
  command_queue_.push("down MIIO_net_change offline");

  this->set_interval("poll", 60000, [this] {
    std::string cmd;
    cmd.reserve(MAX_LINE_LENGTH);
    cmd = "down get_properties";
    for (auto it = listeners_.cbegin(); it != listeners_.cend(); ++it)
      if ((*it).second.poll)
        cmd += str_snprintf(" %" PRIu32 " %" PRIu32, 32, (*it).first.first, (*it).first.second);
    command_queue_.push(cmd);
  });

  if (heartbeat_siid_ != 0 && heartbeat_piid_ != 0)
    this->set_interval("heartbeat", 60000, [this] {
      set_property(heartbeat_siid_, heartbeat_piid_, MiotValue(60));
    });

#ifdef USE_OTA
  ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float progress, uint8_t error) {
    // directly send this to indicate a firmware update, as loop() won't get called anymore
    if (state == ota::OTA_STARTED)
      send_reply_("down MIIO_net_change updating");
    else
      command_queue_.push(std::string("down MIIO_net_change ") + get_net_reply_());
  });
#endif
}

void Miot::loop() {
  uint8_t c;

  if (rx_count_ > 0 && millis() - last_rx_char_timestamp_ > RECEIVE_TIMEOUT) {
    rx_message_[rx_count_] = 0;
    ESP_LOGE(TAG, "Timeout while receiving from MCU, dropping message '%s'", rx_message_);
    rx_count_ = 0;
  }

  while (this->available()) {
    this->read_byte(&c);

    if (c == '\r') {
      rx_message_[rx_count_] = 0;
      ESP_LOGV(TAG, "Received MCU message '%s'", rx_message_);
      process_message_(rx_message_);
      rx_count_ = 0;
      continue;
    }

    if (rx_count_ >= MAX_LINE_LENGTH) {
      rx_message_[rx_count_] = 0;
      ESP_LOGE(TAG, "MCU message too long, dropping message '%s'", rx_message_);
      rx_count_ = 0;
    }

    rx_message_[rx_count_++] = c;
    last_rx_char_timestamp_ = millis();
  }
}

void Miot::dump_config() {
  ESP_LOGCONFIG(TAG, "MIoT:");
  if (!model_.empty())
    ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  if (!mcu_version_.empty())
    ESP_LOGCONFIG(TAG, "  MCU Version: %s", mcu_version_.c_str());
}

void Miot::register_listener(uint32_t siid, uint32_t piid, bool poll, MiotValueType type, const std::function<void(const MiotValue &value)> &func) {
  if (listeners_.find(std::make_pair(siid, piid)) != listeners_.end()) {
    ESP_LOGW(TAG, "Property already has a listener: %" PRIu32 " %" PRIu32, siid, piid);
    return;
  }
  this->listeners_[std::make_pair(siid, piid)] = MiotListener{ .poll = poll, .type = type, .func = func };
}

void Miot::set_property(uint32_t siid, uint32_t piid, const MiotValue &value) {
  std::string cmd;

  switch (value.type) {
  case mvtBool:
    cmd = str_snprintf("down set_properties %" PRIu32 " %" PRIu32 " %s", MAX_LINE_LENGTH, siid, piid, value.as_bool ? "true" : "false");
    break;
  case mvtInt:
    cmd = str_snprintf("down set_properties %" PRIu32 " %" PRIu32 " %d", MAX_LINE_LENGTH, siid, piid, value.as_int);
    break;
  case mvtUInt:
    cmd = str_snprintf("down set_properties %" PRIu32 " %" PRIu32 " %" PRIu32, MAX_LINE_LENGTH, siid, piid, value.as_uint);
    break;
  case mvtFloat:
    cmd = str_snprintf("down set_properties %" PRIu32 " %" PRIu32 " %f", MAX_LINE_LENGTH, siid, piid, value.as_float);
    break;
  case mvtString:
    cmd = str_snprintf("down set_properties %" PRIu32 " %" PRIu32 " %s", MAX_LINE_LENGTH, siid, piid, value.as_string.c_str());
    break;
  }

  command_queue_.push(cmd);
}

void Miot::send_reply_(const char *reply) {
  ESP_LOGV(TAG, "Sending reply '%s' to MCU", reply);
  this->write_str(reply);
  this->write_byte('\r');
  this->flush();
}

void Miot::update_property(uint32_t siid, uint32_t piid, const char *value) {
  if (heartbeat_siid_ != 0 && heartbeat_piid_ != 0 && siid == heartbeat_siid_ && piid == heartbeat_piid_)
    return;

  auto it = listeners_.find(std::make_pair(siid, piid));
  if (it == listeners_.end()) {
    ESP_LOGW(TAG, "Received property value without component: %" PRIu32 " %" PRIu32 " %s", siid, piid, value);
    return;
  }

  switch ((*it).second.type) {
  case mvtBool:
    (*it).second.func(MiotValue(!std::strcmp(value, "true")));
    break;
  case mvtInt:
    (*it).second.func(MiotValue(parse_number<int>(value).value_or(0)));
    break;
  case mvtUInt:
    (*it).second.func(MiotValue(parse_number<uint32_t>(value).value_or(0u)));
    break;
  case mvtFloat:
    (*it).second.func(MiotValue(parse_number<float>(value).value_or(0.0f)));
    break;
  case mvtString:
    (*it).second.func(MiotValue(value));
    break;
  }
}

void Miot::update_properties(char **saveptr, bool with_code) {
  const char *siid, *piid, *code = nullptr, *value;
  const char *delim;

  while (true) {
    delim = " ";

    if (!(siid = strtok_r(nullptr, delim, saveptr)))
      break;
    if (!(piid = strtok_r(nullptr, delim, saveptr)))
      break;
    if (with_code) {
      if (!(code = strtok_r(nullptr, delim, saveptr)))
        break;
      if (std::strcmp(code, "0")) {
        ESP_LOGE(TAG, "Result error on property %s:%s: %s", siid, piid, code);
        continue;
      }
    }
    // peek if the next token starts with a " to properly parse string results containing spaces
    if (*saveptr && **saveptr == '"')
      delim = "\"";
    if (!(value = strtok_r(nullptr, delim, saveptr)))
      break;

    update_property(parse_number<uint32_t>(siid).value_or(0u), parse_number<uint32_t>(piid).value_or(0u), value);
  }
}

const char *Miot::get_net_reply_() {
  if (remote_is_connected())
    return "cloud";
  if (network::is_connected())
    return "lan";
#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active())
    return "uap";
#endif
  if (network::is_disabled())
    return "unprov";
  return "offline";
}

void Miot::process_message_(char *msg) {
  char *saveptr = nullptr;
  const StringRef cmd(strtok_r(msg, " ", &saveptr));

  if (cmd == "get_down") {
    if (command_queue_.empty()) {
      send_reply_("down none");
    } else {
      send_reply_(command_queue_.front().c_str());
      command_queue_.pop();
    }
  } else if (cmd == "properties_changed") {
    update_properties(&saveptr, false);
    send_reply_("ok");
  } else if (cmd == "result") {
    update_properties(&saveptr, true);
    send_reply_("ok");
  } else if (cmd == "net") {
    send_reply_(get_net_reply_());
  } else if (cmd == "time") {
    send_reply_("0"); // TODO
  } else if (cmd == "model") {
    model_ = strtok_r(nullptr, " ", &saveptr);
    if (!model_.empty())
      ESP_LOGI(TAG, "Model: %s", model_.c_str());
    send_reply_("ok");
  } else if (cmd == "mcu_version") {
    mcu_version_ = strtok_r(nullptr, " ", &saveptr);
    if (!mcu_version_.empty())
      ESP_LOGI(TAG, "MCU Version: %s", mcu_version_.c_str());
    send_reply_("ok");
  } else if (cmd == "error") {
    const char *error = strtok_r(nullptr, "\"", &saveptr);
    const char *code = nullptr;
    if (error)
      code = strtok_r(nullptr, " ", &saveptr);
    ESP_LOGE(TAG, "MCU command error %s: %s", code, error);
    send_reply_("ok");
  } else {
    ESP_LOGW(TAG, "Unknown command '%s'", cmd.c_str());
    send_reply_("ok");
  }
}

}  // namespace miot
}  // namespace esphome
