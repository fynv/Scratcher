#pragma once

class TrackBuffer;
class Sampler;
void SampleToTrackBuffer(Sampler& sampler, TrackBuffer& track, int buf_size=1024);
