#pragma once

#include "texture.h"

namespace gge {

class debug_font
{
public:
	debug_font();

	void draw_string(float x, float y, const char *str) const;
	void draw_string_f(float x, float y, const char *fmt, ...) const;

private:
	texture texture_;
};

} // gge
