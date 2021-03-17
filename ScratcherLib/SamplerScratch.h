#pragma once

#include <stdio.h>
#include <vector>
#include <memory>
#include "Sampler.h"

class CHSpline;
class LinearInterpolate;
class TrackBuffer;

class SamplerScratch : public Sampler
{
public:
	SamplerScratch(TrackBuffer* buffer);
	~SamplerScratch();

	TrackBuffer* buffer() const { return m_buffer; }
	
	float start_pos() const;
	float start_slope() const;
	size_t num_control_points() const;
	void control_point(size_t i, float& x, float& y, float& slope) const;	

	void set_start_pos(float y);
	void set_start_slope(float slope);
	int add_control_point(float x, float y);
	void move_control_point(size_t i, float y);
	float move_control_point(size_t i, float x, float y);
	void set_control_point_slope(size_t i, float slope);
	void remove_control_point(size_t i);

	virtual double get_duration();
	void time_map(float x, float& y, float& slope);
	void uniform_time_samples(float interval, std::vector<float>& v);

	float start_volume() const;
	size_t num_volume_control_points() const;
	void volume_control_point(size_t i, float& x, float& y) const;

	void set_start_volume(float y);
	int add_volume_control_point(float x, float y);
	void move_volume_control_point(size_t i, float y);
	float move_volume_control_point(size_t i, float x, float y);
	void remove_volume_control_point(size_t i);

	void volume(float x, float& y);	

	void set_bgm(TrackBuffer* buffer);

	TrackBuffer* bgm() const { return m_buffer_bgm; }

	void set_bgm_volume(float vol) 
	{ 
		m_bgm_volume = vol;
	}

	float bgm_volume() const
	{
		return m_bgm_volume;
	}

	virtual void set_sample_rate(unsigned sample_rate)
	{
		m_sample_rate_out = sample_rate;
	}

	virtual bool get_sample(int i, float& l, float& r);

	void serialize(FILE* fp);
	void deserialize(FILE* fp);

private:
	TrackBuffer* m_buffer;
	unsigned m_sample_rate_in;
	unsigned m_sample_rate_out;

	std::unique_ptr<CHSpline> m_timemap;
	std::unique_ptr<LinearInterpolate> m_volume;

	TrackBuffer* m_buffer_bgm = nullptr;
	unsigned m_sample_rate_in_bgm;
	float m_bgm_volume = 1.0f;
};