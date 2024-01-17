#include "esphome/core/log.h"
#include "miot_binary_sensor.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.binary_sensor";

void MiotBinarySensor::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtBool, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported sensor %" PRIu32 ":%" PRIu32 " is: %s", this->siid_, this->piid_, ONOFF(value.as_bool));
    this->publish_state(value.as_bool);
  });
}

void MiotBinarySensor::dump_config() {
  LOG_BINARY_SENSOR("", "MIoT Binary Sensor", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
}

}  // namespace miot
}  // namespace esphome
