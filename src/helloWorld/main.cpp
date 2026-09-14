#include <iostream>
#include <string_view>

auto main() -> int {
  constexpr std::string_view message{"SDV Sandbox: Hello from the local vECU environment"};
  std::cout << message << '\n';
  return 0;
}
