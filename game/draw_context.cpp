#include <stdio.h>
#include <Windows.h>
#include <gl/GL.h>

#include "draw_context.h"

//-----------------------------------------------------------------------------
// Draw box, no depth buffer
//-----------------------------------------------------------------------------

CDrawBoxNoDepthBuffer::CDrawBoxNoDepthBuffer( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color ) : m_color( color )
{
	if ( vOrigin.x == 0.f && vOrigin.y == 0.f && vOrigin.z == 0.f )
	{
		m_vecDrawOrigin = vMins + ( vMaxs - vMins ) * 0.5f;
	}
	else
	{
		m_vecDrawOrigin = vOrigin;
	}

	m_vecOrigin = vOrigin;
	m_vecMins = vMins;
	m_vecMaxs = vMaxs;
}

void CDrawBoxNoDepthBuffer::Draw()
{
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_TEXTURE_2D );

	Vector vecPoints[ 8 ];

	Vector vecMins = m_vecMins;
	Vector vecMaxs = m_vecMaxs;

	VectorAdd( vecMins, m_vecOrigin, vecMins );
	VectorAdd( vecMaxs, m_vecOrigin, vecMaxs );

	// Build points of box
	vecPoints[ 0 ].x = vecMins.x;
	vecPoints[ 0 ].y = vecMins.y;
	vecPoints[ 0 ].z = vecMins.z;

	vecPoints[ 1 ].x = vecMins.x;
	vecPoints[ 1 ].y = vecMaxs.y;
	vecPoints[ 1 ].z = vecMins.z;

	vecPoints[ 2 ].x = vecMaxs.x;
	vecPoints[ 2 ].y = vecMaxs.y;
	vecPoints[ 2 ].z = vecMins.z;

	vecPoints[ 3 ].x = vecMaxs.x;
	vecPoints[ 3 ].y = vecMins.y;
	vecPoints[ 3 ].z = vecMins.z;

	vecPoints[ 4 ].x = vecMins.x;
	vecPoints[ 4 ].y = vecMins.y;
	vecPoints[ 4 ].z = vecMaxs.z;

	vecPoints[ 5 ].x = vecMins.x;
	vecPoints[ 5 ].y = vecMaxs.y;
	vecPoints[ 5 ].z = vecMaxs.z;

	vecPoints[ 6 ].x = vecMaxs.x;
	vecPoints[ 6 ].y = vecMaxs.y;
	vecPoints[ 6 ].z = vecMaxs.z;

	vecPoints[ 7 ].x = vecMaxs.x;
	vecPoints[ 7 ].y = vecMins.y;
	vecPoints[ 7 ].z = vecMaxs.z;

	glColor4ub( m_color.r, m_color.g, m_color.b, m_color.a );

	for ( int i = 0; i < 4; i++ )
	{
		int j = ( i + 1 ) % 4;

		glBegin( GL_TRIANGLE_STRIP );
		glVertex3f( VectorExpand( vecPoints[ i ] ) );
		glVertex3f( VectorExpand( vecPoints[ j ] ) );
		glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );
		glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );
		glEnd();
	}

	// Bottom
	glBegin( GL_TRIANGLE_STRIP );
	glVertex3f( VectorExpand( vecPoints[ 2 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 1 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 3 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 0 ] ) );
	glEnd();

	// Top
	glBegin( GL_TRIANGLE_STRIP );
	glVertex3f( VectorExpand( vecPoints[ 4 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 5 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 7 ] ) );
	glVertex3f( VectorExpand( vecPoints[ 6 ] ) );
	glEnd();

	glEnable( GL_TEXTURE_2D );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );
	glEnable( GL_DEPTH_TEST );
}

//-----------------------------------------------------------------------------
// Draw wireframe box
//-----------------------------------------------------------------------------

CWireframeBox::CWireframeBox( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Color &color, float width, bool bIgnoreDepthBuffer ) : m_color( color )
{
	if ( vOrigin.x == 0.f && vOrigin.y == 0.f && vOrigin.z == 0.f )
	{
		m_vecDrawOrigin = vMins + ( vMaxs - vMins ) * 0.5f;
	}
	else
	{
		m_vecDrawOrigin = vOrigin;
	}

	m_vecOrigin = vOrigin;
	m_vecMins = vMins;
	m_vecMaxs = vMaxs;

	m_flWidth = width;
	m_bIgnoreDepthBuffer = bIgnoreDepthBuffer;
}

