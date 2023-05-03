uniform sampler2D iChannel0;
uniform float iTime;
uniform float ia_target_gamma; // 2.2
uniform float ia_monitor_gamma; // 2.2
uniform float ia_hue_offset; // 0.0
uniform float ia_saturation; // 1.0
uniform float ia_contrast; // 1.0
uniform float ia_luminance; // 1.0
uniform float ia_black_level; // 0.0
uniform float ia_bright_boost; // 0.0
uniform float ia_R; // 1.0
uniform float ia_G; // 1.0
uniform float ia_B; // 1.0
uniform float ia_GRAIN_STR; // 0.0
// uniform float ia_SHARPEN; // 0.0
uniform vec2 res;

vec3 rgb2hsv(vec3 c)
{
	vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
	vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;

	return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c)
{
	vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);

	return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 filmGrain(vec2 uv, float strength)
{
	if ( strength == 0.0 )
		return vec3(0.0);
	
	float x = ( uv.x + 4.0 ) * ( uv.y + 4.0 ) * ( (iTime + 10.0) * 10.0 );
	return vec3( mod( ( mod(x, 13.0) + 1.0 ) * ( mod(x, 123.0) + 1.0 ), 0.01 ) - 0.005 ) * strength;
}

// vec3 sharp(sampler2D tex, vec2 texCoord)
// {
	// if ( ia_SHARPEN == 0.0 )
		// return vec3(0.0);
	
	// vec2 p = texCoord * res + vec2(0.5, 0.5);
	// vec2 i = floor(p);
	// vec2 f = p - i;
	
	// f = f * f * f * (f * (f * 6.0 - vec2(15.0, 15.0)) + vec2(10.0, 10.0));
	
	// p = i + f;
	// p = (p - vec2(0.5, 0.5)) / res;
	
	// return texture2D(tex, p).rgb;
// }

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	vec3 film_grain = filmGrain(uv, ia_GRAIN_STR);
	vec3 result = texture2D(iChannel0, uv).rgb; 
	
	// result = mix( result, sharp(iChannel0, uv), ia_SHARPEN ) + film_grain;
	
	result = result + film_grain;
	
	vec3 hsv = rgb2hsv( result );
	vec3 gamma = vec3( ia_monitor_gamma / ia_target_gamma );
	
	// hue
	hsv.x += ia_hue_offset;
	
	if ( hsv.x > 1.0 )
		hsv.x -= 1.0;
	
	// saturation and luminance
	vec3 satColor = clamp( hsv2rgb( hsv * vec3(1.0, ia_saturation, ia_luminance) ), 0.0, 1.0 );
	
	// contrast and brightness
	vec3 conColor = clamp( (satColor - 0.5) * ia_contrast + 0.5 + ia_bright_boost, 0.0, 1.0 );

	conColor -= vec3(ia_black_level); // apply black level
	conColor *= (vec3(1.0) / vec3(1.0 - ia_black_level));
	conColor = pow(conColor, 1.0 / vec3(gamma)); // Apply gamma correction
	conColor *= vec3(ia_R, ia_G, ia_B);
	
	gl_FragColor.rgb = conColor;
}