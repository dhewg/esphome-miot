#include "esphome/core/log.h"
#include "miot_button.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.button";

void MiotButton::dump_config() {
  LOG_BUTTON("", "MIoT Button", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  AIID: %" PRIu32, this->aiid_);
  ESP_LOGCONFIG(TAG, "  args: %s", this->action_args_.c_str());
}

void MiotButton::press_action() {
  this->parent_->execute_action(this->siid_, this->aiid_, this->action_args_);
}

}  // namespace miot
}  // namespace esphome
