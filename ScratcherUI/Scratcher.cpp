#define USE_JSON 1

#include <QFileDialog>
#include <QScrollBar>
#include <QFileInfo>
#include <QMessageBox>
#include <QCloseEvent>

#if USE_JSON
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#endif

#include <TrackBuffer.h>
#include <SamplerScratch.h>
#include <AudioReadWrite.h>
#include <SampleToTrackBuffer.h>

#include <stdio.h>
#include <string>
#include <vector>

#include "Scratcher.h"
#include "Player.h"

void serialize_string(FILE* fp, const QString& str)
{
	std::string sstr = str.toLocal8Bit().constData();
	int len = (int)sstr.length();
	fwrite(&len, sizeof(int), 1, fp);
	fwrite(sstr.c_str(), 1, len, fp);
}

QString deserialize_string(FILE *fp)
{
	int len;
	fread(&len, sizeof(int), 1, fp);
	std::vector<char> sstr(len + 1, 0);
	fread(sstr.data(), 1, len, fp);
	return QString::fromLocal8Bit(sstr.data());
}

Scratcher::Scratcher()
{
	m_ui.setupUi(this);
	connect(m_ui.btn_load_src, SIGNAL(clicked()), this, SLOT(Btn_load_src_Click()));
	connect(m_ui.btn_load_bgm, SIGNAL(clicked()), this, SLOT(Btn_load_bgm_Click()));
	connect(m_ui.btn_save_res, SIGNAL(clicked()), this, SLOT(Btn_save_res_Click()));
	connect(m_ui.btn_audio_play, SIGNAL(clicked()), this, SLOT(Btn_play_stop_Click()));
	connect(m_ui.btn_audio_reset, SIGNAL(clicked()), this, SLOT(Btn_reset_Click()));
	
	InitAudioDevice();

	connect(m_ui.combo_audio_device, SIGNAL(currentIndexChanged(int)), this, SLOT(AudioDeviceChange(int)));
	connect(m_ui.scroll_timemap->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(TimeMapHScrollRange(int,int)));
	connect(m_ui.scroll_timemap->verticalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(TimeMapVScrollRange(int, int)));
	connect(m_ui.scroll_timemap->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(TimeMapHScroll(int)));
	connect(m_ui.scroll_timemap->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(TimeMapVScroll(int)));

	connect(m_ui.scroll_volume->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)), this, SLOT(VolumeHScrollRange(int, int)));
	connect(m_ui.scroll_volume->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(VolumeHScroll(int)));

	connect(m_ui.scroll_bgm->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(BgmHScroll(int)));
	
	connect(m_ui.btn_zoom_in_h, SIGNAL(clicked()), this, SLOT(Btn_zoom_in_h_Click()));
	connect(m_ui.btn_zoom_out_h, SIGNAL(clicked()), this, SLOT(Btn_zoom_out_h_Click()));
	connect(m_ui.btn_zoom_in_v, SIGNAL(clicked()), this, SLOT(Btn_zoom_in_v_Click()));
	connect(m_ui.btn_zoom_out_v, SIGNAL(clicked()), this, SLOT(Btn_zoom_out_v_Click()));
	connect(m_ui.slider_baseline, SIGNAL(sliderMoved(int)), this, SLOT(Slider_baseline_Moved(int)));
	connect(m_ui.btn_timemap_delete, SIGNAL(clicked()), m_ui.canvas_timemap, SLOT(delete_ctrl_pnt()));
	connect(m_ui.btn_volume_delete, SIGNAL(clicked()), m_ui.canvas_volume, SLOT(delete_ctrl_pnt()));

	connect(m_ui.canvas_timemap, SIGNAL(sampler_updated()), this, SLOT(Baseline_changed()));
	connect(m_ui.canvas_timemap, SIGNAL(sampler_updated()), this, SLOT(set_changed()));
	connect(m_ui.canvas_timemap, SIGNAL(sampler_updated()), m_ui.canvas_src_view, SLOT(update_sampler()));
	connect(m_ui.canvas_timemap, SIGNAL(sampler_updated()), m_ui.canvas_volume, SLOT(update_sampler()));
	connect(m_ui.canvas_timemap, SIGNAL(sampler_updated()), m_ui.canvas_bgm, SLOT(update_sampler()));

	connect(m_ui.canvas_volume, SIGNAL(sampler_updated()), this, SLOT(set_changed()));

	connect(m_ui.canvas_timemap, SIGNAL(cursor_moved(double)), this, SLOT(Cursor_Moved(double)));
	connect(m_ui.canvas_volume, SIGNAL(cursor_moved(double)), this, SLOT(Cursor_Moved(double)));
	connect(m_ui.canvas_bgm, SIGNAL(cursor_moved(double)), this, SLOT(Cursor_Moved(double)));

	connect(m_ui.slider_bgm_volume, SIGNAL(sliderMoved(int)), this, SLOT(Slider_bgm_volume_Moved(int)));

	connect(m_ui.actionE_xit, SIGNAL(triggered()), this, SLOT(close()));
	connect(m_ui.action_New, SIGNAL(triggered()), this, SLOT(new_project()));
	connect(m_ui.action_Open, SIGNAL(triggered()), this, SLOT(open_project()));
	connect(m_ui.action_Save, SIGNAL(triggered()), this, SLOT(save_project()));
	connect(m_ui.actionSave_As, SIGNAL(triggered()), this, SLOT(save_project_as()));

	set_bgm_visible(false);

	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(refresh()));
	m_timer->start(20);

	update_title();
}

