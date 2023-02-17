uniform sampler2D iChannel0;
uniform int type;
uniform float shift;
uniform float strength;
uniform vec2 dir;
uniform vec2 res;

vec2 barrelDistortion(vec2 coord, float amt)
{
	float dist = 1.0;
	vec2 cc = coord - 0.5;
	
	if ( type == 1 )
		dist = dot(cc, cc);
	
	return coord - cc * dist * amt;
}

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	vec3 color, colorInput = texture2D(iChannel0, uv).rgb;
	
	if ( type == 0 )
	{
		color.r = texture2D(iChannel0, uv + (dir * shift) / res).r;
		color.g = colorInput.g;
		color.b = texture2D(iChannel0, uv - (dir * shift) / res).b;
	}
	else
	{
		color.r = texture2D(iChannel0, barrelDistortion(uv, shift)).r;
		color.g = colorInput.g;
		color.b = texture2D(iChannel0, barrelDistortion(uv, -shift)).b;
	}

	gl_FragColor.rgb = mix(colorInput, color, strength);
}