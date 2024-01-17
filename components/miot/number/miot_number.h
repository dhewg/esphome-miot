#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"
#include "../miot.h"

namespace esphome {
namespace miot {

class MiotNumber : public number::Number, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_miot_config(Miot *parent, uint32_t siid, uint32_t piid, bool poll) {
    this->parent_ = parent;
    this->siid_ = siid;
    this->piid_ = piid;
    this->poll_ = poll;
  }

 protected:
  void control(float value) override;

  Miot *parent_;
  uint32_t siid_{0};
  uint32_t piid_{0};
  bool poll_{true};
};

}  // namespace miot
}  // namespace esphome
