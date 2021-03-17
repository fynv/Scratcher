#include <cmath>
#include <cfloat>
#include <QPainter>
#include <QMouseEvent>
#include <QVector2D>
#include <SamplerScratch.h>
#include "VolumeView.h"
#include "GLUtils.h"


VolumeView::VolumeView(QWidget* parent)
	: QOpenGLWidget(parent)
{

}

VolumeView::~VolumeView()
{
	m_prog = nullptr;
	m_frag_shader = nullptr;
	m_vert_shader = nullptr;
	m_curve_gpu = nullptr;
}

void VolumeView::update_curve()
{
	m_curve_gpu = nullptr;
	if (m_sampler != nullptr)
	{
		double duration = m_sampler->get_duration();
		int width = (int)ceil(duration*m_scale);
		this->setFixedWidth(width);

		std::vector<QVector2D> ctrl_pnts;

		float scale_v = (float)this->height()*m_rate_v;
		{
			float x = 0.0f;
			float pos_y = m_sampler->start_volume();
			float y = (float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v;
			ctrl_pnts.push_back({ x,y });
		}

		size_t num_ctrl_pnts = m_sampler->num_volume_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float pos_x, pos_y;
			m_sampler->volume_control_point(i, pos_x, pos_y);
			float x = pos_x * m_scale;
			float y = (float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v;
			ctrl_pnts.push_back({ x,y });
		}
		{
			float x = (float)this->width();
			float y = ctrl_pnts[ctrl_pnts.size() - 1].y();
			ctrl_pnts.push_back({ x,y });
		}
		
		m_curve_gpu = (std::unique_ptr<GLBuffer>)(new GLBuffer(&m_gl, sizeof(QVector2D)*ctrl_pnts.size()));
		m_curve_gpu->upload(ctrl_pnts.data());

	}

}
void VolumeView::delete_ctrl_pnt()
{
	if (m_sampler == nullptr || m_is_locked || m_selected_id < 0) return;
	m_sampler->remove_volume_control_point(m_selected_id);
	if (m_selected_id >= m_sampler->num_volume_control_points())
	{
		m_selected_id = (int)m_sampler->num_volume_control_points() - 1;
	}
	update_curve();
	update();
	emit sampler_updated();
}

void VolumeView::initializeGL()
{
	m_gl.initializeOpenGLFunctions();
}

static const char* g_vertex_shader =
"#version 430\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 0) uniform vec2 uSize;\n"
"void main()\n"
"{\n"
"	vec2 pos_out;\n"
"	pos_out.x = aPos.x / uSize.x *2.0 - 1.0;\n"
"	pos_out.y = 1.0 - aPos.y / uSize.y *2.0;\n"
"	gl_Position = vec4(pos_out, 0.0, 1.0);\n"
"}\n";

static const char* g_frag_shader =
"#version 430\n"
"out vec4 outColor;\n"
"void main()\n"
"{\n"
"   outColor = vec4(1.0,1.0,1.0,1.0);\n"
"}\n";

void VolumeView::paintGL()
{
	if (m_prog == nullptr)
	{
		m_vert_shader = (std::unique_ptr<GLShader>)(new GLShader(&m_gl, GL_VERTEX_SHADER, g_vertex_shader));
		m_frag_shader = (std::unique_ptr<GLShader>)(new GLShader(&m_gl, GL_FRAGMENT_SHADER, g_frag_shader));
		m_prog = (std::unique_ptr<GLProgram>)(new GLProgram(&m_gl, *m_vert_shader, *m_frag_shader));
	}

	QPainter painter;
	painter.begin(this);

	painter.beginNativePainting();
	m_gl.glViewport(0, 0, this->width(), this->height());
	m_gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	m_gl.glClear(GL_COLOR_BUFFER_BIT);

	if (m_curve_gpu!=nullptr)
	{
		m_gl.glDisable(GL_DEPTH_TEST);

		m_gl.glUseProgram(m_prog->m_id);
		m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_curve_gpu->m_id);
		m_gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_gl.glEnableVertexAttribArray(0);
		m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_gl.glUniform2f(0, (float)this->width(), (float)this->height());
		m_gl.glDrawArrays(GL_LINE_STRIP, 0, m_curve_gpu->m_size / sizeof(QVector2D));
		m_gl.glUseProgram(0);
	}
	painter.endNativePainting();	

	float scale_v = (float)this->height()*m_rate_v;

	if (m_sampler != nullptr)
	{
		{
			float x = 0.0f;
			float pos_y = m_sampler->start_volume();
			int y = (int)((float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v + 0.5f);
			if (m_selected_id == -1)
			{
				painter.setPen(QColor(0, 255, 0));				
			}
			else
			{
				painter.setPen(QColor(255, 255, 255));
			}
			painter.drawRect((int)x - 4, (int)y - 4, 8, 8);
		}

		size_t num_ctrl_pnts = m_sampler->num_volume_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float pos_x, pos_y;
			m_sampler->volume_control_point(i, pos_x, pos_y);
			int x = (int)(pos_x * m_scale + 0.5f);
			int y = (int)((float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v + 0.5f);
			if (m_selected_id == i)
			{
				painter.setPen(QColor(0, 255, 0));				
			}
			else
			{
				painter.setPen(QColor(255, 255, 255));
			}
			painter.drawRect((int)x - 4, (int)y - 4, 8, 8);
		}

		if (m_cursor_pos >= 0.0)
		{
			float x = m_cursor_pos * m_scale;
			float pos_y;
			m_sampler->volume(m_cursor_pos, pos_y);
			float y = (float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v;
			painter.setPen(QColor(255, 0, 0, 100));
			painter.drawLine(QPointF(0.0f, y), QPointF((float)this->width(), y));
			painter.drawLine(QPointF(x, 0.0f), QPointF(x, (float)this->height()));
		}
	}

	{
		int y = this->height();
		painter.fillRect(0, y - 15, this->width(), 15, QColor(255, 255, 255, 80));
	}

	{
		int x = m_scroll_offset;
		painter.fillRect(x, 0, 50, this->height(), QColor(255, 255, 255, 80));
	}

	// ruler-h
	{
		int m = 0;
		float interval = 0.01f;
		while (interval * m_scale < 60.0f)
		{
			if (m == 0)
			{
				interval *= 5.0f;
				m = 1;
			}
			else
			{
				interval *= 2.0f;
				m = 0;
			}
		}

		float y = (float)this->height();

		painter.setPen(QColor(0, 128, 0));
		for (float pos = 0.0f; pos*m_scale < (float)this->width(); pos += interval * 0.1f)
		{
			float x = pos * m_scale;
			painter.drawLine(QPointF(x, y), QPointF(x, y - 8.0f));
		}

		painter.setPen(QColor(0, 255, 0));


		for (float pos = 0.0f; pos*m_scale < (float)this->width(); pos += interval)
		{
			float x = pos * m_scale;
			painter.drawLine(QPointF(x, y), QPointF(x, y - 12.0f));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x + 3.0f, y - 3.0f), QString(buf));
		}
	}

	// ruler-v
	{
		int m = 0;
		float interval = 0.01f;
		while (interval * scale_v < 60.0f)
		{
			if (m == 0)
			{
				interval *= 5.0f;
				m = 1;
			}
			else
			{
				interval *= 2.0f;
				m = 0;
			}
		}

		float x = (float)m_scroll_offset;

		painter.setPen(QColor(0, 128, 0));
		for (float pos = 0.0f; pos * scale_v < (float)this->height() * (1.0f - m_rate_margin); pos += interval * 0.1f)
		{
			float y = (float)this->height() * (1.0f - m_rate_margin) - pos * scale_v;
			painter.drawLine(QPointF(x, y), QPointF(x + 8.0f, y));
		}

		painter.setPen(QColor(0, 255, 0));
		for (float pos = 0.0f; pos * scale_v < (float)this->height() * (1.0f - m_rate_margin); pos += interval)
		{
			float y = (float)this->height() * (1.0f - m_rate_margin) - pos * scale_v;
			painter.drawLine(QPointF(x, y), QPointF(x + 12.0f, y));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x + 10.0f, y - 2.0f), QString(buf));
		}		

	}

	painter.end();
}


