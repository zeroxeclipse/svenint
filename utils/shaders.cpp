#include "shaders.h"

#include <string>
#include <sstream>
#include <regex>

#include <hl_sdk/engine/APIProxy.h>
#include <IMemoryUtils.h>
#include <ISvenModAPI.h>
#include <sys.h>

#include "../game/utils.h"

//-----------------------------------------------------------------------------
// Shader programs abstraction layer
//-----------------------------------------------------------------------------

GLuint CShaderProgram::m_currentProgram = -1;
std::vector<CShaderProgram::CGLShader> CShaderProgram::m_shaders;

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
		Sys_Error("R_CompileShaderFileEx: \"%s\" not found", pszVertexFile);
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
		Sys_Error("R_CompileShaderFileEx: \"%s\" not found", pszFragmentFile);
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

//-----------------------------------------------------------------------------
// OpenGL
//-----------------------------------------------------------------------------

// Vars
typedef void (__cdecl *glBindFn)(GLuint);
glBindFn glBind = NULL;

GLint m_hOldBuffer = 0;

GLuint g_uiDepthBuffer;
GLuint m_hDepthBufferFBO = 0;
GLuint m_hDepthBufferTex = 0;

GLuint m_hGaussianBufferFBO = 0;
GLuint m_hGaussianBufferTex = 0;

GLuint m_hGaussianFastBufferFBO = 0;
GLuint m_hGaussianFastBufferTex = 0;

GLuint m_hBokehBufferFBO = 0;
GLuint m_hBokehBufferTex = 0;

GLuint m_hDoFDepthBuffer = 0;
GLuint m_hDoFBlurBufferFBO = 0;
GLuint m_hDoFBlurBufferTex = 0;

