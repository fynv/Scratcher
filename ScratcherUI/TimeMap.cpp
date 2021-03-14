#include <cmath>
#include <cfloat>
#include <QPainter>
#include <QMouseEvent>
#include <QVector4D>
#include <SamplerScratch.h>
#include "TimeMap.h"
#include "GLUtils.h"

TimeMap::TimeMap(QWidget* parent)
	: QOpenGLWidget(parent)
{

}

TimeMap::~TimeMap()
{
	m_prog = nullptr;
	m_frag_shader = nullptr;
	m_vert_shader = nullptr;
	m_ctrl_pnts_gpu = nullptr;
}

void TimeMap::update_curve()
{
	m_ctrl_pnts_gpu = nullptr;
	if (m_sampler != nullptr)
	{
		double duration = m_sampler->get_duration();
		int width = (int)ceil(duration*m_scale_out);
		this->setFixedWidth(width);

		std::vector<QVector4D> ctrl_pnts;

		float start_pos = m_sampler->start_pos();
		float start_slope = m_sampler->start_slope();
		ctrl_pnts.push_back({ 0.0f, start_pos, start_slope, 0.0 });

		float min_v = start_pos;
		float max_v = -start_pos;
		size_t num_ctrl_pnts = m_sampler->num_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float x, y, slope;
			m_sampler->control_point(i, x, y, slope);
			if (y < min_v) min_v = y;
			if (y > max_v) max_v = y;
			ctrl_pnts.push_back({ x,y,slope, 0.0 });
		}	

		{
			float x = m_sampler->get_duration();
			float y, slope;
			m_sampler->time_map(x, y, slope);
			if (y > max_v) max_v = y;
		}

		int height = (int)(ceil((max_v - min_v + m_bottom_margin)*m_scale_in));
		this->setFixedHeight(height);
		m_offset_in = m_bottom_margin - min_v;

		m_ctrl_pnts_gpu = (std::unique_ptr<GLBuffer>)(new GLBuffer(&m_gl, sizeof(QVector4D)*ctrl_pnts.size(), GL_SHADER_STORAGE_BUFFER));
		m_ctrl_pnts_gpu->upload(ctrl_pnts.data());
	}
}

void TimeMap::delete_ctrl_pnt()
{
	if (m_sampler == nullptr || m_is_locked || m_selected_id < 0) return;
	m_sampler->remove_control_point(m_selected_id);
	if (m_selected_id >= m_sampler->num_control_points())
	{
		m_selected_id = (int)m_sampler->num_control_points() - 1;
	}
	update_curve();
	update();
	emit sampler_updated();
}

void TimeMap::initializeGL() 
{
	m_gl.initializeOpenGLFunctions();
}

