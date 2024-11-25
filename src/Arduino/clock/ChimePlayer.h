#pragma once

//Chime mode flags
//Bit packed for chime mode

class ChimePlayer {

void PlayRandomChime(char* folder, int durationMs);
void PlayChimeOrder(char* folder, int durationMs, int index);

void ChimeVolume(float volume);

bool ChimeDone(int* indexOut);

//Sets callback on track change
void setTrachChangeCallback(void (*callback)(FILE *) = NULL, int index);

void periodic();

private:


};