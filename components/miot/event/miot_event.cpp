#include "esphome/core/log.h"
#include "miot_event.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.event";

void MiotEvent::setup() {
  this->parent_->register_event_listener(this->siid_, this->eiid_, [this]() {
    ESP_LOGV(TAG, "MCU reported event %" PRIu32 ":%" PRIu32 , this->siid_, this->eiid_);
    this->trigger(this->type_);
  });
}

void MiotEvent::dump_config() {
  LOG_EVENT("", "MIoT Event", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  EIID: %" PRIu32, this->eiid_);
  ESP_LOGCONFIG(TAG, "  TYPE: %s", this->type_);
}

}  // namespace miot
}  // namespace esphome