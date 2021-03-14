#pragma once

#include <stdio.h>
#include <vector>

class CHSpline
{
public:
	CHSpline() {}
	~CHSpline() {}

	struct Sample
	{
		float x;
		float y;
		float slope;
	};

	size_t num_samples() const { return m_samples.size(); }
	const Sample& sample(size_t i) const { return m_samples[i]; }
	void left_bound(float& x, float& y, float& slope);
	void right_bound(float& x, float& y, float& slope);

	int Add(float x, float y);
	void Move(size_t i, float y);
	void SetSlope(size_t i, float slope);
	void Remove(size_t i);

	bool f(float x, float& y, float& slope);
	void uniform_samples(float interval, float start_x, std::vector<float>& v);

	void serialize(FILE* fp);
	void deserialize(FILE* fp);

private:
	std::vector<Sample> m_samples;
};