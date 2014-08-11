#pragma once

#include <cstdint>
#include <memory>

class pixmap;
using pixmap_ptr = std::shared_ptr<pixmap>;

class pixmap {
public:
	enum type { GRAY, GRAY_ALPHA, RGB, RGB_ALPHA, INVALID };

	pixmap(int width, int height, type pixmap_type);
	~pixmap();

	static pixmap_ptr load_from_png(const char *path);

	int get_width() const
	{ return width_; }

	int get_height() const
	{ return height_; }

	const uint8_t *get_bits() const
	{ return bits_; }

	uint8_t *get_bits()
	{ return bits_; }

	type get_type() const
	{ return type_; }

	size_t get_pixel_size() const;
	static size_t get_pixel_size(type pixmap_type);

	pixmap_ptr resize(int new_width, int new_height) const;

private:
	int width_;
	int height_;
	uint8_t *bits_;
	type type_;

	pixmap(const pixmap&) = delete;
	pixmap& operator=(const pixmap&) = delete;
};
