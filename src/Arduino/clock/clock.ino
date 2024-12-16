//support all fats
#define SD_FAT_TYPE 3
#include <Arduino.h>
#include <SdFat.h>
#include <Adafruit_SleepyDog.h>
#include "FreeStack.h"
#include "sdios.h"
#include "Mp3Player.h"
#include "millis64.h"
//Audio Libs
#include "tcal9539.h"
#include "RTCZero.h"

//Stepper Lib
#include "clockHand.h"

//Time Libs
#include "PCF2131_I2C.h"
#include <time.h>

//LED lib
#include <SPI.h>
#include <Adafruit_DotStar.h>

// Here's how to control the LEDs from any two pins:
#define DATAPIN    4
#define CLOCKPIN   5
Adafruit_DotStar led(1, 8, 6, DOTSTAR_BRG);

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

bool enColor = false;
void showLed(int r, int g, int b, int bright) {
  if(!enColor) {
    return;
  } 
  led.setPixelColor(0, r, g, b);
  led.setBrightness(bright/4);
  led.show();
  //delay(10);
}


void showRed(uint8_t bright) { showLed(255,0,0,bright);}
void showGreen(uint8_t bright) { showLed(0,255,0,bright);}
void showBlue(uint8_t bright) { showLed(0,0,255,bright);}
void showPurple(uint8_t bright) { showLed(128,0,128,bright);}
void showYellow(uint8_t bright) { showLed(255,255,0,bright);}
void showWhite(uint8_t bright) { showLed(255,0,0,bright);}
void showMagenta(uint8_t bright) { showLed(255,0,255,bright);}
void showCyan(uint8_t bright) { showLed(0,255,255,bright);}

void showOff() { led.clear(); }

/*
 *  Play an MP3 file from an SD card
 */
const char* files[] = {
  "test.mp3",
  "test1.mp3",
  "test2.mp3"
};


ArduinoOutStream cout(Serial);
;
SdFs sd;
FsFile file;
FsFile folder;

//GPIO
static const int USER_SEL     = E0;
static const int USER_UP      = E1;
static const int USER_DOWN    = E2;
static const int USER_POWER   = E3;
static const int E_BUSY       = E4;
static const int RTC_ALARM    = E5;
static const int MINUTE_HALL  = E6;
static const int HOUR_HALL    = E7;
static const int SPARE_INPUT  = E16;

static const int E_CS         = E10;
static const int E_DC         = E11;
static const int SR_CS        = E12;
static const int E_RST        = E13;
static const int E_ENABLE     = E14;
static const int VCC_HALL     = E17;

static const int DAC_L        = A0;
static const int DAC_R        = A1;
static const int VBAT         = A2;
static const int ENABLE_VCC   = 17;
static const int GPIO_INT     = 18;
static const int SD_CS        = 19;
static const int AUDIO_EN     = 2;
static const int STP_A1       = 13;
static const int STP_A2_A3    = 12;
static const int STP_A4       = 11;
static const int STP_B1       = 10;
static const int STP_B2_B3    = 9;
static const int STP_B4       = 7;
static const int STP_EN_N       = 1;

ClockHand* hourHand;
ClockHand* minuteHand;
PCF2131_I2C rtc;
RTCZero rtcInternal;

/**
 * \brief Put the system to sleep waiting for interrupt
 *
 * Executes a device DSB (Data Synchronization Barrier) instruction to ensure
 * all ongoing memory accesses have completed, then a WFI (Wait For Interrupt)
 * instruction to place the device into the sleep mode specified by
 * \ref system_set_sleepmode until woken by an interrupt.
 */
static inline void system_sleep(void)
{
	__DSB();
	__WFI();
}


bool openFileN(const char* path, int count) {
  if(!folder.open(path)) {
    Serial.printf("failed to open %s File %d\n", path, count);
    return false;
  }
  for(int i = 0; i < count; i++) {
    file.openNext(&folder);
  }
  if(!file.isOpen()) {
    Serial.printf("Failed to open %s File %d - not found\n", path, count);
    return true;
  }
  return true;
}

