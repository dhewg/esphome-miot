#pragma once

#include <cinttypes>
#include <map>
#include <queue>
#include <utility>

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

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

class Miot : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override { return setup_priority::DATA; }
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_heartbeat_config(uint32_t siid, uint32_t piid) {
    this->heartbeat_siid_ = siid;
    this->heartbeat_piid_ = piid;
  };
  void register_listener(uint32_t siid, uint32_t piid, bool poll, MiotValueType type, const std::function<void(const MiotValue &value)> &func);
  void queue_command(const std::string &cmd);
  void queue_net_change_command(bool force);
  void set_property(uint32_t siid, uint32_t piid, const MiotValue &value);
  void execute_action(uint32_t siid, uint32_t aiid, const std::string &args);

 protected:
  const char *get_net_reply_();
  void send_reply_(const char *reply);
  void update_property(uint32_t siid, uint32_t piid, const char *value);
  void update_properties(char **saveptr, bool with_code);
  void process_message_(char *msg);

  static const size_t MAX_LINE_LENGTH = 512;

  char rx_message_[MAX_LINE_LENGTH + 1];
  size_t rx_count_{0};
  uint32_t last_rx_char_timestamp_{0};
  const char *last_net_reply_{nullptr};
  std::string model_;
  std::string mcu_version_;
  std::queue<std::string> command_queue_;
  uint32_t heartbeat_siid_{0};
  uint32_t heartbeat_piid_{0};
  std::map<std::pair<uint32_t, uint32_t>, MiotListener> listeners_;
};

}  // namespace miot
}  // namespace esphome
