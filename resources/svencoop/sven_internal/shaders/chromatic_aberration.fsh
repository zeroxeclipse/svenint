uniform sampler2D iChannel0;
uniform float shift;
uniform float strength;
uniform vec2 pixelSize;
uniform vec2 res;

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	vec3 color, colorInput = texture2D(iChannel0, uv).rgb;

	color.r = texture2D(iChannel0, uv + (pixelSize * shift) / res).r;
	color.g = colorInput.g;
	color.b = texture2D(iChannel0, uv - (pixelSize * shift) / res).b;

	gl_FragColor.rgb = colorInput + (color - colorInput) * strength; // lerp
}