#pragma once

#include "canFrame.hpp"

#include <cstdint>
#include <vector>

namespace sdv {

class CanBusSimulator {
public:
  explicit CanBusSimulator(std::vector<std::uint64_t> scriptedPayloads = {});

  [[nodiscard]] auto nextFrame() -> CanFrame;

private:
  std::vector<std::uint64_t> payloads_;
  std::size_t index_{0};
};

}  // namespace sdv
