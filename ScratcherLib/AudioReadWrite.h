#pragma once

class TrackBuffer;

TrackBuffer* ReadAudioFromFile(const char* fileName);
void WriteAudioToFile(TrackBuffer* track, const char* fileName, int bit_rate=192000);
void DumpAudioToRawFile(TrackBuffer* track, const char* fileName);
