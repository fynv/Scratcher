#include <cmath>
#include <cstdint>
#include <cfloat>
#include <vector>
#include <QPainter>
#include <QMouseEvent>
#include <SamplerScratch.h>
#include <TrackBuffer.h>
#include "BgmView.h"
#include "GLUtils.h"

BgmView::BgmView(QWidget* parent)
	: QOpenGLWidget(parent)
{

}

BgmView::~BgmView()
{
	m_prog = nullptr;
	m_frag_shader = nullptr;
	m_vert_shader = nullptr;
	m_shape_gpu = nullptr;
}


void BgmView::update_shape()
{
	m_shape_gpu = nullptr;
	if (m_sampler != nullptr)
	{
		double duration = m_sampler->get_duration();
		int width = (int)ceil(duration * m_scale);
		this->setFixedWidth(width);

		TrackBuffer* buffer = m_sampler->bgm();
		uint32_t sample_rate = buffer->Rate();

		std::vector<float> v_min_v(width, 0.0f);
		std::vector<float> v_max_v(width, 0.0f);

		int i_as = 0;
		for (int i = 0; i < width; i++)
		{
			float t_next = (float)(i + 1) / m_scale;
			float min_v_i = 0.0f;
			float max_v_i = 0.0f;
			for (; (float)i_as / (float)sample_rate < t_next; i_as++)
			{
				if (i_as < buffer->NumberOfSamples())
				{
					float as[2];
					buffer->Sample(i_as, as);
					if (as[0] < min_v_i) min_v_i = as[0];
					if (as[0] > max_v_i) max_v_i = as[0];
					if (as[1] < min_v_i) min_v_i = as[1];
					if (as[1] > max_v_i) max_v_i = as[1];
				}
				else
				{
					break;
				}
			}
			v_min_v[i] = min_v_i;
			v_max_v[i] = max_v_i;
		}

		std::vector<float> verts(width*2);		
		for (size_t i = 0; i < width; i++)
		{	
			verts[i * 2] = -v_max_v[i] * m_v_scale;
			verts[i * 2 + 1] = -v_min_v[i] * m_v_scale;			
		}
		m_shape_gpu = (std::unique_ptr<GLBuffer>)(new GLBuffer(&m_gl, sizeof(float) * verts.size()));
		m_shape_gpu->upload(verts.data());
	}
}

void BgmView::initializeGL()
{
	m_gl.initializeOpenGLFunctions();
}


static const char* g_vertex_shader =
"#version 430\n"
"layout (location = 0) in float aPos;\n"
"layout (location = 0) uniform vec2 uSize;\n"
"void main()\n"
"{\n"
"	vec2 pos_trans = vec2(float(gl_VertexID/2), aPos);\n"
"	vec2 pos_out;\n"
"	pos_out.x = pos_trans.x / uSize.x *2.0 - 1.0;\n"
"	pos_out.y = - pos_trans.y / uSize.y *2.0;\n"
"	gl_Position = vec4(pos_out, 0.0, 1.0);\n"
"}\n";

static const char* g_frag_shader =
"#version 430\n"
"out vec4 outColor;"
"void main()\n"
"{\n"
"   outColor = vec4(0.0,0.0,1.0,1.0);\n"
"}\n";

void BgmView::paintGL()
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

	if (m_shape_gpu != nullptr)
	{
		m_gl.glDisable(GL_DEPTH_TEST);
		m_gl.glUseProgram(m_prog->m_id);
		m_gl.glBindBuffer(GL_ARRAY_BUFFER, m_shape_gpu->m_id);
		m_gl.glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_gl.glEnableVertexAttribArray(0);
		m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_gl.glUniform2f(0, (float)this->width(), (float)this->height());	
		m_gl.glDrawArrays(GL_LINES, 0, m_shape_gpu->m_size / sizeof(float));
		m_gl.glUseProgram(0);
	}
	painter.endNativePainting();

	{
		float x = m_cursor_pos * m_scale;
		painter.setPen(QColor(255, 0, 0, 100));
		painter.drawLine(QPointF(x, 0.0f), QPointF(x, (float)this->height()));
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
		for (float pos = 0.0f; pos * m_scale < (float)this->width(); pos += interval * 0.1f)
		{
			float x = pos * m_scale;
			painter.drawLine(QPointF(x, y), QPointF(x, y - 8.0f));
		}

		painter.setPen(QColor(0, 255, 0));


		for (float pos = 0.0f; pos * m_scale < (float)this->width(); pos += interval)
		{
			float x = pos * m_scale;
			painter.drawLine(QPointF(x, y), QPointF(x, y - 12.0f));

			char buf[64];
			sprintf(buf, "%.2f", pos);
			painter.drawText(QPointF(x + 3.0f, y - 3.0f), QString(buf));
		}
	}

	painter.end();

}

void BgmView::mousePressEvent(QMouseEvent* event)
{
	if (m_sampler == nullptr || m_is_locked) return;	
	float x = (float)event->x();
	float y = (float)event->y();
	
	m_moving_cursor = true;
}


void BgmView::mouseReleaseEvent(QMouseEvent* event)
{
	if (m_sampler == nullptr || m_is_locked) return;	
	if (m_moving_cursor)
	{
		m_moving_cursor = false;
	}
}


void BgmView::mouseMoveEvent(QMouseEvent* event)
{
	if (m_sampler == nullptr || m_is_locked) return;	
	float x = (float)event->x();
	float y = (float)event->y();
	if (m_moving_cursor)
	{
		m_cursor_pos = x / m_scale;
		emit cursor_moved(m_cursor_pos);
	}
}

