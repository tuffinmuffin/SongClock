#include <SdFat.h>
#include "FreeStack.h"
#include "sdios.h"
#include "Mp3Player.h"


/* 
 *  Play an MP3 file from an SD card
 */
const int sdCs = 16;

const char* files[] = {
  "test.mp3",
  "test1.mp3",
  "test2.mp3"
};


ArduinoOutStream cout(Serial);

SdFs sd;
FsFile file;

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

// the setup routine runs once when you press reset:
void setup() {
  pinMode(13, OUTPUT);
  Serial.begin(115200);
  while(!Serial);
  mp3.Init(14, 5);

  //mp3.Play("test.mp3", false);
  while (!sd.begin(sdCs, 12000000)) {
      Serial.println("Card failed, or not present");
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
