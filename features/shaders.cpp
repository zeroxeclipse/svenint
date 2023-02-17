// Shaders

#define SHADERS_COMPILE_FROM_FILE ( 0 )

#include <dbg.h>
#include <convar.h>

#include <IMemoryUtils.h>
#include <ISvenModAPI.h>

#include "shaders.h"

#include "../modules/menu.h"
#include "../game/utils.h"

#include "../patterns.h"
#include "../config.h"

#if !SHADERS_COMPILE_FROM_FILE
#include "../utils/shaders/shaders_raw.h"
#endif

//-----------------------------------------------------------------------------
// Declare hooks
//-----------------------------------------------------------------------------

DECLARE_HOOK(void, __cdecl, ClientDLL_HudRedraw, int intermission);

//-----------------------------------------------------------------------------
// Vars
//-----------------------------------------------------------------------------

CShaders g_Shaders;

static float s_iTime = -1;

// Experimental only
ConVar sc_gl_godrays("sc_gl_godrays", "", FCVAR_CLIENTDLL,
					 "sc_gl_godrays \"<screen width fraction> <screen height fraction> <density> <weight> <decay> <exposure> <samples>\"");

//-----------------------------------------------------------------------------
// Hooks
//-----------------------------------------------------------------------------

DECLARE_FUNC(void, __cdecl, HOOKED_ClientDLL_HudRedraw, int intermission)
{
	g_Shaders.OnPostRenderView();

	ORIG_ClientDLL_HudRedraw( intermission );
}

//-----------------------------------------------------------------------------
// OnPostRenderView
//-----------------------------------------------------------------------------

