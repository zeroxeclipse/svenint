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

	void DrawSSAO(float znear,
				float zfar,
				float strength,
				int samples,
				float radius,
				float aoclamp,
				int noise,
				float noiseamount,
				float diffarea,
				float gdisplace,
				int mist,
				float miststart,
				float mistend,
				float lumInfluence,
				int onlyAO);
	
	void DrawColorCorrection(float targetgamma,
							float monitorgamma,
							float hueoffset,
							float saturation,
							float contrast,
							float luminance,
							float blacklevel,
							float brightboost,
							float redlevel,
							float greenlevel,
							float bluelevel,
							float grain,
							float sharpness);

	void DrawGodRays(float x, float y, float density, float weight, float decay, float exposure, int numSamples);
	void DrawChromaticAberration(int type, float dirX, float dirY, float shift, float strength);
	void DrawDoFBlur(float minDistance, float maxDistance, int interptype, float radius, float samples, float bokeh);
	void DrawRadialBlur(float distance, float strength);
	void DrawBokehBlur(float radius, float samples, float bokeh);
	void DrawGaussianBlur(float radius);
	void DrawGaussianBlurFast(float radius);
	void DrawVignette(float falloff, float amount);

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
	POST_PROCESSING_DEFINE_VARS( m_hSSAO );
	POST_PROCESSING_DEFINE_VARS( m_hColorCorrection );
	POST_PROCESSING_DEFINE_VARS( m_hGodRays );
	POST_PROCESSING_DEFINE_VARS( m_hChromaticAberration );
	POST_PROCESSING_DEFINE_VARS( m_hDoFBlur );
	POST_PROCESSING_DEFINE_VARS( m_hRadialBlur );
	POST_PROCESSING_DEFINE_VARS( m_hBokeh );
	POST_PROCESSING_DEFINE_VARS( m_hGaussianBlur );
	POST_PROCESSING_DEFINE_VARS( m_hGaussianBlurFast );
	POST_PROCESSING_DEFINE_VARS( m_hVignette );

	// Depth Buffer
	SHADER_BEGIN_DESC_MEMBER( CShaderDepthBuffer )
		SHADER_DEFINE_INTERNAL_NAME( "DepthBuffer" )
		SHADER_DEFINE_UNIFORM( znear )
		SHADER_DEFINE_UNIFORM( zfar )
		SHADER_DEFINE_UNIFORM( factor )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderDepthBuffer, m_ShaderDepthBuffer );
	
	// Screen-Space Ambient Occlusion
	SHADER_BEGIN_DESC_MEMBER( CShaderSSAO )
		SHADER_DEFINE_INTERNAL_NAME( "SSAO" )
		SHADER_DEFINE_UNIFORM( iChannel0 )
		SHADER_DEFINE_UNIFORM( depthmap )
		SHADER_DEFINE_UNIFORM( zNear )
		SHADER_DEFINE_UNIFORM( zFar )
		SHADER_DEFINE_UNIFORM( strength )
		SHADER_DEFINE_UNIFORM( samples )
		SHADER_DEFINE_UNIFORM( radius )
		SHADER_DEFINE_UNIFORM( aoclamp )
		SHADER_DEFINE_UNIFORM( noise )
		SHADER_DEFINE_UNIFORM( noiseamount )
		SHADER_DEFINE_UNIFORM( diffarea )
		SHADER_DEFINE_UNIFORM( gdisplace )
		SHADER_DEFINE_UNIFORM( mist )
		SHADER_DEFINE_UNIFORM( miststart )
		SHADER_DEFINE_UNIFORM( mistend )
		SHADER_DEFINE_UNIFORM( onlyAO )
		SHADER_DEFINE_UNIFORM( lumInfluence )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderSSAO, m_ShaderSSAO );

	// Color Correction
	SHADER_BEGIN_DESC_MEMBER( CShaderColorCorrection )
		SHADER_DEFINE_INTERNAL_NAME( "ColorCorrection" )
		SHADER_DEFINE_UNIFORM( iTime )
		SHADER_DEFINE_UNIFORM( ia_target_gamma )
		SHADER_DEFINE_UNIFORM( ia_monitor_gamma )
		SHADER_DEFINE_UNIFORM( ia_hue_offset )
		SHADER_DEFINE_UNIFORM( ia_saturation )
		SHADER_DEFINE_UNIFORM( ia_contrast )
		SHADER_DEFINE_UNIFORM( ia_luminance )
		SHADER_DEFINE_UNIFORM( ia_black_level )
		SHADER_DEFINE_UNIFORM( ia_bright_boost )
		SHADER_DEFINE_UNIFORM( ia_R )
		SHADER_DEFINE_UNIFORM( ia_G )
		SHADER_DEFINE_UNIFORM( ia_B )
		SHADER_DEFINE_UNIFORM( ia_GRAIN_STR )
		//SHADER_DEFINE_UNIFORM( ia_SHARPEN )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderColorCorrection, m_ShaderColorCorrection );
	
	// God Rays
	SHADER_BEGIN_DESC_MEMBER( CShaderGodRays )
		SHADER_DEFINE_INTERNAL_NAME( "GodRays" )
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
		SHADER_DEFINE_INTERNAL_NAME( "ChromaticAberration" )
		SHADER_DEFINE_UNIFORM( type )
		SHADER_DEFINE_UNIFORM( shift )
		SHADER_DEFINE_UNIFORM( strength )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderChromaticAberration, m_ShaderChromaticAberration );
	
	// Depth of Field Bokeh Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderDoFBlur )
		SHADER_DEFINE_INTERNAL_NAME( "DoFBlur" )
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
		SHADER_DEFINE_INTERNAL_NAME( "RadialBlur" )
		SHADER_DEFINE_UNIFORM( distance )
		SHADER_DEFINE_UNIFORM( strength )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderRadialBlur, m_ShaderRadialBlur );

	// Bokeh Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderBokeh )
		SHADER_DEFINE_INTERNAL_NAME( "Bokeh" )
		SHADER_DEFINE_UNIFORM( bokeh )
		SHADER_DEFINE_UNIFORM( samples )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderBokeh, m_ShaderBokeh );

	// Gaussian Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderGaussianBlur )
		SHADER_DEFINE_INTERNAL_NAME( "GaussianBlur" )
		SHADER_DEFINE_UNIFORM( radius )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderGaussianBlur, m_ShaderGaussianBlur );

	// Fast Gaussian Blur
	SHADER_BEGIN_DESC_MEMBER( CShaderGaussianBlurFast )
		SHADER_DEFINE_INTERNAL_NAME( "GaussianBlurFast" )
		SHADER_DEFINE_UNIFORM( dir )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderGaussianBlurFast, m_ShaderGaussianBlurFast );

	// Vignette
	SHADER_BEGIN_DESC_MEMBER( CShaderVignette )
		SHADER_DEFINE_INTERNAL_NAME( "Vignette" )
		SHADER_DEFINE_UNIFORM( falloff )
		SHADER_DEFINE_UNIFORM( amount )
		SHADER_DEFINE_UNIFORM( res )
	SHADER_END_DESC(); SHADER_CREATE( CShaderVignette, m_ShaderVignette );
};

extern CShaders g_Shaders;

#endif // SHADERS_H