Scratcher::~Scratcher()
{
	Unload();
}

void Scratcher::set_bgm_visible(bool show)
{
	m_ui.label_bgm->setVisible(show);
	m_ui.slider_bgm_volume->setVisible(show);
	m_ui.scroll_bgm->setVisible(show);
}

void Scratcher::closeEvent(QCloseEvent *event)
{
	if (!prompt_save())
	{
		event->ignore();
	}
	else
	{
		event->accept();
	}
}

void Scratcher::InitAudioDevice()
{
	int id_default;
	const auto& lst = Player::s_get_list_audio_devices(&id_default);
	m_ui.combo_audio_device->clear();
	for (const auto& item : lst)
	{
		m_ui.combo_audio_device->addItem(QString::fromLocal8Bit(item.c_str()));
	}
	m_ui.combo_audio_device->setCurrentIndex(id_default);
}

void Scratcher::update_title()
{
	QString t = "Scratcher";
	if (m_filename == "")
	{
		t += " - [Untitled]";
	}
	else
	{
		t += " - " + m_filename;
	}
	if (m_changed)
	{
		t += "*";
	}
	setWindowTitle(t);
}

void Scratcher::set_cur_filename(QString fullpath)
{
	m_fullpath = fullpath;
	if (m_fullpath == "")
	{
		m_filename = "";
	}
	else
	{
		QFileInfo fi(fullpath);
		m_filename = fi.fileName();
	}
	update_title();
}

void Scratcher::set_changed(bool changed)
{
	if (m_changed != changed)
	{
		m_changed = changed;
		update_title();
	}
}

bool Scratcher::prompt_save()
{
	if (m_changed)
	{
		QMessageBox msgBox	(this);
		msgBox.setText("The project has been modified.");
		msgBox.setInformativeText("Do you want to save your changes?");
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Yes);
		int ret = msgBox.exec();
		if (ret == QMessageBox::Yes)
		{
			if (!save_project()) return false;
		}
		else if (ret == QMessageBox::Cancel)
		{
			return false;
		}
	}
	return true;
}

bool Scratcher::new_project()
{
	if (!prompt_save()) return false;
	Unload();
	m_cursor_pos = 0.0;
	_emit_cursor_pos(0.0);
	Baseline_changed();
	m_ui.slider_bgm_volume->setValue(100);
	set_changed(false);
	set_cur_filename("");
	return true;
}