void CShaders::OnPostRenderView(void)
{
	s_iTime += 1.f;

	if ( s_iTime >= 1000.f )
		s_iTime = 0.f;

	// Always save depth buffer
	StoreDepthBuffer();

	if ( !g_Config.cvars.shaders )
		return;

	// Depth buffer
	if ( g_Config.cvars.shaders_show_depth_buffer )
	{
		DrawDepthBuffer( g_Config.cvars.shaders_depth_buffer_znear, g_Config.cvars.shaders_depth_buffer_zfar, g_Config.cvars.shaders_depth_buffer_brightness );
	}

	if ( g_Config.cvars.shaders_ssao )
	{
		DrawSSAO( g_Config.cvars.shaders_ssao_znear,
				 g_Config.cvars.shaders_ssao_zfar,
				 g_Config.cvars.shaders_ssao_strength,
				 g_Config.cvars.shaders_ssao_samples,
				 g_Config.cvars.shaders_ssao_radius,
				 g_Config.cvars.shaders_ssao_aoclamp,
				 (int)g_Config.cvars.shaders_ssao_noise,
				 g_Config.cvars.shaders_ssao_noiseamount,
				 g_Config.cvars.shaders_ssao_diffarea,
				 g_Config.cvars.shaders_ssao_gdisplace,
				 (int)g_Config.cvars.shaders_ssao_mist,
				 g_Config.cvars.shaders_ssao_miststart,
				 g_Config.cvars.shaders_ssao_mistend,
				 g_Config.cvars.shaders_ssao_lumInfluence,
				 (int)g_Config.cvars.shaders_ssao_onlyAO );
	}

	// Color correction
	if ( g_Config.cvars.shaders_color_correction )
	{
		DrawColorCorrection( g_Config.cvars.shaders_cc_target_gamma,
							g_Config.cvars.shaders_cc_monitor_gamma,
							g_Config.cvars.shaders_cc_hue_offset,
							g_Config.cvars.shaders_cc_saturation,
							g_Config.cvars.shaders_cc_contrast,
							g_Config.cvars.shaders_cc_luminance,
							g_Config.cvars.shaders_cc_black_level,
							g_Config.cvars.shaders_cc_bright_boost,
							g_Config.cvars.shaders_cc_R,
							g_Config.cvars.shaders_cc_G,
							g_Config.cvars.shaders_cc_B,
							g_Config.cvars.shaders_cc_grain,
							0.f );
	}
	
	// God rays
	// Experimental only, need to draw a sun with this shader and then draw the rest of world (w/o the shader)
	if ( *( sc_gl_godrays.GetString() ) != 0 )
	{
		int numSamples;
		float x, y, density, weight, decay, exposure;

		// Example of good settings: sc_gl_godrays "0.5 0.5 0.97 0.25 0.974 0.24 64"
		// Ranges:
		// density		0 - 2.0
		// weight		0 - 0.1
		// decay		0.95 - 1.05
		// exposure		0 - 2.0
		// numSamples	0 - 100

		if ( sscanf( sc_gl_godrays.GetString(), "%f %f %f %f %f %f %d", &x, &y, &density, &weight, &decay, &exposure, &numSamples ) == 7 )
			DrawGodRays( x, y, density, weight, decay, exposure, numSamples );
	}

	// Chromatic aberration
	if ( g_Config.cvars.shaders_chromatic_aberration )
	{
		DrawChromaticAberration( g_Config.cvars.shaders_chromatic_aberration_type,
								g_Config.cvars.shaders_chromatic_aberration_dir_x,
								g_Config.cvars.shaders_chromatic_aberration_dir_y,
								g_Config.cvars.shaders_chromatic_aberration_shift,
								g_Config.cvars.shaders_chromatic_aberration_strength );
	}

	// Depth of Field blur
	if ( g_Config.cvars.shaders_dof_blur )
	{
		DrawDoFBlur( g_Config.cvars.shaders_dof_blur_min_range,
					g_Config.cvars.shaders_dof_blur_max_range,
					g_Config.cvars.shaders_dof_blur_interp_type - 1,
					g_Config.cvars.shaders_dof_blur_bluriness_range,
					(float)g_Config.cvars.shaders_dof_blur_quality,
					g_Config.cvars.shaders_dof_blur_bokeh );
	}

	// Motion blur, speed dependent only
	if ( g_Config.cvars.shaders_motion_blur )
	{
		float flSpeed = g_pPlayerMove->velocity.Length2D();

		float flMinSpeed = g_Config.cvars.shaders_motion_blur_min_speed;
		float flMaxSpeed = g_Config.cvars.shaders_motion_blur_max_speed;

		if ( flSpeed >= flMinSpeed )
		{
			float dt = 1.f;

			if ( flSpeed < flMaxSpeed )
			{
				dt = (flSpeed - flMinSpeed) / (flMaxSpeed - flMinSpeed);
			}

			dt = 3 * dt * dt - 2 * dt * dt * dt;

			DrawRadialBlur( dt, g_Config.cvars.shaders_motion_blur_strength );
		}
	}

	// Radial blur
	if ( g_Config.cvars.shaders_radial_blur )
	{
		DrawRadialBlur( g_Config.cvars.shaders_radial_blur_distance, g_Config.cvars.shaders_radial_blur_strength );
	}

	// Bokeh blur
	if ( g_Config.cvars.shaders_bokeh_blur )
	{
		DrawBokehBlur( g_Config.cvars.shaders_bokeh_blur_radius, (float)g_Config.cvars.shaders_bokeh_blur_samples, g_Config.cvars.shaders_bokeh_blur_coeff );
	}

	// Gaussian blur
	if ( g_Config.cvars.shaders_gaussian_blur )
	{
		DrawGaussianBlur( g_Config.cvars.shaders_gaussian_blur_radius );
	}

	// Fast gaussian blur
	if ( g_Config.cvars.shaders_gaussian_blur_fast )
	{
		DrawGaussianBlurFast( g_Config.cvars.shaders_gaussian_blur_fast_radius );
	}

	// Menu, background blur 
	if ( g_Config.cvars.menu_blur )
	{
		float flTime = g_pEngineFuncs->Sys_FloatTime();
	
		if ( g_iMenuState == 1 )
		{
			float dt = 1.f;

			if ( flTime - g_bMenuOpenTime <= g_Config.cvars.menu_blur_fadein_duration )
			{
				dt = (flTime - g_bMenuOpenTime) / g_Config.cvars.menu_blur_fadein_duration;

				dt = 3 * dt * dt - 2 * dt * dt * dt;
			}

			DrawBokehBlur( g_Config.cvars.menu_blur_radius * dt, (float)g_Config.cvars.menu_blur_samples, g_Config.cvars.menu_blur_bokeh );
		}
		else if ( g_iMenuState == 2 )
		{
			float dt = 1.f;

			if ( flTime - g_bMenuCloseTime <= g_Config.cvars.menu_blur_fadeout_duration )
			{
				dt = (flTime - g_bMenuCloseTime) / g_Config.cvars.menu_blur_fadeout_duration;

				dt = 3 * dt * dt - 2 * dt * dt * dt;
			}

			dt = 1.f - dt;

			DrawBokehBlur( g_Config.cvars.menu_blur_radius * dt, (float)g_Config.cvars.menu_blur_samples, g_Config.cvars.menu_blur_bokeh );
		}
	}

	// Vignette
	if ( g_Config.cvars.shaders_vignette )
	{
		DrawVignette( g_Config.cvars.shaders_vignette_falloff, g_Config.cvars.shaders_vignette_amount );
	}
}

