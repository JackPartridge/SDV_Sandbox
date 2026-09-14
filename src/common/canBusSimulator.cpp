#include "canBusSimulator.hpp"

namespace sdv {

CanBusSimulator::CanBusSimulator(std::vector<std::uint64_t> scriptedPayloads)
    : payloads_{std::move(scriptedPayloads)} {
  if (payloads_.empty()) {
    payloads_ = {
        0x0BB83C0014000000ULL,
        0x0DACA8001E000000ULL,
        0x09C432000A000000ULL,
        0x0FA0500028000000ULL,
        0x07D02D0000000000ULL,
    };
  }
}

auto CanBusSimulator::nextFrame() -> CanFrame {
  const auto payload = payloads_[index_ % payloads_.size()];
  ++index_;
  return CanFrame{payload};
}

}  // namespace sdv
