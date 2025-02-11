#include "esphome/core/log.h"
#include "miot_fan.h"

namespace esphome {
namespace miot {

static const char *const TAG = "miot.fan";

// preset modes are implemented as described here: https://developers.home-assistant.io/docs/core/entity/fan/#preset-modes
// -> setting a speed percentage unsets the preset
// -> setting a preset sets the speed percentage to zero
// -> mcu speed values are not reported if the device is off or in preset mode

// all properties are currently always polled
// "state" and "oscillating" are currently true booleans ("true" and "false" on the wire)
// "direction" is a true boolean as well, where "true" means REVERSE
// "speed" values are 0=off [1:]=(max-min)/step
// an empty preset mode string declares the corresponding value as "manual mode", which is set before setting a manual speed

// TODO what about "restore_mode", untested, does it require anything here?

void MiotFan::setup() {
  this->parent_->register_listener(this->state_siid_, this->state_piid_, true, mvtString, [this](const MiotValue &value) {
    this->state = value.as_string == "true";
    ESP_LOGV(TAG, "MCU reported state %" PRIu32 ":%" PRIu32 " is: %s", this->state_siid_, this->state_piid_, ONOFF(this->state));
    this->publish_state();
  });

  this->parent_->register_listener(this->speed_siid_, this->speed_piid_, true, mvtUInt, [this](const MiotValue &value) {
    if (!this->state) {
      if (value.as_uint != 0)
        ESP_LOGW(TAG, "Ignoring MCU reported speed, fan is off");
      return;
    }
    if (!this->preset_mode.empty()) {
      ESP_LOGW(TAG, "Ignoring MCU reported speed, in preset mode");
      return;
    }
    this->speed = (value.as_uint - this->speed_min_) / this->speed_step_ + 1;
    ESP_LOGV(TAG, "MCU reported speed %" PRIu32 ":%" PRIu32 " is: %" PRIi32 " (raw: %" PRIu32 ")", this->speed_siid_, this->speed_piid_, this->speed, value.as_uint);
    this->publish_state();
  });

  if (this->oscillating_siid_ != 0 && this->oscillating_piid_ != 0)
    this->parent_->register_listener(this->oscillating_siid_, this->oscillating_piid_, true, mvtString, [this](const MiotValue &value) {
      this->oscillating = value.as_string == "true";
      ESP_LOGV(TAG, "MCU reported oscillating %" PRIu32 ":%" PRIu32 " is: %s", this->oscillating_siid_, this->oscillating_piid_, ONOFF(this->oscillating));
      this->publish_state();
    });

  if (this->direction_siid_ != 0 && this->direction_piid_ != 0)
    this->parent_->register_listener(this->direction_siid_, this->direction_piid_, true, mvtString, [this](const MiotValue &value) {
      this->direction = value.as_string == "true" ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
      ESP_LOGV(TAG, "MCU reported direction %" PRIu32 ":%" PRIu32 " is: %s", this->direction_siid_, this->direction_piid_, fan::fan_direction_to_string(this->direction));
      this->publish_state();
    });

  if (this->preset_modes_siid_ != 0 && this->preset_modes_piid_ != 0)
    this->parent_->register_listener(this->preset_modes_siid_, this->preset_modes_piid_, true, mvtUInt, [this](const MiotValue &value) {
      ESP_LOGV(TAG, "MCU reported preset mode %" PRIu32 ":%" PRIu32 " is: %" PRIu32, this->preset_modes_siid_, this->preset_modes_piid_, value.as_uint);
      if (this->manual_speed_preset_.has_value() && value.as_uint == *this->manual_speed_preset_) {
        this->preset_mode.clear();
      } else {
        auto it = preset_modes_.find(value.as_uint);
        if (it == preset_modes_.end()) {
          ESP_LOGE(TAG, "Unknown preset mode value %" PRIu32 "", value.as_uint);
          return;
        }
        this->preset_mode = it->second;
        this->speed = 0;
      }
      this->publish_state();
    });
}

void MiotFan::dump_config() {
  LOG_FAN("", "MIoT Fan", this);
  ESP_LOGCONFIG(TAG, "  State SIID: %" PRIu32, this->state_siid_);
  ESP_LOGCONFIG(TAG, "  State PIID: %" PRIu32, this->state_piid_);
  ESP_LOGCONFIG(TAG, "  Speed SIID: %" PRIu32, this->speed_siid_);
  ESP_LOGCONFIG(TAG, "  Speed PIID: %" PRIu32, this->speed_piid_);
  ESP_LOGCONFIG(TAG, "  Speed Min/Max/Step: %" PRIu32 "/%" PRIu32 "/%" PRIu32, this->speed_min_, this->speed_max_, this->speed_step_);
  if (this->oscillating_siid_ != 0 && this->oscillating_piid_ != 0) {
    ESP_LOGCONFIG(TAG, "  Oscillating SIID: %" PRIu32, this->oscillating_siid_);
    ESP_LOGCONFIG(TAG, "  Oscillating PIID: %" PRIu32, this->oscillating_piid_);
  }
  if (this->direction_siid_ != 0 && this->direction_piid_ != 0) {
    ESP_LOGCONFIG(TAG, "  Direction SIID: %" PRIu32, this->direction_siid_);
    ESP_LOGCONFIG(TAG, "  Direction PIID: %" PRIu32, this->direction_piid_);
  }
  if (this->preset_modes_siid_ != 0 && this->preset_modes_piid_ != 0) {
    ESP_LOGCONFIG(TAG, "  Preset Modes SIID: %" PRIu32, this->preset_modes_siid_);
    ESP_LOGCONFIG(TAG, "  Preset Modes PIID: %" PRIu32, this->preset_modes_piid_);
    ESP_LOGCONFIG(TAG, "  Preset Modes:");
    for (auto const &it : this->preset_modes_)
      ESP_LOGCONFIG(TAG, "    %u: %s", it.first, it.second.c_str());
    if (this->manual_speed_preset_.has_value())
      ESP_LOGCONFIG(TAG, "  Manual Preset Mode: %" PRIu8, *this->manual_speed_preset_);
  }
}

fan::FanTraits MiotFan::get_traits() {
  fan::FanTraits traits(this->oscillating_siid_ != 0 && this->oscillating_piid_ != 0,
                        this->speed_siid_ != 0 && this->speed_piid_ != 0,
                        this->direction_siid_ != 0 && this->direction_piid_ != 0,
                        (this->speed_max_ - this->speed_min_) / this->speed_step_ + 1);

  std::set<std::string> preset_modes;
  for (auto const &iter : preset_modes_)
    preset_modes.insert(iter.second);
  traits.set_supported_preset_modes(preset_modes);

  return traits;
}

void MiotFan::control(const fan::FanCall &call) {
  optional<uint8_t> mode;

  const std::string &mode_str = call.get_preset_mode();
  if (this->preset_modes_siid_ != 0 && this->preset_modes_piid_ != 0 && !mode_str.empty() && mode_str != this->preset_mode) {
    auto it = std::find_if(preset_modes_.cbegin(),
                           preset_modes_.cend(),
                           [mode_str](auto && pair) {
                              return pair.second == mode_str;
                           });
    if (it != preset_modes_.end())
      mode = it->first;
  }

  if (mode.has_value()) {
    this->speed = 0;
    this->parent_->set_property(this->preset_modes_siid_, this->preset_modes_piid_, MiotValue(*mode));
  } else if (call.get_speed().has_value()) {
    this->preset_mode.clear();
    if (this->manual_speed_preset_.has_value())
      this->parent_->set_property(this->preset_modes_siid_, this->preset_modes_piid_, MiotValue(*this->manual_speed_preset_));
    this->parent_->set_property(this->speed_siid_, this->speed_piid_, MiotValue(this->speed_min_ + *call.get_speed() * this->speed_step_ - 1));
  }
  if (this->oscillating_siid_ != 0 && this->oscillating_piid_ != 0 && call.get_oscillating().has_value())
    this->parent_->set_property(this->oscillating_siid_, this->oscillating_piid_, MiotValue(*call.get_oscillating() ? "true" : "false"));
  if (this->direction_siid_ != 0 && this->direction_piid_ != 0 && call.get_direction().has_value())
    this->parent_->set_property(this->direction_siid_, this->direction_piid_, MiotValue(*call.get_direction() == fan::FanDirection::REVERSE ? "true" : "false"));
  if (call.get_state().has_value())
    this->parent_->set_property(this->state_siid_, this->state_piid_, MiotValue(*call.get_state() ? "true" : "false"));
}

}  // namespace miot
}  // namespace esphome
