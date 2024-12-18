#include "Mp3Player.h"

#define VOLUME_MAX 2047

Mp3Player mp3;


//callbacks
void mp3dma_callback(Adafruit_ZeroDMA *dma) {
  mp3.dma_callback(dma);
}

void mp3DoNothing(Adafruit_ZeroDMA *dma) {
}

int mp3GetMoreData(uint8_t *writeHere, int thisManyBytes) {
  return mp3.getMoreData(writeHere, thisManyBytes);
}

void mp3DecodeCallback(int16_t *data, int len) {
  mp3.decodeCallback(data,len);
}


void Mp3Player::Init(int powerPin, int powerDelayMs = 5) {
    _powerPin = powerPin;
    _powerDelayMs = powerDelayMs;

    //set the DAC to the center of the range
    analogWrite(A0, 2048);
    analogWrite(A1, 2048);
    //begin the player
    _player.begin();

    //this will be how the player asks for data
    _player.setBufferCallback(mp3GetMoreData);
    //this will be how the player asks you to clean the data
    _player.setDecodeCallback(mp3DecodeCallback);


  }

void Mp3Player::startAmp() {
    amp.begin();

    if(_gain >= -28) {
      SetGain(_gain);
    }

    amp.setGain(_gain);
    Serial.printf("Gain at %d\n", amp.getGain());

    amp.setAGCCompression(TPA2016_AGC_4);
    amp.setLimitLevel(31);
    amp.enableChannel(true, true);
    
}

int Mp3Player::periodic() {
  if(_enabled) {
    //TODO add roll over logic
    if((millis() - _lastCommandTime) > _shutdownDelayMs) {
      shutdown();
    }
    return 1;
  }
  //periodic cleanup tasks?
  return 0;
}

void Mp3Player::shutdown() {
      Serial.printf("Mp3 shutdown at %d\n", millis());
      Stop();
      //TODO release sdcard power enable

      //TODO send mute command to amp?

      //Disable Amp Power
      digitalWrite(_powerPin, 0);
      _enabled = false;
}


int Mp3Player::Play(FsFile* file, bool user) {
  if(user) {
    _shutdownDelayMs = 5000;
  }
  else {
    _shutdownDelayMs = 1000;
  }

  _lastCommandTime = millis();

  if(!_enabled) {
    //enable sdcard power
    //TODO add logic for sdcard power
    //enable  audio player
    digitalWrite(_powerPin, 1);
    delay(_powerDelayMs);
    _enabled = true;
    startAmp();
    
  }

  _dataFile = file;

  if(_dataFile == nullptr){
    Serial.printf("File is nullptr\n");
    return -1;
  }

  static char fileName[100];
  file->getName(fileName, 100);

  Serial.printf("Playing file %s\n", fileName);

  _player.play(); //this will automatically fill the first buffer and get the channel info

  if(_player.numChannels == 1)
    initMonoDMA(); //this is a mono file
    
  else if(_player.numChannels == 2)
    initStereoDMA(); //this is a stereo file

  else{
    Serial.println("only mono and stereo files are supported");
    return -1;
  }

  //the DMA controller will dictate what happens from here on out
  _lastUpdateTime = millis();
  _rightDMA.startJob();
  _leftDMA.startJob();

  _playing=true;

  return 0;    
}

int Mp3Player::BlockingPlay(FsFile* file, bool user, bool stop) {
  if(stop) {
    mp3.Stop();
  } else {
    while(Playing()) {
      ;
    }
  }

  int status = Play(file, user);
  if(status > 0) {
    while(Playing());
  }

  //file->close();

  return status;
}

bool Mp3Player::Playing() {
    return _playing;
  }


int Mp3Player::getMoreData(uint8_t *writeHere, int thisManyBytes){
  int bytesRead = 0;
  while(_dataFile->available() && bytesRead < thisManyBytes){
    *writeHere = _dataFile->read();
    writeHere++;
    bytesRead++;
  }
  return bytesRead;
}

//this will get called when data has been decoded
void Mp3Player::decodeCallback(int16_t *data, int len){
  for(int i=0; i<len; i++){
    int val = map(*data, -32768, 32767, 0, VOLUME_MAX);
    *data++ = val;
  }
}

void Mp3Player::Stop() {
  _player.pause();
  _leftDMA.abort();
  _rightDMA.abort();
  _playing = false;
  analogWrite(A0, 0);
  analogWrite(A1, 0);
}