//-----------------------------------------------------------------------------
// Depth buffer
//-----------------------------------------------------------------------------

void CShaders::StoreDepthBuffer()
{
	InitDepthTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hDepthBuffer) );
}

void CShaders::DrawDepthBuffer(float znear, float zfar, float factor)
{
	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hDepthBuffer));

	SHADER_BIND( m_ShaderDepthBuffer );

		SHADER_UNIFORM_1f( m_ShaderDepthBuffer, znear, znear );
		SHADER_UNIFORM_1f( m_ShaderDepthBuffer, zfar, zfar );
		SHADER_UNIFORM_1f( m_ShaderDepthBuffer, factor, factor );
		SHADER_UNIFORM_2f( m_ShaderDepthBuffer, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// Space-screen ambient occlusion
//-----------------------------------------------------------------------------

void CShaders::DrawSSAO(float znear,
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
					  int onlyAO)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hSSAO) );

	GLint texture0, texture1;

	glEnable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
	GL_Bind(POST_PROCESSING_TEX(m_hSSAO));

	glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture1);
	GL_Bind(POST_PROCESSING_TEX(m_hDepthBuffer));

	SHADER_BIND(m_ShaderSSAO);

		SHADER_UNIFORM_1i( m_ShaderSSAO, iChannel0, 0 );
		SHADER_UNIFORM_1i( m_ShaderSSAO, depthmap, 1 );
		SHADER_UNIFORM_1f( m_ShaderSSAO, zNear, znear );
		SHADER_UNIFORM_1f( m_ShaderSSAO, zFar, zfar );
		SHADER_UNIFORM_1f( m_ShaderSSAO, strength, strength );
		SHADER_UNIFORM_1i( m_ShaderSSAO, samples, samples );
		SHADER_UNIFORM_1f( m_ShaderSSAO, radius, radius );
		SHADER_UNIFORM_1f( m_ShaderSSAO, aoclamp, aoclamp );
		SHADER_UNIFORM_1i( m_ShaderSSAO, noise, noise );
		SHADER_UNIFORM_1f( m_ShaderSSAO, noiseamount, noiseamount );
		SHADER_UNIFORM_1f( m_ShaderSSAO, diffarea, diffarea );
		SHADER_UNIFORM_1f( m_ShaderSSAO, gdisplace, gdisplace );
		SHADER_UNIFORM_1i( m_ShaderSSAO, mist, mist );
		SHADER_UNIFORM_1f( m_ShaderSSAO, miststart, miststart );
		SHADER_UNIFORM_1f( m_ShaderSSAO, mistend, mistend );
		SHADER_UNIFORM_1i( m_ShaderSSAO, onlyAO, onlyAO );
		SHADER_UNIFORM_1f( m_ShaderSSAO, lumInfluence, lumInfluence );
		SHADER_UNIFORM_2f( m_ShaderSSAO, res, m_fwidth, m_fheight );

		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();

	GL_Bind(texture1); // unbind from texture unit 1
	glActiveTexture(GL_TEXTURE0 + 0);
	GL_Bind(texture0); // unbind from texture unit 0
}

