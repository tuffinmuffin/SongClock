#pragma once
#include <Arduino.h>


class ClockHand {
  public:
    ClockHand(int a1, int a2, int a3, int a4, int sensor, float sensorLoc, float sensorTolerance);

    //returns false if no work to do
    //returns true if still working
    bool periodic();
    void setHandMinute(int minute);
    void setHandHour(int minute, int hour);
    void setStepDelay(int delayMs) { stepDelayMs_ = delayMs;}
    void calibrate() {calibrating_ = true;}
    bool isCalibrated() {return calibrated_;}
    bool idle() {return stepPos_ == targetStepPos_ && !calibrating_ && !stepping_;}
    bool nextStepTime();
    void setDirection(bool counterClockWise);

    private:
    void uStep();
    bool calibrated_;
    bool calibrating_;
    int pins_[4];
    int sensor_;
    float sensorLoc_;
    float sensorTol_;
    bool dir_ = true;
    int uStepIdx_ = 0;
    int stepPos_ = -1;
    int targetStepPos_ = 0;
    int stepDelayMs_ = 10;
    const uint32_t stepPRot = 720;
    uint64_t nextStepMs_ = 0;
    bool stepping_ = false;

    constexpr static uint8_t steps[6] = { 0x9, 0x1, 0x7, 0x6, 0xE, 0x8 };

};