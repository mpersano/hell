#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <vector>
#include <algorithm>

namespace gge {

enum pixel_type { PIXEL_GRAY, PIXEL_GRAY_ALPHA, PIXEL_RGB, PIXEL_RGB_ALPHA };

template <pixel_type PixelType>
struct pixel_type_to_size;

template <>
struct pixel_type_to_size<PIXEL_GRAY>
{
	static const size_t size = 1;
};

template <>
struct pixel_type_to_size<PIXEL_GRAY_ALPHA>
{
	static const size_t size = 2;
};

template <>
struct pixel_type_to_size<PIXEL_RGB>
{
	static const size_t size = 3;
};

template <>
struct pixel_type_to_size<PIXEL_RGB_ALPHA>
{
	static const size_t size = 4;
};

template <pixel_type PixelType>
struct pixmap
{
	pixmap(size_t width, size_t height)
	: width(width)
	, height(height)
	, data(width*height*pixel_size)
	{ }

	pixmap resize(size_t new_width, size_t new_height) const
	{
		pixmap new_pixmap(new_width, new_height);

		const size_t copy_height = std::min(height, new_height);
		const size_t copy_width = std::min(width, new_width);

		for (size_t i = 0; i < copy_height; i++) {
			uint8_t *dest = &new_pixmap.data[i*new_width*pixel_size];
			const uint8_t *src = &data[i*width*pixel_size];
			::memcpy(dest, src, copy_width*pixel_size);
		}

		return new_pixmap;
	}

	static const size_t pixel_size = pixel_type_to_size<PixelType>::size;

	size_t width;
	size_t height;
	std::vector<uint8_t> data;
};

} // gge