//-----------------------------------------------------------------------------
// Color correction
//-----------------------------------------------------------------------------

void CShaders::DrawColorCorrection(float targetgamma, // holy shit
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
								 float sharpness)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hColorCorrection) );

	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hColorCorrection));

	SHADER_BIND( m_ShaderColorCorrection );

		SHADER_UNIFORM_1f( m_ShaderColorCorrection, iTime, s_iTime );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_target_gamma, targetgamma );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_monitor_gamma, monitorgamma );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_hue_offset, hueoffset );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_saturation, saturation );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_contrast, contrast );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_luminance, luminance );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_black_level, blacklevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_bright_boost, brightboost );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_R, redlevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_G, greenlevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_B, bluelevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_G, greenlevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_B, bluelevel );
		SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_GRAIN_STR, grain );
		//SHADER_UNIFORM_1f( m_ShaderColorCorrection, ia_SHARPEN, sharpness );
		SHADER_UNIFORM_2f( m_ShaderColorCorrection, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// God rays
//-----------------------------------------------------------------------------

void CShaders::DrawGodRays(float x, float y, float density, float weight, float decay, float exposure, int numSamples)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hGodRays) );

	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hGodRays));

	SHADER_BIND( m_ShaderGodRays );

		SHADER_UNIFORM_1f( m_ShaderGodRays, density, density );
		SHADER_UNIFORM_1f( m_ShaderGodRays, weight, weight );
		SHADER_UNIFORM_1f( m_ShaderGodRays, decay, decay );
		SHADER_UNIFORM_1f( m_ShaderGodRays, exposure, exposure );
		SHADER_UNIFORM_1i( m_ShaderGodRays, numSamples, numSamples );
		SHADER_UNIFORM_2f( m_ShaderGodRays, lightpos, x, y );
		SHADER_UNIFORM_2f( m_ShaderGodRays, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// Chromatic aberration
//-----------------------------------------------------------------------------

void CShaders::DrawChromaticAberration(int type, float dirX, float dirY, float shift, float strength)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hChromaticAberration) );

	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hChromaticAberration));

	SHADER_BIND( m_ShaderChromaticAberration );

		SHADER_UNIFORM_1i( m_ShaderChromaticAberration, type, type );
		SHADER_UNIFORM_1f( m_ShaderChromaticAberration, shift, shift );
		SHADER_UNIFORM_1f( m_ShaderChromaticAberration, strength, strength );
		SHADER_UNIFORM_2f( m_ShaderChromaticAberration, dir, dirX, dirY );
		SHADER_UNIFORM_2f( m_ShaderChromaticAberration, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// DoF blur
//-----------------------------------------------------------------------------

void CShaders::DrawDoFBlur(float minDistance, float maxDistance, int interptype, float radius, float samples, float bokeh)
{
	const Vector2D directions[3] =
	{
		{ 0.f, 1.f },
		{ 0.866f / m_aspect, 0.5f },
		{ 0.866f / m_aspect, -0.5f }
	};

	for (int i = 0; i < 3; i++)
	{
		InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hDoFBlur) );

		GLint texture0, texture1;

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE0 + 0); // Texture unit 0
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture0);
		GL_Bind(POST_PROCESSING_TEX(m_hDoFBlur));

		glActiveTexture(GL_TEXTURE0 + 1); // Texture unit 1
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture1);
		GL_Bind(POST_PROCESSING_TEX(m_hDepthBuffer));

		SHADER_BIND( m_ShaderDoFBlur );

			SHADER_UNIFORM_1i( m_ShaderDoFBlur, iChannel0, 0 );
			SHADER_UNIFORM_1i( m_ShaderDoFBlur, depthmap, 1 );
			SHADER_UNIFORM_1i( m_ShaderDoFBlur, interptype, interptype );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, znear, 4.f );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, zfar, maxDistance );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, distance, minDistance / maxDistance );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, bokeh, bokeh );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, samples, samples );
			SHADER_UNIFORM_1f( m_ShaderDoFBlur, radius, radius );
			SHADER_UNIFORM_2f( m_ShaderDoFBlur, dir, directions[i].x, directions[i].y );
			SHADER_UNIFORM_2f( m_ShaderDoFBlur, res, m_fwidth, m_fheight );
		
			glColor4ub(255, 255, 255, 255);
			DrawQuad(m_width, m_height);

		SHADER_UNBIND();

		GL_Bind(texture1); // unbind from texture unit 1
		glActiveTexture(GL_TEXTURE0 + 0);
		GL_Bind(texture0); // unbind from texture unit 0
	}
}

