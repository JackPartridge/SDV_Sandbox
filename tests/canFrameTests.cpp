#include <gtest/gtest.h>

#include "canFrame.hpp"
#include "vehicleSignalFrame.hpp"

namespace {

TEST(CanFrameDecoder, DecodesPracticeTaskSamplePayload) {
  const sdv::CanFrame frame{0x4D013F0014000000ULL};
  EXPECT_EQ(frame.getEngineRpm(), 0x4D01);
  EXPECT_EQ(frame.getVehicleSpeed(), 0x3F);
  EXPECT_EQ(frame.getSteeringAngle(), 0x0014);
}

TEST(VehicleSignalFrame, RoundTripsDecodedCanSample) {
  const sdv::CanFrame can{0x0BB83C0014000000ULL};
  const auto signal = sdv::makeVehicleSignalFrame(3, can, 1'700'000'000'111ULL);
  const auto bytes = sdv::serialiseVehicleSignalFrame(signal);
  ASSERT_EQ(bytes.size(), sdv::vehicleSignalFrameByteSize);

  const auto restored = sdv::processVehicleSignalPayload(bytes);
  ASSERT_TRUE(restored.has_value());
  EXPECT_EQ(*restored, signal);
  EXPECT_EQ(restored->engineRpm, 3000);
  EXPECT_EQ(restored->vehicleSpeed, 60);
  EXPECT_EQ(restored->steeringAngle, 20);
}

TEST(VehicleSignalFrame, RejectsImplausibleRpm) {
  sdv::VehicleSignalFrame frame{
      .sequenceNumber = 1,
      .engineRpm = 25'000,
      .vehicleSpeed = 10,
      .steeringAngle = 5,
      .sourceCanPayload = 1,
      .timestampMs = 2,
  };
  const auto bytes = sdv::serialiseVehicleSignalFrame(frame);
  EXPECT_TRUE(sdv::deserialiseVehicleSignalFrame(bytes).has_value());
  EXPECT_FALSE(sdv::processVehicleSignalPayload(bytes).has_value());
}

}  // namespace
