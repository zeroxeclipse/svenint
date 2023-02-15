#ifndef SHADERS_H
#define SHADERS_H

#ifdef _WIN32
#pragma once
#endif

#include <string>
#include <vector>

#include <IDetoursAPI.h>
#include <base_feature.h>

#include "../modules/opengl.h"

// Mini helpers to make shaders implementation a bit faster
#define POST_PROCESSING_DEFINE_VARS(name) GLuint name##FBO; GLuint name##Tex
#define POST_PROCESSING_RESET_VARS(name) name##FBO = 0; name##Tex = 0
#define POST_PROCESSING_FREE_VARS(name) if ( name##Tex ) glDeleteTextures(1, &(name##Tex)); \
	if ( name##FBO ) glDeleteFramebuffersEXT(1, &(name##FBO));
#define POST_PROCESSING_INIT_VARS_COLOR(name, w, h) glGenFramebuffersEXT(1, &(name##FBO)); name##Tex = GL_GenTextureRGB8(w, h);
#define POST_PROCESSING_INIT_VARS_DEPTH(name, w, h) glGenFramebuffersEXT(1, &(name##FBO)); name##Tex = GL_GenDepthTexture(w, h);

#define POST_PROCESSING_EXPAND_VARS(name) name##FBO, name##Tex
#define POST_PROCESSING_FBO(name) name##FBO
#define POST_PROCESSING_TEX(name) name##Tex

//-----------------------------------------------------------------------------
// Shaders!
//-----------------------------------------------------------------------------

class CShaders : public CBaseFeature
{
public:
	CShaders();

	virtual bool Load();
	virtual void PostLoad();

	virtual void Unload();

public:
	void OnPostRenderView(void);

private:
	static void DrawQuadPos(int x, int y, int w, int h);
	static void DrawQuad(int w, int h);

	void StoreDepthBuffer();

	// Shaders
	void DrawDepthBuffer(float znear, float zfar, float factor);
	void DrawGodRays(float x, float y, float density, float weight, float decay, float exposure, int numSamples);
	void DrawChromaticAberration(float pixelWidth, float pixelHeight, float shift, float strength);
	void DrawDoFBlur(float minDistance, float maxDistance, int interptype, float radius, float samples, float bokeh);
	void DrawRadialBlur(float distance, float strength);
	void DrawBokehBlur(float radius, float samples, float bokeh);
	void DrawGaussianBlur(float radius);
	void DrawGaussianBlurFast(float radius);

	void InitColorTexPostProcessing(GLuint hFBO, GLuint hTex);
	void InitDepthTexPostProcessing(GLuint hFBO, GLuint hTex);

	void Compile();
	void DecryptRawShaders();
	void EncryptRawShaders();

private:
	int m_width;
	int m_height;

	float m_fwidth;
	float m_fheight;

	float m_aspect;

	void *m_pfnClientDLL_HudRedraw;
	DetourHandle_t m_hClientDLL_HudRedraw;

	GLint m_hOldBuffer;

	POST_PROCESSING_DEFINE_VARS( m_hDepthBuffer );
	POST_PROCESSING_DEFINE_VARS( m_hGodRays );
	POST_PROCESSING_DEFINE_VARS( m_hChromaticAberration );
	POST_PROCESSING_DEFINE_VARS( m_hDoFBlur );
	POST_PROCESSING_DEFINE_VARS( m_hRadialBlur );
	POST_PROCESSING_DEFINE_VARS( m_hBokeh );
	POST_PROCESSING_DEFINE_VARS( m_hGaussianBlur );
	POST_PROCESSING_DEFINE_VARS( m_hGaussianBlurFast );

	// Depth Buffer
	SHADER_BEGIN_DESC_MEMBER( CShaderDepthBuffer )
		SHADER_DEFINE_UNIFORM( znear )
		SHADER_DEFINE_UNIFORM( zfar )
		SHADER_DEFINE_UNIFORM( factor )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderDepthBuffer, m_ShaderDepthBuffer );

	// God Rays
	SHADER_BEGIN_DESC_MEMBER( CShaderGodRays )
		SHADER_DEFINE_UNIFORM( density )
		SHADER_DEFINE_UNIFORM( weight )
		SHADER_DEFINE_UNIFORM( decay )
		SHADER_DEFINE_UNIFORM( exposure )
		SHADER_DEFINE_UNIFORM( numSamples )
		SHADER_DEFINE_UNIFORM( lightpos )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderGodRays, m_ShaderGodRays );

	// Chromatic Aberration
	SHADER_BEGIN_DESC_MEMBER( CShaderChromaticAberration )
		SHADER_DEFINE_UNIFORM( shift )
		SHADER_DEFINE_UNIFORM( strength )
		SHADER_DEFINE_UNIFORM( pixelSize )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderChromaticAberration, m_ShaderChromaticAberration );

	// Depth of Field Bokeh Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderDoFBlur )
		SHADER_DEFINE_UNIFORM( iChannel0 )
		SHADER_DEFINE_UNIFORM( depthmap )
		SHADER_DEFINE_UNIFORM( interptype )
		SHADER_DEFINE_UNIFORM( znear )
		SHADER_DEFINE_UNIFORM( zfar )
		SHADER_DEFINE_UNIFORM( distance )
		SHADER_DEFINE_UNIFORM( bokeh )
		SHADER_DEFINE_UNIFORM( samples )
		SHADER_DEFINE_UNIFORM( radius )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderDoFBlur, m_ShaderDoFBlur );

	// Radial Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderRadialBlur )
		SHADER_DEFINE_UNIFORM( distance )
		SHADER_DEFINE_UNIFORM( strength )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderRadialBlur, m_ShaderRadialBlur );

	// Bokeh Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderBokeh )
		SHADER_DEFINE_UNIFORM( bokeh )
		SHADER_DEFINE_UNIFORM( samples )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderBokeh, m_ShaderBokeh );

	// Gaussian Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderGaussianBlur )
		SHADER_DEFINE_UNIFORM( radius )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderGaussianBlur, m_ShaderGaussianBlur );

	// Fast Gaussian Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderGaussianBlurFast )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderGaussianBlurFast, m_ShaderGaussianBlurFast );
};

extern CShaders g_Shaders;

#endif // SHADERS_H