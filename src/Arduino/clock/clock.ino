//support all fats
#define SD_FAT_TYPE 3
#include <Arduino.h>
#include <SdFat.h>
#include <Adafruit_SleepyDog.h>
#include "FreeStack.h"
#include "sdios.h"
#include "Mp3Player.h"

//Audio Libs
#include "tcal9539.h"

//Stepper Lib
#include "clockHand.h"

//Time Libs
#include <PCF2131_I2C.h>
#include <time.h>

/*
 *  Play an MP3 file from an SD card
 */
const char* files[] = {
  "test.mp3",
  "test1.mp3",
  "test2.mp3"
};


ArduinoOutStream cout(Serial);

SdFs sd;
FsFile file;

//GPIO
static const int USER_SEL     = E0;
static const int USER_UP      = E1;
static const int USER_DOWN    = E2;
static const int USER_POWER   = E3;
static const int E_BUSY       = E4;
static const int RTC_ALARM    = E5;
static const int MINUTE_HALL  = E6;
static const int HOUR_HALL    = E7;

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

bool rtc_int_flag0;
void rtc_int_callback0() {
  rtc_int_flag0 = true;
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

void setupGpio() {

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
  pinMode(RTC_ALARM, INPUT_PULLUP | TCAL_INT_ENABLE);
  pinMode(MINUTE_HALL, INPUT | TCAL_INVERT);
  pinMode(HOUR_HALL, INPUT | TCAL_INVERT);
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
}

void set_time(struct tm now_tm) {
  /*  !!!! "strptime()" is not available in Arduino's "time.h" !!!!
  const char* current_time  = "2023-4-7 05:25:30";
  const char* format  = "%Y-%m-%d %H:%M:%S";
  struct tm	tmv;
  strptime( current_time, format, &tmv );
  */

  now_tm.tm_year = 2024 - 1900;
  now_tm.tm_mon = 12 - 1;  // 0 - jan 11 dec
  now_tm.tm_mday = 10;
  now_tm.tm_hour = 19;
  now_tm.tm_min = 46;
  now_tm.tm_sec = 0;

  rtc.set(&now_tm);
}

void int_cause_monitor(uint8_t* status) {
  Serial.print("status:");

  for (int i = 0; i < 3; i++) {
    Serial.print(" ");
    Serial.print(status[i], HEX);
  }
  Serial.print(", ");

  if (status[0] & 0x80) {
    Serial.print("INT:every min/sec, ");

    time_t current_time = rtc.time(NULL);
    Serial.print("time:");
    Serial.print(current_time);
    Serial.print(" ");
    Serial.println(ctime(&current_time));
  }
  if (status[0] & 0x40) {
    Serial.print("INT:watchdog");
  }
  if (status[0] & 0x10) {
    Serial.print("INT:alarm ");
    Serial.println("########## ALARM ########## ");
  }
  if (status[1] & 0x08) {
    Serial.print("INT:battery switch over");
  }
  if (status[1] & 0x04) {
    Serial.print("INT:battery low");
  }
  if (status[2] & 0xF0) {
    for (int i = 0; i < 4; i++) {
      if (status[2] & (0x80 >> i)) {
        Serial.print("INT:timestamp");
        Serial.print(i + 1);
        Serial.println("");
      }
    }
    for (int i = 0; i < 4; i++) {
      Serial.print("  TIMESTAMP");
      Serial.print(i + 1);
      Serial.print(": ");
      time_t ts = rtc.timestamp(i + 1);
      Serial.println(ctime(&ts));
    }
  }
}

void setupRtc() {
    //setup RTC
  rtc.begin();
  struct tm now;
  //set_time(now);
  delay(10);

  rtc.alarm_disable();
  rtc.int_clear();
  rtc.periodic_interrupt_enable(PCF2131_base::periodic_int_select::DISABLE, 1);
  //rtc.periodic_interrupt_enable(PCF2131_base::periodic_int_select::EVERY_MINUTE, 1);

  //enable battery switch over - set PWRMNG = 0 . Default 0b111
    rtc.reg_w(3, rtc.reg_r(3) & ~0xE0);


  //Enable Minute interrupt
  rtc.reg_w(0, rtc.reg_r(0) | 0x2);


  for(int i = 0; i < 0x4; i++) {
    Serial.printf("0x%02x: 0x%02x -\n", i, rtc.reg_r(i));
  }

  bool led = false;

  while(0) {
    digitalWrite(13, led);
    led = !led;
    //delay(1000);
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
    }
    delay(500);
    //system_sleep()
    //USBDevice.detach();
    //int sleepMS = Watchdog.sleep();
    //USBDevice.attach();
    //delay(2000);
    //Serial.printf("Slept for sleepMs %d\n", sleepMS);
    //delay(100);
  }

}

