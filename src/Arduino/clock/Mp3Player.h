#pragma once

//TODO only pull in needed headers in header

#include <Adafruit_MP3.h>
#include <assembly.h>
#include <coder.h>
#include <mp3common.h>
#include <mp3dec.h>
#include <mpadecobjfixpt.h>
#include <statname.h>

#include <Wire.h>
#include "Adafruit_TPA2016.h"

#include <SPI.h>
#include <SdFat.h>
#include <Adafruit_ZeroDMA.h>
#include "utility/dma.h"




class Mp3Player {
  public:
  void Init(int powerPin, int powerDelayMs);
  int BlockingPlay(FsFile* file, bool user);
  int Play(FsFile* file, bool user);
  int QueueNext(const char* file);
  bool Playing();
  void Stop();
  int getPlayTimeMs() { return _playTime; }
  void resetPlayTime() { _playTime = 0;}
  void SetGain(int8_t gain) {
    _gain = gain;
    amp.setGain(_gain);
  }
  void SetVolume(float volume) {
    SetGain(map(volume, 0, 100.0, -29, 30));
  }
  void mute(bool enMute);
  void shutdown();
  void startAmp();

  int periodic();

  //TODO make these private and allowed in callbacks in future
  public:
  //this gets called when the player wants more data
  int getMoreData(uint8_t *writeHere, int thisManyBytes);

  //this will get called when data has been decoded
  void decodeCallback(int16_t *data, int len);

  void dma_callback(Adafruit_ZeroDMA *dma);

  void initMonoDMA();

  void initStereoDMA();

  private:

  Adafruit_ZeroDMA _leftDMA;
  Adafruit_ZeroDMA _rightDMA;
  ZeroDMAstatus    stat; // DMA status codes returned by some functions
  FsFile* _dataFile;
  Adafruit_MP3_DMA _player;
  DmacDescriptor *desc;
  bool _playing = false;

  Adafruit_TPA2016 amp = Adafruit_TPA2016();

  //the output data buffers
  int16_t *ping, *pong;
  //pin to enable
  int _powerPin;
  int _powerDelayMs;
  int _shutdownDelayMs = 100;
  int _lastCommandTime;
  bool _enabled = false;
  uint32_t _playTime = 0;
  uint32_t _lastUpdateTime = 0;
  int8_t _gain = 6;
};


extern Mp3Player mp3;