#include "shaders.h"

#include <string>
#include <sstream>
#include <regex>

#include <hl_sdk/engine/APIProxy.h>
#include <IMemoryUtils.h>
#include <ISvenModAPI.h>
#include <sys.h>

typedef void (__cdecl *glBindFn)(GLuint);
glBindFn glBind = NULL;

GLint m_hOldBuffer = 0;
GLuint m_hGaussianBufferFBO = 0;
GLuint m_hGaussianBufferTex = 0;

void GL_Blur(float ratio)
{
	int w = Utils()->GetScreenWidth();
	int h = Utils()->GetScreenHeight();

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_hOldBuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, m_hGaussianBufferFBO);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_hGaussianBufferTex, 0);
		GL_BlitFrameBufferToFrameBufferColorOnly(m_hOldBuffer, m_hGaussianBufferFBO, w, h, w, h);
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);

	DrawGaussianBlur(m_hGaussianBufferTex, ratio, Utils()->GetScreenWidth(), Utils()->GetScreenHeight());
}

void GL_Init()
{
	DEFINE_PATTERN(glBind_sig, "8B 44 24 04 39 05 ? ? ? ? 74 11 50 68");
	void *pfnglBind = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, glBind_sig );

	if ( pfnglBind == NULL )
	{
		Sys_Error("Failed to find function \"glBind\"");
		return;
	}

	glBind = (glBindFn)pfnglBind;

	GL_ShaderInit();

	glGenFramebuffersEXT(1, &m_hGaussianBufferFBO);
	m_hGaussianBufferTex = GL_GenTextureRGB8(Utils()->GetScreenWidth(), Utils()->GetScreenHeight());
}

void GL_Shutdown()
{
	if (m_hGaussianBufferTex)
		glDeleteTextures(1, &m_hGaussianBufferTex);

	GL_FreeShaders();
}

SHADER_DEFINE(pp_gaussianblur);
void GL_ShaderInit()
{
	pp_gaussianblur.program = R_CompileShaderFile("sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\gaussian_blur_16x.fsh", NULL);

	if (pp_gaussianblur.program)
	{
		SHADER_UNIFORM(pp_gaussianblur, du, "du");
		SHADER_UNIFORM(pp_gaussianblur, res, "res");
	}
}

std::vector<glshader_t> g_ShaderTable;

void GL_FreeShaders()
{
	for (size_t i = 0; i < g_ShaderTable.size(); ++i)
	{
		auto &objs = g_ShaderTable[i].shader_objects;
		for (size_t j = 0; j < objs.size(); ++j)
		{
			glDetachObjectARB(g_ShaderTable[i].program, objs[j]);
			glDeleteObjectARB(objs[j]);
		}
		glDeleteProgramsARB(1, &g_ShaderTable[i].program);
	}
	g_ShaderTable.clear();
}

void GL_CheckShaderError(GLuint shader, const char *code, const char *filename)
{
	int iStatus;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &iStatus);

	if (!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetInfoLogARB(shader, sizeof(szCompilerLog) - 1, &nInfoLength, szCompilerLog);
		szCompilerLog[nInfoLength] = 0;

		Sys_Error("Shader \"%s\" compiled with error:\n%s", filename, szCompilerLog);
		return;
	}
}

GLuint R_CompileShaderObject(int type, const char *code, const char *filename)
{
	auto obj = glCreateShaderObjectARB(type);

	glShaderSource(obj, 1, &code, NULL);

	glCompileShader(obj);

	GL_CheckShaderError(obj, code, filename);

	return obj;
}

GLuint R_CompileShader(const char *vscode, const char *fscode, const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback)
{
	GLuint shader_objects[32];
	int shader_object_used = 0;

	shader_objects[shader_object_used] = R_CompileShaderObject(GL_VERTEX_SHADER_ARB, vscode, vsfile);
	shader_object_used++;

	if (callback)
		callback(shader_objects, &shader_object_used);

	shader_objects[shader_object_used] = R_CompileShaderObject(GL_FRAGMENT_SHADER_ARB, fscode, fsfile);
	shader_object_used++;

	GLuint program = glCreateProgramObjectARB();
	for (int i = 0; i < shader_object_used; ++i)
		glAttachObjectARB(program, shader_objects[i]);
	glLinkProgramARB(program);

	int iStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &iStatus);
	if (!iStatus)
	{
		int nInfoLength;
		char szCompilerLog[1024] = { 0 };
		glGetProgramInfoLog(program, sizeof(szCompilerLog), &nInfoLength, szCompilerLog);

		Sys_Error("Shader \"%s\" compiled with error:\n%s", vscode, szCompilerLog);
	}

	g_ShaderTable.emplace_back(program, shader_objects, shader_object_used);

	return program;
}

