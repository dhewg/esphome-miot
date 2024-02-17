#include <cstring>
#include <vector>

#include "miot.h"
#include "esphome/components/network/util.h"
#include "esphome/core/application.h"
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
 * add "access" to components? read/write/notify
 * automations?
 * reboot mcu on ota reboot
 */

namespace esphome {
namespace miot {

static const char *const TAG = "miot";
static const int RECEIVE_TIMEOUT = 300;
static const char *const NET_OFFLINE = "offline";
static const char *const NET_UNPROV = "unprov";
static const char *const NET_UAP = "uap";
static const char *const NET_LAN = "lan";
static const char *const NET_CLOUD = "cloud";
static const char *const NET_UPDATING = "updating";

void Miot::setup() {
  queue_command("MIIO_mcu_version_req");
  queue_net_change_command(true);

  this->set_interval("poll", 60000, [this] {
    std::string cmd;
    cmd.reserve(MAX_LINE_LENGTH);
    cmd = "get_properties";
    for (auto it = listeners_.cbegin(); it != listeners_.cend(); ++it)
      if ((*it).second.poll)
        cmd += str_snprintf(" %" PRIu32 " %" PRIu32, 32, (*it).first.first, (*it).first.second);
    queue_command(cmd);
  });

  if (heartbeat_siid_ != 0 && heartbeat_piid_ != 0)
    this->set_interval("heartbeat", 60000, [this] {
      set_property(heartbeat_siid_, heartbeat_piid_, MiotValue(60));
    });

#ifdef USE_OTA
  ota::global_ota_component->add_on_state_callback([this](ota::OTAState state, float progress, uint8_t error) {
    switch (state) {
    case ota::OTA_STARTED:
      // directly send this to indicate a firmware update, as loop() won't get called anymore
      send_reply_("down MIIO_net_change updating");
      break;
    case ota::OTA_ERROR:
      queue_net_change_command(true);
      break;
    default:
      break;
    }
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
    if (!this->read_byte(&c))
      continue;

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

    // replace unprintable chars, which is a violation of the spec
    // seen: 0xff for missing serial numbers
    if (c < 32 || c > 127)
      c = '?';

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
#ifdef USE_TIME
    ESP_LOGCONFIG(TAG, "  Time: AVAILABLE");
#else 
    ESP_LOGCONFIG(TAG, "  Time: UNAVAILABLE");
#endif
}

void Miot::register_listener(uint32_t siid, uint32_t piid, bool poll, MiotValueType type, const std::function<void(const MiotValue &value)> &func) {
  if (listeners_.find(std::make_pair(siid, piid)) != listeners_.end()) {
    ESP_LOGE(TAG, "Property already has a listener: %" PRIu32 " %" PRIu32, siid, piid);
    return;
  }
  this->listeners_[std::make_pair(siid, piid)] = MiotListener{ .poll = poll, .type = type, .func = func };
}

void Miot::queue_command(const std::string &cmd) {
  ESP_LOGD(TAG, "Queuing MCU command '%s'", cmd.c_str());
  command_queue_.push(cmd);
}

void Miot::queue_command(const char *fmt, ...) {
  std::string cmd;
  va_list args;

  cmd.resize(MAX_LINE_LENGTH);
  va_start(args, fmt);
  size_t len = vsnprintf(&cmd[0], MAX_LINE_LENGTH + 1, fmt, args);
  va_end(args);
  cmd.resize(len);
  queue_command(cmd);
}

void Miot::queue_net_change_command(bool force) {
  const char *reply = get_net_reply_();
  if (!force && reply == last_net_reply_)
    return;
  ESP_LOGI(TAG, "Network status changed to '%s'", reply);
  queue_command("MIIO_net_change %s", reply);
  last_net_reply_ = reply;
}

void Miot::set_property(uint32_t siid, uint32_t piid, const MiotValue &value) {
  switch (value.type) {
  case mvtInt:
    queue_command("set_properties %" PRIu32 " %" PRIu32 " %d", siid, piid, value.as_int);
    break;
  case mvtUInt:
    queue_command("set_properties %" PRIu32 " %" PRIu32 " %" PRIu32, siid, piid, value.as_uint);
    break;
  case mvtFloat:
    queue_command("set_properties %" PRIu32 " %" PRIu32 " %f", siid, piid, value.as_float);
    break;
  case mvtString:
    queue_command("set_properties %" PRIu32 " %" PRIu32 " %s", siid, piid, value.as_string.c_str());
    break;
  }
}

void Miot::execute_action(uint32_t siid, uint32_t aiid, const std::string &args) {
  if (args.empty())
    queue_command("action %" PRIu32 " %" PRIu32, siid, aiid);
  else
    queue_command("action %" PRIu32 " %" PRIu32 " %s", siid, aiid, args.c_str());
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

// mrfNotify: properties_changed <siid> <piid> <value> ... <siid> <piid> <value>
// mrfSet: result <siid> <piid> <code> ... <siid> <piid> <code>
// mrfAction: result <siid> <aiid> <code> <piid> <value> ... <piid> <value>
void Miot::update_properties(char **saveptr, MiotResultFormat format) {
  const char *siid = nullptr, *piid = nullptr, *aiid, *code = nullptr, *value;
  const char *delim = " ";

  if (format == mrfAction) {
    if (!(siid = strtok_r(nullptr, delim, saveptr)))
      return;
    if (!(aiid = strtok_r(nullptr, delim, saveptr)))
      return;
    if (!(code = strtok_r(nullptr, delim, saveptr)))
      return;
    if (std::strcmp(code, "0")) {
      ESP_LOGE(TAG, "Result error on action %s:%s: %s", siid, aiid, code);
      return;
    }
  }

  while (true) {
    delim = " ";

    if (format != mrfAction)
      if (!(siid = strtok_r(nullptr, delim, saveptr)))
        break;
    if (!(piid = strtok_r(nullptr, delim, saveptr)))
      break;
    if (format == mrfSet) {
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
    return NET_CLOUD;
  if (network::is_connected())
    return NET_LAN;
#ifdef USE_CAPTIVE_PORTAL
  if (captive_portal::global_captive_portal != nullptr && captive_portal::global_captive_portal->is_active())
    return NET_UAP;
#endif
  if (network::is_disabled())
    return NET_UNPROV;
  return NET_OFFLINE;
}

std::string Miot::get_time_reply_(bool posix) {
#ifdef USE_TIME
  if (!time_->now().is_valid()) {
    ESP_LOGW(TAG, "MCU time request: time source not ready yet");
    return "0";
  }

  if (posix) {
    std::string posix_time = time_->utcnow().strftime("%s");
    ESP_LOGD(TAG, "MCU time request: sending posix time \"%s\"", posix_time.c_str());
    return posix_time;
  } else {
    std::string formatted_time = time_->now().strftime("%Y-%m-%d %H:%M:%S");
    ESP_LOGD(TAG, "MCU time request: sending time \"%s\"", formatted_time.c_str());
    return formatted_time;
  }
#else
    ESP_LOGW(TAG, "MCU time request: no time source available");
    return "0";
#endif
}

void Miot::process_message_(char *msg) {
  char *saveptr = nullptr;
  const StringRef cmd(strtok_r(msg, " ", &saveptr));

  queue_net_change_command(false);

  if (cmd == "get_down") {
    if (command_queue_.empty()) {
      send_reply_("down none");
    } else {
      const std::string &next_cmd = command_queue_.front();
      expect_action_result_ = next_cmd.rfind("action ", 0, 7) == 0;
      send_reply_((std::string("down ") + next_cmd).c_str());
      command_queue_.pop();
    }
  } else if (cmd == "properties_changed") {
    update_properties(&saveptr, mrfNotify);
    send_reply_("ok");
  } else if (cmd == "result") {
    update_properties(&saveptr, expect_action_result_ ? mrfAction : mrfSet);
    send_reply_("ok");
  } else if (cmd == "net") {
    send_reply_(get_net_reply_());
  } else if (cmd == "time") {
    send_reply_(get_time_reply_(std::strcmp(strtok_r(nullptr, " ", &saveptr), "posix") == 0).c_str());
  } else if (cmd == "mac") {
    send_reply_(get_mac_address().c_str());
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
  } else if (cmd == "restore") {
    ESP_LOGI(TAG, "Resetting to factory defaults...");
    global_preferences->reset();
    App.safe_reboot();
  } else {
    ESP_LOGW(TAG, "Unknown command '%s'", cmd.c_str());
    send_reply_("ok");
  }
}

}  // namespace miot
}  // namespace esphome