void Scratcher::do_open_project(QString fullpath)
{
#if USE_JSON
	QFile loadFile(fullpath);
	loadFile.open(QIODevice::ReadOnly);
	QByteArray saveData = loadFile.readAll();
	QJsonObject root = QJsonDocument::fromJson(saveData).object();
	QString filename_source = root["filename_source"].toString();
	LoadSource(filename_source);
	if (root.contains("filename_bgm"))
	{
		QString filename_bgm = root["filename_bgm"].toString();
		LoadBgm(filename_bgm);
		float bgm_volume = (float)root["bgm_volume"].toDouble();
		m_sampler->set_bgm_volume(bgm_volume);
	}
	{
		QJsonObject timemap = root["timemap"].toObject();
		m_sampler->set_start_pos((float)timemap["start_pos"].toDouble());
		m_sampler->set_start_slope((float)timemap["start_slope"].toDouble());
		{
			QJsonArray control_pnts = timemap["control_points"].toArray();
			int num_ctrl_pnts = control_pnts.size();
			for (int i = 0; i < num_ctrl_pnts; i++)
			{
				QJsonObject ctrl_pnt = control_pnts[i].toObject();
				float x = (float)ctrl_pnt["x"].toDouble();
				float y = (float)ctrl_pnt["y"].toDouble();
				float slope = (float)ctrl_pnt["slope"].toDouble();
				m_sampler->add_control_point(x, y);
				m_sampler->set_control_point_slope(i, slope);
			}
		}
	}
	{
		QJsonObject volume = root["volume"].toObject();
		m_sampler->set_start_volume((float)volume["start_volume"].toDouble());
		{
			QJsonArray control_pnts = volume["control_points"].toArray();
			int num_ctrl_pnts = control_pnts.size();
			for (int i = 0; i < num_ctrl_pnts; i++)
			{
				QJsonObject ctrl_pnt = control_pnts[i].toObject();
				float x = (float)ctrl_pnt["x"].toDouble();
				float y = (float)ctrl_pnt["y"].toDouble();
				m_sampler->add_volume_control_point(x, y);
			}
		}
	}

#else
	std::string sfilename = fullpath.toLocal8Bit().constData();
	FILE* fp = fopen(sfilename.c_str(), "rb");
	QString filename_source = deserialize_string(fp);
	LoadSource(filename_source);
	QString filename_bgm = deserialize_string(fp);
	if (filename_bgm != "")
		LoadBgm(filename_bgm);
	m_sampler->deserialize(fp);
	fclose(fp);
#endif
}

bool Scratcher::open_project()
{
	if (!prompt_save()) return false;
#if USE_JSON
	QString filename = QFileDialog::getOpenFileName(this, "Open Project", QString(), "Json Files (*.json)");
#else
	QString filename = QFileDialog::getOpenFileName(this, "Open Project", QString(), "Scratch Files (*.scratch)");
#endif
	if (filename == "") return false;
	do_open_project(filename);
	m_ui.canvas_timemap->update_sampler();
	m_ui.canvas_volume->update_sampler();
	m_ui.canvas_src_view->update_sampler();
	m_ui.canvas_bgm->update_sampler();
	m_cursor_pos = 0.0;
	_emit_cursor_pos(0.0);	
	Baseline_changed();
	{
		float bgm_vol = m_sampler->bgm_volume();
		int v = (int)(bgm_vol * 100.0f);
		m_ui.slider_bgm_volume->setValue(v);
	}
	set_changed(false);
	set_cur_filename(filename);
	return true;
}

void Scratcher::do_save_project(QString fullpath)
{
#if USE_JSON
	QJsonObject root;
	root["filename_source"] = m_filename_source;
	if (m_filename_source != "")
	{
		root["filename_bgm"] = m_filename_bgm;
		root["bgm_volume"] = m_sampler->bgm_volume();		
	}	
	{
		QJsonObject timemap;
		timemap["start_pos"] = m_sampler->start_pos();
		timemap["start_slope"] = m_sampler->start_slope();
		{
			QJsonArray control_pnts;
			size_t num_ctrl_pnts = m_sampler->num_control_points();
			for (size_t i = 0; i < num_ctrl_pnts; i++)
			{
				float x, y, slope;
				m_sampler->control_point(i, x, y, slope);
				QJsonObject ctrl_pnt;
				ctrl_pnt["x"] = x;
				ctrl_pnt["y"] = y;
				ctrl_pnt["slope"] = slope;
				control_pnts.append(ctrl_pnt);
			}
			timemap["control_points"] = control_pnts;
		}
		root["timemap"] = timemap;
	}
	{
		QJsonObject volume;
		volume["start_volume"] = m_sampler->start_volume();
		{
			QJsonArray control_pnts;
			size_t num_ctrl_pnts = m_sampler->num_volume_control_points();
			for (size_t i = 0; i < num_ctrl_pnts; i++)
			{
				float x, y;
				m_sampler->volume_control_point(i, x, y);
				QJsonObject ctrl_pnt;
				ctrl_pnt["x"] = x;
				ctrl_pnt["y"] = y;				
				control_pnts.append(ctrl_pnt);
			}
			volume["control_points"] = control_pnts;
		}
		root["volume"] = volume;
	}
	QFile saveFile(fullpath);
	saveFile.open(QIODevice::WriteOnly);
	saveFile.write(QJsonDocument(root).toJson());
#else
	std::string sfilename = fullpath.toLocal8Bit().constData();
	FILE* fp = fopen(sfilename.c_str(), "wb");
	serialize_string(fp, m_filename_source);
	serialize_string(fp, m_filename_bgm);
	m_sampler->serialize(fp);
	fclose(fp);
#endif
}


