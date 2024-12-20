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


//Stepper Lib
#include "clockHand.h"
#include "DateTime.h"

//#define INTERNAL_RTC
#define EXTERNAL_RTC

//Time Libs
#ifdef EXTERNAL_RTC
#include "PCF2131_I2C.h"
#include <time.h>
#endif

#ifdef INTERNAL_RTC
#include "RTC_SAMD51.h"
RTC_SAMD51 rtc;
#endif

//LED lib
#include <SPI.h>
#include <Adafruit_DotStar.h>

bool chimeHour = false;

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
  //delay(2);
}


void showRed(uint8_t bright) { showLed(255,0,0,bright);}
void showGreen(uint8_t bright) { showLed(0,255,0,bright);}
void showBlue(uint8_t bright) { showLed(0,0,255,bright);}
void showPurple(uint8_t bright) { showLed(128,0,128,bright);}
void showYellow(uint8_t bright) { showLed(255,255,0,bright);}
void showWhite(uint8_t bright) { showLed(255,0,0,bright);}
void showMagenta(uint8_t bright) { showLed(255,0,255,bright);}
void showCyan(uint8_t bright) { showLed(0,255,255,bright);}

void showOff() { showLed(0,0,0,0); }

enum MenuMode {
  menIdle = 0,
  menMinute,
  menHour,
  menPm,
  menMute,
  menQuiteVol,
  menQuiteStart
};

struct {
  uint64_t userLastPress;
  bool upPressed;
  bool upHeld;
  uint64_t upHeldNextRepeat;
  bool downPressed;
  bool downHeld;
  uint64_t downHeldNextRepeat;
  bool selectPressed;
  bool selectHeld;
  uint64_t selectHeldNextRepeat;
  bool rstPressed;
  bool userServiceRq;
  time_t time;
  DateTime dtime;
  bool timeLoaded;
  bool scheduleBatteryCheck;
  MenuMode menuMode;
} userInput = {};

ArduinoOutStream cout(Serial);
;
SdFs sd;
FsFile musicFile;
FsFile userFile;
FsFile folder;


//GPIO
static const int USER_SEL     = E0;
static const int USER_UP      = E2;
static const int USER_DOWN    = E1;
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

#ifdef EXTERNAL_RTC
PCF2131_I2C rtc;
#endif

