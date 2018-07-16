#include <cmath>

#include "texture.h"
#include "pixmap.h"
#include "piece_pattern.h"

namespace {
constexpr int BLOCK_SIZE = 32;

constexpr int CORNER_RADIUS = 8;
constexpr int INNER_BORDER = 6;
constexpr int INNER_CORNER_RADIUS = 4;
constexpr int INNER_INNER_BORDER = 2;

struct rect
{
	int x, y, w, h;
};

struct veci2
{
	veci2 operator-(const veci2& v) const
	{ return {x - v.x, y - v.y}; }

	float length() const
	{ return sqrtf(x*x + y*y); }

	int x, y;
};

float
border_color(const veci2& p, const veci2& o, int radius)
{
	float l = (p - o).length();

	if (l < radius)
		return 1;
	else if (l > radius + 1)
		return 0;
	else
		return 1. - (l - radius);
}

float
round_rect_color(const veci2& p, const rect& rc, int corner_radius)
{
	const int x00 = rc.x;
	const int x01 = rc.x + corner_radius;

	const int x10 = rc.x + rc.w - corner_radius - 1;
	const int x11 = rc.x + rc.w - 1;

	const int y00 = rc.y;
	const int y01 = rc.y + corner_radius;

	const int y10 = rc.y + rc.h - corner_radius - 1;
	const int y11 = rc.y + rc.h - 1;

	const int x = p.x;
	const int y = p.y;

	if (x < x00 || x > x11 || y < y00 || y > y11)
		return 0;
	else if (x < x01 && y < y01)
		return border_color(p, {x01, y01}, corner_radius);
	else if (x < x01 && y > y10)
		return border_color(p, {x01, y10}, corner_radius);
	else if (x > x10 && y < y01)
		return border_color(p, {x10, y01}, corner_radius);
	else if (x > x10 && y > y10)
		return border_color(p, {x10, y10}, corner_radius);
	else
		return 1;

}

void
draw_block(uint8_t *pixels, int stride, bool up, bool down, bool left, bool right)
{
	rect outer{0, 0, BLOCK_SIZE, BLOCK_SIZE};
	rect inner{INNER_BORDER, INNER_BORDER, BLOCK_SIZE - 2*INNER_BORDER, BLOCK_SIZE - 2*INNER_BORDER};

	const float s0 = 1., s1 = .8;

	for (int i = 0; i < BLOCK_SIZE; i++) {
		for (int j = 0; j < BLOCK_SIZE; j++) {
			const veci2 p{i, j};

			float t0 = round_rect_color(p, outer, CORNER_RADIUS);

			const int h0 = INNER_BORDER;
			const int h1 = BLOCK_SIZE - INNER_BORDER - 1;

			const int h2 = INNER_BORDER + INNER_CORNER_RADIUS + INNER_INNER_BORDER;
			const int h3 = BLOCK_SIZE - INNER_BORDER - INNER_CORNER_RADIUS - INNER_INNER_BORDER - 1;

			float t1;

			if (i < h0)
				t1 = left && j >= h2 && j <= h3;
			else if (i > h1)
				t1 = right && j >= h2 && j <= h3;
			else if (j < h0)
				t1 = up && i >= h2 && i <= h3;
			else if (j > h1)
				t1 = down && i >= h2 && i <= h3;
			else
				t1 = round_rect_color(p, inner, INNER_CORNER_RADIUS);

			*pixels++ = 255*t0*(s0 + t1*(s1 - s0));
		}

		pixels += stride - BLOCK_SIZE;
	}
}
}

std::shared_ptr<gge::texture>
make_piece_texture(const piece_pattern& p)
{
	const int width = MAX_PIECE_COLS*BLOCK_SIZE;
	const int height = MAX_PIECE_ROWS*BLOCK_SIZE;

	gge::pixmap<gge::pixel_type::GRAY> pm(width, height);

	uint8_t *bits = &pm.data[0];

	for (int r = 0; r < MAX_PIECE_ROWS; r++) {
		for (int c = 0; c < MAX_PIECE_COLS; c++) {
			if (p.pattern[r][c] != '#')
				continue;

			bool left = c > 0 && p.pattern[r][c - 1] == '#';
			bool right = c < MAX_PIECE_COLS - 1 && p.pattern[r][c + 1] == '#';
			bool up = r > 0 && p.pattern[r - 1][c] == '#';
			bool down = r < MAX_PIECE_ROWS - 1 && p.pattern[r + 1][c] == '#';

			draw_block(&bits[BLOCK_SIZE*(r*width + c)], width, left, right, up, down);
		}
	}

	auto tex = std::make_shared<gge::texture>();
	tex->load(pm);
	return tex;
}