bool Scratcher::save_project_as()
{
	if (m_sampler == nullptr || m_filename_source=="") return false;
#if USE_JSON
	QString filename = QFileDialog::getSaveFileName(this, "Save Project", QString(), "Json Files (*.json)");
#else
	QString filename = QFileDialog::getSaveFileName(this, "Save Project", QString(), "Scratch Files (*.scratch)");
#endif
	if (filename == "") return false;
	do_save_project(filename);
	set_changed(false);
	set_cur_filename(filename);
	return true;
}

bool Scratcher::save_project()
{
	if (m_sampler == nullptr || m_filename_source == "") return false;
	if (m_fullpath != "")
	{
		do_save_project(m_fullpath);
		set_changed(false);
		return true;
	}
	return save_project_as();
}

void Scratcher::Unload()
{
	m_bgm_buffer = nullptr;
	m_ui.canvas_bgm->set_sampler(nullptr);
	m_ui.canvas_src_view->set_sampler(nullptr);
	m_ui.canvas_volume->set_sampler(nullptr);
	m_ui.canvas_timemap->set_sampler(nullptr);	
	m_player = nullptr;
	m_sampler = nullptr;
	m_src_buffer = nullptr;
	m_filename_source = "";
	m_filename_bgm = "";
	set_bgm_visible(false);
}

void Scratcher::LoadSource(const QString& filename)
{
	Unload();	
	std::string fn = filename.toLocal8Bit().constData();
	m_src_buffer = (std::unique_ptr<TrackBuffer>)(ReadAudioFromFile(fn.c_str()));
	m_sampler = (std::unique_ptr<SamplerScratch>)(new SamplerScratch(m_src_buffer.get()));	

	m_player = (std::unique_ptr<Player>)(new Player(m_sampler.get(), m_ui.combo_audio_device->currentIndex()));

	m_ui.btn_audio_play->setIcon(QIcon(":/icons/play.png"));
	m_is_playing = false;	

	double len_in = (double)m_src_buffer->NumberOfSamples() / (double)m_src_buffer->Rate();
	m_ui.slider_baseline->setMaximum((int)(len_in*100.0));
	m_ui.slider_baseline->setValue(0);

	m_ui.canvas_timemap->set_sampler(m_sampler.get());
	m_ui.canvas_volume->set_sampler(m_sampler.get());	
	m_ui.canvas_src_view->set_sampler(m_sampler.get());	

	m_filename_source = filename;
}

void Scratcher::LoadBgm(const QString& filename)
{
	if (m_sampler == nullptr) return;
	std::string fn = filename.toLocal8Bit().constData();
	m_bgm_buffer = (std::unique_ptr<TrackBuffer>)(ReadAudioFromFile(fn.c_str()));
	m_sampler->set_bgm(m_bgm_buffer.get());
	set_bgm_visible(true);
	m_ui.canvas_bgm->set_sampler(m_sampler.get());
	m_filename_bgm = filename;
	
}

void Scratcher::SaveResult(const QString& filename)
{	
	std::string fn = filename.toLocal8Bit().constData();
	TrackBuffer buf_out;
	SampleToTrackBuffer(*m_sampler, buf_out);
	WriteAudioToFile(&buf_out, fn.c_str());
}