static const char* g_vertex_shader =
"#version 430\n"
"layout (location = 0) uniform vec2 uSize;\n"
"layout (location = 1) uniform float uInterval;\n"
"layout (location = 2) uniform float uOffsetIn;\n"
"layout (location = 3) uniform float uScaleIn;\n"
"layout (location = 4) uniform int uNumCtrlPnts;\n"
"layout(std430, binding = 0) buffer CtrlPnts\n"
"{\n"
"	vec4 ctrl_pnts[];\n"
"};\n"
"int find(float x)\n"
"{\n"
"    if (x<ctrl_pnts[0].x) return 0;\n"
"    int begin = 0;\n"
"    int end = uNumCtrlPnts;\n"
"    while (end > begin + 1)\n"
"    {\n"
"        int mid = (begin + end) >> 1;\n"
"        if (x < ctrl_pnts[mid].x) end = mid;\n"
"        else begin = mid;\n"
"    }\n"
"    return end;\n"
"}\n"
"void main()\n"
"{\n"
"   float x = uInterval* float(gl_VertexID);\n"
"   float y = 0.0;\n"
"   int i = find(x);\n"
"   if (i>0 && i<uNumCtrlPnts)\n"
"   {\n"
"       vec3 ctrl0 = ctrl_pnts[i-1].xyz;\n"
"       vec3 ctrl1 = ctrl_pnts[i].xyz;\n"
"       float t = (x - ctrl0.x)/(ctrl1.x - ctrl0.x);\n"
"       float t2 = t*t;\n"
"       float t3 = t2*t;\n"
"       y = (2.0*t3 - 3.0*t2 + 1)*ctrl0.y + (-2.0*t3 + 3.0*t2)*ctrl1.y\n"
"                + (ctrl1.x - ctrl0.x)*((t3 - 2.0*t2 + t) * ctrl0.z + (t3 - t2) * ctrl1.z);\n"
"   }\n"
"   else\n"
"   {\n"
"       vec3 ctrl0 = ctrl_pnts[uNumCtrlPnts-1].xyz;\n"
"       float t = x - ctrl0.x;\n"
"       float d_s = 1.0 - ctrl0.z;\n"
"       float A = 5.0;\n"
"       if (d_s < 0.0) A = -5.0;\n"
"       float dur = d_s / A;\n"
"       if (t< dur) y = (0.5 * A)*t*t + ctrl0.z * t + ctrl0.y;\n"
"       else y = (0.5 * A)*dur*dur + ctrl0.z * dur + ctrl0.y + (t - dur);\n"
"   }\n"
"   vec2 pos_pix;\n"
"   pos_pix.x = float(gl_VertexID);\n"
"   pos_pix.y = uSize.y - (y + uOffsetIn)* uScaleIn; \n"
"	vec2 pos_out;\n"
"	pos_out.x = pos_pix.x / uSize.x *2.0 - 1.0;\n"
"	pos_out.y = 1.0 - pos_pix.y / uSize.y *2.0;\n"
"	gl_Position = vec4(pos_out, 0.0, 1.0);\n"
"}\n";

static const char* g_frag_shader =
"#version 430\n"
"out vec4 outColor;\n"
"void main()\n"
"{\n"
"   outColor = vec4(1.0,1.0,1.0,1.0);\n"
"}\n";

