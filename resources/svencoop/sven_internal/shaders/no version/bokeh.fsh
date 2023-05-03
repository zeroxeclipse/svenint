uniform sampler2D iChannel0;
uniform float bokeh;
uniform float samples;
uniform vec2 dir;
uniform vec2 res;

void main()
{
	vec2 uv = vec2(gl_FragCoord.xy / res.xy);
	
	vec4 sum = vec4(0.0); //результирующий цвет
	vec4 msum = vec4(0.0); //максимальное значение цвета выборок

	float delta = 1.0 / samples; //порция цвета в одной выборке
	float di = 1.0 / ( samples - 1.0 ); //вычисляем инкремент
	
	for (float i = -0.5; i < 0.501; i += di)
	{
		vec4 color = texture2D(iChannel0, uv + (dir * i) / res); //делаем выборку в заданном направлении
		sum += color * delta; //суммируем цвет
		msum = max(color, msum); //вычисляем максимальное значение цвета
	}

	gl_FragColor = mix(sum, msum, bokeh); //смешиваем результирующий цвет с максимальным в заданной пропорции
}