#include "sensorSubscriber.hpp"

#include <cstdlib>
#include <iostream>

auto main() -> int {
  sdv::SensorSubscriber subscriber;
  if (!subscriber.init()) {
    return EXIT_FAILURE;
  }

  subscriber.start();
  subscriber.stop();
  return EXIT_SUCCESS;
}
