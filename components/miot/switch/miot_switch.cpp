#include "esphome/core/log.h"
#include "miot_switch.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.switch";

void MiotSwitch::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtBool, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported switch %" PRIu32 ":%" PRIu32 " is: %s", this->siid_, this->piid_, ONOFF(value.as_bool));
    this->publish_state(value.as_bool);
  });
}

void MiotSwitch::write_state(bool state) {
  ESP_LOGV(TAG, "Setting switch %" PRIu32 ":%" PRIu32 " %s", this->siid_, this->piid_, ONOFF(state));
  this->parent_->set_property(this->siid_, this->piid_, MiotValue(state));
  this->publish_state(state);
}

void MiotSwitch::dump_config() {
  LOG_SWITCH("", "MIoT Switch", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
}

}  // namespace miot
}  // namespace esphome