void Scratcher::_emit_cursor_pos(double pos)
{
	m_ui.canvas_timemap->set_cursor_pos(pos);
	m_ui.canvas_timemap->update();
	m_ui.canvas_volume->set_cursor_pos(pos);
	m_ui.canvas_volume->update();
	m_ui.canvas_bgm->set_cursor_pos(pos);
	m_ui.canvas_bgm->update();
	TimeMapTrackPositionH();
	VolumeTrackPositionH();
	BgmTrackPositionH();
	TimeMapTrackPositionV();
	m_ui.canvas_src_view->set_cursor_pos(pos);
	m_ui.canvas_src_view->update();
}

void Scratcher::Play()
{
	if (m_player != nullptr)
	{
		m_ui.canvas_timemap->set_locked(true);
		m_ui.canvas_volume->set_locked(true);
		m_ui.canvas_bgm->set_locked(true);
		m_ui.slider_baseline->setEnabled(false);
		m_ui.slider_bgm_volume->setEnabled(false);
		m_player->set_position((uint64_t)(m_cursor_pos*1000000.0));
		m_player->start();
		m_ui.btn_audio_play->setIcon(QIcon(":/icons/stop.png"));
		m_is_playing = true;
	}
}

void Scratcher::Stop()
{
	if (m_player != nullptr)
	{
		m_is_playing = false;
		m_player->stop();
		m_player->set_position(0);

		_emit_cursor_pos(m_cursor_pos);		

		m_ui.btn_audio_play->setIcon(QIcon(":/icons/play.png"));
		m_ui.slider_baseline->setEnabled(true);
		m_ui.slider_bgm_volume->setEnabled(true);
		m_ui.canvas_timemap->set_locked(false);
		m_ui.canvas_volume->set_locked(false);
		m_ui.canvas_bgm->set_locked(false);
	}
}


void Scratcher::TimeMapTrackPositionH()
{
	if (m_player != nullptr)
	{	
		float pos = (float)m_ui.canvas_timemap->cursor_pos();
		float pix_pos = pos * m_ui.canvas_timemap->scale_out();
		float delta = pix_pos - (float)m_ui.scroll_timemap->width() *0.5f;
		float max_delta = (float)m_ui.scroll_timemap->horizontalScrollBar()->maximum();	
		if (delta < 0.0f) delta = 0.0f;
		if (delta > max_delta) delta = max_delta;
		m_ui.scroll_timemap->horizontalScrollBar()->setValue((int)delta);
		m_ui.canvas_timemap->set_scroll_offset_out((int)delta);
		m_ui.canvas_timemap->update();	
	}
}

void Scratcher::TimeMapTrackPositionV()
{
	if (m_player != nullptr)
	{
		float pos_x = (float)m_ui.canvas_timemap->cursor_pos();
		float pos_y, slope;
		m_sampler->time_map(pos_x, pos_y, slope);

		float pix_pos = (float) m_ui.canvas_timemap->height() - (pos_y + m_ui.canvas_timemap->offset_in()) * m_ui.canvas_timemap->scale_in();
		float delta = pix_pos - (float)m_ui.scroll_timemap->height() *0.5f;
		float max_delta = (float)m_ui.scroll_timemap->verticalScrollBar()->maximum();	

		if (delta < 0.0f) delta = 0.0f;
		if (delta > max_delta) delta = max_delta;
		m_ui.scroll_timemap->verticalScrollBar()->setValue((int)delta);
		m_ui.canvas_timemap->set_scroll_offset_in((int)(max_delta - delta));
		m_ui.canvas_timemap->update();
	}
}

void Scratcher::VolumeTrackPositionH()
{
	if (m_player != nullptr)
	{
		float pos = (float)m_ui.canvas_volume->cursor_pos();
		float pix_pos = pos * m_ui.canvas_volume->scale();
		float delta = pix_pos - (float)m_ui.scroll_volume->width() *0.5f;
		float max_delta = (float)m_ui.scroll_volume->horizontalScrollBar()->maximum();
		if (delta < 0.0f) delta = 0.0f;
		if (delta > max_delta) delta = max_delta;
		m_ui.scroll_volume->horizontalScrollBar()->setValue((int)delta);
		m_ui.canvas_volume->set_scroll_offset((int)delta);
		m_ui.canvas_volume->update();
	}
}


