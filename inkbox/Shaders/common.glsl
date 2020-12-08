

vec4 bilerp(sampler2D field, vec2 pos, vec2 rdv)
{
    vec2 st = pos / rdv - 0.5;
    vec2 p = floor(st);
    vec2 frac = fract(st);

    vec4 a = texture2D(field, (p + vec2(0.5, 0.5)) * rdv);
    vec4 b = texture2D(field, (p + vec2(1.5, 0.5)) * rdv);
    vec4 c = texture2D(field, (p + vec2(0.5, 1.5)) * rdv);
    vec4 d = texture2D(field, (p + vec2(1.5, 1.5)) * rdv);

    return mix(mix(a, b, frac.x), mix(c, d, frac.x), frac.y);
}

float clipf(float value, float magnitude)
{
	if (abs(value) > magnitude)
	{
		return sign(value) * magnitude;
	}
	else
	{
		return value;
	}
}

vec2 clipvec2(vec2 v, vec2 mag)
{
	return v;
	return vec2(clipf(v.x, mag.x), clipf(v.y, mag.y));
}