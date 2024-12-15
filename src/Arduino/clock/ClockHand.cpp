#include "ClockHand.h"
#include "millis64.h"


ClockHand::ClockHand(int a1, int a2, int a3, int a4, int sensor, float sensorLocDeg, float sensorToleranceDeg) {
        pins_[0] = a1;
        pins_[1] = a2;
        pins_[2] = a3,
        pins_[3] = a4;
        calibrated_ = false;
        sensor_ = sensor;
        sensorLoc_ = (int) ((stepPRot / 360.0 ) * sensorLocDeg) % stepPRot;
        sensorTol_ = (int) ((stepPRot / 360.0 ) * sensorToleranceDeg) % stepPRot;
        dir_ = false; //false = CW
        uStepIdx_ = 0;
        nextStepMs_ = 0;
        stepPos_ = 0;
        targetStepPos_ = 0;
    }

bool ClockHand::nextStepTime() {
  return millis64() >= nextStepMs_;
}

void ClockHand::calibrate() {
  requestCal_ = false;
  calibrated_ = false;
  calibrating_ = true; 
  calibrationStarted_ = false;
  calibrationOnLoc_ = -1;
  calibrationOffLoc_ = -1;
  calibrationCenter_ = -1;
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
    if(calibrating_) {
      calibratePeriodic();
      return true;
    }

    //if not calibrating check if we are out of calibration
    //Temp
    /*static bool sensor_det = false;
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
    }*/

    //start a micro step
    if(stepPos_ != targetStepPos_) {
      //Serial.printf("Starting step at %d %d\n", stepPos_, millis());
      stepping_ = true;
      //schedule next
      nextStepMs_ = millis64() + stepDelayMs_;
    }

    if(searchingForLoc_) {
      bool senState = digitalRead(sensor_);
      if(senState) {
        if(stepPos_ < (calibrationOnLoc_ - sensorTol_)) {
          Serial.printf("Sensor Triggered Early at %d. Expected Range %d\n", stepPos_, (calibrationOnLoc_ - sensorTol_));
          requestCal();
        }
        else if (stepPos_ > (calibrationOnLoc_ + sensorTol_)) {
          Serial.printf("Sensor Triggered Late at %d. Expected Range %d\n", stepPos_, (calibrationOffLoc_ + sensorTol_));
          requestCal();
        } 
        else
        {
          Serial.printf("Sensor in range %d\n", sensor_);
          searchingForLoc_ = false;
        }
      }
    } else {
      if ( stepPos_ > (calibrationOffLoc_ + sensorTol_)) {
        if ( digitalRead(sensor_)) {
          Serial.printf("Sensor is on while past postion stalled?\n");
          requestCal();
        }
        else {
          searchingForLoc_ = true;
          //Serial.printf("Sensor search reset %d\n", stepPos_);
        }
      }
    }

    return true;
}

void ClockHand::calibratePeriodic() {

  //finish step
  if(stepping_) {
        uStep();
        return;
  }
  //before we start clear the sensor
  if(!calibrationStarted_) {
    if(digitalRead(sensor_)) {
      step();
      return;
    } else {
      calibrationStarted_ = true;
    }
  }

  //if start point not found
  //Serial.printf("Stat on %d off %d stp %04d sens %d\n", calibrationOnLoc_, calibrationOffLoc_, stepPos_, digitalRead(sensor_));
  if(calibrationOnLoc_ < 0) {
    //check if sensor tripped- if yes start point marked for start, mark start
    if(digitalRead(sensor_)) {
      //avoid rollover issues, 
      stepPos_ = stepPRot / 2;
      calibrationOnLoc_ = stepPos_;

      stepDelaySavedMs_ = stepDelayMs_;
      stepDelayMs_ = stepDelayMs_ * 4;
    }
    
  } 
  else if  (calibrationOnLoc_ > 0) {
    //else mark start && sensor not trippped
  // cal done, update values
    if(!digitalRead(sensor_)) {
      calibrationOffLoc_ = stepPos_;
      //does not support calibration across zero point currently
      int calCenter = (calibrationOffLoc_ - calibrationOnLoc_)/2;
      calibrationCenter_ = stepPos_ - (calibrationOffLoc_ - calibrationOnLoc_)/2;
      Serial.printf("Before: on %d, off %d, center %d, pos %d\n", calibrationOnLoc_, calibrationOffLoc_, calibrationCenter_, stepPos_);
      stepPos_ = (calibrationOffLoc_ - calibrationOnLoc_)/2 + sensorLoc_;
      stepPos_ = stepPos_ % stepPRot;
      calibrationCenter_ = sensorLoc_;
      calibrationOffLoc_ = calibrationCenter_ + calCenter;
      calibrationOnLoc_ = calibrationCenter_ - calCenter;
      Serial.printf("Sensor calibrated. Step Pos %d - on %d, off %d, target %d half %d\n", stepPos_, calibrationOnLoc_, calibrationOffLoc_, targetStepPos_, (calibrationOffLoc_ - calibrationOnLoc_)/2);
      stepDelayMs_ =  stepDelaySavedMs_;
      calibrated_ = true;
      calibrationStarted_ = false;
      calibrating_ = false;
      searchingForLoc_ = true;
      return;
    }

  }
  //else wait for next full step

  //keep searching
  // Do a full step
  step();
}

void ClockHand::step() {
  stepping_ = true;
}

void ClockHand::uStep() {
    if(!stepping_) {
      return;
    }

    if(!nextStepTime()) {
      return;
    }
    if(dir_) {
        uStepIdx_ = ++uStepIdx_;
        if(uStepIdx_ == 6) {
          uStepIdx_ = 0;
          stepping_ = false;
          stepPos_ = (--stepPos_) % stepPRot;
        }

        if(uStepIdx_ == 3) {
          stepping_ = false;
          stepPos_ = stepPos_ = (--stepPos_) % stepPRot;
        }
    }
    else {
        uStepIdx_ = --uStepIdx_;
        if(uStepIdx_ == 0xFF ) {
          uStepIdx_ = 5;
          stepping_ = false;
          stepPos_ = (++stepPos_) % stepPRot;
        }
        
        if(uStepIdx_ == 3) {
          stepping_ = false;
          stepPos_ = (++stepPos_) % stepPRot;
        }
    }

    //Serial.printf("Next step! dir %d, uSetpIdx_ %d, stepping %d, stepPost_ %d gl %d steps 0x%02x\n", dir_, uStepIdx_, stepping_, stepPos_, targetStepPos_, steps[uStepIdx_]);
    //write new index
    //ideally this would be a port write
    uint8_t state = steps[uStepIdx_];
    for(int i = 0; i < 4; i++) {
      digitalWrite(pins_[i], state & 1);
      state >>= 1;
    }
    
    nextStepMs_ = millis64() + stepDelayMs_;
}

void ClockHand::setHandMinute(int minute) {
    //map between 0-59
    targetStepPos_ = stepPRot / 60 * (minute % 60);
    Serial.printf("setHandMinute %d - goal %d curr %d\n", minute, targetStepPos_, stepPos_);
}
void ClockHand::setHandHour(int minute, int hour) {
    //Hour 0 = 0, 
    minute = minute % 60;
    hour = hour % 12;
    targetStepPos_ = ((hour * stepPRot/12) + (minute * stepPRot/(12 * 60))) % stepPRot ;
    Serial.printf("setHandHour %d:%d - goal %d curr %d\n", hour, minute, targetStepPos_, stepPos_);
}
