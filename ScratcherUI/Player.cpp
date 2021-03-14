#include <cstdint>
#include <chrono>
#include <QIODevice>
#include <QtMultimedia/QAudioOutput>
#include <QAudioDeviceInfo>
#include "Player.h"
#include "AudioPlayback.h"
#include "Sampler.h"

inline uint64_t time_micro_sec()
{
	std::chrono::time_point<std::chrono::system_clock> tpSys = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> tpMicro
		= std::chrono::time_point_cast<std::chrono::microseconds>(tpSys);
	return tpMicro.time_since_epoch().count();
}

const std::vector<std::string>& Player::s_get_list_audio_devices(int* id_default)
{
	static std::vector<std::string> s_list_devices;
	static int s_id_default;
	if (s_list_devices.size() == 0)
	{
		auto lst = QAudioDeviceInfo::availableDevices(QAudio::Mode::AudioOutput);
		auto def = QAudioDeviceInfo::defaultOutputDevice();

		for (int i = 0; i < lst.size(); i++)
		{
			const auto& info = lst[i];
			s_list_devices.push_back(info.deviceName().toLocal8Bit().constData());
			if (info == def)
			{
				s_id_default = i;
			}
		}
	}
	if (id_default != nullptr)
	{
		*id_default = s_id_default;
	}
	return s_list_devices;
}

Player::Player(Sampler* sampler, int audio_device_id)
	: m_sampler(sampler)
	, m_audio_device_id(audio_device_id)
{
	m_start_pos = 0;
	m_start_time = (uint64_t)(-1);
}

Player::~Player()
{
	stop();
}


bool Player::is_playing() const
{
	return m_audio_playback != nullptr;
}

bool Player::is_eof_reached() const
{
	return m_audio_playback != nullptr && m_is_eof_reached;
}


uint64_t Player::get_position() const
{
	if (m_audio_playback == nullptr)
	{
		return m_start_pos;
	}
	else if (m_is_eof_reached)
	{
		return (uint64_t)(m_sampler->get_duration()*1000000.0);
	}
	else
	{
		uint64_t start_time, start_pos;
		_get_sync_point(start_time, start_pos);
		if (start_time == (uint64_t)(-1)) return start_pos;
		return start_pos + (time_micro_sec() - start_time);
	}
}

void Player::stop()
{
	if (m_audio_playback != nullptr)
	{
		uint64_t pos = get_position();
		m_is_playing = false;
		m_audio_playback = nullptr;
		m_start_pos = pos;
	}
}

void Player::_start(uint64_t pos)
{
	stop();
	m_start_time = (uint64_t)(-1);
	m_start_pos = pos;

	m_is_playing = true;
	m_is_eof_reached = false;
	m_audio_playback = (std::unique_ptr<AudioPlayback>)(new AudioPlayback(m_audio_device_id, this));
}


void Player::start()
{
	if (m_audio_playback == nullptr)
	{
		_start(m_start_pos);
	}
}

void Player::set_position(uint64_t pos)
{

	if (m_audio_playback != nullptr)
	{
		_start(pos);
	}
	else
	{
		m_start_pos = pos;
	}

}

void Player::set_audio_device(int audio_device_id)
{
	if (audio_device_id != m_audio_device_id)
	{
		m_audio_device_id = audio_device_id;
		if (m_audio_playback != nullptr)
		{
			m_is_playing = false;
			m_audio_playback = nullptr;
			m_is_playing = true;
			m_is_eof_reached = false;
			m_audio_playback = (std::unique_ptr<AudioPlayback>)(new AudioPlayback(m_audio_device_id, this));
		}
	}
}

void Player::_set_sync_point(uint64_t start_time, uint64_t start_pos)
{
	m_mutex_sync.lock();
	m_start_time = start_time;
	m_start_pos = start_pos;
	m_mutex_sync.unlock();
}

void Player::_get_sync_point(uint64_t& start_time, uint64_t& start_pos) const
{
	m_mutex_sync.lock();
	start_time = m_start_time;
	start_pos = m_start_pos;
	m_mutex_sync.unlock();
}