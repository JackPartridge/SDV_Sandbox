#pragma once

#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

namespace sdv {

inline auto logLine(std::string_view line) -> void {
  static std::mutex mutex;
  std::scoped_lock lock{mutex};
  std::cout << line << '\n';
}

}  // namespace sdv
