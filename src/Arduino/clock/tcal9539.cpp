#include "tcal9539.h"
#include <Wire.h>
#include "LowPower.h"

//global data structure
int8_t tcal9539_count = 0;
uint8_t tcal9539_addr_map[MAX_TCAL_DEVICES] = {TCAL_ADDR_LL};
struct tcal_pin tcal_data[MAX_TCAL_DEVICES][TCAL9539_CHANNELS] = {};

struct tcal_pin* getTcal9539Chan(uint32_t ulPin) {
    return &tcal_data[0][ulPin & 0xFF];
}

uint8_t getTcal9539Addr(uint32_t pin) {
    uint8_t addr = (uint8_t) (pin >> TCAL_ADDR_SHIFT) & 0xFF;
    for(int i = 0; i < tcal9539_count; i++) {
        if(tcal9539_addr_map[i] == addr) {
            return i;
        }
    }
    //not found return default. May be an issue
    return 0x0;
}

static intTriggered = false;
//TODO expand to support multiple interupts
void tcalInt() {
  //in case we get an interrupt while doing i2c transaction we don't operate here.
  //
  intTriggered = true;
}


bool initTcal9539(uint8_t addr, int32_t intPin) {
    if(tcal9539_count == MAX_TCAL_DEVICES) {
      Serial.printf("Too many TCAL9539. Failed for addr %d\n", addr);
      return false;
    }

    Wire.begin();
    tcal9539_addr_map[tcal9539_count] = addr;
    tcal9539_count++;


    //TODO register interrupt
    if(intPin > 0) {
      attachInterrupt(digitalPinToInterrupt(intPin), tcalInt, RISING);
      LowPower.attachInterruptWakeup()
    }
    
    return true;
}

uint8_t tcal9539_reg8Read(uint8_t device, uint8_t addr) {
    Wire.beginTransmission(device);
    Wire.write(addr);
    uint8_t data = Wire.read();
    Wire.endTransmission();
    return data;
}

void tcal9539_reg8Write(uint8_t device, uint8_t addr, uint8_t val) {
    Wire.beginTransmission(device);
    Wire.write(addr);
    Wire.write(val);
    Wire.endTransmission();
    return;
}

uint16_t tcal9539_reg16Read(uint8_t device, uint8_t addr) {
    Wire.beginTransmission(device);
    Wire.write(addr);
    uint16_t data = Wire.read();
    data |= (uint16_t)Wire.read() << 8;
    Wire.endTransmission();

    return data;
}

void tcal9539_reg16Write(uint8_t device, uint8_t addr, uint16_t val) {
    Wire.beginTransmission(device);
    Wire.write(addr);
    Wire.write((uint8_t) (val & 0xFF));
    Wire.write((uint8_t) (val >> 8));
    Wire.endTransmission();
    return;
}


uint8_t tcal9539_regRMW(uint8_t device, uint8_t addr, uint8_t val, uint8_t mask) {
    uint8_t data = tcal9539_reg8Read(device, addr);
    uint8_t data0 = data;
    data &= mask;
    data |= val & mask;
    tcal9539_reg8Write(device,addr,data);
    return data0;
}

void tcal9539_pinMode( uint32_t ulPin, uint32_t ulMode ) {
    struct tcal_pin* pin = getTcal9539Chan(ulPin);
    if(!pin) { 
      Serial.printf("Invalid TCAL %d\n", ulPin);
      return;
      }
    if(pin->i2cAddr == 0) {

      pin->i2cAddr = getTcal9539Addr(ulPin);
      //init structure for first call
      if(ulPin & 0xFF > 10) {
        pin->index = ulPin & 0x7;
        pin->rdAddr = TCAL9539_INPUT0;
        pin->wrAddr = TCAL9539_OUTPUT0;
        pin->dirAddr = TCAL9539_CONFIG0;
        pin->pullEnAddr = TCAL9539_PULLEN0;
        pin->pullDirAddr = TCAL9539_PULLDIR0;
      }
      else {
        pin->index = (ulPin - 10) & 0x7;
        pin->rdAddr = TCAL9539_INPUT1;
        pin->wrAddr = TCAL9539_OUTPUT1;
        pin->dirAddr = TCAL9539_CONFIG1;
        pin->pullEnAddr = TCAL9539_PULLEN1;
        pin->pullDirAddr = TCAL9539_PULLDIR1;           
      }
      pin->minAssertTimeUs = 0;
      pin->minAssertHeldUs = 500000;
    }

switch ( ulMode )
  {
    case INPUT:
      pin->input = true;
      pin->invertPol = false;
      pin->pullEn = false;
    break ;

    case INPUT_PULLUP:
      pin->input = true;
      pin->invertPol = false;
      pin->pullEn = true;
      pin->pullUp = true;    
    break ;

    case INPUT_PULLDOWN:
      pin->input = true;
      pin->invertPol = false;
      pin->pullEn = true;
      pin->pullUp = false;    
    break ;

    case OUTPUT:
      pin->input = false;
    break ;

    default:
      // do nothing
    break ;
  }

  uint8_t mask = 1 << pin->index;

  if(pin->input == false) {
    tcal9539_regRMW(pin->i2cAddr, pin->dirAddr, 0, mask);
    return;
  }

  tcal9539_regRMW(pin->i2cAddr, pin->dirAddr, mask, mask);
  tcal9539_regRMW(pin->i2cAddr, pin->pullDirAddr, pin->pullUp << pin->index, mask);
  tcal9539_regRMW(pin->i2cAddr, pin->pullEnAddr, pin->pullEn << pin->index, mask);

}

void tcal9539_digitalWrite( uint32_t ulPin, uint32_t ulVal ) {
    struct tcal_pin* pin = getTcal9539Chan(ulPin);
    if(!pin || !pin->i2cAddr) {
        Serial.printf("TCAL write failed %d\n", ulPin);
        return;
    }
    int mask = 1 << pin->index;
    if(ulVal) {
        tcal9539_regRMW(pin->i2cAddr, pin->wrAddr, mask, mask);
    }
    else {
        tcal9539_regRMW(pin->i2cAddr, pin->wrAddr, 0, mask);
    }
 }


int tcal9539_digitalRead( uint32_t ulPin) {
    struct tcal_pin* pin = getTcal9539Chan(ulPin);
    if(!pin || !pin->i2cAddr) {
        Serial.printf("TCAL read failed %d\n", ulPin);
        return 0;
    }  

    uint8_t data = tcal9539_reg8Read(pin->i2cAddr, pin->rdAddr);

    return (data >> pin->index) & 0x1;
 }

int tcal9539_isOwned(uint32_t ulPin) {
  return (ulPin & TCAL_MASK) != 0; 
}

