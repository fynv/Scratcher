#include <memory.h>
#include "SampleToTrackBuffer.h"
#include "Sampler.h"
#include "TrackBuffer.h"

void SampleToTrackBuffer(Sampler& sampler, TrackBuffer& track, int buf_size)
{
	int sample_rate = track.Rate();
	sampler.set_sample_rate(sample_rate);	

	NoteBuffer buf;
	buf.m_sampleRate = sample_rate;
	buf.m_channelNum = 2;
	buf.m_sampleNum = buf_size;
	buf.m_cursorDelta = (float)buf_size;
	buf.Allocate();

	bool reading = true;
	int pos = 0;
	while (reading)
	{
		int i = 0;
		for (; i < buf_size; i++)
		{
			float l, r;
			bool res = sampler.get_sample(pos, l, r);
			pos++;
			if (!res)
			{
				reading = false;
				break;
			}
			buf.m_data[i * 2] = l;
			buf.m_data[i * 2 + 1] = r;
		}
		if (i < buf_size)
		{
			memset(buf.m_data + i * 2, 0, (buf_size - i) * sizeof(float)* 2);
		}
		track.WriteBlend(buf);
	}	
}