void TimeMap::paintGL()
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

	if (m_ctrl_pnts_gpu != nullptr)
	{
		m_gl.glDisable(GL_DEPTH_TEST);
		m_gl.glUseProgram(m_prog->m_id);		
		m_gl.glUniform2f(0, (float)this->width(), (float)this->height());
		m_gl.glUniform1f(1, 1.0f / m_scale_out);
		m_gl.glUniform1f(2, m_offset_in);
		m_gl.glUniform1f(3, m_scale_in);
		m_gl.glUniform1i(4, m_ctrl_pnts_gpu->m_size / sizeof(QVector4D));
		m_gl.glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ctrl_pnts_gpu->m_id);
		m_gl.glDrawArrays(GL_LINE_STRIP, 0, this->width());
		m_gl.glUseProgram(0);
	}
	painter.endNativePainting();


	if (m_sampler != nullptr)
	{
		{
			float x = 0.0f;
			float pos_y = m_sampler->start_pos();
			float y = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
			if (m_selected_id == -1)
			{
				painter.setPen(QColor(0, 255, 0));
				float slope = m_sampler->start_slope();
				float pix_slope = -slope * m_scale_in / m_scale_out;
				float dx = 1.0f / sqrtf(pix_slope * pix_slope + 1.0f);
				float dy = pix_slope * dx;
				float end_x = x + dx * m_len_slope_vec;
				float end_y = y + dy * m_len_slope_vec;
				painter.drawLine(QPointF(x, y), QPointF(end_x, end_y));
				painter.drawRect((int)end_x - 4, (int)end_y - 4, 8, 8);
			}
			else
			{
				painter.setPen(QColor(255, 255, 255));
			}
			painter.drawRect((int)x - 4, (int)y -4, 8,8);
		}

		size_t num_ctrl_pnts = m_sampler->num_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float pos_x, pos_y, slope;
			m_sampler->control_point(i, pos_x, pos_y, slope);
			float x = pos_x * m_scale_out;
			float y = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
			if (m_selected_id == i)
			{
				painter.setPen(QColor(0, 255, 0));
				float pix_slope = -slope * m_scale_in / m_scale_out;
				float dx = 1.0f / sqrtf(pix_slope * pix_slope + 1.0f);
				float dy = pix_slope * dx;
				float end_x = x + dx * m_len_slope_vec;
				float end_y = y + dy * m_len_slope_vec;
				painter.drawLine(QPointF(x, y), QPointF(end_x, end_y));
				painter.drawRect((int)end_x - 4, (int)end_y - 4, 8, 8);
			}
			else
			{
				painter.setPen(QColor(255, 255, 255));
			}			
			painter.drawRect((int)x - 4, (int)y - 4, 8, 8);
		}

		if (m_cursor_pos >= 0.0)
		{
			float x = m_cursor_pos * m_scale_out;
			float pos_y, slope;
			m_sampler->time_map(m_cursor_pos, pos_y, slope);
			float y = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
			painter.setPen(QColor(255, 0, 0, 100));
			painter.drawLine(QPointF(0.0f, y), QPointF((float)this->width(), y));
			painter.drawLine(QPointF(x, 0.0f), QPointF(x, (float)this->height()));
		}
	}


	{
		int y = this->height() - m_scroll_offset_in;
		painter.fillRect(0, y - 15, this->width(), 15, QColor(255,255,255, 80));
	}

	{
		int x = m_scroll_offset_out;
		painter.fillRect(x, 0, 50, this->height(), QColor(255, 255, 255, 80));
	}

	
	// ruler-h
	{
		int m = 0;
		float interval = 0.01f;
		while (interval * m_scale_out < 60.0f)
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

		float y = (float)(this->height() - m_scroll_offset_in);

		painter.setPen(QColor(0, 128, 0));
		for (float pos = 0.0f; pos*m_scale_out < (float)this->width(); pos += interval * 0.1f)
		{
			float x = pos * m_scale_out;
			painter.drawLine(QPointF(x, y), QPointF(x,y - 8.0f));
		}

		painter.setPen(QColor(0, 255, 0));

		
		for (float pos = 0.0f; pos*m_scale_out < (float)this->width(); pos += interval)
		{
			float x = pos * m_scale_out;
			painter.drawLine(QPointF(x, y), QPointF(x, y - 12.0f));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x+3.0f, y-3.0f), QString(buf));
		}
	}

	// ruler-v
	{
		int m = 0;
		float interval = 0.01f;
		while (interval * m_scale_in < 60.0f)
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

		float x = (float)m_scroll_offset_out;

		painter.setPen(QColor(0, 128, 0));
		for (float pos = 0.0f; (pos + m_offset_in) * m_scale_in < (float)this->height(); pos += interval*0.1f)
		{
			float y = (float)this->height() - (pos + m_offset_in) * m_scale_in;
			painter.drawLine(QPointF(x, y), QPointF(x+8.0f, y));
		}

		for (float pos = -interval * 0.1f; (pos + m_offset_in) * m_scale_in >= 0.0f; pos -= interval * 0.1f)
		{
			float y = (float)this->height() - (pos + m_offset_in) * m_scale_in;
			painter.drawLine(QPointF(x, y), QPointF(x + 8.0f, y));
		}

		painter.setPen(QColor(0, 255, 0));
		for (float pos = 0.0f; (pos+ m_offset_in) * m_scale_in < (float)this->height(); pos += interval)
		{
			float y = (float)this->height() - (pos + m_offset_in) * m_scale_in;
			painter.drawLine(QPointF(x, y), QPointF(x + 12.0f, y));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x+10.0f, y - 2.0f), QString(buf));
		}

		for (float pos = -interval; (pos + m_offset_in) * m_scale_in >= 0.0f; pos -= interval)
		{
			float y = (float)this->height() - (pos + m_offset_in) * m_scale_in;
			painter.drawLine(QPointF(x, y), QPointF(x + 12.0f, y));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x+10.0f, y - 2.0f), QString(buf));
		}

	}	

	painter.end();
}

void TimeMap::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (m_sampler==nullptr || m_is_locked) return;
	float x = (float)event->x();
	float y = (float)event->y();
	float pos_x = x / m_scale_out; 
	float pos_y = ((float)this->height() - y) / m_scale_in - m_offset_in;
	m_selected_id = m_sampler->add_control_point(pos_x, pos_y);
	update_curve();
	update();
	emit sampler_updated();
}

