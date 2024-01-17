#pragma once

#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/select/select.h"
#include "../miot.h"

namespace esphome {
namespace miot {

class MiotSelect : public select::Select, public Component {
 public:
  void setup() override;
  void dump_config() override;

  void set_miot_config(Miot *parent, uint32_t siid, uint32_t piid, bool poll) {
    this->parent_ = parent;
    this->siid_ = siid;
    this->piid_ = piid;
    this->poll_ = poll;
  }
  void set_select_mappings(std::vector<uint8_t> mappings) { this->mappings_ = std::move(mappings); }

 protected:
  void control(const std::string &value) override;

  Miot *parent_;
  uint32_t siid_{0};
  uint32_t piid_{0};
  bool poll_{true};
  std::vector<uint8_t> mappings_;
};

}  // namespace miot
}  // namespace esphome
