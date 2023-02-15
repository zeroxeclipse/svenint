#include <regex>

#include <dbg.h>
#include <ISvenModAPI.h>

#include "opengl.h"

#include "../utils/xorstr.h"
#include "../patterns.h"

GL_BindFn GL_Bind = NULL;

//-----------------------------------------------------------------------------
// Startup & shutdown
//-----------------------------------------------------------------------------

bool GL_Init()
{
	// Init glew
	GLenum status = glewInit();

	if ( status != GLEW_OK )
	{
		Warning(xs("[Sven Internal] Failed to initialize GLEW. Reason: %s\n"), glewGetErrorString(status));
		return false;
	}

	void *pfnGL_Bind = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::GL_Bind );

	if ( pfnGL_Bind == NULL )
	{
		Warning(xs("Failed to find function \"GL_Bind\"\n"));
		return false;
	}

	GL_Bind = (GL_BindFn)pfnGL_Bind;

	return true;
}

void GL_Shutdown()
{
}

//-----------------------------------------------------------------------------
// Textures
//-----------------------------------------------------------------------------

GLuint GL_GenTexture(void)
{
	GLuint tex;
	glGenTextures(1, &tex);
	return tex;
}

void GL_UploadDepthStencilTexture(int texId, int w, int h)
{
	glBindTexture(GL_TEXTURE_2D, texId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, w, h);
	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenDepthStencilTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();
	GL_UploadDepthStencilTexture(texid, w, h);
	return texid;
}

GLuint GL_GenDepthTexture(int w, int h)
{
	GLuint texid = GL_GenTexture();

	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texid;
}