void printDateTime(DateTime& time) {
  char timeStr[] = "DDD, DD MMM YYYY hh:mm:ss";
  char timeString[100]  = {};
  Serial.print(time.toString(timeString));
}

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
  for(int i = 0; i <= count; i++) {
    musicFile.openNext(&folder);
  }
  if(!musicFile.isOpen()) {
    Serial.printf("Failed to open %s File %d - not found\n", path, count);
    return false;
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

uint64_t holdOffSleepUntil = 0;
//UI callbacks
void userSelPressed(void* data, bool state) {
  Serial.printf("userSelPressed %d\n", state);
  userInput.selectPressed = state;
  userInput.userServiceRq = true;
  holdOffSleepUntil = millis64() + 60000;
}

void userSelHeld(void* data, bool state) {
  Serial.printf("userSelHeld %d\n", state);
  userInput.selectHeld = state;
  userInput.userServiceRq = true;
}

void userUpPressed(void* data, bool state) {
  Serial.printf("userUpPressed %d\n", state);
  userInput.upPressed = state;
  userInput.userServiceRq = true;
  holdOffSleepUntil = millis64() + 60000;
}

void userUpHeld(void* data, bool state) {
  Serial.printf("userUpHeld %d\n", state);
  userInput.upHeld = state;
  userInput.userServiceRq = true;
  holdOffSleepUntil = millis64() + 60000;
}

void userDownPressed(void* data, bool state) {
  Serial.printf("userDownPressed %d\n", state);
  userInput.downPressed = state;
  userInput.userServiceRq = true;
  holdOffSleepUntil = millis64() + 60000;
}

void userDownHeld(void* data, bool state) {
  Serial.printf("userDownHeld %d\n", state);  
  userInput.downHeld = state;
  userInput.userServiceRq = true;
  holdOffSleepUntil = millis64() + 60000;
}

void userRstPressed(void* data, bool state) {
  Serial.printf("userRstPressed %d\n", state);
  userInput.rstPressed = state;
  userInput.userServiceRq = true; 
  //mark hands to recal
  digitalWrite(VCC_HALL, 1);
  digitalWrite(STP_EN_N, 0);
  hourHand->calibrate();
  minuteHand->calibrate();
    
  holdOffSleepUntil = millis64() + 60000;
}

bool rtcEventService = false;
DateTime rtcEvent;
uint64_t rtcPlayerExpire = 0;
const static uint64_t CHIME_PLAYTIME_MS = (60 * 1000);

#ifdef EXTERNAL_RTC


void rtcPcaEvent(void* data, bool state) {
      static int playingHour = -1;
      Serial.printf("RTC Minute %d %d\n", state, digitalRead(RTC_ALARM));
      uint8_t status[3];
      rtc.int_clear(status);
      Serial.printf("0x%02x 0x%02x 0x%02x\n", status[0], status[1], status[2]);
      //int_cause_monitor(status);
      if (status[0] & 0x80)
      {
        rtcEventService = true;
        Serial.print("INT:every min/sec, ");

        time_t current_time = rtc.time(NULL);
        rtcEvent = DateTime(current_time);
        Serial.print("time:");
        Serial.print(current_time);
        Serial.print(" ");
        Serial.println(ctime(&current_time));
        digitalWrite(STP_EN_N, 0);
        digitalWrite(VCC_HALL, 1);
        //allow voltages to stablize

        bool chimeTime = false;
        int chimeValue = 0;
        if(chimeHour) {
            chimeTime = rtcEvent.minute() == 0;
            playingHour = rtcEvent.hour() % 12; //1-12 or 0-24 maps to 0-11 (0 == noon/midniht)
        } else {
            chimeTime = rtcEvent.minute() == 0;
            chimeValue = rtcEvent.minute() / 5;
        }

        if(chimeTime) {
          //start playing
          Serial.printf("Imagine a song\n");
          rtcPlayerExpire = millis64() + CHIME_PLAYTIME_MS;
          playingHour = chimeValue;
        }
      }
}

#endif


#ifdef INTERNAL_RTC
DateTime rtcEvent;
void rtcInternalEvent(uint32_t val) {
  rtcEventService = true;
  rtcEvent = rtc.now();
}
#endif

bool rtcPeriodic() {
  bool retStatus = false;
  static int playingHour = -1;
  //keep rolling sounds if sounds
  if(rtcPlayerExpire != 0 && millis64() < rtcPlayerExpire) {
    if(!mp3.Playing()) {
        Serial.printf("Starting next file at %d. (%d left)\n", millis64(), rtcPlayerExpire - millis64());
        bool status = openRandomSong(playingHour);
        if(status) {
          Serial.printf("Playing a song for %d\n", playingHour);
        }
        else 
        {
          Serial.printf("Failed to open a song for %d", playingHour);
        }
        
        mp3.Play(&musicFile, false);
    }
    //check if song is playing still, if not play next
    retStatus = true;
  } else {
    //done playing
    rtcPlayerExpire = 0;
  }

  //stop if no event
  if(!rtcEventService) {
    return retStatus;
  }

  rtcEventService = false;

  Serial.printf("RTC Alarm:  %02d/%02d/%04d %02d:%02d:%02d\n", rtcEvent.month(), rtcEvent.day(), rtcEvent.year(), rtcEvent.hour(), rtcEvent.minute(), rtcEvent.second());

  if(userInput.menuMode == menHour || userInput.menuMode == menMinute) {
    //
  } 
  else 
  {
    minuteHand->setHandMinute(rtcEvent.minute());
    hourHand->setHandHour(rtcEvent.minute(), rtcEvent.hour());
  }

    bool chimeTime = false;
    int chimeValue = 0;
    if(chimeHour) {
        Serial.printf("Hour Chime mode\n");
        chimeTime = rtcEvent.minute() == 0;
        chimeValue = rtcEvent.hour() % 12; //1-12 or 0-24 maps to 0-11 (0 == noon/midniht)
    } else {
        Serial.printf("Minute Chime mode\n");
        chimeTime = rtcEvent.minute() % 5 == 0;
        chimeValue = rtcEvent.minute() / 5;
    }

  
  if(chimeTime) {
    //start playing
    Serial.printf("Imagine a song playing at %d:%d\n", rtcEvent.hour(), rtcEvent.minute());
    rtcPlayerExpire = millis64() + CHIME_PLAYTIME_MS;
    playingHour = chimeValue;
    if(playingHour == 0) {
      playingHour = 12;
    }       
  }


  digitalWrite(STP_EN_N, 0);
  digitalWrite(VCC_HALL, 1);
  //allow voltages to stablize
  delay(50);

  retStatus = true;

  if(rtcEvent.minute() == 2) {
    userInput.scheduleBatteryCheck = true;
  }

  return retStatus;

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
}

#ifdef EXTERNAL_RTC

void set_timePca(struct tm* tm_in) {
  /*  !!!! "strptime()" is not available in Arduino's "time.h" !!!!
  const char* current_time  = "2023-4-7 05:25:30";
  const char* format  = "%Y-%m-%d %H:%M:%S";
  struct tm	tmv;
  strptime( current_time, format, &tmv );
  */
  struct tm now_tm;
  now_tm.tm_year = 2024 - 1900;
  now_tm.tm_mon = 12 - 1;  // 0 - jan 11 dec
  now_tm.tm_mday = 17;
  now_tm.tm_hour = 1;
  now_tm.tm_min = 8;
  now_tm.tm_sec = 0;

  if(tm_in == nullptr) {
    tm_in = &now_tm;
  }

  rtc.set(tm_in);
}

void setupRtcPca2131() {


    //setup RTC
  rtc.begin();
  struct tm now;
  //set_timePca(now);
  delay(10);

  //rtc.alarm_disable();
  rtc.int_clear();
  rtc.periodic_interrupt_enable(PCF2131_base::periodic_int_select::DISABLE, 1);
  rtc.periodic_interrupt_enable(PCF2131_base::periodic_int_select::EVERY_MINUTE, 1);
  //DO not use perdioc interrupt enable, it has a bug

  //enable battery switch over - set PWRMNG = 0 . Default 0b111
  rtc.reg_w(2, rtc.reg_r(2) & ~0xE0);
  

  //Enable Minute interrupt
  rtc.reg_w(0, rtc.reg_r(0) | 0x2);

  //set all interrupts 
  rtc.reg_w(0x31, 0x0);
  rtc.reg_w(0x32, 0xFF);


  for(int i = 0; i < 0x8; i++) {
    Serial.printf("0x%02x: 0x%02x -\n", i, rtc.reg_r(i));
  }
  for(int i = 0x31; i < 0x35; i++) {
    Serial.printf("0x%02x: 0x%02x -\n", i, rtc.reg_r(i));
  }

}

#endif

#ifdef INTERNAL_RTC
void setTimeInternal(struct tm* now_tm) {
  struct tm local_tm;
  if(now_tm == nullptr)  {
    now_tm = &local_tm;
    now_tm->tm_year = 2024 - 1900;
    now_tm->tm_mon = 12 - 1;  // 0 - jan 11 dec
    now_tm->tm_mday = 16;
    now_tm->tm_hour = 2;
    now_tm->tm_min = 34;
    now_tm->tm_sec = 0;
  }

  DateTime now = DateTime(F(__DATE__), F(__TIME__));
  //!!! notice The year is limited to 2000-2099
  Serial.println("adjust time!");
  rtc.adjust(now);
}



void setupInternalRtc() {

  //setTimeInternal(nullptr);
  rtc.begin();

  DateTime alarm = DateTime(0, 0, 0, 0, 0, 0);
  rtc.setAlarm(0, alarm);
  rtc.enableAlarm(0, rtc.MATCH_SS);
  rtc.attachInterrupt(rtcInternalEvent); // callback while alarm is matc

}
#endif

static const uint32_t BRAM_VALID = 0xCAFE1234;

union BRAM {
  //8KB
  uint8_t buff[1024 * 8];
  struct {
    uint32_t key;
    uint8_t hourHand[sizeof(ClockHand)];
    uint8_t minuteHand[sizeof(ClockHand)];

    //TODO add data
  } data;
  uint32_t key;
};

BRAM* bram = (BRAM*) 0x47000000;
static_assert(sizeof(BRAM) <= (1024*8), "BRAM must be < 8KB");
bool bramRestored = false;
void restoreBram() {
  Serial.printf("BRAM key is 0x%08x; Valid key is 0x%08x\n", bram->key, BRAM_VALID);
  if(bram->data.key == BRAM_VALID) {
    Serial.printf("BRAM recovered\n");
    bramRestored = true;
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

bool openRandomSong(int hour) {
  int hourO = hour;
  hour = (hour % 12);
  int index = random(0, songCount[hour]);
  static char fileName[100];
  if(hour == 0) {
    hour = 12;
  }  
  snprintf(fileName, 100, "/%d", hour);
  Serial.printf("Random song %s (hour %d/%d)\n", fileName, hour, hourO);
  currSong[hour] = index;

  return openFileN(fileName, index);
}


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

      while(musicFile.openNext(&folder)) {
        songCount[i-1]++;
        // musicFile.getName(name, 100);
        // Serial.printf("%s/%s - %d\n", path, name, musicFile.dataLength());
        musicFile.close();
      }
      folder.close();
    }

    for(int i = 1; i < 13; i++) {
      Serial.printf("Time %d : %d songs\n", i-1, songCount[i-1]);
    }

    // for(int i = 1; i < 13; i++) {
    //   snprintf(path, 100, "/%d", i);
    //   openFileN(path, songCount[i-1]);
    //   musicFile.getName(name, 100);
    //   Serial.printf("%d:%d - %s\n", i, songCount[i-1], name);
    //   musicFile.close();
    // }

  /*Serial.printf("Starting MP3\n");
  if(!mp3.Playing()) {
    Serial.printf("MP3 not playing\n");
    if(!openFileN("/1", 2)) {
      cout << "Failed to open musicFile" << endl;
    }
    musicFile.getName(name, 100);
    Serial.printf("musicFile %s. Len %d\n", name, musicFile.dataLength());
    //musicFile.open("/number/4.mp3");
    mp3.Play(&musicFile, false);
  } else {
    Serial.printf("MP3 Playing??\n");
  }

  while(1) {
     mp3.periodic();
   }*/

}


const static char* AUDIO_EXIT = "/menu/exit.mp3";
const static char* AUDIO_VOLUME = "/menu/volume.mp3";
const static char* AUDIO_VOLUMEUP = "/menu/volup.mp3";
const static char* AUDIO_VOLUMEDOWN = "/menu/voldown.mp3";
const static char* AUDIO_PAUSE = "/menu/500msp.mp3";
const static char* AUDIO_MINUTE = "/menu/minute.mp3";
const static char* AUDIO_HOUR = "/menu/hour.mp3";
const static char* AUDIO_AM = "/menu/am.mp3";
const static char* AUDIO_LOW_BATT = "/menu/lowbatt.mp3";

void playUserUi(const char* file, bool stopCurr = false) {
  if(!userFile.open(file)) {
    Serial.printf("Failed to play user file %s\n", file);
    return;
  }
  mp3.BlockingPlay(&userFile, true, stopCurr);
}


void volumeControl() {
  static bool volumeReset = false;
  if(userInput.upHeld && userInput.downHeld && !volumeReset) {
    if(volumeReset) { return ; }
    volumeReset = true;
    userInput.upPressed = false;
    userInput.downPressed = false;
    Serial.printf("Volume default - -12");
    mp3.SetGain(-12);
    playUserUi(AUDIO_VOLUME, true);
    return;
  }
  if(userInput.upPressed && !userInput.downPressed) {
    userInput.upPressed = false;
    mp3.SetGain(mp3.GetGain() + 3);
    Serial.printf("Volume up - gain %d\n", mp3.GetGain());
    playUserUi(AUDIO_VOLUMEUP, true);
  }
  else if(userInput.downPressed && !userInput.upPressed) {
    userInput.downPressed = false;
    mp3.SetGain(mp3.GetGain() - 3);
    Serial.printf("Volume down - gain %d\n", mp3.GetGain());
    playUserUi(AUDIO_VOLUMEDOWN, true);
  }

  volumeReset = false;
}

void minuteControl() {
  if(!userInput.timeLoaded) {
    userInput.timeLoaded = true;
#ifdef EXTERNAL_RTC
    userInput.time = rtc.time(nullptr);
#endif
#ifdef INTERNAL_RTC
    userInput.dtime = rtc.now();
    userInput.dtime = userInput.dtime - TimeSpan(userInput.dtime.second());
#endif
    Serial.printf("Minute Control: Time Loaded\n");
  }

  digitalWrite(STP_EN_N, 0);
  digitalWrite(VCC_HALL, 1);
  delay(10);
#ifdef EXTERNAL_RTC
  struct tm curr_tm;
  gmtime_r(&userInput.time, &curr_tm);
  Serial.printf("Start time: ");
  Serial.println(ctime(&userInput.time));
#endif

#ifdef EXTERNAL_RTC
  if(userInput.upPressed) {
    userInput.upPressed = false;
    #ifdef EXTERNAL_RTC
    curr_tm.tm_min++;
    #endif
  }
  if(userInput.downPressed) {
    userInput.downPressed = false;
    //curr_tm.tm_min--;
  }

  //clamp to an hour
  curr_tm.tm_min = curr_tm.tm_min % 60;
  curr_tm.tm_sec = 0;

  Serial.printf("New time: ");
  userInput.time = mktime(&curr_tm);
  Serial.println(ctime(&userInput.time));
  char fileName[20];
  snprintf(fileName, 20, "/number/%d.mp3",curr_tm.tm_min);
  //playUserUi(fileName, true);
  minuteHand->setHandMinute(curr_tm.tm_min);
  #endif
  #ifdef INTERNAL_RTC
  if(userInput.upPressed) {
    userInput.upPressed = false;
    userInput.dtime = userInput.dtime + TimeSpan(60);
  }
  if(userInput.downPressed) {
    userInput.downPressed = false;
    //curr_tm.tm_min--;
  }  
  minuteHand->setHandMinute(userInput.dtime.minute());
  char fileName[20];
  snprintf(fileName, 20, "/number/%d.mp3",userInput.dtime.minute());
  //playUserUi(fileName, true);  
  #endif
}

void hourControl() {
  if(!userInput.timeLoaded) {
    userInput.timeLoaded = true;
    #ifdef EXTERNA_RTC
    userInput.time = rtc.time(nullptr);
    #endif
#ifdef INTERNAL_RTC
    userInput.dtime = rtc.now();
    userInput.dtime = userInput.dtime - TimeSpan(userInput.dtime.second());
#endif
    Serial.printf("Hour Control: Time Loaded\n");
  }

  digitalWrite(STP_EN_N, 0);
  digitalWrite(VCC_HALL, 1);
  delay(10);
#ifdef EXTERNAL_RTC
  struct tm curr_tm;
  gmtime_r(&userInput.time, &curr_tm);
  Serial.printf("Start time: ");
  Serial.println(ctime(&userInput.time));

  if(userInput.upPressed) {
    userInput.upPressed = false;
    curr_tm.tm_hour++;
  }
  if(userInput.downPressed) {
    userInput.downPressed = false;
    //curr_tm.tm_min--;
  }
  //clamp to an hour
  curr_tm.tm_hour = curr_tm.tm_hour % 12;
  curr_tm.tm_sec = 0;

  Serial.printf("New time: ");
  userInput.time = mktime(&curr_tm);
  Serial.println(ctime(&userInput.time));
  char fileName[20];
  snprintf(fileName, 20, "/number/%d.mp3",curr_tm.tm_hour);
  //playUserUi(fileName, true);
  hourHand->setHandHour(0, curr_tm.tm_hour);
  #endif

  #ifdef INTERNAL_RTC
  if(userInput.upPressed) {
    userInput.upPressed = false;
    userInput.dtime = userInput.dtime + TimeSpan(60*60);
  }
  if(userInput.downPressed) {
    userInput.downPressed = false;
    //curr_tm.tm_min--;
  }

  char fileName[20];
  snprintf(fileName, 20, "/number/%d.mp3",userInput.dtime.hour());
  //playUserUi(fileName, true);
  hourHand->setHandHour(0, userInput.dtime.hour());
  #endif
}

bool userPeriodic() {
  const static int menuTimeout = 10000;
  if(userInput.userServiceRq) {
    Serial.printf("User Rq Serviced\n");
    userInput.userServiceRq = false;
    //user interaction
    userInput.userLastPress = millis64();
    if(userInput.selectPressed) {
      userInput.selectPressed = false;
      Serial.printf("select next\n");
      //minute -> hour -> am/pm -> idle

      switch(userInput.menuMode)
      {
      case menIdle:
        userInput.menuMode = menMinute;
        playUserUi(AUDIO_MINUTE, false);
        break;
      case menMinute:
        userInput.menuMode = menHour;
        playUserUi(AUDIO_HOUR, false);
        break;
      case menHour:
        userInput.menuMode = menPm;
        playUserUi(AUDIO_AM, false);
        break;
      case menPm:
        userInput.menuMode = menIdle;
        playUserUi(AUDIO_VOLUME, false);
        break;
      default:
        userInput.menuMode = menIdle;
        playUserUi(AUDIO_VOLUME, false);
      }

      return true;
    }

    if(userInput.menuMode == menIdle) {
      Serial.printf("Volume Control\n");
      volumeControl();
    }

    if(userInput.menuMode == menMinute) {
      Serial.printf("Set Minute\n");
      minuteControl();
    }    

    if(userInput.menuMode == menHour) {
      Serial.printf("Set Hour\n");
      hourControl();
    }

    if(userInput.menuMode == menPm) {
      Serial.printf("Set AM/PM\n");
    }


    return true;
  }


  if(userInput.userLastPress == 0) {
    return false;
  }
  if(userInput.userLastPress + menuTimeout < millis64() ) {
    userInput.userLastPress = 0;
    if(userInput.timeLoaded) {
      struct tm now_tm;
      gmtime_r(&userInput.time, &now_tm);
#ifdef EXTERNAL_RTC
      rtc.set(&now_tm);
      userInput.timeLoaded = false;
      minuteHand->setHandMinute(now_tm.tm_min);
      hourHand->setHandHour(now_tm.tm_min, now_tm.tm_hour);      
#endif

#ifdef INTERNAL_RTC
      userInput.timeLoaded = false;
      rtc.adjust(userInput.dtime);
      minuteHand->setHandMinute(userInput.dtime.minute());
      hourHand->setHandHour(userInput.dtime.minute(), userInput.dtime.hour());
#endif
      Serial.printf("User Time updated\n");
    }
    userInput.menuMode = menIdle;
    Serial.printf("Menu exit\n");
    playUserUi(AUDIO_EXIT,false);

    return false;
  } 

  return true;
}


// the setup routine runs once when you press reset:
void setup() {
  Watchdog.disable();
  led.begin();
  Wire.begin();
  Wire.setClock(10000);

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
  digitalWrite(STP_EN_N, 0);
  delay(10);

  Serial.println("GPIO Done\n");

#ifdef EXTERNAL_RTC
  setupRtcPca2131();
#endif
#ifdef INTERNAL_RTC
  setupInternalRtc();
#endif
  //bramRestored = false;
  if(bramRestored && (Watchdog.resetCause() != 0x40)) {
    Serial.printf("Clock Hands Restored\n");
    //restore from bram
  } else {
    ClockHand minTmp(STP_A1, STP_A2_A3, STP_A2_A3, STP_A4, MINUTE_HALL, 274, 5);
    ClockHand hourTmp(STP_B4, STP_B2_B3, STP_B2_B3, STP_B1, HOUR_HALL, 94, 3);
    memcpy(bram->data.minuteHand, &minTmp, sizeof(ClockHand));
    memcpy(bram->data.hourHand, &hourTmp, sizeof(ClockHand));

  //use if removed from bram
  //minuteHand = new ClockHand(STP_A1, STP_A2_A3, STP_A2_A3, STP_A4, MINUTE_HALL, 274, 5);
  //hourHand = new ClockHand(STP_B4, STP_B2_B3, STP_B2_B3, STP_B1, HOUR_HALL, 94, 3);
  }
  minuteHand = (ClockHand*) bram->data.minuteHand;
  hourHand = (ClockHand*) bram->data.hourHand;
  
  //minuteHand->setHandMinute(30);
  //delay(2000);
  minuteHand->setStepDelay(10);
  //hourHand->setHandMinute(30);
  hourHand->setStepDelay(10);
  //set inital time
#ifdef EXTERNAL_RTC
  uint8_t status[3];
  rtc.int_clear(status);

  registerCallbackPressed(RTC_ALARM, rtcPcaEvent, nullptr, 0);
  time_t current_time = rtc.time(NULL);
  struct tm curr_tm;
  gmtime_r(&current_time, &curr_tm);
  Serial.println(ctime(&current_time));
  minuteHand->setHandMinute(curr_tm.tm_min);
  hourHand->setHandHour(curr_tm.tm_min, curr_tm.tm_hour);
#endif


#ifdef INTERNAL_RTC
  DateTime now = rtc.now();
  Serial.printf("Setting time to %02d:%02d\n", now.hour(), now.minute());
  minuteHand->setHandMinute(now.minute());
  hourHand->setHandHour(now.minute(), now.hour());
#endif

  if(!bramRestored) {
    hourHand->calibrate();
    minuteHand->calibrate();
  }

  Serial.println("Steppers Created");

  mp3.Init(AUDIO_EN, 200);

  // //mp3.Play("test.mp3", false);

  while (!sd.begin(SD_CS, 12000000)) {
      Serial.println("Card failed, or not present");
      delay(10000);
      break;
    }

    readSdInit();

  //watchdog every 65 seconds
  //Watchdog.enable(1000*120);

  Serial.printf("Done with init\n");

}

double getBatteryVoltage() {
  //3.3V/Range * Voltage Divider Gain (100k/100k)
  uint32_t data = analogRead(VBAT);
  return data * 3.3 / 1024.0 * 2.0;
}

int i = 0;
// the loop routine runs over and over again forever:
int nextTime = millis()+1000;
bool idleLogged = false;
bool workLogged = false;
uint64_t acculmSleep = 0;
void loop() {
    Watchdog.enable(10000);
    showGreen(64);
  //Serial.printf("Loop %d\n", millis());
    bool workHour = hourHand->periodic();
    bool workMinute = minuteHand->periodic();
    bool workTcal =  tcal_periodic();
    bool workMp3 = mp3.periodic();
    bool workUserInput = userPeriodic();
    bool workRtc = rtcPeriodic();  

    if(userInput.scheduleBatteryCheck) {
      userInput.scheduleBatteryCheck = false;
      Serial.printf("Checking battery\n");
      double batV = getBatteryVoltage();
      if(batV < 4.10) {
        musicFile.open(AUDIO_LOW_BATT);
        mp3.BlockingPlay(&musicFile, true, false);
      }
    }

    showOff();
    Watchdog.disable();

    bool work = workHour || workMinute || workTcal || workMp3 || workUserInput || workRtc;
    //delay(100);

    //only run if no work
    if(!work) {
      if(!idleLogged) {
        Serial.printf("%d idling. Battery ", millis());
        Serial.println(getBatteryVoltage());
        idleLogged = true;
        digitalWrite(STP_EN_N, 1);
        digitalWrite(VCC_HALL, 0);
      } else {
        if(Serial) {
          //Serial.println("Not sleeping\n");
          delay(1000);
        }
        else if(holdOffSleepUntil == 0 || holdOffSleepUntil < millis64() )
        {
          Serial.println("Sleeping\n");
          showBlue(64);
          USBDevice.detach();
          Serial.end();
          acculmSleep += Watchdog.sleep(10000);

          if(rtcEventService || tcal_isrNeeded() || (acculmSleep > 40000)) {
            USBDevice.attach();
            delay(200);
            Serial.begin(115200);
            int loop_cnt = 0;
            while(!Serial) {
              if(loop_cnt > 20) {
                break;
              }
              loop_cnt++;
              delay(100);
            }
            Serial.printf("Slept for %d. Waking now to do work\n", acculmSleep);

            if(acculmSleep > 50000) {
              Serial.printf("simluated wdt\n");
            }

            acculmSleep = 0;
          }
        }
      }
      showOff();

    } else {
      idleLogged = false;
    }

  if(hourHand->requestedCal() || minuteHand->requestedCal()) {
    Serial.printf("cal requested hour %d min %d, time %d\n", hourHand->requestedCal(), minuteHand->requestedCal(), millis());
    hourHand->calibrate();
    minuteHand->calibrate();
  }
}
