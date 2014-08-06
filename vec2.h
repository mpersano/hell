#pragma once

struct vec2
{
	vec2(float x = 0, float y = 0)
	: x(x), y(y)
	{ }

	vec2 operator*(float s) const
	{ return vec2(x*s, y*s); }

	vec2& operator*=(float s)
	{ x *= s; y *= s; return *this; }

	vec2 operator+(const vec2& v) const
	{ return vec2(x + v.x, y + v.y); }

	vec2& operator+=(const vec2& v)
	{ x += v.x; y += v.y; return *this; }

	vec2 operator-(const vec2& v) const
	{ return vec2(x - v.x, y - v.y); }

	vec2 operator-() const
	{ return vec2(-x, -y); }

	vec2& operator-=(const vec2& v)
	{ x -= v.x; y -= v.y; return *this; }

	float dot(const vec2& v) const
	{ return x*v.x + y*v.y; }

	float length_squared() const
	{ return x*x + y*y; }

	float length() const
	{ return sqrtf(length_squared()); }

	vec2& normalize()
	{
		const float EPSILON = 1e-5;
		float l = length();
		if (fabs(l) > EPSILON)
			*this *= 1./l;
		return *this;
	}

	vec2& set_length(float l)
	{ *this *= l/length(); return *this; }

	float distance(const vec2& v) const
	{ 
		vec2 d = v - *this;
		return d.length();
	}

	float x, y;
};

inline vec2
operator*(float s, const vec2& v)
{
	return vec2(s*v.x, s*v.y);
}
