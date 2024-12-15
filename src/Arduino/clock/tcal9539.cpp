#include "tcal9539.h"
#include <Wire.h>
#include "millis64.h"

void showLed(int r, int g, int b, int bright = 255);
void showRed(uint8_t bright = 255);
void showGreen(uint8_t bright = 255);
void showBlue(uint8_t bright = 255);
void showPurple(uint8_t bright = 255);
void showYellow(uint8_t bright = 255);
void showWhite(uint8_t bright = 255);
void showMagenta(uint8_t bright = 255);
void showCyan(uint8_t bright = 255);
void showOff();

#ifndef DEBUG
#define PRINTF(...)
#else
#define PRINTF(...) Serial.printf(...)
#endif
//global data structure
int8_t tcal9539_count = 0;
uint8_t tcal9539_addr_map[MAX_TCAL_DEVICES] = {TCAL_ADDR_LL};
struct tcal_pin tcal_data[MAX_TCAL_DEVICES][TCAL9539_CHANNELS] = {};
int intPinList[MAX_TCAL_DEVICES] = {};

struct tcal_pin* getTcal9539Chan(uint32_t ulPin) {
    if(tcal9539_count == 0) {
      return nullptr;
    }
    return &tcal_data[0][ulPin & 0xFF];
}

uint8_t getTcal9539Addr(uint32_t pin) {
    return (uint8_t) (pin >> TCAL_ADDR_SHIFT) & 0xFF;
    /*uint8_t addr = (uint8_t) (pin >> TCAL_ADDR_SHIFT) & 0xFF;
    for(int i = 0; i < tcal9539_count; i++) {
        if(tcal9539_addr_map[i] == addr) {
            return i;
        }
    }
    //not found return default. May be an issue
    return 0x0;*/
}

static bool intTriggered = false;
//TODO expand to support multiple interupts
void tcalInt() {
  //in case we get an interrupt while doing i2c transaction we don't operate here.
  //
  intTriggered = true;
  Serial.printf("Interrupt %d\n", millis());
}


bool initTcal9539(uint8_t addr, int32_t intPin) {
    if(tcal9539_count == MAX_TCAL_DEVICES) {
      Serial.printf("Too many TCAL9539. Failed for addr %d\n", addr);
      return false;
    }

    //Wire.begin(); //TODO track down why call twice
    tcal9539_addr_map[tcal9539_count] = addr;
    intPinList[tcal9539_count] = intPin;
    tcal9539_count++;

    pinMode(intPin, INPUT);
    
    if(intPin > 0) {
      pinMode(intPin, INPUT);
      attachInterrupt(digitalPinToInterrupt(intPin), tcalInt, FALLING);
    }
    
    return true;
}

uint8_t tcal9539_reg8Read(uint8_t device, uint8_t addr) {
    //flush
    showRed();
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x stared\n", device, addr);
    while(Wire.available()) { Wire.read();}
    showBlue();
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x begin\n", device, addr);
    Wire.beginTransmission(device);
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x ws\n", device, addr);
    Wire.write(addr);
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x we\n", device, addr);
    int status = Wire.endTransmission();
    showYellow();
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x et\n", device, addr);
    //Serial.printf("read8 %02x %02x %d\n", device, addr, status);
    Wire.requestFrom(device, 1);
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x rf\n", device, addr);
    //TODO error handle while(Wire.available()< 1) {;}
    uint8_t val = Wire.read();
    PRINTF("tcal9539_reg8Read 0x%2x 0x%02x rd\n", device, addr);
    //Serial.printf("%02x read: %02x %02x\n", device, addr, val);
    showGreen();
    return val;
}

void tcal9539_reg8Write(uint8_t device, uint8_t addr, uint8_t val) {
  showRed();
    //Serial.printf("%02x write: %02x %02x\n", device, addr, val);
    PRINTF("tcal9539_reg8Write 0x%2x 0x%02x %d stared\n", device, addr, val);
    Wire.beginTransmission(device);
    showYellow();
    PRINTF("tcal9539_reg8Write 0x%2x 0x%02x %d br\n", device, addr, val);
    Wire.write(addr);
    PRINTF("tcal9539_reg8Write 0x%2x 0x%02x %d wradd\n", device, addr, val);
    Wire.write(val);
    PRINTF("tcal9539_reg8Write 0x%2x 0x%02x %d wr\n", device, addr, val);
    Wire.endTransmission();
    PRINTF("tcal9539_reg8Write 0x%2x 0x%02x %d end\n", device, addr, val);
    showGreen();
    return;
}

