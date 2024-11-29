#include <Arduino.h>
#include <stdint.h>

//Mode bitmasks
#define TCAL_INT_ENABLE   0x10
#define TCAL_DRIVE_WEAK   0x20
#define TCAL_INVERT       0x40
#define TCAL_INPUT_LATCH  0x80
#define TCAL_OPEN_DRAIN   0x100

#define MAX_TCAL_DEVICES (1)

//9/10 invalid
#define TCAL9539_CHANNELS (18)

#ifndef TCAL_MASK
#define TCAL_MASK (0xFF0000)
#endif

//See Table 8-2 data sheet
#define TCAL_ADDR_LL (0x74)
#define TCAL_ADDR_LH (0x75)
#define TCAL_ADDR_HL (0x76)
#define TCAL_ADDR_HH (0x77)

#define TCAL_ADDR_SHIFT (16)

//0x00FF00YY where FF is the i2c addree and YY is the pin 0-7/10-18
#define TCALPIN(addr, x) ((TCAL_MASK & ((uint32_t)addr << TCAL_ADDR_SHIFT)) | ~(TCAL_MASK) & x)

#define E0 TCALPIN(TCAL_ADDR_LL, 0)
#define E1 TCALPIN(TCAL_ADDR_LL, 1)
#define E2 TCALPIN(TCAL_ADDR_LL, 2)
#define E3 TCALPIN(TCAL_ADDR_LL, 3)
#define E4 TCALPIN(TCAL_ADDR_LL, 4)
#define E5 TCALPIN(TCAL_ADDR_LL, 5)
#define E6 TCALPIN(TCAL_ADDR_LL, 6)
#define E7 TCALPIN(TCAL_ADDR_LL, 7)
#define E10 TCALPIN(TCAL_ADDR_LL, 10)
#define E11 TCALPIN(TCAL_ADDR_LL, 11)
#define E12 TCALPIN(TCAL_ADDR_LL, 12)
#define E13 TCALPIN(TCAL_ADDR_LL, 13)
#define E14 TCALPIN(TCAL_ADDR_LL, 14)
#define E15 TCALPIN(TCAL_ADDR_LL, 15)
#define E16 TCALPIN(TCAL_ADDR_LL, 16)
#define E17 TCALPIN(TCAL_ADDR_LL, 17)
#define E18 TCALPIN(TCAL_ADDR_LL, 18)


#define TCAL9539_INPUT0 (0x00)
#define TCAL9539_INPUT1 (0x01)

#define TCAL9539_OUTPUT0 (0x02)
#define TCAL9539_OUTPUT1 (0x03)

#define TCAL9539_POLINVERT0 (0x04)
#define TCAL9539_POLINVERT1 (0x05)

#define TCAL9539_CONFIG0 (0x06)
#define TCAL9539_CONFIG1 (0x07)

#define TCAL9539_DRIVESTR00 (0x40)
#define TCAL9539_DRIVESTR01 (0x41)
#define TCAL9539_DRIVESTR10 (0x42)
#define TCAL9539_DRIVESTR11 (0x43)


#define TCAL9539_INPUTLATCH0 (0x44)
#define TCAL9539_INPUTLATCH1 (0x45)

#define TCAL9539_PULLEN0 (0x46)
#define TCAL9539_PULLEN1 (0x47)

#define TCAL9539_PULLDIR0 (0x48)
#define TCAL9539_PULLDIR1 (0x49)

#define TCAL9539_INTMASK0 (0x4A)
#define TCAL9539_INTMASK1 (0x4B)

#define TCAL9539_INTSTATUS0 (0x4C)
#define TCAL9539_INTSTATUS1 (0x4D)

#define TCAL9539_OUTPORTCFG1 (0x4F)


#ifdef __cplusplus
 extern "C" {
#endif
//C API for wiring_digial.c plugins. Requires mod of base file
int tcal9539_isOwned(uint32_t ulPin);
extern void tcal9539_pinMode( uint32_t ulPin, uint32_t ulMode );
extern void tcal9539_digitalWrite( uint32_t ulPin, uint32_t ulVal );
extern int tcal9539_digitalRead( uint32_t ulPin);
bool tcal9539_init(uint8_t addr);
#ifdef __cplusplus
 }
#endif

typedef void (*gpioCallback) (void* data);

struct tcal_pin {
    uint8_t i2cAddr;
    uint8_t rdAddr;
    uint8_t wrAddr;
    uint8_t dirAddr;
    uint8_t pullEnAddr;
    uint8_t pullDirAddr;
    uint8_t interruptMaskAddr;
    uint8_t interruptStatAddr;
    uint8_t inputLatchAddr;
    uint8_t polarityInvAddr;
    uint8_t index;
    uint8_t fullIndex;
    bool input;
    bool pullEn;
    bool pullUp;
    bool interruptEn;
    uint32_t assertTime;
    bool assertFalse;
    uint32_t minAssertTimeUs;
    uint32_t minAssertHeldUs;
    int8_t pullStr;
    bool invertPol;
    void* callbackPressedData;
    void* callbackHeldData;
    gpioCallback callbackPressed;
    gpioCallback callbackHeld;
};


//i2c address, intPin to use for interrupts else -1 to disable
bool initTcal9539(uint8_t addr, int32_t intPin);
//call in main loop to handle servicing interrupts and other periodic tasks like button HELD callbacks
void tcal_periodic();

struct tcal_pin* getTcalData(uint32_t pin);
void registerCallbackPressed(uint32_t pin, gpioCallback callbackPressed, void* data, uint32_t minTimeUs);
void registerCallbackHeld(uint32_t pin, gpioCallback callbackHeld, void* data, uint32_t minTimeUs);

void enableInterrupt(uint32_t pin);
void disableInterrupt(uint32_t pin);
void setDriveStr(uint8_t str);
