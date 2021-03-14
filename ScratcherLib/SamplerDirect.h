#pragma once

#include "Sampler.h"

class TrackBuffer;
class SamplerDirect : public Sampler
{
public:
	SamplerDirect(TrackBuffer* buffer);
	~SamplerDirect();

	virtual double get_duration();

	virtual void set_sample_rate(unsigned sample_rate)
	{
		m_sample_rate_out = sample_rate;
	}

	virtual bool get_sample(int i, float& l, float& r);

private:
	TrackBuffer* m_buffer;
	unsigned m_sample_rate_in;
	unsigned m_sample_rate_out;	
};