void VolumeView::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	float scale_v = (float)this->height()*m_rate_v;
	float x = (float)event->x();
	float y = (float)event->y();
	float pos_x = x / m_scale;
	float pos_y = ((float)this->height() * (1.0f - m_rate_margin) - y) / scale_v;
	m_selected_id = m_sampler->add_volume_control_point(pos_x, pos_y);
	update_curve();
	update();
	emit sampler_updated();
}

void VolumeView::mousePressEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	float scale_v = (float)this->height()*m_rate_v;

	float x = (float)event->x();
	float y = (float)event->y();

	{
		float x_ctrl = 0.0f;
		float pos_y = m_sampler->start_volume();
		float y_ctrl = (float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v;		

		if (fabsf(x - x_ctrl) < m_size_ctrl_pnt && fabsf(y - y_ctrl) < m_size_ctrl_pnt)
		{
			m_selected_id = -1;
			m_dragging_ctrl = true;
			update();
			return;
		}
	}

	size_t num_ctlr_pnt = m_sampler->num_volume_control_points();
	for (size_t i = 0; i < num_ctlr_pnt; i++)
	{
		float pos_x, pos_y;
		m_sampler->volume_control_point(i, pos_x, pos_y);
		float x_ctrl = pos_x * m_scale;		
		float y_ctrl = (float)this->height() * (1.0f - m_rate_margin) - pos_y * scale_v;
		if (fabsf(x - x_ctrl) < m_size_ctrl_pnt && fabsf(y - y_ctrl) < m_size_ctrl_pnt)
		{
			m_selected_id = i;
			m_dragging_ctrl = true;
			update();
			return;
		}
	}

	m_moving_cursor = true;
}


void VolumeView::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	if (m_dragging_ctrl)
	{
		m_dragging_ctrl = false;
	}
	if (m_moving_cursor)
	{
		m_moving_cursor = false;
	}
}


void VolumeView::mouseMoveEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	float scale_v = (float)this->height()*m_rate_v;

	float x = (float)event->x();
	float y = (float)event->y();

	if (m_dragging_ctrl)
	{
		float pos_x = x / m_scale;
		float pos_y = ((float)this->height() * (1.0f - m_rate_margin) - y) / scale_v;		
		if (pos_y < 0.0f) pos_y = 0.0f;
		if (pos_y > 1.0f) pos_y = 1.0f;

		if (m_selected_id == -1)
		{
			m_sampler->set_start_volume(pos_y);
			m_cursor_pos = 0.0;			
		}
		else
		{			
			m_cursor_pos = m_sampler->move_volume_control_point(m_selected_id, pos_x, pos_y);
		}
		update_curve();
		emit cursor_moved(m_cursor_pos);
		emit sampler_updated();
	}
	else if (m_moving_cursor)
	{
		m_cursor_pos = x / m_scale;		
		emit cursor_moved(m_cursor_pos);
	}
}