// Initialize
void GL_Init()
{
	int w = Utils()->GetScreenWidth();
	int h = Utils()->GetScreenHeight();

	DEFINE_PATTERN(glBind_sig, "8B 44 24 04 39 05 ? ? ? ? 74 11 50 68");
	void *pfnglBind = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, glBind_sig );

	if ( pfnglBind == NULL )
	{
		Sys_Error("Failed to find function \"glBind\"");
		return;
	}

	glBind = (glBindFn)pfnglBind;

	GL_ShaderInit();

	glGenTextures(1, &g_uiDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, g_uiDepthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffersEXT(1, &m_hGaussianBufferFBO);
	m_hGaussianBufferTex = GL_GenTextureRGB8(w, h);
	
	glGenFramebuffersEXT(1, &m_hDepthBufferFBO);
	m_hDepthBufferTex = GL_GenTextureRGB8(w, h);

	glGenFramebuffersEXT(1, &m_hGaussianFastBufferFBO);
	m_hGaussianFastBufferTex = GL_GenTextureRGB8(w, h);

	glGenFramebuffersEXT(1, &m_hBokehBufferFBO);
	m_hBokehBufferTex = GL_GenTextureRGB8(w, h);

	glGenTextures(1, &m_hDoFDepthBuffer);
	glBindTexture(GL_TEXTURE_2D, m_hDoFDepthBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffersEXT(1, &m_hDoFBlurBufferFBO);
	m_hDoFBlurBufferTex = GL_GenTextureRGB8(w, h);
}

// Shutdown
void GL_Shutdown()
{
	if ( g_uiDepthBuffer )
		glDeleteTextures(1, &g_uiDepthBuffer);
	
	if ( m_hDepthBufferTex )
		glDeleteTextures(1, &m_hDepthBufferTex);
	
	if ( m_hGaussianBufferTex )
		glDeleteTextures(1, &m_hGaussianBufferTex);
	
	if ( m_hGaussianFastBufferTex )
		glDeleteTextures(1, &m_hGaussianFastBufferTex);
	
	if ( m_hBokehBufferTex )
		glDeleteTextures(1, &m_hBokehBufferTex);

	SHADER_FREE_ALL();
}

//-----------------------------------------------------------------------------
// Shaders
//-----------------------------------------------------------------------------

SHADER_CREATE(CShaderDepthBuffer, g_DepthBufferShader);
SHADER_CREATE(CShaderGaussianBlur, g_GaussianBlurShader);
SHADER_CREATE(CShaderGaussianBlurFast, g_GaussianBlurFastShader);
SHADER_CREATE(CShaderBokeh, g_BokehShader);
SHADER_CREATE(CShaderDoFBlur, g_DoFBlurShader);

void GL_ShaderInit()
{
	SHADER_BEGIN_COMPILE_FILE( g_DepthBufferShader, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\depth_buffer.fsh" )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, iChannel0 )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, depthmap )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, znear )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, zfar )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, factor )
		SHADER_LOCATE_UNIFORM( g_DepthBufferShader, res )
	SHADER_END_COMPILE();
	
	SHADER_BEGIN_COMPILE_FILE( g_GaussianBlurShader, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\gaussian_blur.fsh" )
		SHADER_LOCATE_UNIFORM( g_GaussianBlurShader, radius )
		SHADER_LOCATE_UNIFORM( g_GaussianBlurShader, dir )
		SHADER_LOCATE_UNIFORM( g_GaussianBlurShader, res )
	SHADER_END_COMPILE();

	SHADER_BEGIN_COMPILE_FILE( g_GaussianBlurFastShader, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\gaussian_blur_fast.fsh" )
		SHADER_LOCATE_UNIFORM( g_GaussianBlurFastShader, dir )
		SHADER_LOCATE_UNIFORM( g_GaussianBlurFastShader, res )
	SHADER_END_COMPILE();

	SHADER_BEGIN_COMPILE_FILE( g_BokehShader, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\bokeh.fsh" )
		SHADER_LOCATE_UNIFORM( g_BokehShader, bokeh )
		SHADER_LOCATE_UNIFORM( g_BokehShader, samples )
		SHADER_LOCATE_UNIFORM( g_BokehShader, dir )
		SHADER_LOCATE_UNIFORM( g_BokehShader, res )
	SHADER_END_COMPILE();

	SHADER_BEGIN_COMPILE_FILE( g_DoFBlurShader, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\dof_blur.fsh" )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, iChannel0 )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, depthmap )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, znear )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, zfar )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, distance )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, bokeh )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, samples )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, radius )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, dir )
		SHADER_LOCATE_UNIFORM( g_DoFBlurShader, res )
	SHADER_END_COMPILE();
}

//-----------------------------------------------------------------------------
// Textures stuff
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
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst);
	glBlitFramebuffer(0, 0, w1, h1, 0, 0, w2, h2, GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

static void DrawQuadPos(int x, int y, int w, int h)
{
	glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex3i(x, y + h, -1);
		glTexCoord2i(0, 1);
		glVertex3i(x, y, -1);
		glTexCoord2i(1, 1);
		glVertex3i(x + w, y, -1);
		glTexCoord2i(1, 0);
		glVertex3i(x + w, y + h, -1);
	glEnd();
}

static void DrawQuad(int w, int h)
{
	DrawQuadPos(0, 0, w, h);
}

//===================================================
// Hooks
//===================================================

void GL_PreRenderScene()
{
}

void GL_PostRenderScene()
{
	//int w = g_ScreenInfo.width;
	//int h = g_ScreenInfo.height;
}

//===================================================
// Depth buffer
//===================================================

void GL_DepthBuffer(float znear, float zfar, float factor)
{
	int w = g_ScreenInfo.width;
	int h = g_ScreenInfo.height;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, m_hDepthBufferFBO);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hOldBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_hDepthBufferFBO);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, g_uiDepthBuffer, 0);
		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hDepthBufferTex, 0);
		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);
	
	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0); // Texture unit 0
	glBindTexture(GL_TEXTURE_2D, m_hDepthBufferTex);
	glActiveTexture(GL_TEXTURE1); // Texture unit 1
	glBindTexture(GL_TEXTURE_2D, g_uiDepthBuffer);

	SHADER_BIND( g_DepthBufferShader );

	SHADER_UNIFORM_1i( g_DepthBufferShader, iChannel0, 0 );
	SHADER_UNIFORM_1i( g_DepthBufferShader, depthmap, 1 );
	SHADER_UNIFORM_1f( g_DepthBufferShader, znear, znear );
	SHADER_UNIFORM_1f( g_DepthBufferShader, zfar, zfar );
	SHADER_UNIFORM_1f( g_DepthBufferShader, factor, factor );
	SHADER_UNIFORM_2f( g_DepthBufferShader, res, (float)w, (float)h );
		
	glColor4ub(255, 255, 255, 255);
	DrawQuad(w, h);

	SHADER_UNBIND();

	glActiveTexture(GL_TEXTURE0);
}

//===================================================
// Blurs
//===================================================

void GL_GaussianBlur(float radius)
{
	int w = g_ScreenInfo.width;
	int h = g_ScreenInfo.height;

	const Vector2D directions[2] =
	{
		{ 1.f, 0.f },
		{ 0.f, 1.f }
	};

	for (int i = 0; i < 2; i++)
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, m_hGaussianBufferFBO);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hGaussianBufferTex, 0);
			GL_BlitFrameBufferToFrameBufferColorOnly(m_hOldBuffer, m_hGaussianBufferFBO, w, h, w, h);
	
		glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);

		glEnable(GL_TEXTURE_2D);
		glBind(m_hGaussianBufferTex);

		SHADER_BIND( g_GaussianBlurShader );

		SHADER_UNIFORM_1f( g_GaussianBlurShader, radius, radius );
		SHADER_UNIFORM_2f( g_GaussianBlurShader, dir, directions[i].x, directions[i].y );
		SHADER_UNIFORM_2f( g_GaussianBlurShader, res, (float)w, (float)h );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(w, h);

		SHADER_UNBIND();
	}
}

