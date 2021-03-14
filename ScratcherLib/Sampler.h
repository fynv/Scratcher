#pragma once

class Sampler
{
public:
	Sampler() {}
	virtual ~Sampler() {}
	virtual double get_duration() = 0;
	virtual void set_sample_rate(unsigned sample_rate) = 0;
	virtual bool get_sample(int i, float& l, float& r) = 0;
};