void TimeMap::mousePressEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;

	float x = (float)event->x();
	float y = (float)event->y();

	{
		float pos_x, pos_y, slope;
		if (m_selected_id == -1)
		{
			pos_x = 0.0f;
			pos_y = m_sampler->start_pos();
			slope = m_sampler->start_slope();
		}
		else
		{
			m_sampler->control_point(m_selected_id, pos_x, pos_y, slope);
		}
		float x_ctrl = pos_x * m_scale_out;
		float y_ctrl = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
		float pix_slope = -slope * m_scale_in / m_scale_out;
		float dx = 1.0f / sqrtf(pix_slope * pix_slope + 1.0f);
		float dy = pix_slope * dx;
		float end_x = x_ctrl + dx * m_len_slope_vec;
		float end_y = y_ctrl + dy * m_len_slope_vec;

		if (fabsf(x - end_x) < m_size_ctrl_pnt && fabsf(y - end_y) < m_size_ctrl_pnt)
		{
			m_dragging_slope = true;
			return;
		}		
	}


	{
		float x_ctrl = 0.0f;
		float pos_y = m_sampler->start_pos();
		float y_ctrl = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;		

		if (fabsf(x - x_ctrl) < m_size_ctrl_pnt && fabsf(y - y_ctrl) < m_size_ctrl_pnt)
		{
			m_selected_id = -1;
			m_dragging_ctrl = true;
			update();
			return;
		}
	}
	
	size_t num_ctrl_pnt = m_sampler->num_control_points();
	for (size_t i = 0; i < num_ctrl_pnt; i++)
	{
		float pos_x, pos_y, slope;
		m_sampler->control_point(i, pos_x, pos_y, slope);
		float x_ctrl = pos_x * m_scale_out;
		float y_ctrl = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
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

void TimeMap::mouseReleaseEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	if (m_dragging_ctrl)
	{	
		m_dragging_ctrl = false;
	}
	if (m_dragging_slope)
	{
		m_dragging_slope = false;
	}
	if (m_moving_cursor)
	{
		m_moving_cursor = false;
	}
}

void TimeMap::mouseMoveEvent(QMouseEvent *event)
{
	if (m_sampler == nullptr || m_is_locked) return;
	float x = (float)event->x();
	float y = (float)event->y();

	if (m_dragging_slope)
	{
		float pos_x, pos_y;
		if (m_selected_id == -1)
		{
			pos_x = 0.0f;
			pos_y = m_sampler->start_pos();			
		}
		else
		{
			float slope;
			m_sampler->control_point(m_selected_id, pos_x, pos_y, slope);
		}
		float x_ctrl = pos_x * m_scale_out;
		float y_ctrl = (float)this->height() - (pos_y + m_offset_in) * m_scale_in;
		float dx = x - x_ctrl;
		float dy = y - y_ctrl;
		float slope = 0.0f;
		if (dx > 0.0f)
		{
			slope = -dy / dx * m_scale_out / m_scale_in;
			if (slope < -m_slope_limit) slope = -m_slope_limit;
			if (slope > m_slope_limit) slope = m_slope_limit;
			
		}
		else
		{
			if (dy > 0.0f) slope = -m_slope_limit;
			else if (dy < 0.0f) slope = m_slope_limit;
		}

		if (m_selected_id == -1)
		{
			m_sampler->set_start_slope(slope);
		}
		else
		{
			m_sampler->set_control_point_slope(m_selected_id, slope);
		}

		update_curve();
		update();
		emit sampler_updated();
	}
	else if (m_dragging_ctrl)
	{
		float pos_x = x / m_scale_out;
		float pos_y = ((float)this->height() - y) / m_scale_in - m_offset_in;

		if (m_selected_id == -1)
		{
			m_sampler->set_start_pos(pos_y);
			m_cursor_pos = 0.0f;			
		}
		else
		{
			float x_old, y_old, slope_old;
			m_sampler->control_point(m_selected_id, x_old, y_old, slope_old);
			m_sampler->remove_control_point(m_selected_id);
			m_selected_id = m_sampler->add_control_point(pos_x, pos_y);
			m_sampler->set_control_point_slope(m_selected_id, slope_old);
			m_cursor_pos = pos_x;			
		}
		update_curve();
		emit cursor_moved(m_cursor_pos);
		emit sampler_updated();
	}
	else if (m_moving_cursor)
	{
		m_cursor_pos = x / m_scale_out;		
		emit cursor_moved(m_cursor_pos);
	}

}
