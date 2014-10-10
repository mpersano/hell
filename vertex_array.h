#pragma once

#include <GL/glew.h>

#include <vector>

namespace gge {

struct vertex_flat
{
	vertex_flat(float x, float y)
	: pos { x, y }
	{ }

	GLfloat pos[2];
};

struct vertex_texuv
{
	vertex_texuv(float x, float y, float u, float v)
	: pos { x, y }
	, texuv { u, v }
	{ }

	GLfloat pos[2];
	GLfloat texuv[2];
};

namespace detail {

// RAII <3 <3 <3

template <typename VertexType>
struct client_state;

template <>
struct client_state<vertex_flat>
{
	client_state(const vertex_flat *verts)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof *verts, verts->pos);
	}

	~client_state()
	{
		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

template <>
struct client_state<vertex_texuv>
{
	client_state(const vertex_texuv *verts)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, sizeof *verts, verts->pos);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof *verts, verts->texuv);
	}

	~client_state()
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

} // detail

template <class Vertex>
class vertex_array
{
public:
	vertex_array() = default;

	vertex_array(int capacity)
	{ verts_.reserve(capacity); }

	void clear()
	{ verts_.clear(); }

	void add_vertex(float x, float y)
	{ verts_.push_back(Vertex(x, y)); }

	void add_vertex(float x, float y, float u, float v)
	{ verts_.push_back(Vertex(x, y, u, v)); }

	void draw(GLenum mode) const
	{
		detail::client_state<Vertex> state(&verts_[0]);
		glDrawArrays(mode, 0, verts_.size());
	}

	size_t get_num_verts() const
	{ return verts_.size(); }

private:
  	vertex_array(const vertex_array&) = delete;
	vertex_array& operator=(const vertex_array&) = delete;

	std::vector<Vertex> verts_;
};

using vertex_array_flat = vertex_array<vertex_flat>;
using vertex_array_texuv = vertex_array<vertex_texuv>;

} // gge
