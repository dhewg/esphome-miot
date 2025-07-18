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
#include "esphome/components/ota/ota_backend.h"
#endif

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
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
static const char *const NET_LOCAL = "local";
static const char *const NET_CLOUD = "cloud";
static const size_t MAX_COMMAND_LENGTH = 60;
static const size_t MAX_MCU_LOG_LENGTH = 32;

void Miot::setup() {
  // discard initial incoming serial buffer
  uint8_t c;
  while (this->available())
    if (this->read_byte(&c))
      if (rx_count_ < MAX_LINE_LENGTH)
        rx_message_[rx_count_++] = c;

  if (rx_count_ > 0) {
    mcu_log("Discarding initial MCU data: %s", get_printable_rx_message().c_str());
    rx_count_ = 0;
  }

  // these two commands have to be the first two in that order to get
  // the deerma.humidifier.jsq5 mcu into its main app loop
  queue_net_change_command(true);
  queue_command("none");

  queue_command("MIIO_mcu_version_req");

  this->set_interval("poll", 60000, [this] {
    std::string cmd, part;
    cmd.reserve(MAX_LINE_LENGTH);
    part.reserve(MAX_COMMAND_LENGTH);

    for (auto it = listeners_.cbegin(); it != listeners_.cend(); ++it) {
      if ((*it).second.poll) {
        part = str_snprintf(" %" PRIu32 " %" PRIu32, 32, (*it).first.first, (*it).first.second);

        if (cmd.size() + part.size() > MAX_COMMAND_LENGTH) {
          queue_command("get_properties" + cmd);
          cmd.clear();
        }

        cmd += part;
      }
    }

    if (!cmd.empty())
      queue_command("get_properties" + cmd);
  });

  if (heartbeat_siid_ != 0 && heartbeat_piid_ != 0)
    this->set_interval("heartbeat", 60000, [this] {
      set_property(heartbeat_siid_, heartbeat_piid_, MiotValue(60));
    });

#ifdef USE_OTA
  ota::get_global_ota_callback()->add_on_state_callback(
    [this](ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) {
      switch (state) {
      case ota::OTA_STARTED:
        // directly send this to indicate a firmware update, as loop() won't get called anymore
        send_reply_((std::string("down MIIO_net_change ") + ota_net_indicator_).c_str());
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
    ESP_LOGE(TAG, "Timeout while receiving from MCU, dropping message '%s'", get_printable_rx_message().c_str());
    rx_count_ = 0;
  }

  while (this->available()) {
    if (!this->read_byte(&c))
      break;

    if (c == '\r') {
      ESP_LOGV(TAG, "Received MCU message '%s'", get_printable_rx_message().c_str());
      process_message_();
      rx_count_ = 0;
      break;
    }

    if (rx_count_ >= MAX_LINE_LENGTH) {
      ESP_LOGE(TAG, "MCU message too long, dropping '%s'", get_printable_rx_message().c_str());
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
  ESP_LOGCONFIG(TAG, "  Heartbeat SIID: %" PRIu32, this->heartbeat_siid_);
  ESP_LOGCONFIG(TAG, "  Heartbeat PIID: %" PRIu32, this->heartbeat_piid_);
  ESP_LOGCONFIG(TAG, "  OTA net indicator: %s", this->ota_net_indicator_.c_str());
#ifdef USE_TIME
  ESP_LOGCONFIG(TAG, "  Time: AVAILABLE");
#else
  ESP_LOGCONFIG(TAG, "  Time: UNAVAILABLE");
#endif
  if (!mcu_log_.empty()) {
    ESP_LOGCONFIG(TAG, "  MCU Log:");
    for (auto it = mcu_log_.cbegin(); it != mcu_log_.cend(); ++it)
      ESP_LOGCONFIG(TAG, "    %s", (*it).c_str());
  }
}

std::string Miot::get_printable_string(const uint8_t *data, size_t len) {
  std::string s;
  s.reserve(len);

  for (size_t i = 0; i < len; ++i)
    if (std::isprint(data[i]))
      s += data[i];
    else
      s += str_snprintf("\\x%02X", 4, data[i]);
  return s;
}

std::string Miot::get_printable_string(const std::string &str) {
  return get_printable_string(reinterpret_cast<const uint8_t *>(str.c_str()), str.size());
}

void Miot::register_listener(uint32_t siid, uint32_t piid, bool poll, MiotValueType type, const std::function<void(const MiotValue &value)> &func) {
  if (listeners_.find(std::make_pair(siid, piid)) != listeners_.end()) {
    ESP_LOGE(TAG, "Property already has a listener: %" PRIu32 " %" PRIu32, siid, piid);
    return;
  }
  this->listeners_[std::make_pair(siid, piid)] = MiotListener{ .poll = poll, .type = type, .func = func };
}

#ifdef USE_EVENT
void Miot::register_event_listener(uint32_t siid, uint32_t eiid, const std::function<void()> &func) {
  if (event_listeners_.find(std::make_pair(siid, eiid)) != event_listeners_.end()) {
    ESP_LOGE(TAG, "Event already has a listener: %" PRIu32 " %" PRIu32, siid, eiid);
    return;
  }
  this->event_listeners_[std::make_pair(siid, eiid)] = MiotEventListener{ .func = func };
}
#endif

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

void Miot::process_event_(uint32_t siid, uint32_t eiid) {
#ifdef USE_EVENT
  auto it = event_listeners_.find(std::make_pair(siid, eiid));
  if (it == event_listeners_.end()) {
    ESP_LOGW(TAG, "Received event without component: %" PRIu32 " %" PRIu32, siid, eiid);
    return;
  }

  (*it).second.func();
#else
  ESP_LOGW(TAG, "Received event without component: %" PRIu32 " %" PRIu32, siid, eiid);
#endif
}

// mrfNotify: properties_changed <siid> <piid> <value> ... <siid> <piid> <value>
// mrfSet: result <siid> <piid> <code> ... <siid> <piid> <code>
// mrfAction: result <siid> <aiid> <code> <piid> <value> ... <piid> <value>
// mrfEvent: event_occurred <siid> <eiid> <piid> <value> ... <piid> <value>
void Miot::update_properties(char **saveptr, MiotResultFormat format) {
  const char *siid = nullptr, *piid = nullptr, *aiid, *eiid = nullptr, *code = nullptr, *value;
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

  if (format == mrfEvent) {
    if (!(siid = strtok_r(nullptr, delim, saveptr)))
      return;
    if (!(eiid = strtok_r(nullptr, delim, saveptr)))
      return;
  }

  while (true) {
    delim = " ";

    if (format != mrfAction && format != mrfEvent)
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

  if (format == mrfEvent) {
    // send event after all properties have been updated
    process_event_(parse_number<uint32_t>(siid).value_or(0u), parse_number<uint32_t>(eiid).value_or(0u));
  }
}

const char *Miot::get_net_reply_() {
  if (remote_is_connected())
    return NET_CLOUD;
  if (network::is_connected())
    return NET_LOCAL;
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
  auto now = ESPTime::from_epoch_local(::time(nullptr));

  if (!now.is_valid()) {
    ESP_LOGW(TAG, "MCU time request: time source not ready yet");
    return "0";
  }

  if (posix)
    return to_string((int64_t)now.timestamp);

  return now.strftime("%Y-%m-%d %H:%M:%S");
#else
  ESP_LOGW(TAG, "MCU time request: no time source available");
  return "0";
#endif
}

void Miot::mcu_log(const char *fmt, ...) {
  std::string log;
  va_list args;

  log.resize(MAX_LINE_LENGTH);
  va_start(args, fmt);
  size_t len = vsnprintf(&log[0], MAX_LINE_LENGTH + 1, fmt, args);
  va_end(args);
  log.resize(len);

  if (std::find(mcu_log_.cbegin(), mcu_log_.cend(), log) == mcu_log_.cend()) {
    if (mcu_log_.size() >= MAX_MCU_LOG_LENGTH)
      mcu_log_.pop_front();
    mcu_log_.push_back(log);
  }

  ESP_LOGW(TAG, "%s", log.c_str());
}

void Miot::process_message_() {
  rx_message_[rx_count_] = 0;
  char *saveptr = nullptr;
  const StringRef cmd(strtok_r(reinterpret_cast<char *>(rx_message_), " ", &saveptr));

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
  } else if (cmd == "event_occured") {
    update_properties(&saveptr, mrfEvent);
    send_reply_("ok");
  } else if (cmd == "result") {
    update_properties(&saveptr, expect_action_result_ ? mrfAction : mrfSet);
    send_reply_("ok");
  } else if (cmd == "net") {
    send_reply_(get_net_reply_());
  } else if (cmd == "time") {
    const char *arg = strtok_r(nullptr, " ", &saveptr);
    bool posix = arg && *arg && std::strcmp(arg, "posix") == 0;
    auto reply = get_time_reply_(posix);
    ESP_LOGD(TAG, "MCU time request: %s", reply.c_str());
    send_reply_(reply.c_str());
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
    ESP_LOGE(TAG, "MCU command error %s: %s", code ? code : "", error ? error : "");
    send_reply_("ok");
  } else if (cmd == "restore") {
    ESP_LOGI(TAG, "Resetting to factory defaults...");
    send_reply_("ok");
    global_preferences->reset();
    App.safe_reboot();
  } else if (cmd.size() < 3 || !std::islower(cmd[0]) || !std::islower(cmd[1]) || !std::islower(cmd[2])) {
    // required for deerma.humidifier.jsq5 and other devices with picky MCUs
    // that enforce MIoT spec compliance, and print e.g. debug info over UART.
    mcu_log("Invalid command '%s'", get_printable_rx_message().c_str());
    send_reply_("error");
  } else {
    mcu_log("Unknown command '%s'", get_printable_rx_message().c_str());
    send_reply_("ok");
  }
}

}  // namespace miot
}  // namespace esphome