void R_CompileShaderAppendInclude(std::string &str, const char *filename)
{
	std::regex pattern("#include[< \"]+([a-zA-Z_\\.]+)[> \"]");
	std::smatch result;
	std::regex_search(str, result, pattern);

	std::string skipped;

	std::string::const_iterator searchStart(str.cbegin());

	while (std::regex_search(searchStart, str.cend(), result, pattern) && result.size() >= 2)
	{
		std::string prefix = result.prefix();
		std::string suffix = result.suffix();

		auto includeFileName = result[1].str();

		char slash = 0;

		std::string includePath = filename;
		for (size_t j = includePath.length() - 1; j > 0; --j)
		{
			if (includePath[j] == '\\' || includePath[j] == '/')
			{
				slash = includePath[j];
				includePath.resize(j);
				break;
			}
		}
		includePath += slash;
		includePath += includeFileName;
		auto pFile = g_pEngineFuncs->COM_LoadFile((char *)includePath.c_str(), 5, NULL);
		if (pFile)
		{
			std::string wbinding((char *)pFile);

			g_pEngineFuncs->COM_FreeFile(pFile);

			if (searchStart != str.cbegin())
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

void R_CompileShaderAppendDefine(std::string &str, const std::string &def)
{
	std::regex pattern("(#version [0-9a-z ]+)");
	std::smatch result;
	std::regex_search(str, result, pattern);

	if (result.size() >= 1)
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

GLuint R_CompileShaderFileEx(const char *vsfile, const char *fsfile, const char *vsdefine, const char *fsdefine, ExtraShaderStageCallback callback)
{
	auto vscode = (char *)g_pEngineFuncs->COM_LoadFile((char *)vsfile, 5, 0);
	std::string vs;
	if (!vscode)
		Sys_Error("R_CompileShaderFileEx: \"%s\" not found", vsfile);
	else
		vs = std::string(vscode);

	R_CompileShaderAppendDefine(vs, "#define IS_VERTEX_SHADER\n");
	if (vsdefine)
	{
		R_CompileShaderAppendDefine(vs, vsdefine);
	}

	g_pEngineFuncs->COM_FreeFile(vscode);

	auto fscode = (char *)g_pEngineFuncs->COM_LoadFile((char *)fsfile, 5, 0);
	std::string fs;
	if (!fscode)
		Sys_Error("R_CompileShaderFileEx: \"%s\" not found", fsfile);
	else
		fs = std::string(fscode);

	R_CompileShaderAppendDefine(fs, "#define IS_FRAGMENT_SHADER\n");
	if (fsdefine)
	{
		R_CompileShaderAppendDefine(fs, fsdefine);
	}

	g_pEngineFuncs->COM_FreeFile(fscode);

	if (vs.find("#include") != std::string::npos)
	{
		R_CompileShaderAppendInclude(vs, vsfile);
	}

	if (fs.find("#include") != std::string::npos)
	{
		R_CompileShaderAppendInclude(fs, fsfile);
	}

	return R_CompileShader(vs.c_str(), fs.c_str(), vsfile, fsfile, callback);
}

GLuint R_CompileShaderFile(const char *vsfile, const char *fsfile, ExtraShaderStageCallback callback)
{
	return R_CompileShaderFileEx(vsfile, fsfile, NULL, NULL, callback);
}

void GL_UseProgram(GLuint program)
{
	static int currentprogram = -1;

	if (currentprogram != program)
	{
		currentprogram = program;
		glUseProgramObjectARB(program);
	}
}

GLuint GL_GetUniformLoc(GLuint program, const char *name)
{
	return glGetUniformLocationARB(program, name);
}

GLuint GL_GetAttribLoc(GLuint program, const char *name)
{
	return glGetAttribLocationARB(program, name);
}

void GL_Uniform1i(GLuint loc, int v0)
{
	glUniform1i(loc, v0);
}

void GL_Uniform2i(GLuint loc, int v0, int v1)
{
	glUniform2iARB(loc, v0, v1);
}

void GL_Uniform3i(GLuint loc, int v0, int v1, int v2)
{
	glUniform3iARB(loc, v0, v1, v2);
}

void GL_Uniform4i(GLuint loc, int v0, int v1, int v2, int v3)
{
	glUniform4iARB(loc, v0, v1, v2, v3);
}

void GL_Uniform1f(GLuint loc, float v0)
{
	glUniform1f(loc, v0);
}

void GL_Uniform2f(GLuint loc, float v0, float v1)
{
	glUniform2fARB(loc, v0, v1);
}

void GL_Uniform3f(GLuint loc, float v0, float v1, float v2)
{
	glUniform3f(loc, v0, v1, v2);
}

void GL_Uniform4f(GLuint loc, float v0, int v1, int v2, int v3)
{
	glUniform4f(loc, v0, v1, v2, v3);
}

void GL_VertexAttrib3f(GLuint index, float x, float y, float z)
{
	glVertexAttrib3f(index, x, y, z);
}

void GL_VertexAttrib3fv(GLuint index, float *v)
{
	glVertexAttrib3fv(index, v);
}

void GL_MultiTexCoord2f(GLenum target, float s, float t)
{
	glMultiTexCoord2fARB(target, s, t);
}

void GL_MultiTexCoord3f(GLenum target, float s, float t, float r)
{
	glMultiTexCoord3fARB(target, s, t, r);
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

GLuint GL_GenTexture(void)
{
	GLuint tex;
	glGenTextures(1, &tex);
	return tex;
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
	glTexImage2D(GL_TEXTURE_2D, 0, iInternalFormat, w, h, 0, GL_RGBA,
				 (iInternalFormat != GL_RGBA && iInternalFormat != GL_RGBA8) ? GL_FLOAT : GL_UNSIGNED_BYTE, 0);

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

void DrawGaussianBlur(GLint tex, float ratio, int w, int h)
{
	glEnable(GL_TEXTURE_2D);
	glBind(tex);
	GL_UseProgram(pp_gaussianblur.program);
	GL_Uniform1f(pp_gaussianblur.du, ratio);
	GL_Uniform2f(pp_gaussianblur.res, (float)w, (float)h);
	glColor4ub(255, 255, 255, 255);
	DrawQuad(w, h);
	GL_UseProgram(0);
}