//-----------------------------------------------------------------------------
// Radial blur
//-----------------------------------------------------------------------------

void CShaders::DrawRadialBlur(float distance, float strength)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hRadialBlur) );

	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hRadialBlur));

	SHADER_BIND( m_ShaderRadialBlur );

		SHADER_UNIFORM_1f( m_ShaderRadialBlur, distance, distance );
		SHADER_UNIFORM_1f( m_ShaderRadialBlur, strength, strength );
		SHADER_UNIFORM_2f( m_ShaderRadialBlur, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// Bokeh blur
//-----------------------------------------------------------------------------

void CShaders::DrawBokehBlur(float radius, float samples, float bokeh)
{
	const Vector2D directions[3] =
	{
		{ 0.f, 1.f },
		{ 0.866f / m_aspect, 0.5f },
		{ 0.866f / m_aspect, -0.5f }
	};

	for (int i = 0; i < 3; i++)
	{
		InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hBokeh) );

		glEnable(GL_TEXTURE_2D);

		GL_Bind(POST_PROCESSING_TEX(m_hBokeh));

		SHADER_BIND( m_ShaderBokeh );

			SHADER_UNIFORM_1f( m_ShaderBokeh, bokeh, bokeh );
			SHADER_UNIFORM_1f( m_ShaderBokeh, samples, samples );
			SHADER_UNIFORM_2f( m_ShaderBokeh, dir, radius * directions[i].x, radius * directions[i].y );
			SHADER_UNIFORM_2f( m_ShaderBokeh, res, m_fwidth, m_fheight );
		
			glColor4ub(255, 255, 255, 255);
			DrawQuad(m_width, m_height);

		SHADER_UNBIND();
	}
}

//-----------------------------------------------------------------------------
// Gaussian blur
//-----------------------------------------------------------------------------

void CShaders::DrawGaussianBlur(float radius)
{
	const Vector2D directions[2] =
	{
		{ 1.f, 0.f },
		{ 0.f, 1.f }
	};

	for (int i = 0; i < 2; i++)
	{
		InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hGaussianBlur) );

		glEnable(GL_TEXTURE_2D);

		GL_Bind(POST_PROCESSING_TEX(m_hGaussianBlur));

		SHADER_BIND( m_ShaderGaussianBlur );

			SHADER_UNIFORM_1f( m_ShaderGaussianBlur, radius, radius );
			SHADER_UNIFORM_2f( m_ShaderGaussianBlur, dir, directions[i].x, directions[i].y );
			SHADER_UNIFORM_2f( m_ShaderGaussianBlur, res, m_fwidth, m_fheight );
		
			glColor4ub(255, 255, 255, 255);
			DrawQuad(m_width, m_height);

		SHADER_UNBIND();
	}
}

//-----------------------------------------------------------------------------
// Fast (should be) gaussian blur
//-----------------------------------------------------------------------------

void CShaders::DrawGaussianBlurFast(float radius)
{
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
		InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hGaussianBlurFast) );

		glEnable(GL_TEXTURE_2D);

		GL_Bind(POST_PROCESSING_TEX(m_hGaussianBlurFast));

		SHADER_BIND( m_ShaderGaussianBlurFast );

			SHADER_UNIFORM_2f( m_ShaderGaussianBlurFast, dir, radius * directions[i].x, radius * directions[i].y );
			SHADER_UNIFORM_2f( m_ShaderGaussianBlurFast, res, m_fwidth, m_fheight );
		
			glColor4ub(255, 255, 255, 255);
			DrawQuad(m_width, m_height);

		SHADER_UNBIND();
	}
}