void CWireframeBox::Draw()
{
	glEnable( GL_BLEND );

	if ( m_bIgnoreDepthBuffer )
		glDisable( GL_DEPTH_TEST );

	glDisable( GL_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_TEXTURE_2D );

	Vector vecPoints[ 8 ];

	Vector vecMins = m_vecMins;
	Vector vecMaxs = m_vecMaxs;

	VectorAdd( vecMins, m_vecOrigin, vecMins );
	VectorAdd( vecMaxs, m_vecOrigin, vecMaxs );

	// Build points of box
	vecPoints[ 0 ].x = vecMins.x;
	vecPoints[ 0 ].y = vecMins.y;
	vecPoints[ 0 ].z = vecMins.z;

	vecPoints[ 1 ].x = vecMins.x;
	vecPoints[ 1 ].y = vecMaxs.y;
	vecPoints[ 1 ].z = vecMins.z;

	vecPoints[ 2 ].x = vecMaxs.x;
	vecPoints[ 2 ].y = vecMaxs.y;
	vecPoints[ 2 ].z = vecMins.z;

	vecPoints[ 3 ].x = vecMaxs.x;
	vecPoints[ 3 ].y = vecMins.y;
	vecPoints[ 3 ].z = vecMins.z;

	vecPoints[ 4 ].x = vecMins.x;
	vecPoints[ 4 ].y = vecMins.y;
	vecPoints[ 4 ].z = vecMaxs.z;

	vecPoints[ 5 ].x = vecMins.x;
	vecPoints[ 5 ].y = vecMaxs.y;
	vecPoints[ 5 ].z = vecMaxs.z;

	vecPoints[ 6 ].x = vecMaxs.x;
	vecPoints[ 6 ].y = vecMaxs.y;
	vecPoints[ 6 ].z = vecMaxs.z;

	vecPoints[ 7 ].x = vecMaxs.x;
	vecPoints[ 7 ].y = vecMins.y;
	vecPoints[ 7 ].z = vecMaxs.z;

	glColor4ub( m_color.r, m_color.g, m_color.b, m_color.a );
	glLineWidth( m_flWidth );

	for ( int i = 0; i < 4; i++ )
	{
		int j = ( i + 1 ) % 4;

		glBegin( GL_LINES );
			glVertex3f( VectorExpand( vecPoints[ i ] ) );
			glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );

			glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );
			glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );

			glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );
			glVertex3f( VectorExpand( vecPoints[ i ] ) );

			glVertex3f( VectorExpand( vecPoints[ i ] ) );
			glVertex3f( VectorExpand( vecPoints[ j ] ) );
		glEnd();
	}
	
	// Bottom & Top
	glBegin( GL_LINES );
		glVertex3f( VectorExpand( vecPoints[ 0 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 2 ] ) );

		glVertex3f( VectorExpand( vecPoints[ 4 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 6 ] ) );
	glEnd();

	/*
	// Turn on wireframe mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	for ( int i = 0; i < 4; i++ )
	{
		int j = ( i + 1 ) % 4;

		glBegin( GL_TRIANGLE_STRIP );
			glVertex3f( VectorExpand( vecPoints[ i ] ) );
			glVertex3f( VectorExpand( vecPoints[ j ] ) );
			glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );
			glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );
		glEnd();
	}

	// Bottom
	glBegin( GL_TRIANGLE_STRIP );
		glVertex3f( VectorExpand( vecPoints[ 2 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 1 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 3 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 0 ] ) );
	glEnd();

	// Top
	glBegin( GL_TRIANGLE_STRIP );
		glVertex3f( VectorExpand( vecPoints[ 4 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 5 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 7 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 6 ] ) );
	glEnd();

	// Turn off wireframe mode
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	*/

	glLineWidth( 1.f );

	glEnable( GL_TEXTURE_2D );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );

	if ( m_bIgnoreDepthBuffer )
		glEnable( GL_DEPTH_TEST );
}

//-----------------------------------------------------------------------------
// Draw rotated wireframe box
//-----------------------------------------------------------------------------

CWireframeBoxAngles::CWireframeBoxAngles( const Vector &vOrigin, const Vector &vMins, const Vector &vMaxs, const Vector &vAngles, const Color &color, float width, bool bIgnoreDepthBuffer ) : m_color( color )
{
	if ( vOrigin.x == 0.f && vOrigin.y == 0.f && vOrigin.z == 0.f )
	{
		m_vecDrawOrigin = vMins + ( vMaxs - vMins ) * 0.5f;
	}
	else
	{
		m_vecDrawOrigin = vOrigin;
	}

	m_vecOrigin = vOrigin;
	m_vecAngles = vAngles;
	m_vecMins = vMins;
	m_vecMaxs = vMaxs;

	m_flWidth = width;
	m_bIgnoreDepthBuffer = bIgnoreDepthBuffer;
}

