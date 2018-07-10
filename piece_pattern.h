#pragma once

#include <memory>

namespace gge {
class texture;
}

enum
{
	MAX_PIECE_ROWS = 4,
	MAX_PIECE_COLS = 4,
};

struct rgb
{
	float r, g, b;
};

struct piece_pattern
{
	std::shared_ptr<gge::texture> make_texture() const;

	char pattern[MAX_PIECE_ROWS][MAX_PIECE_COLS + 1]; // 'x' marks the spot
	rgb color;
};
