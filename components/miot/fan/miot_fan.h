#pragma once

#include "esphome/core/component.h"
#include "esphome/components/miot/miot.h"
#include "esphome/components/fan/fan.h"

namespace esphome {
namespace miot {

class MiotFan : public Component, public fan::Fan {
 public:
  MiotFan(Miot *parent) : parent_(parent) {}
  void setup() override;
  void dump_config() override;

  void set_state_config(uint32_t siid, uint32_t piid) {
    this->state_siid_ = siid;
    this->state_piid_ = piid;
  }

  void set_speed_config(uint32_t siid, uint32_t piid,
                        uint32_t min, uint32_t max, uint32_t step) {
    this->speed_siid_ = siid;
    this->speed_piid_ = piid;
    this->speed_min_ = min;
    this->speed_max_ = max;
    this->speed_step_ = step;
  }

  void set_oscillating_config(uint32_t siid, uint32_t piid) {
    this->oscillating_siid_ = siid;
    this->oscillating_piid_ = piid;
  }

  void set_direction_config(uint32_t siid, uint32_t piid) {
    this->direction_siid_ = siid;
    this->direction_piid_ = piid;
  }

  void set_preset_modes_config(uint32_t siid, uint32_t piid) {
    this->preset_modes_siid_ = siid;
    this->preset_modes_piid_ = piid;
  }

  // Called at YAML parsing stage
  void set_preset_mode_name(uint8_t key, const char *value) {
    if (value == nullptr || strlen(value) == 0) {
      manual_speed_preset_ = key;
    } else {
      this->preset_modes_[key] = std::string(value);
    }
  }

  fan::FanTraits get_traits() override;

 protected:
  void control(const fan::FanCall &call) override;

  Miot *parent_;

  uint32_t state_siid_{0};
  uint32_t state_piid_{0};

  uint32_t speed_siid_{0};
  uint32_t speed_piid_{0};
  uint32_t speed_min_{0}, speed_max_{0}, speed_step_{0};

  uint32_t oscillating_siid_{0};
  uint32_t oscillating_piid_{0};

  uint32_t direction_siid_{0};
  uint32_t direction_piid_{0};

  uint32_t preset_modes_siid_{0};
  uint32_t preset_modes_piid_{0};

  // key -> preset name (string, not char*)
  std::map<uint8_t, std::string> preset_modes_;

  // which key is "manual speed"
  optional<uint8_t> manual_speed_preset_;
};

}  // namespace miot
}  // namespace esphome
