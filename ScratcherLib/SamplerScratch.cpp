#include "SamplerScratch.h"
#include "TrackBuffer.h"
#include "CHSpline.h"
#include "LinearInterpolate.h"
#include <cstdint>
#include <cmath>

SamplerScratch::SamplerScratch(TrackBuffer* buffer)
	: m_buffer(buffer), m_sample_rate_in(buffer->Rate()), m_sample_rate_out(44100), m_timemap(new CHSpline), m_volume(new LinearInterpolate)
{
	m_timemap->Add(0.0f, 0.0f);
	m_volume->Add(0.0f, 1.0f);
}

SamplerScratch::~SamplerScratch()
{

}

float SamplerScratch::start_pos() const
{
	return m_timemap->sample(0).y;
}

float SamplerScratch::start_slope() const
{
	return m_timemap->sample(0).slope;
}

size_t SamplerScratch::num_control_points() const
{
	return m_timemap->num_samples() - 1;
}

void SamplerScratch::control_point(size_t i, float& x, float& y, float& slope) const
{
	const auto& sample = m_timemap->sample(i + 1);
	x = sample.x;
	y = sample.y;
	slope = sample.slope;
}


void SamplerScratch::set_start_pos(float y)
{
	m_timemap->Move(0, y);
}

void SamplerScratch::set_start_slope(float slope)
{
	m_timemap->SetSlope(0, slope);
}

int SamplerScratch::add_control_point(float x, float y)
{
	return m_timemap->Add(x, y) - 1;
}

void SamplerScratch::move_control_point(size_t i, float y)
{
	m_timemap->Move(i + 1, y);
}

float SamplerScratch::move_control_point(size_t i, float x, float y)
{
	return m_timemap->Move(i + 1, x, y);
}

void SamplerScratch::set_control_point_slope(size_t i, float slope)
{
	m_timemap->SetSlope(i + 1, slope);
}


void SamplerScratch::remove_control_point(size_t i)
{
	m_timemap->Remove(i + 1);
}

double SamplerScratch::get_duration()
{
	static float acc = 5.0f;
	float x0, y0, slope0;
	m_timemap->right_bound(x0, y0, slope0);

	float d_s = 1.0f - slope0;
	float A = acc;
	if (d_s < 0.0f) A = -acc;
	float dur = d_s / A;

	y0 = (0.5f * A)*dur*dur + slope0 * dur + y0;
	double remain = (double)m_buffer->NumberOfSamples() / (double)m_sample_rate_in - y0;
	if (remain < 0.0) remain = 0.0;
	if (remain > 1.5) remain = 1.5;
	return (double)(x0 + dur) + remain;
}

void SamplerScratch::time_map(float x, float& y, float& slope)
{
	bool res = m_timemap->f(x, y, slope);
	if (!res)
	{
		static float acc = 5.0f;
		float x0, y0, slope0;
		m_timemap->right_bound(x0, y0, slope0);

		float t = x - x0;

		float d_s = 1.0f - slope0;
		float A = acc;
		if (d_s < 0.0f) A = -acc;
		float dur = d_s / A;

		if (t < dur)
		{
			y = (0.5f * A)*t*t + slope0 * t + y0;
			slope = A * t + slope0;
		}
		else
		{
			y = (0.5f * A)*dur*dur + slope0 * dur + y0 + (t - dur);
			slope = 1.0f;
		}
	}	
}

void SamplerScratch::uniform_time_samples(float interval, std::vector<float>& v)
{
	m_timemap->uniform_samples(interval, 0.0f, v);
	float x = interval * (float)v.size();

	static float acc = 5.0f;
	float x0, y0, slope0;
	m_timemap->right_bound(x0, y0, slope0);

	float d_s = 1.0f - slope0;
	float A = acc;
	if (d_s < 0.0f) A = -acc;
	float dur = d_s / A;

	while (x < x0 + dur)
	{
		float t = x - x0;
		float y = (0.5f * A)*t*t + slope0 * t + y0;
		v.push_back(y);
		x += interval;
	}

	y0 = (0.5f * A)*dur*dur + slope0 * dur + y0;

	double total = get_duration();
	while (x<total)
	{
		float y = x - (x0 + dur) + y0;	
		v.push_back(y);
		x += interval;
	}
}

float SamplerScratch::start_volume() const
{
	return m_volume->sample(0).y;
}