void Mp3Player::dma_callback(Adafruit_ZeroDMA *dma) {
  
  //digitalWrite(13, HIGH);
  //try to fill the next buffer
  if(_player.fill()){
    //stop
    _leftDMA.abort();
    _rightDMA.abort();
    _playing = false;
    analogWrite(A0, 0);
    analogWrite(A1, 0);
    return;
  }
  //heartbeat
  uint32_t currentTime = millis();
  _playTime += currentTime;
  _lastCommandTime = currentTime;
  //digitalWrite(13, LOW);
}


void Mp3Player::initMonoDMA(){
  //set up the DMA channels
  _leftDMA.setTrigger(MP3_DMA_TRIGGER);
  _leftDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = _leftDMA.allocate();

  //ask for the buffers we're going to use
  _player.getBuffers(&ping, &pong);

  //make the descriptors
  desc = _leftDMA.addDescriptor(
    ping,                    // move data from here
    (void *)(&DAC->DATA[0]), // to here (M4)
    MP3_OUTBUF_SIZE,
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false);
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = _leftDMA.addDescriptor(
    pong,                    // move data from here
    (void *)(&DAC->DATA[0]), // to here (M4)
    MP3_OUTBUF_SIZE,
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false);                   // increment dest addr?
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  //make the descriptor list loop
  _leftDMA.loop(true);
  _leftDMA.setCallback(mp3dma_callback);

  _rightDMA.setTrigger(MP3_DMA_TRIGGER);
  _rightDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = _rightDMA.allocate();

  //make the descriptors
  desc = _rightDMA.addDescriptor(
    ping + 1,                    // move data from here
    (void *)(&DAC->DATA[1]), // to here (M4)
    MP3_OUTBUF_SIZE,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false);                           // increment dest addr?
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = _rightDMA.addDescriptor(
    pong + 1,                    // move data from here
    (void *)(&DAC->DATA[1]), // to here (M4)
    MP3_OUTBUF_SIZE,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false);                           // increment dest addr?
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  //make the descriptor list loop
  _rightDMA.loop(true);
  _rightDMA.setCallback(mp3DoNothing);
}

void Mp3Player::initStereoDMA(){
    //######### LEFT CHANNEL DMA ##############//
  
  //set up the DMA channels
  _leftDMA.setTrigger(MP3_DMA_TRIGGER);
  _leftDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = _leftDMA.allocate();

  //ask for the buffers we're going to use
  _player.getBuffers(&ping, &pong);

  //make the descriptors
  desc = _leftDMA.addDescriptor(
    ping,                    // move data from here
    (void *)(&DAC->DATA[0]), // to here (M4)
    MP3_OUTBUF_SIZE >> 1,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           // increment dest addr?

  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = _leftDMA.addDescriptor(
    pong,                    // move data from here
    (void *)(&DAC->DATA[0]), // to here (M4)
    MP3_OUTBUF_SIZE >> 1,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           // increment dest addr?
    
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  //make the descriptor list loop
  _leftDMA.loop(true);
  _leftDMA.setCallback(mp3dma_callback);

  //######### RIGHT CHANNEL DMA ##############//

  _rightDMA.setTrigger(MP3_DMA_TRIGGER);
  _rightDMA.setAction(DMA_TRIGGER_ACTON_BEAT);
  stat = _rightDMA.allocate();

  //make the descriptors
  desc = _rightDMA.addDescriptor(
    ping + 1,                    // move data from here
    (void *)(&DAC->DATA[1]), // to here (M4)
    MP3_OUTBUF_SIZE >> 1,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           // increment dest addr?
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  desc = _rightDMA.addDescriptor(
    pong + 1,                    // move data from here
    (void *)(&DAC->DATA[1]), // to here (M4)
    MP3_OUTBUF_SIZE >> 1,                      // this many...
    DMA_BEAT_SIZE_HWORD,               // bytes/hword/words
    true,                             // increment source addr?
    false,
    DMA_ADDRESS_INCREMENT_STEP_SIZE_2,
    DMA_STEPSEL_SRC);                           // increment dest addr?
  desc->BTCTRL.bit.BLOCKACT = DMA_BLOCK_ACTION_INT;

  //make the descriptor list loop
  _rightDMA.loop(true);
  _rightDMA.setCallback(mp3DoNothing);
}