void GL_GaussianBlurFast(float radius)
{
	int w = g_ScreenInfo.width;
	int h = g_ScreenInfo.height;

	constexpr float d = 22.5f * M_PI / 180.f;

	const Vector2D directions[8] =
	{
		{ 1.f, 0.f },
		{ 0.f, 1.f },

		{ cos(M_PI / 4), sin(M_PI / 4) },
		{ cos(M_PI - (M_PI / 4)), sin(M_PI - (M_PI / 4)) },

		{ cos(d), sin(d) },
		{ cos((M_PI / 2) + d), sin((M_PI / 2) + d) },

		{ cos((M_PI / 2) - d), sin((M_PI / 2) - d) },
		{ cos(M_PI - d), sin(M_PI - d) },
	};

	for (int i = 0; i < 8; i++)
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, m_hGaussianFastBufferFBO);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hGaussianFastBufferTex, 0);
			GL_BlitFrameBufferToFrameBufferColorOnly(m_hOldBuffer, m_hGaussianFastBufferFBO, w, h, w, h);
	
		glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);

		glEnable(GL_TEXTURE_2D);
		glBind(m_hGaussianFastBufferTex);

		SHADER_BIND( g_GaussianBlurFastShader );

		SHADER_UNIFORM_2f( g_GaussianBlurFastShader, dir, radius * directions[i].x, radius * directions[i].y );
		SHADER_UNIFORM_2f( g_GaussianBlurFastShader, res, (float)w, (float)h );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(w, h);

		SHADER_UNBIND();
	}
}

void GL_Bokeh(float radius, float samples, float bokeh)
{
	int w = g_ScreenInfo.width;
	int h = g_ScreenInfo.height;

	float aspect = (float)w / (float)h;

	const Vector2D directions[3] =
	{
		{ 0.f, 1.f },
		{ 0.866f / aspect, 0.5f },
		{ 0.866f / aspect, -0.5f }
	};

	for (int i = 0; i < 3; i++)
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, m_hBokehBufferFBO);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hBokehBufferTex, 0);
			GL_BlitFrameBufferToFrameBufferColorOnly(m_hOldBuffer, m_hBokehBufferFBO, w, h, w, h);
	
		glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);

		glEnable(GL_TEXTURE_2D);
		glBind(m_hBokehBufferTex);

		SHADER_BIND( g_BokehShader );

		SHADER_UNIFORM_1f( g_BokehShader, bokeh, bokeh );
		SHADER_UNIFORM_1f( g_BokehShader, samples, samples );
		SHADER_UNIFORM_2f( g_BokehShader, dir, radius * directions[i].x, radius * directions[i].y );
		SHADER_UNIFORM_2f( g_BokehShader, res, (float)w, (float)h );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(w, h);

		SHADER_UNBIND();
	}
}

void GL_DoFBlur(float minDistance, float maxDistance, float radius, float samples, float bokeh)
{
	int w = g_ScreenInfo.width;
	int h = g_ScreenInfo.height;

	float aspect = (float)w / (float)h;

	const Vector2D directions[3] =
	{
		{ 0.f, 1.f },
		{ 0.866f / aspect, 0.5f },
		{ 0.866f / aspect, -0.5f }
	};

	for (int i = 0; i < 3; i++)
	{
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, m_hDoFBlurBufferFBO);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hOldBuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_hDoFBlurBufferFBO);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_hDoFDepthBuffer, 0);
			glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hDoFBlurBufferTex, 0);
			glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE0); // Texture unit 0
		glBindTexture(GL_TEXTURE_2D, m_hDoFBlurBufferTex);
		glActiveTexture(GL_TEXTURE1); // Texture unit 1
		glBindTexture(GL_TEXTURE_2D, m_hDoFDepthBuffer);

		SHADER_BIND( g_DoFBlurShader );

		SHADER_UNIFORM_1i( g_DoFBlurShader, iChannel0, 0 );
		SHADER_UNIFORM_1i( g_DoFBlurShader, depthmap, 1 );
		SHADER_UNIFORM_1f( g_DoFBlurShader, znear, 4.f );
		SHADER_UNIFORM_1f( g_DoFBlurShader, zfar, maxDistance );
		SHADER_UNIFORM_1f( g_DoFBlurShader, distance, minDistance / maxDistance );
		SHADER_UNIFORM_1f( g_DoFBlurShader, bokeh, bokeh );
		SHADER_UNIFORM_1f( g_DoFBlurShader, samples, samples );
		SHADER_UNIFORM_1f( g_DoFBlurShader, radius, radius );
		SHADER_UNIFORM_2f( g_DoFBlurShader, dir,  directions[i].x, directions[i].y );
		SHADER_UNIFORM_2f( g_DoFBlurShader, res, (float)w, (float)h );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(w, h);

		SHADER_UNBIND();

		glActiveTexture(GL_TEXTURE0);
	}
}