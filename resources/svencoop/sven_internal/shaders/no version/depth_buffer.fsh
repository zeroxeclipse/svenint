uniform sampler2D iChannel0; // normal texture
// uniform sampler2D depthmap; // depth buffer
uniform float znear;
uniform float zfar;
uniform float factor;
uniform vec2 res;

float LinearizeDepth(in vec2 uv)
{
    // float znear = 0.5;    // TODO: Replace by the zNear of your perspective projection
    // float zfar  = 100.0; // TODO: Replace by the zFar  of your perspective projection
    float depth = texture2D(iChannel0, uv).x; // .x
	depth = 2.0 * depth - 1.0;
    return (2.0 * znear) / (zfar + znear - depth * (zfar - znear));
}

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
    float depth = (1.0 - LinearizeDepth(uv)) * factor;
	
    gl_FragColor = vec4(vec3(depth), 1.0);
	
    // gl_FragColor = vec4(vec3(gl_FragCoord.z), 1.0);
    // gl_FragColor = vec4(vec3(1.0 - texture2D(iChannel0, uv).z), 1.0);
	
	// vec4 color = texture2D(iChannel0, uv);
    // gl_FragColor = vec4(color.x, color.y, color.z, 1.0);
}