//-----------------------------------------------------------------------------
// Vignette
//-----------------------------------------------------------------------------

void CShaders::DrawVignette(float falloff, float amount)
{
	InitColorTexPostProcessing( POST_PROCESSING_EXPAND_VARS(m_hVignette) );

	glEnable(GL_TEXTURE_2D);

	GL_Bind(POST_PROCESSING_TEX(m_hVignette));

	SHADER_BIND( m_ShaderVignette );

		SHADER_UNIFORM_1f( m_ShaderVignette, falloff, falloff );
		SHADER_UNIFORM_1f( m_ShaderVignette, amount, amount );
		SHADER_UNIFORM_2f( m_ShaderVignette, res, m_fwidth, m_fheight );
		
		glColor4ub(255, 255, 255, 255);
		DrawQuad(m_width, m_height);

	SHADER_UNBIND();
}

//-----------------------------------------------------------------------------
// Utility draw functions
//-----------------------------------------------------------------------------

void CShaders::DrawQuadPos(int x, int y, int w, int h)
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

void CShaders::DrawQuad(int w, int h)
{
	DrawQuadPos(0, 0, w, h);
}

//-----------------------------------------------------------------------------
// Bind framebuffer and texture to prepare post processing
//-----------------------------------------------------------------------------

void CShaders::InitColorTexPostProcessing(GLuint hFBO, GLuint hTex)
{
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, hFBO);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, hTex, 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hOldBuffer);

		glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);
}

void CShaders::InitDepthTexPostProcessing(GLuint hFBO, GLuint hTex)
{
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &m_hOldBuffer);

	glBindFramebuffer(GL_FRAMEBUFFER, hFBO);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, hTex, 0);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_hOldBuffer);

		glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	
	glBindFramebuffer(GL_FRAMEBUFFER, m_hOldBuffer);
}

//-----------------------------------------------------------------------------
// Encrypt & decrypt sequence of bytes of shaders
//-----------------------------------------------------------------------------

void CShaders::DecryptRawShaders()
{
#if !SHADERS_COMPILE_FROM_FILE
	auto decryptRawShader = [](char *bytes, unsigned int size)
	{
		for (unsigned int i = 0; i < size; i++)
		{
			bytes[i] ^= 0xFF;
		}
	};

	decryptRawShader( pp_fullscreen_bytes, pp_fullscreen_size );
	decryptRawShader( bloom_bytes, bloom_size );
	decryptRawShader( bokeh_bytes, bokeh_size );
	decryptRawShader( color_correction_bytes, color_correction_size );
	decryptRawShader( chromatic_aberration_bytes, chromatic_aberration_size );
	decryptRawShader( depth_buffer_bytes, depth_buffer_size );
	decryptRawShader( dof_blur_bytes, dof_blur_size );
	decryptRawShader( gaussian_blur_bytes, gaussian_blur_size );
	decryptRawShader( gaussian_blur_fast_bytes, gaussian_blur_fast_size );
	decryptRawShader( godrays_bytes, godrays_size );
	decryptRawShader( radial_blur_bytes, radial_blur_size );
	decryptRawShader( ssao_bytes, ssao_size );
	decryptRawShader( vignette_bytes, vignette_size );
#endif
}

void CShaders::EncryptRawShaders()
{
	DecryptRawShaders();
}

//-----------------------------------------------------------------------------
// Compile shaders
//-----------------------------------------------------------------------------

