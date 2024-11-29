//support all fats
#define SD_FAT_TYPE 3

#include <SdFat.h>
#include "FreeStack.h"
#include "sdios.h"
#include "Mp3Player.h"

#include "tcal9539.h"

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
static const int VABT         = A2;
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
static const int STP_EN       = 1;


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
  //external Input
  pinMode(USER_SEL, INPUT_PULLDOWN);
  //while(1) {;}
  pinMode(USER_UP, INPUT_PULLDOWN);
  pinMode(USER_DOWN, INPUT_PULLDOWN);
  pinMode(USER_POWER, INPUT_PULLDOWN);
  pinMode(E_BUSY, INPUT);
  pinMode(RTC_ALARM, INPUT_PULLUP);
  pinMode(MINUTE_HALL, INPUT);
  pinMode(HOUR_HALL, INPUT);
  //TODO setup interrupts

  //external inputs
  pinMode(E_CS, OUTPUT);
  pinMode(E_DC, OUTPUT);
  pinMode(SR_CS, OUTPUT);
  pinMode(E_RST, OUTPUT);
  pinMode(E_ENABLE, OUTPUT);
  pinMode(VCC_HALL, OUTPUT);

  //internal inputs
  pinMode(GPIO_INT, INPUT);
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
  pinMode(STP_EN, OUTPUT);
}

// the setup routine runs once when you press reset:
void setup() {
  Wire.begin();

  pinMode(13, OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  Serial.println("GPIO init\n");

  setupGpio();

  digitalWrite(ENABLE_VCC, 1);
  digitalWrite(E_ENABLE, 1);

  Serial.println("GPIO Done\n");
  
  mp3.Init(14, 5);
  while(1) {
    Serial.printf("%d %d %d %d\n", digitalRead(USER_SEL), digitalRead(USER_UP), digitalRead(USER_DOWN), digitalRead(USER_POWER));
    delay(1000);
  }

  //mp3.Play("test.mp3", false);
  while (!sd.begin(SD_CS, 12000000)) {
      Serial.println("Card failed, or not present");
      delay(10000);
    }

    cidDmp();
}
int i = 0;
// the loop routine runs over and over again forever:
void loop() {
  if(!mp3.Playing()) {
    Serial.printf("On index %d, %s\n", i, files[i]);
    if(file.open(files[i]), O_RDONLY) {
      cout << "Failed to open file" << endl;
    }

    mp3.Play(&file, false);
    i = (i+1) % 3;
  }

  mp3.periodic();
}
