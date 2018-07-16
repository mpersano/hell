#pragma once

#include <memory>

namespace gge {
class texture;
}

constexpr auto MAX_PIECE_ROWS = 4;
constexpr auto MAX_PIECE_COLS = 4;

struct rgb
{
	float r, g, b;
};

struct piece_pattern
{
	char pattern[MAX_PIECE_ROWS][MAX_PIECE_COLS + 1]; // 'x' marks the spot
	rgb color;
};

std::shared_ptr<gge::texture> make_piece_texture(const piece_pattern& p);
