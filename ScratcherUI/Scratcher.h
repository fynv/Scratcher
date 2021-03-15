#pragma once

#include <memory>
#include <QMainWindow>
#include <QTimer>
#include "ui_Scratcher.h"

class TrackBuffer;
class SamplerScratch;
class Player;
class Scratcher : public QMainWindow
{
	Q_OBJECT
public:
	Scratcher();
	virtual ~Scratcher();

protected:
	virtual void closeEvent(QCloseEvent *event) override;

private:
	Ui_Scratcher m_ui;
	QTimer *m_timer;

	void InitAudioDevice();
	void Unload();
	void LoadSource(const QString& filename);
	void LoadBgm(const QString& filename);
	void SaveResult(const QString& filename);

	void Play();
	void Stop();

	void TimeMapTrackPositionH();
	void TimeMapTrackPositionV();
	void VolumeTrackPositionH();
	void BgmTrackPositionH();


private slots:
	void refresh();
	void AudioDeviceChange(int idx);
	void Btn_load_src_Click();
	void Btn_load_bgm_Click();
	void Btn_save_res_Click();
	void Btn_play_stop_Click();
	void Btn_reset_Click();

	void TimeMapHScrollRange(int min_v, int max_v);
	void TimeMapVScrollRange(int min_v, int max_v);
	void TimeMapHScroll(int v);
	void TimeMapVScroll(int v);

	void VolumeHScrollRange(int min_v, int max_v);
	void VolumeHScroll(int v);

	void BgmHScroll(int v);

	void Btn_zoom_in_h_Click();
	void Btn_zoom_out_h_Click();
	void Btn_zoom_in_v_Click();
	void Btn_zoom_out_v_Click();

	void Slider_baseline_Moved(int v);
	void Baseline_changed();

	void Cursor_Moved(double pos);

	void Slider_bgm_volume_Moved(int v);

private:	
	bool m_changed = false;
	QString m_fullpath = "";
	QString m_filename = "";
	void update_title();
	void set_cur_filename(QString fullpath);
	void set_changed(bool changed);
	bool prompt_save();
	void do_open_project(QString fullpath);
	void do_save_project(QString fullpath);
	void set_bgm_visible(bool show);

private slots:
	bool new_project();
	bool open_project();	
	bool save_project_as();
	bool save_project();
	void set_changed() { set_changed(true); }

private:
	QString m_filename_source = "";
	QString m_filename_bgm = "";
	std::unique_ptr<TrackBuffer> m_src_buffer;
	std::unique_ptr<TrackBuffer> m_bgm_buffer;
	std::unique_ptr<SamplerScratch> m_sampler;
	std::unique_ptr<Player> m_player;
	bool m_is_playing = false;
	double m_cursor_pos = 0.0;

	void _set_cursor_pos(double pos);
	void _emit_cursor_pos(double pos);
};
