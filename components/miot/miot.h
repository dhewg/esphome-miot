#pragma once

#include <cinttypes>
#include <deque>
#include <map>
#include <queue>
#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#ifdef USE_OTA_STATE_LISTENER
#include "esphome/components/ota/ota_backend.h"
#endif

namespace esphome {
namespace miot {

enum MiotValueType {
  mvtInt,
  mvtUInt,
  mvtFloat,
  mvtString,
};

struct MiotValue {
  explicit MiotValue(int value) { type = mvtInt; as_int = value; };
  explicit MiotValue(uint32_t value) { type = mvtUInt; as_uint = value; };
  explicit MiotValue(float value) { type = mvtFloat; as_float = value; };
  explicit MiotValue(const char *value) { type = mvtString; as_string = value; };
  explicit MiotValue(const std::string &value) { type = mvtString; as_string = value; };

  MiotValueType type;
  union {
    int as_int;
    uint32_t as_uint;
    float as_float;
  };
  std::string as_string;
};

struct MiotListener {
  bool poll;
  MiotValueType type;
  std::function<void(const MiotValue &value)> func;
};

#ifdef USE_EVENT
struct MiotEventListener {
  std::function<void()> func;
};
#endif

enum MiotResultFormat {
  mrfNotify,
  mrfSet,
  mrfAction,
  mrfEvent,
};

class Miot : public Component,
#ifdef USE_OTA_STATE_LISTENER
             public ota::OTAGlobalStateListener,
#endif
             public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void loop() override;
  void dump_config() override;

  std::string get_printable_string(const uint8_t *data, size_t len);
  std::string get_printable_string(const std::string &str);

  void set_heartbeat_config(uint32_t siid, uint32_t piid) {
    this->heartbeat_siid_ = siid;
    this->heartbeat_piid_ = piid;
  };
  void set_ota_net_indicator(const std::string &indicator) {
    this->ota_net_indicator_ = indicator;
  };
  void register_listener(uint32_t siid, uint32_t piid, bool poll, MiotValueType type, const std::function<void(const MiotValue &value)> &func);
  void queue_command(const std::string &cmd);
  void queue_command(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
  void queue_net_change_command(bool force);
  void set_property(uint32_t siid, uint32_t piid, const MiotValue &value);
  void execute_action(uint32_t siid, uint32_t aiid, const std::string &args);
#ifdef USE_EVENT
  void register_event_listener(uint32_t siid, uint32_t eiid, const std::function<void()> &func);
#endif

 protected:
  const char *get_net_reply_();
  std::string get_time_reply_(bool posix);
  void send_reply_(const char *reply);
  void update_property(uint32_t siid, uint32_t piid, const char *value);
  void update_properties(char **saveptr, MiotResultFormat format);
  void mcu_log(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
  void process_message_();
  void process_event_(uint32_t siid, uint32_t eiid);

#ifdef USE_OTA_STATE_LISTENER
  void on_ota_global_state(ota::OTAState state, float progress, uint8_t error, ota::OTAComponent *comp) override;
#endif

  std::string get_printable_rx_message() {
    return get_printable_string(rx_message_, rx_count_);
  }

  static const size_t MAX_LINE_LENGTH = 512;

  uint8_t rx_message_[MAX_LINE_LENGTH + 1];
  size_t rx_count_{0};
  uint32_t last_rx_char_timestamp_{0};
  const char *last_net_reply_{nullptr};
  std::string model_;
  std::string mcu_version_;
  std::deque<std::string> mcu_log_;
  std::queue<std::string> command_queue_;
  bool expect_action_result_{false};
  uint32_t heartbeat_siid_{0};
  uint32_t heartbeat_piid_{0};
  std::string ota_net_indicator_;
  std::map<std::pair<uint32_t, uint32_t>, MiotListener> listeners_;
#ifdef USE_EVENT
  std::map<std::pair<uint32_t, uint32_t>, MiotEventListener> event_listeners_;
#endif

};

}  // namespace miot
}  // namespace esphome
