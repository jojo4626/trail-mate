#pragma once

class SensorBHI260AP;

// Shared motion sensor board capability contract.
class MotionBoard
{
  public:
    virtual ~MotionBoard() = default;

    virtual SensorBHI260AP& getMotionSensor() = 0;
    virtual bool isSensorReady() const = 0;
};