uint16_t tcal9539_reg16Read(uint8_t device, uint8_t addr) {
  showRed();
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x started\n", device, addr);
    while(Wire.available()) { Wire.read();}
    showYellow();
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x f\n", device, addr);
    Wire.beginTransmission(device);
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x br\n", device, addr);
    Wire.write(addr);
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x wr\n", device, addr);
    int status = Wire.endTransmission();  
    showBlue();
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x et\n", device, addr);

    Wire.requestFrom(device, 2);
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x rf\n", device, addr);
    while(Wire.available()< 2) {;}
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x wait\n", device, addr);
    uint16_t val = Wire.read();
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x rd1\n", device, addr);
    val |= (uint16_t)Wire.read() << 8;
    PRINTF("tcal9539_reg16Read 0x%2x 0x%02x done\n", device, addr);
    showGreen();
    

    return val;
}

void tcal9539_reg16Write(uint8_t device, uint8_t addr, uint16_t val) {
    Wire.beginTransmission(device);
    Wire.write(addr);
    Wire.write((uint8_t) (val & 0xFF));
    Wire.write((uint8_t) (val >> 8));
    Wire.endTransmission();
    return;
}

void tcal9539_reset(uint8_t addr) {
  Serial.printf("Resetting 0x%02x\n", addr);
  Wire.beginTransmission(0);
  delay(10);
  Wire.write(0x6);
  delay(10);
  Wire.endTransmission(true);  
  delay(10);  
}

uint8_t tcal9539_regRMW(uint8_t device, uint8_t addr, uint8_t val, uint8_t mask) {
    uint8_t data = tcal9539_reg8Read(device, addr);
    uint8_t data0 = data;
    data &= ~mask;
    data |= val & mask;
    tcal9539_reg8Write(device,addr,data);
    return data0;
}

void tcal9539_pinMode( uint32_t ulPin, uint32_t ulMode ) {
    //Serial.printf("Configuring 0x%08x as %d\n", ulPin, ulMode);
    struct tcal_pin* pin = getTcal9539Chan(ulPin);
    if(!pin) { 
      //Serial.printf("Invalid TCAL %d\n", ulPin);
      return;
      }
    if(pin->i2cAddr == 0) {

      pin->i2cAddr = getTcal9539Addr(ulPin);
      //Serial.printf("addr is now %02x\n", pin->i2cAddr);
      //init structure for first call
      if((ulPin & 0xFF) < 10) {
        pin->index = ulPin & 0x7;
        pin->rdAddr = TCAL9539_INPUT0;
        pin->wrAddr = TCAL9539_OUTPUT0;
        pin->dirAddr = TCAL9539_CONFIG0;
        pin->pullEnAddr = TCAL9539_PULLEN0;
        pin->pullDirAddr = TCAL9539_PULLDIR0;
        pin->inputLatchAddr = TCAL9539_INPUTLATCH0;
        pin->interruptMaskAddr = TCAL9539_INTMASK0;
        pin->interruptStatAddr = TCAL9539_INTSTATUS0;
        pin->polarityInvAddr = TCAL9539_POLINVERT0;
        //TODO Add drive strength
        //TODO add open drain
      }
      else {
        pin->index = (ulPin - 10) & 0x7;
        pin->rdAddr = TCAL9539_INPUT1;
        pin->wrAddr = TCAL9539_OUTPUT1;
        pin->dirAddr = TCAL9539_CONFIG1;
        pin->pullEnAddr = TCAL9539_PULLEN1;
        pin->pullDirAddr = TCAL9539_PULLDIR1;
        pin->inputLatchAddr = TCAL9539_INPUTLATCH1;
        pin->interruptMaskAddr = TCAL9539_INTMASK1;
        pin->interruptStatAddr = TCAL9539_INTSTATUS1;
        pin->polarityInvAddr = TCAL9539_POLINVERT1;
        //TODO Add drive strength
        //TODO add open drain
      }
      pin->minAssertTimeMs = 0;
      pin->minAssertHeldMs = 500000;
    }

  switch ( ulMode & 0xF )
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
  printf("Starting TCAL writes\n");
  if(pin->input == false) {
    tcal9539_regRMW(pin->i2cAddr, pin->dirAddr, 0, mask);
    printf("TCAL input donw\n");
    return;
  }

  tcal9539_regRMW(pin->i2cAddr, pin->dirAddr, mask, mask);
  tcal9539_regRMW(pin->i2cAddr, pin->pullDirAddr, pin->pullUp << pin->index, mask);
  tcal9539_regRMW(pin->i2cAddr, pin->pullEnAddr, pin->pullEn << pin->index, mask);
  printf("Starting i2c outputs 1/2\n");
  uint8_t data = ulMode & TCAL_INT_ENABLE ? 0 : mask;
  Serial.printf("0x%04x:0x%04x, int data 0x%08x & 0x%08x\n", ulPin, ulMode, data, mask);
  tcal9539_regRMW(pin->i2cAddr, pin->interruptMaskAddr, data, mask);

  data = ulMode & TCAL_INVERT ? mask : 0;
  tcal9539_regRMW(pin->i2cAddr, pin->polarityInvAddr, data, mask);

  data = ulMode & TCAL_INPUT_LATCH ? mask : 0;
  tcal9539_regRMW(pin->i2cAddr, pin->inputLatchAddr, data, mask);

}

