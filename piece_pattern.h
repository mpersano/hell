#pragma once

#include "pixmap.h"

enum
{
	MAX_PIECE_ROWS = 4,
	MAX_PIECE_COLS = 4,
};

struct rgb
{
	rgb(float r, float g, float b)
	: r(r), g(g), b(b)
	{ }

	float r, g, b;
};

struct piece_pattern
{
	pixmap_ptr make_pixmap() const;

	char pattern[MAX_PIECE_ROWS][MAX_PIECE_COLS + 1]; // 'x' marks the spot
	rgb color;
};