uint8_t tcal9539_reg8Read(uint8_t device, uint8_t addr);

// the setup routine runs once when you press reset:
void setup() {
  Wire.begin();
  Wire.setTimeout(10);

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
  Serial.println("GPIO init\n");

  setupGpio();

  digitalWrite(ENABLE_VCC, 1);
  digitalWrite(E_ENABLE, 1);
  digitalWrite(VCC_HALL, 1);
  digitalWrite(STP_EN_N, 1);
  delay(1000);
  digitalWrite(STP_EN_N, 0);

  Serial.println("GPIO Done\n");

  setupRtc();

  minuteHand = new ClockHand(STP_A1, STP_A2_A3, STP_A2_A3, STP_A4, MINUTE_HALL, 274, 5);
  hourHand = new ClockHand(STP_B4, STP_B2_B3, STP_B2_B3, STP_B1, HOUR_HALL, 94, 3);
  //minuteHand->setHandMinute(30);
  //delay(2000);
  minuteHand->setStepDelay(15);
  //hourHand->setHandMinute(30);
  hourHand->setStepDelay(15);
  //set inital time
  time_t current_time = rtc.time(NULL);
  struct tm curr_tm;
  gmtime_r(&current_time, &curr_tm);
  minuteHand->setHandMinute(curr_tm.tm_min);
  hourHand->setHandHour(curr_tm.tm_min, curr_tm.tm_hour);

  hourHand->calibrate();
  minuteHand->calibrate();

  Serial.println("Steppers Created");

  pinMode(13, OUTPUT);
  int led = 0;
  int nextTimeL = millis()+1000;
  while(0) {
    if(nextTimeL < millis()) {
      //Serial.printf("hour %d min %d millis %08d\n", digitalRead(HOUR_HALL), digitalRead(MINUTE_HALL), millis());
      nextTimeL = millis() + 1000;
    }

    hourHand->periodic();
    minuteHand->periodic();
  }



  mp3.Init(14, 5);
  /*
  while(1) {
    digitalWrite(13, led);
    led = !led;

    //Serial.printf("0x%02x:0x%02x:0x%02x\n",  (uint32_t)tcal9539_reg8Read(0x74, 0x4A), (uint32_t)tcal9539_reg8Read(0x74, 0x4C), (uint32_t)tcal9539_reg8Read(0x74, 0));
    //Serial.printf("%d\n", tcal9539_reg8Read(0x74, 0));
    bool intStat = digitalRead(GPIO_INT);
    Serial.printf("%d %d %d %d int %d\n", digitalRead(USER_SEL), digitalRead(USER_UP), digitalRead(USER_DOWN), digitalRead(USER_POWER), intStat);
    //int sleepMS = Watchdog.sleep();
    int sleepMS = 0;
    //USBDevice.attach();
    //while (!Serial)
    //  delay(10);
    Serial.printf("Slept for %d\n", sleepMS);
    delay(1000);
    break;
  }
  */
  //mp3.Play("test.mp3", false);
  /*while (!sd.begin(SD_CS, 12000000/2)) {
      Serial.println("Card failed, or not present");
      delay(10000);
    }

    cidDmp();
    */

  while(0) {
    time_t current_time = 0;

    current_time = rtc.time(NULL);
    Serial.print("time : ");
    Serial.print(current_time);
    Serial.print(", ");
    Serial.println(ctime(&current_time));

    delay(1000);
  }
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
    bool work = hourHand->periodic();
    work |= minuteHand->periodic();

    //only run if no work
    if(!work) {
      if(!idleLogged) {
        Serial.printf("%d idling. Battery %1.02f\n", millis(), getBatteryVoltage());
        idleLogged = true;
        delay(50);
        digitalWrite(STP_EN_N, 1);
        digitalWrite(VCC_HALL, 0);
      }
      /*
      if(nextTime < millis()) {
        digitalWrite(STP_EN_N, 0);
        digitalWrite(VCC_HALL, 1);
        delay(10);
        Serial.printf("%d working. Battery  %1.02f\n", millis(), getBatteryVoltage());
        nextTime = millis()+1000;
        i++;
        minuteHand->setHandMinute(i % 60);
        hourHand->setHandHour(i % 60, (i/60) % 12);
        idleLogged = false;
        workLogged = false;
        Serial.printf("Timing %d: min %d hour %d\n", i, i % 60, (i/60) % 12);
      }*/
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
        struct tm curr_tm;
        gmtime_r(&current_time, &curr_tm);
        minuteHand->setHandMinute(curr_tm.tm_min);
        hourHand->setHandHour(curr_tm.tm_min, curr_tm.tm_hour);
      }

    } else {

    }


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
