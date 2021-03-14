#include <cstdint>
#include <chrono>
#include "Sampler.h"
#include "AudioPlayback.h"

inline uint64_t time_micro_sec()
{
	std::chrono::time_point<std::chrono::system_clock> tpSys = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMicro
		= std::chrono::time_point_cast<std::chrono::microseconds>(tpSys);
	return tpMicro.time_since_epoch().count();
}

Player::AudioPlayback::AudioPlayback(int audioDevId, Player* player) : m_player(player)
{
	open(QIODevice::ReadOnly| QIODevice::Unbuffered);

	auto lst = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput);
	const auto& dev = lst[audioDevId];

	uint32_t sample_rate = 8000;
	auto lst_prop = dev.supportedSampleRates();
	for (int i = 0; i < lst_prop.size(); i++)
	{
		if (lst_prop[i] > sample_rate)
			sample_rate = lst_prop[i];
	}	

	m_i = (int)((double)m_player->m_start_pos / 1000000.0 *  (double)sample_rate);
	player->m_sampler->set_sample_rate(sample_rate);

	m_format.setSampleRate(sample_rate);
	m_format.setChannelCount(2);
	m_format.setSampleSize(32);
	m_format.setCodec("audio/pcm");
	m_format.setByteOrder(QAudioFormat::LittleEndian);
	m_format.setSampleType(QAudioFormat::Float);

	m_audioOutput = new QAudioOutput(dev, m_format, this);
	m_audioOutput->start(this);

	connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(playbackStateChanged(QAudio::State)));

}

Player::AudioPlayback::~AudioPlayback()
{
	m_audioOutput->stop();
	close();
}

qint64 Player::AudioPlayback::readData(char *data, qint64 len)
{
	float* buf = (float*)data;
	size_t buf_size = len / sizeof(float) / 2;

	Sampler* sampler = m_player->m_sampler;

	if (!m_player->m_is_playing || m_eof)
	{
		memset(buf, 0, sizeof(float)*buf_size * 2);
		return 0;
	}

	size_t i = 0;
	for (; i < buf_size; i++, m_i++)
	{
		float l, r;
		bool res = sampler->get_sample(m_i, l, r);
		if (!res)
		{
			m_eof = true;
			break;
		}
		buf[i * 2] = l;
		buf[i * 2 + 1] = r;
	}

	if (i < buf_size)
	{
		float* p_out = buf + i * 2;
		size_t bytes = sizeof(float) * (buf_size - i) * 2;
		memset(p_out, 0, bytes);
	}

	static int s_sync_interval = 10;	
	if (m_sync_count == 0)
	{
		uint32_t sample_rate = (uint32_t)m_format.sampleRate();
		uint64_t start_time = time_micro_sec();
		uint64_t start_pos = (uint64_t)((double)m_i / (double)sample_rate* 1000000.0);
		m_player->_set_sync_point(start_time, start_pos);
	}
	m_sync_count = (m_sync_count + 1) % s_sync_interval;

	return len;
}

qint64 Player::AudioPlayback::writeData(const char *data, qint64 len)
{
	Q_UNUSED(data);
	Q_UNUSED(len);

	return 0;
}

qint64 Player::AudioPlayback::bytesAvailable() const
{
	if (!m_player->m_is_playing || m_eof)
	{
		return 0;
	}
	else
	{
		return 40000 * sizeof(short) + QIODevice::bytesAvailable();
	}

}

void Player::AudioPlayback::playbackStateChanged(QAudio::State state)
{
	if (state == QAudio::State::IdleState)
	{		
		m_player->m_is_eof_reached = true;
	}
}

