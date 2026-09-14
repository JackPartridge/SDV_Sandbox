#include "sensorPublisher.hpp"

#include <cstdlib>
#include <iostream>

auto main() -> int {
  sdv::SensorPublisher publisher;
  if (!publisher.init()) {
    return EXIT_FAILURE;
  }

  publisher.start();
  publisher.stop();
  return EXIT_SUCCESS;
}