void GL_UploadTextureColorFormat(int texid, int w, int h, int iInternalFormat)
{
	glBindTexture(GL_TEXTURE_2D, texid);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	//glTexStorage2D doesnt work with qglCopyTexImage2D so we use glTexImage2D here
	glTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA, (iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint GL_GenTextureColorFormat(int w, int h, int iInternalFormat)
{
	GLuint texid = GL_GenTexture();
	GL_UploadTextureColorFormat(texid, w, h, iInternalFormat);
	return texid;
}

GLuint GL_GenTextureRGBA8(int w, int h)
{
	return GL_GenTextureColorFormat(w, h, GL_RGBA8);
}

GLuint GL_GenTextureRGB8(int w, int h)
{
	return GL_GenTextureColorFormat(w, h, GL_RGB8);
}

void GL_BlitFrameBufferToFrameBufferColorOnly(GLuint src, GLuint dst, int w1, int h1, int w2, int h2)
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBlitFramebuffer(0, 0, w1, h1, 0, 0, w2, h2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

//-----------------------------------------------------------------------------
// Shader program abstraction layer
//-----------------------------------------------------------------------------

GLuint CShaderProgram::m_currentProgram = -1;
std::vector<CShaderProgram::CGLShader> CShaderProgram::m_shaders;

CShaderProgram::CShaderProgram() : m_program(0)
{
}

bool CShaderProgram::Compile( const char *pszVertexCode, const char *pszFragmentCode, const char *pszVertexDefine /* = NULL */, const char *pszFragmentDefine /* = NULL */ )
{
	std::string vs = std::string(pszVertexCode);

	AppendDefine(vs, "#define IS_VERTEX_SHADER\n");
	
	if ( pszVertexDefine )
	{
		AppendDefine(vs, pszVertexDefine);
	}

	std::string fs = std::string(pszFragmentCode);

	AppendDefine(fs, "#define IS_FRAGMENT_SHADER\n");
	
	if ( pszFragmentDefine )
	{
		AppendDefine(fs, pszFragmentDefine);
	}

	// Not supported
/*
	if ( vs.find("#include") != std::string::npos )
	{
		AppendInclude(vs, pszVertexFile);
	}

	if ( fs.find("#include") != std::string::npos )
	{
		AppendInclude(fs, pszFragmentFile);
	}
*/

	return ( m_program = InternalCompile(vs.c_str(), fs.c_str(), NULL, NULL) ) != 0;
}

bool CShaderProgram::CompileFile( const char *pszVertexFile, const char *pszFragmentFile, const char *pszVertexDefine /* = NULL */, const char *pszFragmentDefine /* = NULL */ )
{
	auto vscode = (char *)g_pEngineFuncs->COM_LoadFile( (char *)pszVertexFile, 5, 0 );
	
	std::string vs;

	if ( !vscode )
		Sys_Error("CShaderProgram::CompileFile: \"%s\" not found", pszVertexFile);
	else
		vs = std::string(vscode);

	AppendDefine(vs, "#define IS_VERTEX_SHADER\n");
	
	if ( pszVertexDefine )
	{
		AppendDefine(vs, pszVertexDefine);
	}

	g_pEngineFuncs->COM_FreeFile( vscode );

	auto fscode = (char *)g_pEngineFuncs->COM_LoadFile( (char *)pszFragmentFile, 5, 0 );
	
	std::string fs;
	
	if ( !fscode )
		Sys_Error("CShaderProgram::CompileFile: \"%s\" not found", pszFragmentFile);
	else
		fs = std::string(fscode);

	AppendDefine(fs, "#define IS_FRAGMENT_SHADER\n");
	
	if ( pszFragmentDefine )
	{
		AppendDefine(fs, pszFragmentDefine);
	}

	g_pEngineFuncs->COM_FreeFile( fscode );

	if ( vs.find("#include") != std::string::npos )
	{
		AppendInclude(vs, pszVertexFile);
	}

	if ( fs.find("#include") != std::string::npos )
	{
		AppendInclude(fs, pszFragmentFile);
	}

	return ( m_program = InternalCompile(vs.c_str(), fs.c_str(), pszVertexFile, pszFragmentFile) ) != 0;
}

void CShaderProgram::Bind( void ) const
{
	if ( m_currentProgram != m_program )
	{
		m_currentProgram = m_program;
		glUseProgramObjectARB(m_program);
	}
}

void CShaderProgram::Unbind( void )
{
	if ( m_currentProgram != 0 )
	{
		m_currentProgram = 0;
		glUseProgramObjectARB(0);
	}
}

bool CShaderProgram::Free( void )
{
	if ( !Compiled() )
		return false;

	for (size_t i = 0; i < m_shaders.size(); ++i)
	{
		if ( m_shaders[i].program == m_program )
		{
			auto &objs = m_shaders[i].shader_objects;

			for (size_t j = 0; j < objs.size(); ++j)
			{
				glDetachObjectARB( m_program, objs[j] );
				glDeleteObjectARB( objs[j] );
			}

			glDeleteProgramsARB( 1, &m_program );

			m_shaders.erase( m_shaders.begin() + i );
			return true;
		}
	}

	return false;
}

void CShaderProgram::FreeShaders( void )
{
	for (size_t i = 0; i < m_shaders.size(); ++i)
	{
		auto &objs = m_shaders[i].shader_objects;

		for (size_t j = 0; j < objs.size(); ++j)
		{
			glDetachObjectARB( m_shaders[i].program, objs[j] );
			glDeleteObjectARB( objs[j] );
		}

		glDeleteProgramsARB( 1, &m_shaders[i].program );
	}

	m_shaders.clear();
}

GLuint CShaderProgram::InternalCompile(const char *vscode, const char *fscode, const char *vsfile, const char *fsfile)
{
	auto CompileShaderObject = [](int type, const char *code, const char *filename) -> GLuint
	{
		auto obj = glCreateShaderObjectARB(type);

		glShaderSource(obj, 1, &code, NULL);

		glCompileShader(obj);

		// Check for errors
		int iStatus;
		glGetShaderiv(obj, GL_COMPILE_STATUS, &iStatus);

		if ( !iStatus )
		{
			int nInfoLength;
			char szCompilerLog[1024] = { 0 };
			glGetInfoLogARB(obj, sizeof(szCompilerLog) - 1, &nInfoLength, szCompilerLog);
			szCompilerLog[nInfoLength] = 0;

			Sys_Error("Shader \"%s\" compiled with error:\n%s", code /* filename */, szCompilerLog);
		}

		return obj;
	};

	GLuint shader_objects[32];
	int shader_object_used = 0;

	shader_objects[shader_object_used] = CompileShaderObject(GL_VERTEX_SHADER_ARB, vscode, vsfile);
	shader_object_used++;

	shader_objects[shader_object_used] = CompileShaderObject(GL_FRAGMENT_SHADER_ARB, fscode, fsfile);
	shader_object_used++;

	GLuint program = glCreateProgramObjectARB();
	for (int i = 0; i < shader_object_used; ++i)
		glAttachObjectARB(program, shader_objects[i]);
	glLinkProgramARB(program);

	int iStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &iStatus);

	if ( !iStatus )
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetProgramInfoLog(program, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

		Sys_Error("Shader \"%s\" compiled with error:\n%s", vscode, szCompilerLog);
	}

	m_shaders.emplace_back( program, shader_objects, shader_object_used );

	return program;
}

void CShaderProgram::AppendInclude(std::string &str, const char *filename)
{
	std::regex pattern("#include[< \"]+([a-zA-Z_\\.]+)[> \"]");
	std::smatch result;
	std::regex_search(str, result, pattern);

	std::string skipped;

	std::string::const_iterator searchStart(str.cbegin());

	while ( std::regex_search(searchStart, str.cend(), result, pattern) && result.size() >= 2 )
	{
		std::string prefix = result.prefix();
		std::string suffix = result.suffix();

		auto includeFileName = result[1].str();

		char slash = 0;

		std::string includePath = filename;

		for (size_t j = includePath.length() - 1; j > 0; --j)
		{
			if ( includePath[j] == '\\' || includePath[j] == '/' )
			{
				slash = includePath[j];
				includePath.resize(j);

				break;
			}
		}

		includePath += slash;
		includePath += includeFileName;

		auto pFile = g_pEngineFuncs->COM_LoadFile( (char *)includePath.c_str(), 5, NULL );
		
		if ( pFile )
		{
			std::string wbinding( (char *)pFile );

			g_pEngineFuncs->COM_FreeFile( pFile );

			if ( searchStart != str.cbegin() )
			{
				str = skipped + prefix;
			}
			else
			{
				str = prefix;
			}

			str += wbinding;

			auto currentLength = str.length();

			str += suffix;

			skipped = str.substr(0, currentLength);
			searchStart = str.cbegin() + currentLength;

			continue;
		}

		searchStart = result.suffix().first;
	}
}

void CShaderProgram::AppendDefine(std::string &str, const std::string &def)
{
	std::regex pattern("(#version [0-9a-z ]+)");
	std::smatch result;
	std::regex_search(str, result, pattern);

	if ( result.size() >= 1 )
	{
		std::string prefix = result[0];
		std::string suffix = result.suffix();

		str = prefix;
		str += "\n\n";
		str += def;
		str += "\n\n";
		str += suffix;
	}
	else
	{
		std::string suffix = str;

		str = def;
		str += "\n\n";
		str += suffix;
	}
}