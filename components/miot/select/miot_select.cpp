#include "esphome/core/log.h"
#include "miot_select.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.select";

void MiotSelect::setup() {
  this->parent_->register_listener(this->siid_, this->piid_, this->poll_, mvtUInt, [this](const MiotValue &value) {
    ESP_LOGV(TAG, "MCU reported select %" PRIu32 ":%" PRIu32 " is: %" PRIu32, this->siid_, this->piid_, value.as_uint);

    auto options = this->traits.get_options();
    auto mappings = this->mappings_;
    auto it = std::find(mappings.cbegin(), mappings.cend(), value.as_uint);
    if (it == mappings.end()) {
      ESP_LOGW(TAG, "Invalid value %" PRIu32 "", value.as_uint);
      return;
    }

    size_t mapping_idx = std::distance(mappings.cbegin(), it);
    auto mapping_value = this->at(mapping_idx);
    this->publish_state(mapping_value.value());
  });
}

void MiotSelect::control(const std::string &value) {
  auto idx = this->index_of(value);
  if (!idx.has_value()) {
    ESP_LOGW(TAG, "Invalid value %s", value.c_str());
    return;
  }

  uint32_t mapping = this->mappings_.at(idx.value());
  ESP_LOGV(TAG, "Setting select %" PRIu32 ":%" PRIu32 " value to %" PRIu32 ":%s", this->siid_, this->piid_, mapping, value.c_str());
  this->parent_->set_property(this->siid_, this->piid_, MiotValue(mapping));
}

void MiotSelect::dump_config() {
  LOG_SELECT("", "MIoT Select", this);
  ESP_LOGCONFIG(TAG, "  SIID: %" PRIu32, this->siid_);
  ESP_LOGCONFIG(TAG, "  PIID: %" PRIu32, this->piid_);
  ESP_LOGCONFIG(TAG, "  Options are:");
  auto options = this->traits.get_options();
  for (auto i = 0; i < this->mappings_.size(); i++)
    ESP_LOGCONFIG(TAG, "    %i: %s", this->mappings_.at(i), options.at(i).c_str());
}

}  // namespace miot
}  // namespace esphome