void CShaders::Compile()
{
	DecryptRawShaders();

	// Depth buffer
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderDepthBuffer, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\depth_buffer.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderDepthBuffer, pp_fullscreen_bytes, depth_buffer_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderDepthBuffer, znear )
		SHADER_LOCATE_UNIFORM( m_ShaderDepthBuffer, zfar )
		SHADER_LOCATE_UNIFORM( m_ShaderDepthBuffer, factor )
		SHADER_LOCATE_UNIFORM( m_ShaderDepthBuffer, res )
	SHADER_END_COMPILE();

	// Screen-Space Ambient Occlusion
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderSSAO, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\ssao.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderSSAO, pp_fullscreen_bytes, ssao_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, iChannel0 )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, depthmap )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, zNear )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, zFar )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, strength )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, samples )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, radius )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, aoclamp )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, noise )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, noiseamount )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, diffarea )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, gdisplace )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, mist )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, miststart )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, mistend )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, onlyAO )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, lumInfluence )
		SHADER_LOCATE_UNIFORM( m_ShaderSSAO, res )
	SHADER_END_COMPILE();
	
	// Color correction
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderColorCorrection, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\color_correction.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderColorCorrection, pp_fullscreen_bytes, color_correction_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, iTime )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_target_gamma )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_monitor_gamma )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_hue_offset )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_saturation )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_contrast )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_luminance )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_black_level )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_bright_boost )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_R )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_G )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_B )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_GRAIN_STR )
		//SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, ia_SHARPEN )
		SHADER_LOCATE_UNIFORM( m_ShaderColorCorrection, res )
	SHADER_END_COMPILE();
	
	// Chromatic aberration
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderChromaticAberration, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\chromatic_aberration.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderChromaticAberration, pp_fullscreen_bytes, chromatic_aberration_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderChromaticAberration, type )
		SHADER_LOCATE_UNIFORM( m_ShaderChromaticAberration, shift )
		SHADER_LOCATE_UNIFORM( m_ShaderChromaticAberration, strength )
		SHADER_LOCATE_UNIFORM( m_ShaderChromaticAberration, dir )
		SHADER_LOCATE_UNIFORM( m_ShaderChromaticAberration, res )
	SHADER_END_COMPILE();

	// God rays
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderGodRays, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\godrays.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderGodRays, pp_fullscreen_bytes, godrays_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, density )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, weight )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, decay )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, exposure )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, numSamples )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, lightpos )
		SHADER_LOCATE_UNIFORM( m_ShaderGodRays, res )
	SHADER_END_COMPILE();
	
	// Radial blur
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderRadialBlur, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\radial_blur.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderRadialBlur, pp_fullscreen_bytes, radial_blur_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderRadialBlur, distance )
		SHADER_LOCATE_UNIFORM( m_ShaderRadialBlur, strength )
		SHADER_LOCATE_UNIFORM( m_ShaderRadialBlur, res )
	SHADER_END_COMPILE();
	
	// Gaussian blur
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderGaussianBlur, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\gaussian_blur.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderGaussianBlur, pp_fullscreen_bytes, gaussian_blur_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderGaussianBlur, radius )
		SHADER_LOCATE_UNIFORM( m_ShaderGaussianBlur, dir )
		SHADER_LOCATE_UNIFORM( m_ShaderGaussianBlur, res )
	SHADER_END_COMPILE();

	// Gaussian blur fast
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderGaussianBlurFast, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\gaussian_blur_fast.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderGaussianBlurFast, pp_fullscreen_bytes, gaussian_blur_fast_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderGaussianBlurFast, dir )
		SHADER_LOCATE_UNIFORM( m_ShaderGaussianBlurFast, res )
	SHADER_END_COMPILE();

	// Bokeh blur
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderBokeh, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\bokeh.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderBokeh, pp_fullscreen_bytes, bokeh_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderBokeh, bokeh )
		SHADER_LOCATE_UNIFORM( m_ShaderBokeh, samples )
		SHADER_LOCATE_UNIFORM( m_ShaderBokeh, dir )
		SHADER_LOCATE_UNIFORM( m_ShaderBokeh, res )
	SHADER_END_COMPILE();

	// DoF blur
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderDoFBlur, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\dof_blur.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderDoFBlur, pp_fullscreen_bytes, dof_blur_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, iChannel0 )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, depthmap )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, interptype )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, znear )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, zfar )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, distance )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, bokeh )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, samples )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, radius )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, dir )
		SHADER_LOCATE_UNIFORM( m_ShaderDoFBlur, res )
	SHADER_END_COMPILE();
	
	// Vignette
#if SHADERS_COMPILE_FROM_FILE
	SHADER_BEGIN_COMPILE_FILE( m_ShaderVignette, "sven_internal\\shaders\\pp_fullscreen.vsh", "sven_internal\\shaders\\vignette.fsh" )