void CWireframeBoxAngles::Draw()
{
	glEnable( GL_BLEND );

	if ( m_bIgnoreDepthBuffer )
		glDisable( GL_DEPTH_TEST );

	glDisable( GL_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_TEXTURE_2D );

	Vector vecPoints[ 8 ];

	Vector vecMins = m_vecMins;
	Vector vecMaxs = m_vecMaxs;

	// Build points of box
	vecPoints[ 0 ].x = vecMins.x;
	vecPoints[ 0 ].y = vecMins.y;
	vecPoints[ 0 ].z = vecMins.z;

	vecPoints[ 1 ].x = vecMins.x;
	vecPoints[ 1 ].y = vecMaxs.y;
	vecPoints[ 1 ].z = vecMins.z;

	vecPoints[ 2 ].x = vecMaxs.x;
	vecPoints[ 2 ].y = vecMaxs.y;
	vecPoints[ 2 ].z = vecMins.z;

	vecPoints[ 3 ].x = vecMaxs.x;
	vecPoints[ 3 ].y = vecMins.y;
	vecPoints[ 3 ].z = vecMins.z;

	vecPoints[ 4 ].x = vecMins.x;
	vecPoints[ 4 ].y = vecMins.y;
	vecPoints[ 4 ].z = vecMaxs.z;

	vecPoints[ 5 ].x = vecMins.x;
	vecPoints[ 5 ].y = vecMaxs.y;
	vecPoints[ 5 ].z = vecMaxs.z;

	vecPoints[ 6 ].x = vecMaxs.x;
	vecPoints[ 6 ].y = vecMaxs.y;
	vecPoints[ 6 ].z = vecMaxs.z;

	vecPoints[ 7 ].x = vecMaxs.x;
	vecPoints[ 7 ].y = vecMins.y;
	vecPoints[ 7 ].z = vecMaxs.z;

	// Transform
	Vector temp;
	float localSpaceToWorld[ 3 ][ 4 ];

	AngleMatrix( m_vecAngles, localSpaceToWorld );

	localSpaceToWorld[ 0 ][ 3 ] = m_vecOrigin[ 0 ];
	localSpaceToWorld[ 1 ][ 3 ] = m_vecOrigin[ 1 ];
	localSpaceToWorld[ 2 ][ 3 ] = m_vecOrigin[ 2 ];

	for ( int i = 0; i < 8; i++ )
	{
		VectorTransform( vecPoints[ i ], localSpaceToWorld, temp );

		vecPoints[ i ] = temp;
	}

	glColor4ub( m_color.r, m_color.g, m_color.b, m_color.a );
	glLineWidth( m_flWidth );

	for ( int i = 0; i < 4; i++ )
	{
		int j = ( i + 1 ) % 4;

		glBegin( GL_LINES );
			glVertex3f( VectorExpand( vecPoints[ i ] ) );
			glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );

			glVertex3f( VectorExpand( vecPoints[ i + 4 ] ) );
			glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );

			glVertex3f( VectorExpand( vecPoints[ j + 4 ] ) );
			glVertex3f( VectorExpand( vecPoints[ i ] ) );

			glVertex3f( VectorExpand( vecPoints[ i ] ) );
			glVertex3f( VectorExpand( vecPoints[ j ] ) );
		glEnd();
	}
	
	// Bottom & Top
	glBegin( GL_LINES );
		glVertex3f( VectorExpand( vecPoints[ 0 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 2 ] ) );

		glVertex3f( VectorExpand( vecPoints[ 4 ] ) );
		glVertex3f( VectorExpand( vecPoints[ 6 ] ) );
	glEnd();

	glLineWidth( 1.f );

	glEnable( GL_TEXTURE_2D );

	glDisable( GL_BLEND );
	glDisable( GL_ALPHA_TEST );

	if ( m_bIgnoreDepthBuffer )
		glEnable( GL_DEPTH_TEST );
}

//-----------------------------------------------------------------------------
// Draw linear trajectory
//-----------------------------------------------------------------------------

CDrawTrajectory::CDrawTrajectory(const Color &lineColor, const Color &impactColor, float flWidth)
{
	m_lineColor = lineColor;
	m_impactColor = impactColor;
	m_flWidth = flWidth;
}

CDrawTrajectory::~CDrawTrajectory()
{
	m_trajectoryLines.clear();
	m_impacts.clear();
}

