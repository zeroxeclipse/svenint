uniform sampler2D iChannel0;
uniform float radius;
uniform vec2 dir;
uniform vec2 res;

#define INV_SQRT_2PI_X3 1.1968268412042980338198381798031

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	float r = radius;

	float exp_value = -4.5 / r / r;
	float sqrt_value = INV_SQRT_2PI_X3 / r;

	float sum = 0.0;
	vec4 value = vec4(0.0);

	float x = 1.0;
	
	while ( x <= r )
	{
		float currentScale = exp(exp_value * x * x);
		sum += currentScale;

		vec2 dudv = ( dir.xy * x ) / res;
		value += currentScale * (texture(iChannel0, uv - dudv) + texture(iChannel0, uv + dudv) );
		x += 1.0;
	}

	float correction = 1.0 / sqrt_value - 2.0 * sum;
	value += texture(iChannel0, uv) * correction;

	gl_FragColor = value * sqrt_value;
}