#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <GL/glew.h>

#include <memory>

class pixmap;
using pixmap_ptr = std::shared_ptr<pixmap>;

class texture {
public:
	texture(pixmap_ptr pixmap);
	~texture();

	int get_orig_width() const
	{ return orig_width_; }

	int get_orig_height() const
	{ return orig_height_; }

	int get_width() const
	{ return pixmap_->get_width(); }

	int get_height() const
	{ return pixmap_->get_height(); }

	void bind() const;

	// GL_REPEAT, GL_CLAMP
	void set_wrap_s(GLint wrap) const;
	void set_wrap_t(GLint wrap) const;

	// GL_NEAREST, GL_LINEAR
	void set_mag_filter(GLint filter) const;
	void set_min_filter(GLint filter) const;

	// XXX: affects the texture unit, no matter which texture is
	// bound, thus static
	// GL_MODULATE, ...
	static void set_env_mode(GLint mode);

private:
	void set_parameter(GLenum name, GLint param) const;
	void load();

	int orig_width_, orig_height_;
	pixmap_ptr pixmap_;

	GLuint texture_id_;

	texture(const texture&) = delete;
	texture& operator=(const texture&) = delete;
};

#endif // TEXTURE_H_