//------------------------------------------------------------------------------
// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))
//------------------------------------------------------------------------------
void cidDmp() {
  cid_t cid;
  if (!sd.card()->readCID(&cid)) {
    error("readCID failed");
  }
  cout << F("\nManufacturer ID: ");
  cout << uppercase << showbase << hex << int(cid.mid) << dec << endl;
  cout << F("OEM ID: ") << cid.oid[0] << cid.oid[1] << endl;
  cout << F("Product: ");
  for (uint8_t i = 0; i < 5; i++) {
    cout << cid.pnm[i];
  }
  cout << F("\nRevision: ") << cid.prvN() << '.' << cid.prvM() << endl;
  cout << F("Serial number: ") << hex << cid.psn() << dec << endl;
  cout << F("Manufacturing date: ");
  cout << cid.mdtMonth() << '/' << cid.mdtYear() << endl;
  cout << endl;
}


//UI callbacks
void userSelPressed(void* data, bool state) {
  Serial.printf("userSelPressed %d\n", state);
}

void userSelHeld(void* data, bool state) {
  Serial.printf("userSelHeld %d\n", state);
}

void userUpPressed(void* data, bool state) {
  Serial.printf("userUpPressed %d\n", state);
}

void userUpHeld(void* data, bool state) {
  Serial.printf("userUpHeld %d\n", state);
  while(1) {
    Serial.printf("waiting on reset\n");
    delay(10000);
  }
}

void userDownPressed(void* data, bool state) {
  Serial.printf("userDownPressed %d\n", state);
}

void userDownHeld(void* data, bool state) {
  Serial.printf("userDownHeld %d\n", state);
}

void userRstPressed(void* data, bool state) {
  Serial.printf("userRstPressed %d\n", state);
}

void rtcPcaEvent(void* data, bool state) {
      Serial.printf("RTC Minute\n");
      uint8_t status[3];
      rtc.int_clear(status);
      //int_cause_monitor(status);
      if (status[0] & 0x80)
      {
        Serial.print("INT:every min/sec, ");

        time_t current_time = rtc.time(NULL);
        Serial.print("time:");
        Serial.print(current_time);
        Serial.print(" ");
        Serial.println(ctime(&current_time));
        digitalWrite(STP_EN_N, 0);
        digitalWrite(VCC_HALL, 1);
        //allow voltages to stablize
        delay(50);
        struct tm curr_tm;
        gmtime_r(&current_time, &curr_tm);
        minuteHand->setHandMinute(curr_tm.tm_min);
        hourHand->setHandHour(curr_tm.tm_min, curr_tm.tm_hour);
      }

      Watchdog.reset();
}

bool rtcInternalEventService = false;
void rtcInternalEvent() {
  rtcInternalEventService = true;
}

bool rtcPeriodic() {
  if(!rtcInternalEventService) {
    return false;
  }
  rtcInternalEventService = false;
  int year = rtcInternal.getYear();
  int month = rtcInternal.getMonth();
  int day = rtcInternal.getDay();
  int hour = rtcInternal.getHours();
  int min = rtcInternal.getMinutes();
  int sec = rtcInternal.getSeconds();

  Serial.printf("RTC Alarm: 10/24/2224 %02d/%02d\%04d %02d:%02d:%02d\n", month, day, year, hour, min, sec);

  digitalWrite(STP_EN_N, 0);
  digitalWrite(VCC_HALL, 1);
  //allow voltages to stablize
  delay(50);

  minuteHand->setHandMinute(min);
  hourHand->setHandHour(sec, min);

  return false;

}

void userRstHeld(void* data, bool state) {
  Serial.printf("userRstHeld %d\n", state);
  invalidateBram(false);
  NVIC_SystemReset();
}

