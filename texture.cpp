#include <cstdio>

#include "panic.h"
#include "pixmap.h"
#include "texture.h"

static int
next_power_of_2(int n)
{
	int p = 1;

	while (p < n)
		p *= 2;

	return p;
}

texture::texture(pixmap_ptr orig_pixmap)
: orig_width_(orig_pixmap->get_width())
, orig_height_(orig_pixmap->get_height())
, pixmap_(orig_pixmap->resize(next_power_of_2(orig_width_), next_power_of_2(orig_height_)))
, texture_id_(0)
{
	glGenTextures(1, &texture_id_);
	load();
}

texture::~texture()
{
	glDeleteTextures(1, &texture_id_);
}

void
texture::bind() const
{
	glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void
texture::set_wrap_s(GLint wrap) const
{
	set_parameter(GL_TEXTURE_WRAP_S, wrap);
}

void
texture::set_wrap_t(GLint wrap) const
{
	set_parameter(GL_TEXTURE_WRAP_T, wrap);
}

void
texture::set_mag_filter(GLint filter) const
{
	set_parameter(GL_TEXTURE_MAG_FILTER, filter);
}

void
texture::set_min_filter(GLint filter) const
{
	set_parameter(GL_TEXTURE_MIN_FILTER, filter);
}

void
texture::set_parameter(GLenum name, GLint param) const
{
	bind();
	glTexParameteri(GL_TEXTURE_2D, name, param);
}

void
texture::set_env_mode(GLint mode)
{
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
}

void
texture::load()
{
	bind();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	GLint format = 0;

	switch (pixmap_->get_type()) {
		case pixmap::GRAY:
			format = GL_LUMINANCE;
			break;

		case pixmap::GRAY_ALPHA:
			format = GL_LUMINANCE_ALPHA;
			break;

		case pixmap::RGB:
			format = GL_RGB;
			break;

		case pixmap::RGB_ALPHA:
			format = GL_RGBA;
			break;

		default:
			panic("invalid pixmap format");
			break;
	}


	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		format,
		pixmap_->get_width(),
		pixmap_->get_height(),
		0,
		format,
		GL_UNSIGNED_BYTE,
		pixmap_->get_bits());
}
