// https://github.com/Erkaman/glsl-godrays

uniform sampler2D iChannel0;
uniform float density;
uniform float weight;
uniform float decay;
uniform float exposure;
uniform int numSamples;
uniform vec2 lightpos;
uniform vec2 res;

/*

Default:
1 0.01 1 1 100

Good:
0.97 0.25 0.974 0.24 64

Ranges:
density		0 - 2.0
weight		0 - 0.1
decay		0.95 - 1.05
exposure	0 - 2.0
numSamples	0 - 100

*/

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);

	vec3 fragColor = vec3(0.0,0.0,0.0);

	vec2 deltaTextCoord = vec2( uv - lightpos.xy );

	vec2 textCoord = uv.xy;
	deltaTextCoord *= (1.0 / float(numSamples)) * density;
	
	float illuminationDecay = 1.0;

	for (int i = 0; i < numSamples ; i++)
	{
		textCoord -= deltaTextCoord;
		vec3 sample = texture2D(iChannel0, textCoord).xyz;
		sample *= illuminationDecay * weight;
		fragColor += sample;
		illuminationDecay *= decay;
	}

	fragColor *= exposure;

	gl_FragColor = vec4(fragColor, 1.0);
}

/* https://www.shadertoy.com/view/4dyXWy

define DITHER			//Dithering toggle
#define QUALITY		0	//0- low, 1- medium, 2- high

#define DECAY		.974
#define EXPOSURE	.24
#if (QUALITY==2)
 #define SAMPLES	64
 #define DENSITY	.97
 #define WEIGHT		.25
#else
#if (QUALITY==1)
 #define SAMPLES	32
 #define DENSITY	.95
 #define WEIGHT		.25
#else
 #define SAMPLES	16
 #define DENSITY	.93
 #define WEIGHT		.36
#endif
#endif

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    
    vec2 coord = uv;
    vec2 lightpos = texture2D(iChannel0, uv).zw;
   	
    float occ = texture2D(iChannel0, uv).x; //light
    float obj = texture2D(iChannel0, uv).y; //objects
    float dither = texture2D(iChannel1, fragCoord/iChannelResolution[1].xy).r;    
        
    vec2 dtc = (coord - lightpos) * (1. / float(SAMPLES) * DENSITY);
    float illumdecay = 1.;
    
    for(int i=0; i<SAMPLES; i++)
    {
        coord -= dtc;
        #ifdef DITHER
        	float s = texture2D(iChannel0, coord+(dtc*dither)).x;
        #else
        	float s = texture2D(iChannel0, coord).x;
        #endif
        s *= illumdecay * WEIGHT;
        occ += s;
        illumdecay *= DECAY;
    }
        
	fragColor = vec4(vec3(0., 0., obj*.333)+occ*EXPOSURE,1.0);
}

*/