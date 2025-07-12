#include "esphome/core/log.h"
#include "miot_text_sensor.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.text_sensor";

void MiotTextSensor::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtString, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported text sensor %" PRIu32 ":%" PRIu32 " is: %s", this->siid_, this->piid_, value.as_string.c_str());
    this->publish_state(this->parent_->get_printable_string(value.as_string));
  });
}

void MiotTextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "MIoT Text Sensor", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
}

}  // namespace miot
}  // namespace esphome
