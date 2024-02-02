#include "esphome/core/log.h"
#include "miot_binary_sensor.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.binary_sensor";

void MiotBinarySensor::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtString, [this](const MiotValue &value) {
    bool v = value.as_string == true_value_;
    ESP_LOGV(TAG, "MCU reported sensor %" PRIu32 ":%" PRIu32 " is: %s", this->siid_, this->piid_, ONOFF(v));
    this->publish_state(v);
  });
}

void MiotBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "MIoT Binary Sensor", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
}

}  // namespace miot
}  // namespace esphome
