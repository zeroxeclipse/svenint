// Expand IRender interface with user IDrawContext

#ifndef DRAW_CONTEXT_H
#define DRAW_CONTEXT_H

#ifdef _WIN32
#pragma once
#endif

#include <hl_sdk/engine/APIProxy.h>
#include <math/vector.h>
#include <IRender.h>

#include <vector>

//-----------------------------------------------------------------------------
// Draw box, no depth buffer
//-----------------------------------------------------------------------------

class CDrawBoxNoDepthBuffer : public IDrawContext
{
public:
	CDrawBoxNoDepthBuffer( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color );
	virtual ~CDrawBoxNoDepthBuffer( void ) {}

	virtual void Draw( void ) override;
	virtual bool ShouldStopDraw( void ) override { return false; };

	virtual const Vector &GetDrawOrigin( void ) const override { return m_vecDrawOrigin; };

private:
	Vector m_vecDrawOrigin;
	Vector m_vecOrigin;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Color m_color;
};

//-----------------------------------------------------------------------------
// Draw wireframe box
//-----------------------------------------------------------------------------

class CWireframeBox : public IDrawContext
{
public:
	CWireframeBox( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color, float width, bool bIgnoreDepthBuffer );
	virtual ~CWireframeBox( void ) {}

	virtual void Draw( void ) override;
	virtual bool ShouldStopDraw( void ) override { return false; };

	virtual const Vector &GetDrawOrigin( void ) const override { return m_vecDrawOrigin; };

private:
	Vector m_vecDrawOrigin;
	Vector m_vecOrigin;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Color m_color;

	float m_flWidth;
	bool m_bIgnoreDepthBuffer;
};

//-----------------------------------------------------------------------------
// Draw wireframe rotated box
//-----------------------------------------------------------------------------

class CWireframeBoxAngles : public IDrawContext
{
public:
	CWireframeBoxAngles( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Vector &vAngles, const Color &color, float width, bool bIgnoreDepthBuffer );
	virtual ~CWireframeBoxAngles( void ) {}

	virtual void Draw( void ) override;
	virtual bool ShouldStopDraw( void ) override { return false; };

	virtual const Vector &GetDrawOrigin( void ) const override { return m_vecDrawOrigin; };

private:
	Vector m_vecDrawOrigin;
	Vector m_vecOrigin;
	Vector m_vecAngles;

	Vector m_vecMins;
	Vector m_vecMaxs;

	Color m_color;

	float m_flWidth;
	bool m_bIgnoreDepthBuffer;
};

//-----------------------------------------------------------------------------
// Draw linear trajectory
//-----------------------------------------------------------------------------

class CDrawTrajectory : public IDrawContext
{
public:
	CDrawTrajectory( const Color &lineColor, const Color &impactColor, float flWidth = 2.f );
	virtual ~CDrawTrajectory();

	virtual void Draw() override;
	virtual bool ShouldStopDraw() override;

	virtual const Vector &GetDrawOrigin() const override;

public:
	void AddLine( const Vector &start, const Vector &end );
	void AddImpact( const Vector &impact );

private:
	std::vector<Vector> m_trajectoryLines;
	std::vector<Vector> m_impacts;

	Color m_lineColor;
	Color m_impactColor;

	float m_flWidth;
};

//-----------------------------------------------------------------------------
// Draws
//-----------------------------------------------------------------------------

void DrawBox( const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, float r, float g, float b, float alpha, float width, bool wireframe );
void DrawBoxAngles( const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, const Vector &vecAngles, float r, float g, float b, float alpha, float width, bool wireframe );

#endif // DRAW_CONTEXT_H