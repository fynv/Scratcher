#include <QOpenGLExtraFunctions>
#include "GLUtils.h"


GLBuffer::GLBuffer(QOpenGLExtraFunctions* gl, size_t size, unsigned target) 
	: m_gl(gl)
	, m_target(target)
	, m_size(size)
{
	m_gl->glGenBuffers(1, &m_id);
	m_gl->glBindBuffer(m_target, m_id);
	m_gl->glBufferData(m_target, m_size, nullptr, GL_STATIC_DRAW);
	m_gl->glBindBuffer(m_target, 0);
}

GLBuffer::~GLBuffer()
{
	if (m_id != -1)
		m_gl->glDeleteBuffers(1, &m_id);
}

void GLBuffer::upload(const void* data)
{
	m_gl->glBindBuffer(m_target, m_id);
	m_gl->glBufferData(m_target, m_size, data, GL_STATIC_DRAW);
	m_gl->glBindBuffer(m_target, 0);
}

const GLBuffer& GLBuffer::operator = (const GLBuffer& in)
{
	m_gl->glBindBuffer(GL_COPY_READ_BUFFER, in.m_id);
	m_gl->glBindBuffer(GL_COPY_WRITE_BUFFER, m_id);
	m_gl->glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size);
	m_gl->glBindBuffer(GL_COPY_READ_BUFFER, 0);
	m_gl->glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
	return *this;
}


GLShader::GLShader(QOpenGLExtraFunctions* gl, unsigned type, const char* code)
	: m_gl(gl)
	, m_type(type)
{
	m_id = m_gl->glCreateShader(type);
	m_gl->glShaderSource(m_id, 1, &code, nullptr);
	m_gl->glCompileShader(m_id);

	GLint compileResult;
	m_gl->glGetShaderiv(m_id, GL_COMPILE_STATUS, &compileResult);
	if (compileResult == 0)
	{
		GLint infoLogLength;
		m_gl->glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<GLchar> infoLog(infoLogLength);
		m_gl->glGetShaderInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

		printf("Shader compilation failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
	}	
}

GLShader::~GLShader()
{
	if (m_id != -1)
		m_gl->glDeleteShader(m_id);
}


GLProgram::GLProgram(QOpenGLExtraFunctions* gl, const GLShader& vertexShader, const GLShader& fragmentShader) : m_gl(gl)
{
	m_id = m_gl->glCreateProgram();
	m_gl->glAttachShader(m_id, vertexShader.m_id);
	m_gl->glAttachShader(m_id, fragmentShader.m_id);
	m_gl->glLinkProgram(m_id);

	GLint linkResult;
	m_gl->glGetProgramiv(m_id, GL_LINK_STATUS, &linkResult);
	if (linkResult == 0)
	{
		GLint infoLogLength;
		m_gl->glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<GLchar> infoLog(infoLogLength);
		m_gl->glGetProgramInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

		printf("Shader link failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
	}
}

GLProgram::GLProgram(QOpenGLExtraFunctions* gl, const GLShader& computeShader)
{
	m_id = m_gl->glCreateProgram();
	m_gl->glAttachShader(m_id, computeShader.m_id);
	m_gl->glLinkProgram(m_id);

	GLint linkResult;
	m_gl->glGetProgramiv(m_id, GL_LINK_STATUS, &linkResult);
	if (linkResult == 0)
	{
		GLint infoLogLength;
		m_gl->glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::vector<GLchar> infoLog(infoLogLength);
		m_gl->glGetProgramInfoLog(m_id, (GLsizei)infoLog.size(), NULL, infoLog.data());

		printf("Shader link failed: %s", std::string(infoLog.begin(), infoLog.end()).c_str());
	}
}

GLProgram::~GLProgram()
{
	if (m_id != -1)
		m_gl->glDeleteProgram(m_id);
}
