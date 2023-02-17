uniform sampler2D iChannel0;
uniform float falloff;
uniform float amount;
uniform vec2 res;

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
    vec4 color = texture2D(iChannel0, uv);
    
    float dist = distance(uv, vec2(0.5, 0.5));
    color.rgb *= smoothstep(0.8, falloff * 0.799, dist * (amount + falloff));
    
    gl_FragColor = color;
}