#else
	SHADER_BEGIN_COMPILE( m_ShaderVignette, pp_fullscreen_bytes, vignette_bytes )
#endif
		SHADER_LOCATE_UNIFORM( m_ShaderVignette, falloff )
		SHADER_LOCATE_UNIFORM( m_ShaderVignette, amount )
		SHADER_LOCATE_UNIFORM( m_ShaderVignette, res )
	SHADER_END_COMPILE();

	EncryptRawShaders();
}

//-----------------------------------------------------------------------------
// CShaders implementation
//-----------------------------------------------------------------------------

CShaders::CShaders()
{
	m_width = 0;
	m_height = 0;
	
	m_fwidth = 0.f;
	m_fheight = 0.f;

	m_aspect = 0.f;

	m_pfnClientDLL_HudRedraw = NULL;
	m_hClientDLL_HudRedraw = 0;

	m_hOldBuffer = 0;

	POST_PROCESSING_RESET_VARS( m_hDepthBuffer );
	POST_PROCESSING_RESET_VARS( m_hSSAO );
	POST_PROCESSING_RESET_VARS( m_hColorCorrection );
	POST_PROCESSING_RESET_VARS( m_hGodRays );
	POST_PROCESSING_RESET_VARS( m_hChromaticAberration );
	POST_PROCESSING_RESET_VARS( m_hDoFBlur );
	POST_PROCESSING_RESET_VARS( m_hRadialBlur );
	POST_PROCESSING_RESET_VARS( m_hBokeh );
	POST_PROCESSING_RESET_VARS( m_hGaussianBlur );
	POST_PROCESSING_RESET_VARS( m_hGaussianBlurFast );
	POST_PROCESSING_RESET_VARS( m_hVignette );
}

bool CShaders::Load()
{
	m_pfnClientDLL_HudRedraw = MemoryUtils()->FindPattern( SvenModAPI()->Modules()->Hardware, Patterns::Hardware::ClientDLL_HudRedraw );

	if ( m_pfnClientDLL_HudRedraw == NULL )
	{
		Warning("Failed to find function \"ClientDLL_HudRedraw\"\n");
		return false;
	}

	return true;
}

void CShaders::PostLoad()
{
	m_width = g_ScreenInfo.width;
	m_height = g_ScreenInfo.height;

	m_fwidth = (float)m_width;
	m_fheight = (float)m_height;

	m_aspect = m_fwidth / m_fheight;

	m_hClientDLL_HudRedraw = DetoursAPI()->DetourFunction( m_pfnClientDLL_HudRedraw, HOOKED_ClientDLL_HudRedraw, GET_FUNC_PTR(ORIG_ClientDLL_HudRedraw) );

	Compile();

	POST_PROCESSING_INIT_VARS_DEPTH( m_hDepthBuffer, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hSSAO, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hColorCorrection, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hGodRays, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hChromaticAberration, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hDoFBlur, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hRadialBlur, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hBokeh, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hGaussianBlur, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hGaussianBlurFast, m_width, m_height );
	POST_PROCESSING_INIT_VARS_COLOR( m_hVignette, m_width, m_height );
}

void CShaders::Unload()
{
	DetoursAPI()->RemoveDetour( m_hClientDLL_HudRedraw );

	POST_PROCESSING_FREE_VARS( m_hDepthBuffer );
	POST_PROCESSING_FREE_VARS( m_hSSAO );
	POST_PROCESSING_FREE_VARS( m_hColorCorrection );
	POST_PROCESSING_FREE_VARS( m_hGodRays );
	POST_PROCESSING_FREE_VARS( m_hChromaticAberration );
	POST_PROCESSING_FREE_VARS( m_hDoFBlur );
	POST_PROCESSING_FREE_VARS( m_hRadialBlur );
	POST_PROCESSING_FREE_VARS( m_hBokeh );
	POST_PROCESSING_FREE_VARS( m_hGaussianBlur );
	POST_PROCESSING_FREE_VARS( m_hGaussianBlurFast );
	POST_PROCESSING_FREE_VARS( m_hVignette );
}