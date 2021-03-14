#pragma once

#include <QMutex>

class Sampler;
#include <memory>
#include <string>
#include <vector>

class Player
{
public:
	static const std::vector<std::string>& s_get_list_audio_devices(int* id_default);

	Player(Sampler* sampler, int audio_device_id);
	~Player();

	bool is_playing() const;
	bool is_eof_reached() const;
	uint64_t get_position() const;

	void stop();
	void start();
	void set_position(uint64_t pos);
	void set_audio_device(int audio_device_id);


private:
	class AudioPlayback;

	Sampler* m_sampler;
	std::unique_ptr<AudioPlayback> m_audio_playback;
	bool m_is_playing = false;
	bool m_is_eof_reached = true;

	uint64_t m_start_time;
	uint64_t m_start_pos;
	mutable QMutex m_mutex_sync;
	void _set_sync_point(uint64_t start_time, uint64_t start_pos);
	void _get_sync_point(uint64_t& start_time, uint64_t& start_pos) const;

	int m_audio_device_id;

	void _start(uint64_t pos);

};