void CDrawTrajectory::Draw()
{
	glEnable( GL_BLEND );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_ALPHA_TEST );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDisable( GL_TEXTURE_2D );

	if ( !m_trajectoryLines.empty() )
	{
		glColor4ub( m_lineColor.r, m_lineColor.g, m_lineColor.b, m_lineColor.a );
		glLineWidth( m_flWidth );

		glBegin( GL_LINES );

		for (const Vector &point : m_trajectoryLines)
		{
			glVertex3f( VectorExpand(point) );
		}

		glEnd();

		glLineWidth( 1.f );
	}

	if ( !m_impacts.empty() )
	{
		Vector vecPoints[8];

		glColor4ub( m_impactColor.r, m_impactColor.g, m_impactColor.b, m_impactColor.a );

		for (const Vector &impact : m_impacts)
		{
			Vector vecMins(-2, -2, -2);
			Vector vecMaxs(2, 2, 2);

			VectorAdd( vecMins, impact, vecMins );
			VectorAdd( vecMaxs, impact, vecMaxs );

			// Build points of box
			vecPoints[0].x = vecMins.x;
			vecPoints[0].y = vecMins.y;
			vecPoints[0].z = vecMins.z;

			vecPoints[1].x = vecMins.x;
			vecPoints[1].y = vecMaxs.y;
			vecPoints[1].z = vecMins.z;

			vecPoints[2].x = vecMaxs.x;
			vecPoints[2].y = vecMaxs.y;
			vecPoints[2].z = vecMins.z;

			vecPoints[3].x = vecMaxs.x;
			vecPoints[3].y = vecMins.y;
			vecPoints[3].z = vecMins.z;

			vecPoints[4].x = vecMins.x;
			vecPoints[4].y = vecMins.y;
			vecPoints[4].z = vecMaxs.z;

			vecPoints[5].x = vecMins.x;
			vecPoints[5].y = vecMaxs.y;
			vecPoints[5].z = vecMaxs.z;

			vecPoints[6].x = vecMaxs.x;
			vecPoints[6].y = vecMaxs.y;
			vecPoints[6].z = vecMaxs.z;

			vecPoints[7].x = vecMaxs.x;
			vecPoints[7].y = vecMins.y;
			vecPoints[7].z = vecMaxs.z;

			for (int i = 0; i < 4; i++)
			{
				int j = (i + 1) % 4;

				glBegin( GL_TRIANGLE_STRIP );
					glVertex3f( VectorExpand(vecPoints[i]) );
					glVertex3f( VectorExpand(vecPoints[j]) );
					glVertex3f( VectorExpand(vecPoints[i + 4]) );
					glVertex3f( VectorExpand(vecPoints[j + 4]) );
				glEnd();
			}

			// Bottom
			glBegin( GL_TRIANGLE_STRIP );
				glVertex3f( VectorExpand(vecPoints[2]) );
				glVertex3f( VectorExpand(vecPoints[1]) );
				glVertex3f( VectorExpand(vecPoints[3]) );
				glVertex3f( VectorExpand(vecPoints[0]) );
			glEnd();

			// Top
			glBegin( GL_TRIANGLE_STRIP );
				glVertex3f( VectorExpand(vecPoints[4]) );
				glVertex3f( VectorExpand(vecPoints[5]) );
				glVertex3f( VectorExpand(vecPoints[7]) );
				glVertex3f( VectorExpand(vecPoints[6]) );
			glEnd();
		}
	}

	glEnable( GL_TEXTURE_2D );

	glEnable( GL_DEPTH_TEST );
	glDisable( GL_BLEND );
}

bool CDrawTrajectory::ShouldStopDraw()
{
	return false;
}

const Vector &CDrawTrajectory::GetDrawOrigin() const
{
	return g_vecZero;
}

void CDrawTrajectory::AddLine(const Vector &start, const Vector &end)
{
	m_trajectoryLines.push_back( start );
	m_trajectoryLines.push_back( end );
}

void CDrawTrajectory::AddImpact(const Vector &impact)
{
	m_impacts.push_back( impact );
}

//-----------------------------------------------------------------------------
// Draw
//-----------------------------------------------------------------------------

void DrawBox( const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, float r, float g, float b, float alpha, float width, bool wireframe )
{
	if ( wireframe )
	{
		CWireframeBox *pWireframeBox = new CWireframeBox( vecOrigin, vecMins, vecMaxs, Color( r, g, b, alpha ), width, false );

		Render()->AddDrawContext( pWireframeBox );
	}
	else
	{
		Render()->DrawBox( vecOrigin,
						   vecMins,
						   vecMaxs,
						   r,
						   g,
						   b,
						   alpha );
	}
}

void DrawBoxAngles( const Vector &vecOrigin, const Vector &vecMins, const Vector &vecMaxs, const Vector &vecAngles, float r, float g, float b, float alpha, float width, bool wireframe )
{
	if ( wireframe )
	{
		CWireframeBoxAngles *pWireframeBoxAngles = new CWireframeBoxAngles( vecOrigin, vecMins, vecMaxs, vecAngles, Color( r, g, b, alpha ), width, false );

		Render()->AddDrawContext( pWireframeBoxAngles );
	}
	else
	{
		Render()->DrawBoxAngles( vecOrigin, vecMins, vecMaxs, vecAngles, r, g, b, alpha );
	}
}