void tcal9539_digitalWrite( uint32_t ulPin, uint32_t ulVal ) {
    struct tcal_pin* pin = getTcal9539Chan(ulPin);
    if(!pin || !pin->i2cAddr) {
        Serial.printf("TCAL write failed 0x%08x %04x - %d\n", ulPin, pin->i2cAddr, ulVal);
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


void registerCallbackPressed(uint32_t pin, gpioCallback callbackPressed, void* cbData, uint32_t minTimeMs) {
  struct tcal_pin* data = getTcal9539Chan(pin);
  if(nullptr == data) {
    Serial.printf("Failed to set callback for 0x04x\n", pin);
    return;
  }
  data->minAssertTimeMs = minTimeMs;
  data->callbackPressedData = cbData;
  data->callbackPressed = callbackPressed;
  data->assertTime = 0;
  data->pressedCalled = false;
}

void registerCallbackHeld(uint32_t pin, gpioCallback callbackHeld, void* cbData, uint32_t minTimeMs) {
  struct tcal_pin* data = getTcal9539Chan(pin);
  if(nullptr == data) {
    Serial.printf("Failed to set callback for 0x04x\n", pin);
    return;
  }

  data->minAssertHeldMs = minTimeMs;
  data->callbackHeldData = cbData;
  data->callbackHeld = callbackHeld;
  data->assertTime = 0;
}


bool tcal_periodic() {
  //if int triggered we want to say we processed data.
  bool registeredAsserted = intTriggered;
  intTriggered = false;
  uint64_t now = millis64();
  //read GPIO and direction registers. Only check inputs
  uint16_t gpio = tcal9539_reg16Read(getTcal9539Addr(E0), TCAL9539_INPUT0);
  gpio &= tcal9539_reg16Read(getTcal9539Addr(E0), TCAL9539_CONFIG0);
  //Serial.printf("gpio %04x\n", gpio);
  //return 0;

  for(int i = 0; i < 16; i++) {
    bool assert = (gpio >> i) & 0x1;
    struct tcal_pin* data = getTcal9539Chan(i);
    //handle logic if asserted
    if(assert) {
      //Serial.printf("%i: %d\n", i, assert);
      //if asserted start timing
      if (data->assertTime == 0) {
        data->assertTime = now;
      }
      /*if(i < 4) {
        Serial.printf("%i: %p - press %d: now %ld delta %ld, min %d\n", i, data->callbackPressed, data->pressedCalled, now, now-data->assertTime, data->minAssertTimeMs);
      }*/
      //check ifpressed long enough and not pressed before
      if(data->callbackPressed && !data->pressedCalled && (now - data->assertTime) > data->minAssertTimeMs) {
        data->callbackPressed(data->callbackPressedData, assert);
        data->pressedCalled = true;
        Serial.printf("%d pressed\n");
      }
      //if help call callback again
      if(data->callbackHeld && (now - data->assertTime) > data->minAssertHeldMs) {
        data->callbackHeld(data->callbackHeldData, assert);
      }
      //if gpio is assert and callback is registered, we are active
      if(data->callbackHeld || data->callbackPressed) {
        registeredAsserted = true;
      }
    }
    //handle false logic
    else {
      if(data->assertTime == 0) {
        //already was off continue
        continue;
      }
      //newley off
      
      //if held, and now off, callback with 0 value to say not held
      if(data->callbackHeld && (now - data->assertTime) > data->minAssertHeldMs) {
        data->callbackHeld(data->callbackHeldData, assert);
      }

      //reset time and called status
      data->assertTime = 0;
      data->pressedCalled = 0;
    }
  }

  return registeredAsserted;
}