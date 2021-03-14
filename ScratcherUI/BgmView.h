#pragma once

#include <memory>
#include <vector>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>

class SamplerScratch;
struct GLBuffer;
struct GLShader;
struct GLProgram;
class BgmView : public QOpenGLWidget
{
	Q_OBJECT
public:
	BgmView(QWidget* parent);
	~BgmView();

	void set_sampler(SamplerScratch* sampler)
	{
		m_sampler = sampler;
		update_shape();
		m_cursor_pos = 0.0;
	}

	void update_shape();

	float scale() const { return m_scale; }
	int scroll_offset() const { return m_scroll_offset; }

	void set_scale(float scale)
	{
		m_scale = scale;
		update_shape();
	}

	void set_scroll_offset(int v)
	{
		m_scroll_offset = v;
	}

	double cursor_pos() const { return m_cursor_pos; }

	void set_cursor_pos(double pos)
	{
		m_cursor_pos = pos;
	}

	void set_locked(bool locked)
	{
		m_is_locked = locked;
	}

public slots:
	void update_sampler()
	{
		update_shape();
		update();
	}

signals:
	void cursor_moved(double pos);

protected:
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;

private:
	QOpenGLExtraFunctions m_gl;

	float m_margin = 1.0f;
	float m_v_scale = 80.0f;

	SamplerScratch* m_sampler = nullptr;
	float m_scale = 200.0f;

	std::unique_ptr<GLBuffer> m_shape_gpu;
	std::unique_ptr<GLShader> m_vert_shader;
	std::unique_ptr<GLShader> m_frag_shader;
	std::unique_ptr<GLProgram> m_prog;

	int m_scroll_offset = 0;
	double m_cursor_pos = 0.0;
	bool m_is_locked = false;
	bool m_moving_cursor = false;

};

