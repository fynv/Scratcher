#include <cmath>
#include <cstdint>
#include <cfloat>
#include <vector>
#include <QPainter>
#include <QVector2D>
#include <SamplerScratch.h>
#include <TrackBuffer.h>
#include "SourceView.h"
#include "GLUtils.h"

SourceView::SourceView(QWidget* parent)
	: QOpenGLWidget(parent)
{

}

SourceView::~SourceView()
{
	m_prog = nullptr;
	m_frag_shader = nullptr;
	m_vert_shader = nullptr;
	m_shape_gpu = nullptr;
}

void SourceView::set_cursor_pos(float pos)
{
	if (m_sampler != nullptr)
	{
		float pos_y, slope;
		m_sampler->time_map(pos, pos_y, slope);
		m_cursor_pos = pos_y;
	}	
}

void SourceView::update_shape()
{
	m_shape_gpu = nullptr;
	if (m_sampler != nullptr)
	{
		TrackBuffer* buffer = m_sampler->buffer();

		float start_pos = m_sampler->start_pos();

		float min_v = start_pos;
		float max_v = start_pos;
		size_t num_ctrl_pnts = m_sampler->num_control_points();
		for (size_t i = 0; i < num_ctrl_pnts; i++)
		{
			float x, y, slope;
			m_sampler->control_point(i, x, y, slope);
			if (y < min_v) min_v = y;
			if (y > max_v) max_v = y;
		}

		{
			float x = m_sampler->get_duration();
			float y, slope;
			m_sampler->time_map(x, y, slope);
			if (y > max_v) max_v = y;
		}

		min_v -= m_margin;
		max_v += m_margin;

		uint32_t sample_rate = buffer->Rate();
		size_t num_audio_samples = (size_t)ceilf((max_v - min_v)*(float)sample_rate);
		int start_audio_sample = (int)(min_v*(float)sample_rate);
		size_t num_pixels = (size_t)ceilf((max_v - min_v)*m_scale);
		if (num_pixels < 1) return;

		std::vector<float> v_min_v(num_pixels, 0.0f);
		std::vector<float> v_max_v(num_pixels, 0.0f);

		int i_as = start_audio_sample;
		for (size_t i = 0; i < num_pixels; i++)
		{
			float t_next = (float)(i + 1) / m_scale + min_v;
			float min_v_i = 0.0f;
			float max_v_i = 0.0f;
			for (; (float)i_as / (float)sample_rate < t_next; i_as++)
			{
				if (i_as >= 0 && i_as < buffer->NumberOfSamples())
				{
					float as[2];
					buffer->Sample(i_as, as);
					if (as[0] < min_v_i) min_v_i = as[0];
					if (as[0] > max_v_i) max_v_i = as[0];
					if (as[1] < min_v_i) min_v_i = as[1];
					if (as[1] > max_v_i) max_v_i = as[1];
				}
			}
			v_min_v[i] = min_v_i;
			v_max_v[i] = max_v_i;
		}

		std::vector<QVector2D> verts;

		float pix_offset = min_v * m_scale;
		for (size_t i = 0; i < num_pixels; i++)
		{
			float x = (float)i + pix_offset;
			float y_min = -v_max_v[i] * m_v_scale;
			float y_max = -v_min_v[i] * m_v_scale;
			verts.push_back({ x,y_min });
			verts.push_back({ x,y_max });
		}
		m_shape_gpu = (std::unique_ptr<GLBuffer>)(new GLBuffer(&m_gl, sizeof(QVector2D)*verts.size()));
		m_shape_gpu->upload(verts.data());
	}
}


void SourceView::initializeGL()
{
	m_gl.initializeOpenGLFunctions();
}

static const char* g_vertex_shader =
"#version 430\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 0) uniform vec2 uSize;\n"
"layout (location = 1) uniform vec2 uTranslate;\n"
"void main()\n"
"{\n"
"	vec2 pos_trans = aPos + uTranslate;\n"
"	vec2 pos_out;\n"
"	pos_out.x = pos_trans.x / uSize.x *2.0 - 1.0;\n"
"	pos_out.y = 1.0 - pos_trans.y / uSize.y *2.0;\n"
"	gl_Position = vec4(pos_out, 0.0, 1.0);\n"
"}\n";


static const char* g_frag_shader =
"#version 430\n"
"out vec4 outColor;"
"void main()\n"
"{\n"
"   outColor = vec4(0.0,0.0,1.0,1.0);\n"
"}\n";

void SourceView::paintGL()
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
		m_gl.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
		m_gl.glEnableVertexAttribArray(0);
		m_gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
		m_gl.glUniform2f(0, (float)this->width(), (float)this->height());
		m_gl.glUniform2f(1, (float)this->width()*0.5f - m_cursor_pos * m_scale, (float)this->height()*0.5f);
		m_gl.glDrawArrays(GL_LINES, 0, m_shape_gpu->m_size / sizeof(QVector2D));
		m_gl.glUseProgram(0);
	}
	painter.endNativePainting();

	painter.setPen(QColor(0, 255, 0));
	painter.drawLine(QPointF((double)this->width()*0.5, 0.0), QPointF((double)this->width()*0.5, (double)this->height()));
	painter.end();
}
