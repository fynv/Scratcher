#include <algorithm>
#include "LinearInterpolate.h"

int LinearInterpolate::Add(float x, float y)
{
	Sample temp = { x, y  };
	auto iter = std::upper_bound(m_samples.begin(), m_samples.end(), temp, [](const Sample& lhs, const Sample& rhs) -> bool { return lhs.x < rhs.x; });
	size_t i = m_samples.size();
	if (iter != m_samples.end())
		i = iter - m_samples.begin();
	if (i > 0 && m_samples[i - 1].x == x)
	{
		Move(i - 1, y);
		return i - 1;
	}
	else
	{
		m_samples.insert(m_samples.begin() + i, temp);
		return i;
	}
}

void LinearInterpolate::Move(size_t i, float y)
{
	m_samples[i].y = y;
}

float LinearInterpolate::Move(size_t i, float x, float y)
{
	if (i > 0 && x < m_samples[i - 1].x) x = m_samples[i - 1].x;
	if (i < m_samples.size() - 1 && x > m_samples[i + 1].x) x = m_samples[i + 1].x;
	m_samples[i].x = x;
	m_samples[i].y = y;
	return x;
}

void LinearInterpolate::Remove(size_t i)
{
	m_samples.erase(m_samples.begin() + i);
}

bool LinearInterpolate::f(float x, float& y)
{
	Sample temp = { x, y };
	auto iter = std::upper_bound(m_samples.begin(), m_samples.end(), temp, [](const Sample& lhs, const Sample& rhs) -> bool { return lhs.x < rhs.x; });
	if (iter == m_samples.begin() || iter == m_samples.end()) return false;
	size_t i = iter - m_samples.begin() - 1;

	float x0 = m_samples[i].x;
	float y0 = m_samples[i].y;
	float x1 = m_samples[i + 1].x;
	float y1 = m_samples[i + 1].y;
	float t = (x - x0) / (x1 - x0);
	y = y0 * (1.0f - t) + y1 * t;

	return true;
}

void LinearInterpolate::serialize(FILE* fp)
{
	int num_samples = (int)(m_samples.size());
	fwrite(&num_samples, sizeof(int), 1, fp);
	fwrite(m_samples.data(), sizeof(Sample), m_samples.size(), fp);
}

void LinearInterpolate::deserialize(FILE* fp)
{
	int num_samples;
	fread(&num_samples, sizeof(int), 1, fp);
	m_samples.resize(num_samples);
	fread(m_samples.data(), sizeof(Sample), m_samples.size(), fp);
}

