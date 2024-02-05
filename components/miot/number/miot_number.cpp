#include "esphome/core/log.h"
#include "miot_number.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.number";

void MiotNumber::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtFloat, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported number %" PRIu32 ":%" PRIu32 " is: %f", this->siid_, this->piid_, value.as_float);
    this->publish_state(value.as_float);
  });
}

void MiotNumber::control(float value) {
  ESP_LOGV(TAG, "Setting number %" PRIu32 ":%" PRIu32 " %f", this->siid_, this->piid_, value);
  this->parent_->set_property(this->siid_, this->piid_, MiotValue(value_accuracy_to_string(value, step_to_accuracy_decimals(this->traits.get_step()))));
  this->publish_state(value);
}

void MiotNumber::dump_config() {
  LOG_NUMBER("", "MIoT Number", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32 "", this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32 "", this->piid_);
}

}  // namespace miot
}  // namespace esphome