void setupGpio() {
  //reset the device(s)
  tcal9539_reset(0);

  //internal inputs
  pinMode(GPIO_INT, INPUT);

  //setup TCAL
  initTcal9539(0x74, GPIO_INT);
  //external Input
  pinMode(USER_SEL, INPUT_PULLDOWN | TCAL_INT_ENABLE);
  pinMode(USER_UP, INPUT_PULLDOWN | TCAL_INT_ENABLE);
  pinMode(USER_DOWN, INPUT_PULLDOWN | TCAL_INT_ENABLE);
  pinMode(USER_POWER, INPUT_PULLDOWN | TCAL_INT_ENABLE);
  pinMode(E_BUSY, INPUT);
  pinMode(RTC_ALARM, INPUT_PULLUP | TCAL_INT_ENABLE | TCAL_INVERT);
  pinMode(MINUTE_HALL, INPUT | TCAL_INVERT);
  pinMode(HOUR_HALL, INPUT | TCAL_INVERT);
  pinMode(SPARE_INPUT, INPUT_PULLDOWN);
  pinMode(E15, INPUT_PULLDOWN); //Unused pull down no float
  //TODO setup interrupts

  //external inputs
  pinMode(E_CS, OUTPUT);
  pinMode(E_DC, OUTPUT);
  pinMode(SR_CS, OUTPUT);
  pinMode(E_RST, OUTPUT);
  pinMode(E_ENABLE, OUTPUT);
  pinMode(VCC_HALL, OUTPUT);

  //TODO add interrupt

  //internal outputs
  pinMode(ENABLE_VCC, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(AUDIO_EN, OUTPUT);
  pinMode(STP_A1, OUTPUT);
  pinMode(STP_A2_A3, OUTPUT);
  pinMode(STP_A4, OUTPUT);
  pinMode(STP_B1, OUTPUT);
  pinMode(STP_B2_B3, OUTPUT);
  pinMode(STP_B4, OUTPUT);
  pinMode(STP_EN_N, OUTPUT);


  //setup input buttons
  registerCallbackPressed(USER_SEL, userSelPressed, nullptr, 10);
  registerCallbackHeld(USER_SEL, userSelHeld, nullptr, 1000);
  registerCallbackPressed(USER_UP, userUpPressed, nullptr, 10);
  registerCallbackHeld(USER_UP, userUpHeld, nullptr, 1000);
  registerCallbackPressed(USER_DOWN, userDownPressed, nullptr, 10);
  registerCallbackHeld(USER_DOWN, userDownHeld, nullptr, 1000);
  registerCallbackPressed(USER_POWER, userRstPressed, nullptr, 1000);
  registerCallbackHeld(USER_POWER, userRstHeld, nullptr, 5000);

  registerCallbackPressed(RTC_ALARM, rtcPcaEvent, nullptr, 0);

}

void set_timePca(struct tm now_tm) {
  /*  !!!! "strptime()" is not available in Arduino's "time.h" !!!!
  const char* current_time  = "2023-4-7 05:25:30";
  const char* format  = "%Y-%m-%d %H:%M:%S";
  struct tm	tmv;
  strptime( current_time, format, &tmv );
  */

  now_tm.tm_year = 2024 - 1900;
  now_tm.tm_mon = 12 - 1;  // 0 - jan 11 dec
  now_tm.tm_mday = 15;
  now_tm.tm_hour = 23;
  now_tm.tm_min = 12;
  now_tm.tm_sec = 0;

  rtc.set(&now_tm);
}

void setupRtcPca2131() {

    //setup RTC
  rtc.begin();
  struct tm now;
  set_timePca(now);
  delay(10);

  rtc.alarm_disable();
  rtc.int_clear();
  rtc.periodic_interrupt_enable(PCF2131_base::periodic_int_select::DISABLE, 1);
  //DO not use perdioc interrupt enable, it has a bug

  //enable battery switch over - set PWRMNG = 0 . Default 0b111
  rtc.reg_w(2, rtc.reg_r(2) & ~0xE0);
  

  //Enable Minute interrupt
  rtc.reg_w(0, rtc.reg_r(0) | 0x2);

  //set all interrupts 
  rtc.reg_w(0x31, 0);
  rtc.reg_w(0x32, 0);


  for(int i = 0; i < 0x16; i++) {
    Serial.printf("0x%02x: 0x%02x -\n", i, rtc.reg_r(i));
  }

  uint8_t status[3];
  rtc.int_clear(status);

}

void setTimeInternal(struct tm* now_tm) {
  struct tm local_tm;
  if(now_tm == nullptr)  {
    now_tm = &local_tm;
    now_tm->tm_year = 2024 - 1900;
    now_tm->tm_mon = 12 - 1;  // 0 - jan 11 dec
    now_tm->tm_mday = 15;
    now_tm->tm_hour = 23;
    now_tm->tm_min = 12;
    now_tm->tm_sec = 0;
  }

  rtcInternal.setDate(now_tm->tm_mday, now_tm->tm_mon - 1 , now_tm->tm_year - 100 );
  rtcInternal.setTime(now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec);
}



void setupInternalRtc() {
  rtcInternal.begin();

  //set time
  setTimeInternal(nullptr);

  //setup alarm and register callback
  rtcInternal.setAlarmSeconds(0);
  rtcInternal.attachInterrupt(rtcInternalEvent);
  rtcInternal.enableAlarm(rtcInternal.MATCH_SS);

}


static const uint32_t BRAM_VALID = 0xCAFE1234;

union BRAM {
  //8KB
  uint8_t buff[1024 * 8];
  struct {
    uint32_t key;
    //TODO add data
  } data;
  uint32_t key;
};

BRAM* bram = (BRAM*) 0x47000000;
static_assert(sizeof(BRAM) <= (1024*8), "BRAM must be < 8KB");
void restoreBram() {
  Serial.printf("BRAM key is 0x%08x; Valid key is 0x%08x\n", bram->key, BRAM_VALID);
  if(bram->data.key == BRAM_VALID) {
    Serial.printf("BRAM recovered\n");
    return;
  }
  memset(bram, 0, sizeof(BRAM));
  bram->key = BRAM_VALID;
  Serial.printf("BRAM has been inited\n");
}

void invalidateBram(bool clear) {
  bram->key = 0;
  if(clear) {
    memset(bram, 0, sizeof(BRAM));
  }
}

uint8_t tcal9539_reg8Read(uint8_t device, uint8_t addr);

int songCount[12] = {};
int currSong[12] = {};

void readSdInit() {
    cidDmp();
    
    char path[100];
    char name[100];
    for(int i = 1; i < 13; i++) {
      snprintf(path, 100, "/%d", i);
      
      if(!folder.open(path)) {
        Serial.printf("Failed to open %s\n", path);
        break;
      }

      while(file.openNext(&folder)) {
        songCount[i-1]++;
        // file.getName(name, 100);
        // Serial.printf("%s/%s - %d\n", path, name, file.dataLength());
        file.close();
      }
      folder.close();
    }

    for(int i = 1; i < 13; i++) {
      Serial.printf("Time %d : %d songs\n", i-1, songCount[i-1]);
    }

    // for(int i = 1; i < 13; i++) {
    //   snprintf(path, 100, "/%d", i);
    //   openFileN(path, songCount[i-1]);
    //   file.getName(name, 100);
    //   Serial.printf("%d:%d - %s\n", i, songCount[i-1], name);
    //   file.close();
    // }

  // Serial.printf("Starting MP3\n");
  // if(!mp3.Playing()) {
  //   Serial.printf("MP3 not playing\n");
  //   if(openFileN("/1", 1)) {
  //     cout << "Failed to open file" << endl;
  //   }
  //   file.getName(name, 100);
  //   Serial.printf("file %s. Len %d\n", name, file.dataLength());
  //   //file.open("/number/4.mp3");
  //   mp3.Play(&file, false);
  // } else {
  //   Serial.printf("MP3 Playing??\n");
  // }

  // while(1) {
  //    mp3.periodic();
  //  }

}



// the setup routine runs once when you press reset:
void setup() {
  Watchdog.disable();
  led.begin();
  Wire.begin();
  Wire.setClock(50000);
  showLed(255,0,0, 128);

  pinMode(13, OUTPUT);
  Serial.begin(115200);
  int loop_cnt = 0;
  while(!Serial) {
    if(loop_cnt > 20) {
      break;
    }
    loop_cnt++;
    delay(100);

  }

  restoreBram();

  Serial.println("GPIO init\n");
  setupGpio();

  Serial.printf("Reset Reason 0x%08x\n", Watchdog.resetCause());

  showLed(0,0,255, 128);

  digitalWrite(ENABLE_VCC, 1);
  digitalWrite(E_ENABLE, 1);
  digitalWrite(VCC_HALL, 1);
  digitalWrite(STP_EN_N, 1);
  
  digitalWrite(STP_EN_N, 0);
  delay(100);

  Serial.println("GPIO Done\n");

  //setupRtcPca2131();
  setupInternalRtc();

  minuteHand = new ClockHand(STP_A1, STP_A2_A3, STP_A2_A3, STP_A4, MINUTE_HALL, 274, 5);
  hourHand = new ClockHand(STP_B4, STP_B2_B3, STP_B2_B3, STP_B1, HOUR_HALL, 94, 3);
  //minuteHand->setHandMinute(30);
  //delay(2000);
  minuteHand->setStepDelay(15);
  //hourHand->setHandMinute(30);
  hourHand->setStepDelay(15);
  //set inital time
  /*
  time_t current_time = rtc.time(NULL);
  struct tm curr_tm;
  gmtime_r(&current_time, &curr_tm);
  Serial.println(ctime(&current_time));
  minuteHand->setHandMinute(curr_tm.tm_min);
  hourHand->setHandHour(curr_tm.tm_min, curr_tm.tm_hour);*/

  int minute = rtcInternal.getMinutes();
  int hour = rtcInternal.getHours();
  Serial.printf("Setting time to %02d:%02d\n", hour, minute);
  minuteHand->setHandMinute(minute);
  hourHand->setHandHour(hour, minute);


  hourHand->calibrate();
  minuteHand->calibrate();

  Serial.println("Steppers Created");

  // mp3.Init(AUDIO_EN, 5);

  // //mp3.Play("test.mp3", false);

  while (!sd.begin(SD_CS, 12000000)) {
      Serial.println("Card failed, or not present");
      delay(10000);
      break;
    }

    readSdInit();

  //watchdog every 65 seconds
  //Watchdog.enable(1000*65);

  Serial.printf("Done with init\n");

}

float getBatteryVoltage() {
  //3.3V/Range * Voltage Divider Gain (100k/100k)
  return analogRead(VBAT) * 3.3 / 1024.0 * 2.0;
}

int i = 0;
// the loop routine runs over and over again forever:
int nextTime = millis()+1000;
bool idleLogged = false;
bool workLogged = false;
void loop() {

  //Serial.printf("Loop %d\n", millis());
    bool workHour = hourHand->periodic();
    bool workMinute = minuteHand->periodic();
    bool workTcal =  tcal_periodic();

    bool work = workHour || workMinute || workTcal;
    //delay(100);

    //only run if no work
    if(!work) {
      if(!idleLogged) {
        Serial.printf("%d idling. Battery %1.02f\n", millis(), getBatteryVoltage());
        idleLogged = true;
        delay(50);
        digitalWrite(STP_EN_N, 1);
        digitalWrite(VCC_HALL, 0);
        enColor = true;
        showOff();
      }

    } else {
      idleLogged = false;
    }

  if(hourHand->requestedCal() || minuteHand->requestedCal()) {
    Serial.printf("cal requested hour %d min %d, time %d\n", hourHand->requestedCal(), minuteHand->requestedCal(), millis());
    hourHand->calibrate();
    minuteHand->calibrate();
  }
  //delay(2000);
  /*
  if(!mp3.Playing()) {
    Serial.printf("On index %d, %s\n", i, files[i]);
    if(file.open(files[i]), O_RDONLY) {
      cout << "Failed to open file" << endl;
    }

    mp3.Play(&file, false);
    i = (i+1) % 3;
  }

  mp3.periodic();*/
}
