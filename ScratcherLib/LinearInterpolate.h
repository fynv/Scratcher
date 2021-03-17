#pragma once

#include <stdio.h>
#include <vector>

class LinearInterpolate
{
public:
	LinearInterpolate() {}
	~LinearInterpolate() {}
	
	struct Sample
	{
		float x;
		float y;
	};

	size_t num_samples() const { return m_samples.size(); }
	const Sample& sample(size_t i) const { return m_samples[i]; }

	int Add(float x, float y);
	void Move(size_t i, float y);
	float Move(size_t i, float x, float y);
	void Remove(size_t i);

	bool f(float x, float& y);

	void serialize(FILE* fp);
	void deserialize(FILE* fp);

private:
	std::vector<Sample> m_samples;
};
