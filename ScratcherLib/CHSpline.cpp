#include <algorithm>
#include "CHSpline.h"

void CHSpline::left_bound(float& x, float& y, float& slope)
{
	const auto& sample = m_samples[0];
	x = sample.x;
	y = sample.y;
	slope = sample.slope;
}

void CHSpline::right_bound(float& x, float& y, float& slope)
{
	const auto& sample = m_samples[m_samples.size() - 1];
	x = sample.x;
	y = sample.y;
	slope = sample.slope;
}

int CHSpline::Add(float x, float y)
{
	Sample temp = { x, y, 0.0f };
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


void CHSpline::Move(size_t i, float y)
{
	m_samples[i].y = y;
}

float CHSpline::Move(size_t i, float x, float y)
{
	if (i > 0 && x < m_samples[i - 1].x) x = m_samples[i - 1].x;
	if (i < m_samples.size()-1 && x > m_samples[i + 1].x) x = m_samples[i + 1].x;
	m_samples[i].x = x;
	m_samples[i].y = y;
	return x;
}

void CHSpline::SetSlope(size_t i, float slope)
{
	m_samples[i].slope = slope;
}

void CHSpline::Remove(size_t i)
{
	m_samples.erase(m_samples.begin() + i);
}


bool CHSpline::f(float x, float& y, float& slope)
{
	Sample temp = { x, y, 0.0f };
	auto iter = std::upper_bound(m_samples.begin(), m_samples.end(), temp, [](const Sample& lhs, const Sample& rhs) -> bool { return lhs.x < rhs.x; });
	if (iter == m_samples.begin() || iter == m_samples.end()) return false;
	size_t i = iter - m_samples.begin() - 1;

	float x0 = m_samples[i].x;
	float y0 = m_samples[i].y;
	float s0 = m_samples[i].slope;
	float x1 = m_samples[i + 1].x;
	float y1 = m_samples[i + 1].y;
	float s1 = m_samples[i + 1].slope;
	float t = (x - x0) / (x1 - x0);
	float t2 = t * t;
	float t3 = t2 * t;


	y = (2.0f*t3 - 3.0f*t2 + 1)*y0 + (-2.0f*t3 + 3.0f*t2)*y1
		+ (x1 - x0)*((t3 - 2.0f*t2 + t) * s0 + (t3 - t2) * s1);

	slope = ((6.0f*t2 - 6.0f*t) * y0 + (-6.0f*t2 + 6.0f*t)*y1) / (x1 - x0)
		+ (3.0f*t2 - 4.0f*t + 1.0f)*s0 + (3.0f*t2 - 2.0f*t)*s1;

	return true;
}

void CHSpline::uniform_samples(float interval, float start_x, std::vector<float>& v)
{
	Sample temp = { start_x, 0.0f, 0.0f };
	auto iter = std::upper_bound(m_samples.begin(), m_samples.end(), temp, [](const Sample& lhs, const Sample& rhs) -> bool { return lhs.x < rhs.x; });
	if (iter == m_samples.begin() || iter == m_samples.end()) return;
	size_t i = iter - m_samples.begin() - 1;
	float x = start_x;
	v.clear();
	while (i < m_samples.size() - 1)
	{
		float x0 = m_samples[i].x;
		float y0 = m_samples[i].y;
		float s0 = m_samples[i].slope;
		float x1 = m_samples[i + 1].x;
		float y1 = m_samples[i + 1].y;
		float s1 = m_samples[i + 1].slope;

		while (x < x1)
		{
			float t = (x - x0) / (x1 - x0);
			float t2 = t * t;
			float t3 = t2 * t;

			float y = (2.0f*t3 - 3.0f*t2 + 1)*y0 + (-2.0f*t3 + 3.0f*t2)*y1
				+ (x1 - x0)*((t3 - 2.0f*t2 + t) * s0 + (t3 - t2) * s1);

			v.push_back(y);

			x += interval;
		}
		i++;
	}
}


void CHSpline::serialize(FILE* fp)
{
	int num_samples = (int)(m_samples.size());
	fwrite(&num_samples, sizeof(int), 1, fp);
	fwrite(m_samples.data(), sizeof(Sample), m_samples.size(), fp);
}

void CHSpline::deserialize(FILE* fp)
{
	int num_samples;
	fread(&num_samples, sizeof(int), 1, fp);
	m_samples.resize(num_samples);
	fread(m_samples.data(), sizeof(Sample), m_samples.size(), fp);
}



