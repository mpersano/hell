#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>

#include <algorithm>

#include <png.h>

#include "pixmap.h"
#include "panic.h"

#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

pixmap_ptr
pixmap::load_from_png(const char *path)
{
	png_structp png_ptr;
	if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0)) == 0)
		panic("png_create_read_struct failed?");

	png_infop info_ptr;
	if ((info_ptr = png_create_info_struct(png_ptr)) == 0)
		panic("png_create_info_struct failed?");

	if (setjmp(png_jmpbuf(png_ptr)))
		panic("some kind of png error?");

	FILE *in = fopen(path, "rb");
	if (!in)
		panic("failed to open %s: %s\n", path, strerror(errno));

	png_init_io(png_ptr, in);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);

	fclose(in);

	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	if (bit_depth != 8)
		panic("invalid bit depth in PNG");

	type pixmap_type = INVALID;

	switch (color_type) {
		case PNG_COLOR_TYPE_GRAY:
			pixmap_type = GRAY;
			break;

		case PNG_COLOR_TYPE_GRAY_ALPHA:
			pixmap_type = GRAY_ALPHA;
			break;

		case PNG_COLOR_TYPE_RGB:
			pixmap_type = RGB;
			break;

		case PNG_COLOR_TYPE_RGBA:
			pixmap_type = RGB_ALPHA;
			break;

		default:
			panic("invalid color type in PNG");
	}

	const int width = png_get_image_width(png_ptr, info_ptr);
	const int height = png_get_image_height(png_ptr, info_ptr);

	pixmap_ptr pm = std::make_shared<pixmap>(width, height, pixmap_type);

	png_bytep *rows = png_get_rows(png_ptr, info_ptr);

	const size_t stride = width*get_pixel_size(pixmap_type);

	for (int i = 0; i < height; i++)
		memcpy(&pm->bits_[i*stride], rows[i], stride);

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);

	return pm;
}

pixmap::pixmap(int width, int height, type pixmap_type)
: width_(width)
, height_(height)
, bits_(new uint8_t[width*height*get_pixel_size(pixmap_type)])
, type_(pixmap_type)
{
	memset(bits_, 0, width*height*get_pixel_size());
}

pixmap::~pixmap()
{
	delete[] bits_;
}

pixmap_ptr
pixmap::resize(int new_width, int new_height) const
{
	pixmap_ptr new_pm = std::make_shared<pixmap>(new_width, new_height, type_);

	size_t pixel_size = get_pixel_size();

	const int copy_height = std::min(height_, new_height);
	const int copy_width = std::min(width_, new_width);

	for (int i = 0; i < copy_height; i++) {
		uint8_t *dest = &new_pm->bits_[i*new_width*pixel_size];
		uint8_t *src = &bits_[i*width_*pixel_size];
		::memcpy(dest, src, copy_width*pixel_size);
	}

	return new_pm;
}

size_t
pixmap::get_pixel_size() const
{
	return get_pixel_size(type_);
}

size_t
pixmap::get_pixel_size(type pixmap_type)
{
	switch (pixmap_type) {
		case GRAY:
			return 1;

		case GRAY_ALPHA:
			return 2;

		case RGB:
			return 3;

		case RGB_ALPHA:
			return 4;

		default:
			assert(0);
			return 0;
	}
}
