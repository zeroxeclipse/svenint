uniform sampler2D iChannel0;
uniform vec2 dir;
uniform vec2 res;

void main()
{
	vec4 color = vec4(0.0);
	
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	vec2 off1 = vec2(1.411764705882353) * dir;
	vec2 off2 = vec2(3.2941176470588234) * dir;
	vec2 off3 = vec2(5.176470588235294) * dir;
	
	color += texture2D(iChannel0, uv) * 0.1964825501511404;
	color += texture2D(iChannel0, uv + (off1 / res)) * 0.2969069646728344;
	color += texture2D(iChannel0, uv - (off1 / res)) * 0.2969069646728344;
	color += texture2D(iChannel0, uv + (off2 / res)) * 0.09447039785044732;
	color += texture2D(iChannel0, uv - (off2 / res)) * 0.09447039785044732;
	color += texture2D(iChannel0, uv + (off3 / res)) * 0.010381362401148057;
	color += texture2D(iChannel0, uv - (off3 / res)) * 0.010381362401148057;
	
	gl_FragColor = color;
}
