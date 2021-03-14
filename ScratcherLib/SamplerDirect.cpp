#include "SamplerDirect.h"
#include "TrackBuffer.h"
#include <cstdint>
#include <cmath>

SamplerDirect::SamplerDirect(TrackBuffer* buffer)
	:m_buffer(buffer), m_sample_rate_in(buffer->Rate()), m_sample_rate_out(44100)
{
	
}

SamplerDirect::~SamplerDirect()
{

}

double SamplerDirect::get_duration()
{
	return (double)m_buffer->NumberOfSamples() / (double)m_sample_rate_in;
}

bool SamplerDirect::get_sample(int i, float& l, float& r)
{
	double step = (double)m_sample_rate_in / (double)m_sample_rate_out;
	double pos = (double)((uint64_t)i * (uint64_t)m_sample_rate_in / (uint64_t)m_sample_rate_out);
	if (pos >= (double)m_buffer->NumberOfSamples())
	{
		l = 0.0f;
		r = 0.0f;
		return false;
	}
	if (step <= 1.0)
	{
		uint32_t u_pos = (uint32_t)(pos);
		double frac = pos - (double)u_pos;
		float a[2], b[2];
		m_buffer->Sample(u_pos, a);
		m_buffer->Sample(u_pos + 1, b);
		l = a[0] * (1.0 - frac) + b[0] * frac;
		r = a[1] * (1.0 - frac) + b[1] * frac;
	}
	else
	{
		int i_pos = (int)ceil(pos - step);
		double sum_w = 0.0;
		double sum_l = 0.0;
		double sum_r = 0.0;
		while ((double)i_pos < pos + step)
		{			
			double w = step - fabs((double)i_pos - pos);
			sum_w += w;
			if (i_pos >= 0)
			{
				float v[2];
				m_buffer->Sample(i_pos, v);
				sum_l += w * v[0];
				sum_r += w * v[1];
			}
			i_pos++;
		}		
		l = sum_l / sum_w;
		r = sum_r / sum_w;
	}

	return true;
}