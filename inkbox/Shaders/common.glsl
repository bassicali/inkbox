
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