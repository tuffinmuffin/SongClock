#include "ClockHand.h"
#include "millis64.h"


ClockHand::ClockHand(int a1, int a2, int a3, int a4, int sensor, float sensorLoc, float sensorTolerance) {
        pins_[0] = a1;
        pins_[1] = a2;
        pins_[2] = a3,
        pins_[3] = a4;
        calibrated_ = false;
        sensor_ = sensor;
        sensorLoc_ = sensorLoc;
        sensorTol_ = sensorTolerance;
        dir_ = false;
        uStepIdx_ = 0;
        nextStepMs_ = 0;
        stepPos_ = -1;
        targetStepPos_ = 0;
    }

bool ClockHand::nextStepTime() {
  return millis64() >= nextStepMs_;
}


bool ClockHand::periodic() {
    if(idle()) {
        return false;
    }

    //if partial step, finish partial step
    if(stepping_) {
        uStep();
        return true;
    }

    //if calibrating check for calibration

    //if not calibrating check if we are out of calibration
    //Temp
    static bool sensor_det = false;
    if(digitalRead(sensor_)) {
      if(!sensor_det)
      {
        sensor_det = true;
        Serial.printf("Sensor Tripped %d\n", stepPos_);
      }
    } else {
      if(sensor_det) {
        Serial.printf("Sensor Lost %d\n", stepPos_);
        sensor_det = false;
      }
    }

    //start a micro step
    if(stepPos_ != targetStepPos_) {
      Serial.printf("Starting step at %d %d\n", stepPos_, millis());
      stepping_ = true;
      //schedule next
      nextStepMs_ = millis64() + stepDelayMs_;
    }
    return true;
}

void ClockHand::uStep() {
    if(!stepping_) {
      return;
    }

    if(!nextStepTime()) {
      return;
    }

    if(dir_) {
        uStepIdx_++;
        if(uStepIdx_ == 6) {
            stepping_ = false;
            stepPos_ = ++stepPos_ % stepPRot;
        }
    }
    else {
        uStepIdx_--;
        if(uStepIdx_ < 0) {
            stepping_ = false;
            stepPos_ = --stepPos_ % stepPRot;
        }
    }

    //write new index
    //ideally this would be a port write
    digitalWrite(pins_[0], (steps[uStepIdx_ % 6] >> 0) & 1);
    digitalWrite(pins_[1], (steps[uStepIdx_ % 6] >> 1) & 1);
    digitalWrite(pins_[2], (steps[uStepIdx_ % 6] >> 2) & 1);
    digitalWrite(pins_[3], (steps[uStepIdx_ % 6] >> 3) & 1);

    nextStepMs_ = millis64() + stepDelayMs_;
}

void ClockHand::setHandMinute(int minute) {
    //map between 0-59
    targetStepPos_ = stepPRot / 60 * (minute % 60);
}
void ClockHand::setHandHour(int minute, int hour) {
    targetStepPos_ = stepPRot % (hour % 60) + (stepPRot/12) % (minute % 60) ;
}