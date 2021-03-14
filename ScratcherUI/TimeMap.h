#pragma once

#include <vector>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>

class SamplerScratch;
struct GLBuffer;
struct GLShader;
struct GLProgram;
class TimeMap : public QOpenGLWidget
{
	Q_OBJECT
public:
	TimeMap(QWidget* parent);
	~TimeMap();

	void set_sampler(SamplerScratch* sampler)
	{
		m_sampler = sampler;
		update_curve();
		m_cursor_pos = 0.0;
	}

	void update_curve();

	float scale_out() const { return m_scale_out; }
	float scale_in() const { return m_scale_in;  }
	float offset_in() const { return m_offset_in;  }
	double cursor_pos() const { return m_cursor_pos; }

	int scroll_offset_out() const { return m_scroll_offset_out; }
	int scroll_offset_in() const { return m_scroll_offset_in; }
	
	void set_scale_out(float scale)
	{
		m_scale_out = scale;
		update_curve();
	}

	void set_scale_in(float scale)
	{
		m_scale_in = scale;
		update_curve();
	}	

	void set_scroll_offset_out(int v)
	{
		m_scroll_offset_out = v;
	}

	void set_scroll_offset_in(int v)
	{
		m_scroll_offset_in = v;
	}

	void set_cursor_pos(double pos)
	{
		m_cursor_pos = pos;
	}

	void set_locked(bool locked)
	{
		m_is_locked = locked;
	}

public slots:
	void delete_ctrl_pnt();
	void update_sampler()
	{
		update_curve();
		update();
	}

signals:
	void cursor_moved(double pos);
	void sampler_updated();

protected:
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void mouseReleaseEvent(QMouseEvent *event) override;
	virtual void mouseMoveEvent(QMouseEvent *event) override;

private:
	QOpenGLExtraFunctions m_gl;
	float m_bottom_margin = 0.2f;
	float m_len_slope_vec = 30.0f;
	float m_slope_limit = 20.0f;
	float m_size_ctrl_pnt = 4.0f;

	SamplerScratch* m_sampler = nullptr;
	float m_offset_in = m_bottom_margin;
	float m_scale_out = 200.0f;
	float m_scale_in = 200.0f;			

	std::unique_ptr<GLBuffer> m_ctrl_pnts_gpu;
	std::unique_ptr<GLShader> m_vert_shader;
	std::unique_ptr<GLShader> m_frag_shader;
	std::unique_ptr<GLProgram> m_prog;

	int m_scroll_offset_out = 0;
	int m_scroll_offset_in = 0;

	double m_cursor_pos = 0.0;

	bool m_is_locked = false;

	int m_selected_id = -1;
	bool m_moving_cursor = false;
	bool m_dragging_ctrl = false;
	bool m_dragging_slope = false;
	
};
