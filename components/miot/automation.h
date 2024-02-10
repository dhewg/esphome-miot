#pragma once

#include "miot.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace miot {

template<typename... Ts> class MiotAction : public Action<Ts...> {
 public:
  explicit MiotAction(Miot *miot, uint32_t siid, uint32_t aiid, const std::string &args)
  : miot_(miot),
    siid_(siid),
    aiid_(aiid),
    args_(args)
  {}

  void play(Ts... x) override {
    this->miot_->execute_action(siid_, aiid_, args_);
  }

 protected:
  Miot *miot_;
  uint32_t siid_;
  uint32_t aiid_;
  std::string args_;
};

}  // namespace miot
}  // namespace esphome
