uniform sampler2D iChannel0; // normal texture
uniform sampler2D depthmap; // depth buffer
uniform int interptype;
uniform float znear;
uniform float zfar;
uniform float distance; // fraction of  minDistance / zFar (which is maxDistance)
uniform float bokeh;
uniform float samples;
uniform float radius;
uniform vec2 dir;
uniform vec2 res;

float curvilinear_value(in float time)
{
	switch (interptype)
	{
	// Simple spline
	case 0:
	{
		float time_squared = time * time;
		return 3 * time_squared - 2 * time_squared * time;
		break;
	}
	
	// Parabolic
	case 1:
	{
		float time_squared = time * time;
		return 2 * time_squared - time_squared;
		break;
	}

	// Parabolic inverted
	case 2:
		return 2 * time - time * time;
		break;
	
	// Cubic
	case 3:
	{
		float time_squared = time * time;
		return 3 * time_squared * time - 2 * time_squared * time_squared;
		break;
	}
	}
	
	// Linear, no change
	return time;
}

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	float depth = texture2D(depthmap, uv).x;
	depth = (2.0 * znear) / (zfar + znear - depth * (zfar - znear));
	
	if ( distance >= depth )
	{
		// gl_FragColor = texture2D(iChannel0, uv);
		return;
	}
	
	float time = (depth - distance) / (1.0 - distance);
	
	// Change linear correlation to curvilinear
	time = curvilinear_value( time );
	
	// Simple spline
	// float time_squared = time * time;
	// time = 3 * time_squared - 2 * time_squared * time;
	
	vec2 radiusDir = dir * ( radius * ( time ) );
	
	vec4 sum = vec4(0.0);
	vec4 msum = vec4(0.0);

	float delta = 1.0 / samples;
	float di = 1.0 / ( samples - 1.0 );
	
	for (float i = -0.5; i < 0.501; i += di)
	{
		vec4 color = texture2D(iChannel0, uv + (radiusDir * i) / res);
		sum += color * delta;
		msum = max(color, msum);
	}

	gl_FragColor = mix(sum, msum, bokeh);
}