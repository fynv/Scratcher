#pragma once

class QOpenGLExtraFunctions;

struct GLBuffer
{
	QOpenGLExtraFunctions* m_gl;
	unsigned m_id = -1;
	unsigned m_target = 0x8892;
	size_t m_size = 0;
	GLBuffer(QOpenGLExtraFunctions* gl, size_t size, unsigned target = 0x8892 /*GL_ARRAY_BUFFER*/);
	~GLBuffer();
	void upload(const void* data);
	const GLBuffer& operator = (const GLBuffer& in);
};


struct GLShader
{
	QOpenGLExtraFunctions* m_gl;
	unsigned m_type = 0;
	unsigned m_id = -1;
	GLShader(QOpenGLExtraFunctions* gl, unsigned type, const char* code);
	~GLShader();

};

struct GLProgram
{
	QOpenGLExtraFunctions* m_gl;
	unsigned m_id = -1;
	GLProgram(QOpenGLExtraFunctions* gl, const GLShader& vertexShader, const GLShader& fragmentShader);
	GLProgram(QOpenGLExtraFunctions* gl, const GLShader& computeShader);
	~GLProgram();

};