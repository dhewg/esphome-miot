#include "esphome/core/log.h"
#include "miot_sensor.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.sensor";

void MiotSensor::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtFloat, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported sensor %" PRIu32 ":%" PRIu32 " is: %f", this->siid_, this->piid_, value.as_float);
    this->publish_state(value.as_float);
  });
}

void MiotSensor::dump_config() {
  LOG_SENSOR("", "MIoT Sensor", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
}

}  // namespace miot
}  // namespace esphome
