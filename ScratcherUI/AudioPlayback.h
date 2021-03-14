#pragma once

#include <QIODevice>
#include <QtMultimedia/QAudioOutput>
#include "Player.h"

class Player::AudioPlayback : public QIODevice
{
	Q_OBJECT
public:
	AudioPlayback(int audioDevId, Player* player);
	~AudioPlayback();
	virtual qint64 readData(char *data, qint64 len);
	virtual qint64 writeData(const char *data, qint64 len);
	virtual qint64 bytesAvailable() const;

private:
	Player* m_player;
	QAudioFormat m_format;
	QAudioOutput* m_audioOutput;

	bool m_eof = false;
	int m_sync_count = 0;
	int m_i = 0;

private slots:
	void playbackStateChanged(QAudio::State state);
};