void Scratcher::BgmTrackPositionH()
{
	if (m_player != nullptr)
	{
		float pos = (float)m_ui.canvas_bgm->cursor_pos();
		float pix_pos = pos * m_ui.canvas_bgm->scale();
		float delta = pix_pos - (float)m_ui.scroll_bgm->width() * 0.5f;
		float max_delta = (float)m_ui.scroll_bgm->horizontalScrollBar()->maximum();
		if (delta < 0.0f) delta = 0.0f;
		if (delta > max_delta) delta = max_delta;
		m_ui.scroll_bgm->horizontalScrollBar()->setValue((int)delta);
		m_ui.canvas_bgm->set_scroll_offset((int)delta);
		m_ui.canvas_bgm->update();
	}
}

void Scratcher::refresh()
{
	if (m_player != nullptr && m_player->is_playing())
	{
		if (!m_player->is_eof_reached())
		{
			double pos = (double)m_player->get_position() / 1000000.0;
			if (pos >= 0.0)
			{
				_emit_cursor_pos(pos);
			}
		}
		else
		{			
			Stop();
		}

	}
	
}

void Scratcher::AudioDeviceChange(int idx)
{
	if (m_player != nullptr)
	{
		m_player->set_audio_device(idx);
	}
}


void Scratcher::Btn_load_src_Click()
{
	QString filename = QFileDialog::getOpenFileName(this, "Open Source Audio", QString(), "Audio Files (*.mp3 *.wav) ");
	if (filename != "")
	{
		LoadSource(filename);
		set_changed();
	}
}

void Scratcher::Btn_load_bgm_Click()
{
	if (m_sampler == nullptr) return;
	QString filename = QFileDialog::getOpenFileName(this, "Open BGM Audio", QString(), "Audio Files (*.mp3 *.wav) ");
	if (filename != "")
	{
		LoadBgm(filename);
		set_changed();
	}
}

void Scratcher::Btn_save_res_Click()
{
	if (m_sampler == nullptr) return;
	Stop();
	QString filename = QFileDialog::getSaveFileName(this, "Save Result Audio", QString(), "Audio Files (*.mp3) ");
	if (filename != "")
		SaveResult(filename);
}

void Scratcher::Btn_play_stop_Click()
{
	if (m_player == nullptr) return;
	if (m_is_playing)
	{
		Stop();
	}
	else
	{
		Play();
	}
}

void Scratcher::Btn_reset_Click()
{
	if (!m_is_playing)
	{
		m_cursor_pos = 0.0;
		_emit_cursor_pos(0.0);
	}
}

void Scratcher::TimeMapHScrollRange(int min_v, int max_v)
{
	m_ui.scroll_timemap->horizontalScrollBar()->setValue(m_ui.canvas_timemap->scroll_offset_out());
}

void Scratcher::TimeMapVScrollRange(int min_v, int max_v)
{
	m_ui.scroll_timemap->verticalScrollBar()->setValue(max_v - m_ui.canvas_timemap->scroll_offset_in());
}

void Scratcher::VolumeHScrollRange(int min_v, int max_v)
{
	m_ui.scroll_volume->horizontalScrollBar()->setValue(m_ui.canvas_volume->scroll_offset());
}

void Scratcher::TimeMapHScroll(int v)
{
	m_ui.canvas_timemap->set_scroll_offset_out(v);
	m_ui.canvas_timemap->update();
	m_ui.scroll_volume->horizontalScrollBar()->setValue(v);
	m_ui.scroll_bgm->horizontalScrollBar()->setValue(v);
}

void Scratcher::TimeMapVScroll(int v)
{	
	m_ui.canvas_timemap->set_scroll_offset_in(m_ui.scroll_timemap->verticalScrollBar()->maximum() - v);
	m_ui.canvas_timemap->update();
}

void Scratcher::VolumeHScroll(int v)
{
	m_ui.canvas_volume->set_scroll_offset(v);
	m_ui.canvas_volume->update();
	m_ui.scroll_timemap->horizontalScrollBar()->setValue(v);
	m_ui.scroll_bgm->horizontalScrollBar()->setValue(v);
}


