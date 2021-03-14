#pragma once

#include <vector>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>

class SamplerScratch;
struct GLBuffer;
struct GLShader;
struct GLProgram;
class SourceView : public QOpenGLWidget
{
	Q_OBJECT
public:
	SourceView(QWidget* parent);
	~SourceView();

	void set_sampler(SamplerScratch* sampler)
	{
		m_sampler = sampler;
		update_shape();
		m_cursor_pos = 0.0;
	}

	void update_shape();

	void set_scale(float scale)
	{
		m_scale = scale;
		update_shape();
	}

	void set_cursor_pos(float pos);

public slots:
	void update_sampler()
	{
		update_shape();
		update();
	}

protected:
	virtual void initializeGL() override;
	virtual void paintGL() override;

private:
	QOpenGLExtraFunctions m_gl;

	float m_margin = 1.0f;
	float m_v_scale = 80.0f;

	SamplerScratch* m_sampler = nullptr;
	float m_scale = 200.0f;
	float m_cursor_pos = 0.0f;

	std::unique_ptr<GLBuffer> m_shape_gpu;
	std::unique_ptr<GLShader> m_vert_shader;
	std::unique_ptr<GLShader> m_frag_shader;
	std::unique_ptr<GLProgram> m_prog;
};

