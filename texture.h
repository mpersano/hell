#pragma once

#include <GL/glew.h>

#include "pixmap.h"

template <pixel_type PixelType>
struct pixel_type_to_format;

template <>
struct pixel_type_to_format<Gray>
{
	static const GLint format = GL_LUMINANCE;
};

template <>
struct pixel_type_to_format<GrayAlpha>
{
	static const GLint format = GL_LUMINANCE_ALPHA;
};

template <>
struct pixel_type_to_format<RGB>
{
	static const GLint format = GL_RGB;
};

template <>
struct pixel_type_to_format<RGBAlpha>
{
	static const GLint format = GL_RGBA;
};

namespace {

template <typename T>
static T
next_power_of_2(T n)
{
	T p = 1;

	while (p < n)
		p *= 2;

	return p;
}

}

class texture
{
public:
	texture()
	: orig_width_(0)
	, width_(0)
	, orig_height_(0)
	, height_(0)
	{ glGenTextures(1, &name_); }

	~texture()
	{ glDeleteTextures(1, &name_); }

	void bind() const
	{ glBindTexture(GL_TEXTURE_2D, name_); }

	void set_wrap_s(GLint wrap) const
	{ set_parameter(GL_TEXTURE_WRAP_S, wrap); }

	void set_wrap_t(GLint wrap) const
	{ set_parameter(GL_TEXTURE_WRAP_T, wrap); }

	void set_mag_filter(GLint filter) const
	{ set_parameter(GL_TEXTURE_MAG_FILTER, filter); }

	void set_min_filter(GLint filter) const
	{ set_parameter(GL_TEXTURE_MIN_FILTER, filter); }

	void set_parameter(GLenum name, GLint value) const
	{ bind(); glTexParameteri(GL_TEXTURE_2D, name, value); }

	static void set_env_mode(GLint mode)
	{ glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode); }

	template <pixel_type PixelType>
	void load(const pixmap<PixelType>& pm)
	{
		orig_width_ = pm.width;
		width_ = next_power_of_2(orig_width_);

		orig_height_ = pm.height;
		height_ = next_power_of_2(orig_height_);

		bind();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		static const GLint format = pixel_type_to_format<PixelType>::format;

		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			format,
			width_, height_,
			0,
			format,
			GL_UNSIGNED_BYTE,
			&pm.resize(width_, height_).data[0]);
	}

	size_t get_width() const
	{ return width_; }

	size_t get_orig_width() const
	{ return orig_width_; }

	size_t get_height() const
	{ return height_; }

	size_t get_orig_height() const
	{ return orig_height_; }

private:
	texture(const texture&) = delete;
	texture& operator=(const texture&) = delete;

	size_t orig_width_, width_;
	size_t orig_height_, height_;
	GLuint name_;
};