void Scratcher::BgmHScroll(int v)
{
	m_ui.canvas_bgm->set_scroll_offset(v);
	m_ui.canvas_bgm->update();
	m_ui.scroll_timemap->horizontalScrollBar()->setValue(v);
	m_ui.scroll_volume->horizontalScrollBar()->setValue(v);
}

void Scratcher::Btn_zoom_in_h_Click()
{
	if (m_is_playing) return;
	float scale = m_ui.canvas_timemap->scale_out();
	if (scale >= 512.0f) return;
	m_ui.canvas_timemap->set_scale_out(scale*1.5f);
	m_ui.canvas_timemap->update();
	m_ui.canvas_volume->set_scale(scale*1.5f);
	m_ui.canvas_volume->update();
	m_ui.canvas_bgm->set_scale(scale * 1.5f);
	m_ui.canvas_bgm->update();
}

void Scratcher::Btn_zoom_out_h_Click()
{
	if (m_is_playing) return;
	float scale = m_ui.canvas_timemap->scale_out();
	m_ui.canvas_timemap->set_scale_out(scale/1.5f);
	m_ui.canvas_timemap->update();
	m_ui.canvas_volume->set_scale(scale/1.5f);
	m_ui.canvas_volume->update();
	m_ui.canvas_bgm->set_scale(scale / 1.5f);
	m_ui.canvas_bgm->update();
}

void Scratcher::Btn_zoom_in_v_Click()
{
	if (m_is_playing) return;
	float scale = m_ui.canvas_timemap->scale_in();
	if (scale >= 512.0f) return;
	m_ui.canvas_timemap->set_scale_in(scale * 1.5f);
	m_ui.canvas_timemap->update();
	m_ui.canvas_src_view->set_scale(scale * 1.5f);
	m_ui.canvas_src_view->update();
}

void Scratcher::Btn_zoom_out_v_Click()
{
	if (m_is_playing) return;
	float scale = m_ui.canvas_timemap->scale_in();
	m_ui.canvas_timemap->set_scale_in(scale / 1.5f);
	m_ui.canvas_timemap->update();
	m_ui.canvas_src_view->set_scale(scale / 1.5f);
	m_ui.canvas_src_view->update();
}

void Scratcher::Slider_baseline_Moved(int v)
{
	if (m_sampler != nullptr && !m_is_playing)
	{
		float old_pos = m_sampler->start_pos();
		float pos = (float)v / 100.0f;
		m_sampler->set_start_pos(pos);
		size_t num_ctrl_pnts = m_sampler->num_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float x, y, slope;
			m_sampler->control_point(i, x, y, slope);
			m_sampler->move_control_point(i, y + (pos - old_pos));
		}

		m_ui.canvas_timemap->update_sampler();
		m_ui.canvas_volume->update_sampler();
		m_ui.canvas_bgm->update_sampler();
		m_ui.canvas_src_view->set_cursor_pos(m_cursor_pos);
		m_ui.canvas_src_view->update_sampler();

		set_changed();		
	}
}

void Scratcher::Baseline_changed()
{
	if (m_sampler != nullptr)
	{
		float pos = m_sampler->start_pos();
		int v = (int)(pos * 100.0f);
		if (v < 0) v = 0;
		if (v > m_ui.slider_baseline->maximum()) v = m_ui.slider_baseline->maximum();
		m_ui.slider_baseline->setValue(v);
	}
	else
	{
		m_ui.slider_baseline->setValue(0);
	}
}

void Scratcher::Cursor_Moved(double pos)
{
	if (!m_is_playing)
	{
		m_cursor_pos = pos;
		m_ui.canvas_timemap->set_cursor_pos(pos);
		m_ui.canvas_timemap->update();
		m_ui.canvas_volume->set_cursor_pos(pos);
		m_ui.canvas_volume->update();
		m_ui.canvas_bgm->set_cursor_pos(pos);
		m_ui.canvas_bgm->update();
		m_ui.canvas_src_view->set_cursor_pos(pos);
		m_ui.canvas_src_view->update();
	}
}

void Scratcher::Slider_bgm_volume_Moved(int v)
{
	if (m_sampler != nullptr)
	{
		float vol = (float)v / 100.0f;
		m_sampler->set_bgm_volume(vol);
		set_changed();
	}

}