size_t SamplerScratch::num_volume_control_points() const
{
	return m_volume->num_samples() - 1;
}

void SamplerScratch::volume_control_point(size_t i, float& x, float& y) const
{
	const auto& sample = m_volume->sample(i + 1);
	x = sample.x;
	y = sample.y;
}

void SamplerScratch::set_start_volume(float y)
{
	m_volume->Move(0, y);
}

int SamplerScratch::add_volume_control_point(float x, float y)
{
	return m_volume->Add(x, y) - 1;
}

void SamplerScratch::move_volume_control_point(size_t i, float y)
{
	m_volume->Move(i + 1, y);
}

float SamplerScratch::move_volume_control_point(size_t i, float x, float y)
{
	return m_volume->Move(i + 1, x, y);
}

void SamplerScratch::remove_volume_control_point(size_t i)
{
	m_volume->Remove(i + 1);
}

void SamplerScratch::volume(float x, float& y)
{
	bool res = m_volume->f(x, y);
	if (!res)
	{
		if (x < 0.0f)
		{
			y = m_volume->sample(0).y;
		}
		else
		{
			y = m_volume->sample(m_volume->num_samples() - 1).y;
		}			 
	}
}

void SamplerScratch::set_bgm(TrackBuffer* buffer)
{
	m_buffer_bgm = buffer;
	if (m_buffer_bgm != nullptr)
		m_sample_rate_in_bgm = buffer->Rate();
}

bool SamplerScratch::get_sample(int i, float& l, float& r)
{
	l = 0.0f;
	r = 0.0f;
	float t_out = (float)i / (float)m_sample_rate_out;
	if (t_out >= get_duration()) return false;	

	do
	{
		float t_in, changing_rate;
		time_map(t_out, t_in, changing_rate);
		double pos = t_in * (double)m_sample_rate_in;
		double step = fabs(changing_rate * (double)m_sample_rate_in / (double)m_sample_rate_out);
		if (pos < 0.0 || pos >= (double)m_buffer->NumberOfSamples()) break;

		float v_l, v_r;

		if (step <= 1.0)
		{
			uint32_t u_pos = (uint32_t)(pos);
			double frac = pos - (double)u_pos;
			float a[2], b[2];
			m_buffer->Sample(u_pos, a);
			m_buffer->Sample(u_pos + 1, b);
			v_l = a[0] * (1.0 - frac) + b[0] * frac;
			v_r = a[1] * (1.0 - frac) + b[1] * frac;
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
			v_l = sum_l / sum_w;
			v_r = sum_r / sum_w;
		}

		float amp;
		volume(t_out, amp);
		v_l *= amp;
		v_r *= amp;

		l += v_l;
		r += v_r;

	} while (false);

	if (m_buffer_bgm != nullptr)
	{
		do
		{
			double step = (double)m_sample_rate_in_bgm / (double)m_sample_rate_out;
			double pos = (double)((uint64_t)i * (uint64_t)m_sample_rate_in_bgm / (uint64_t)m_sample_rate_out);
			if (pos >= (double)m_buffer_bgm->NumberOfSamples()) break;

			float v_l, v_r;

			if (step <= 1.0)
			{
				uint32_t u_pos = (uint32_t)(pos);
				double frac = pos - (double)u_pos;
				float a[2], b[2];
				m_buffer_bgm->Sample(u_pos, a);
				m_buffer_bgm->Sample(u_pos + 1, b);
				v_l = a[0] * (1.0 - frac) + b[0] * frac;
				v_r = a[1] * (1.0 - frac) + b[1] * frac;
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
						m_buffer_bgm->Sample(i_pos, v);
						sum_l += w * v[0];
						sum_r += w * v[1];
					}
					i_pos++;
				}
				v_l = sum_l / sum_w;
				v_r = sum_r / sum_w;
			}

			v_l *= m_bgm_volume;
			v_r *= m_bgm_volume;

			l += v_l;
			r += v_r;

		} while (false);
	}

	return true;

}

void SamplerScratch::serialize(FILE* fp)
{
	m_timemap->serialize(fp);
	m_volume->serialize(fp);
	fwrite(&m_bgm_volume, sizeof(float), 1, fp);
}

void SamplerScratch::deserialize(FILE* fp)
{
	m_timemap->deserialize(fp);
	m_volume->deserialize(fp);
	fread(&m_bgm_volume, sizeof(float), 1, fp);
}