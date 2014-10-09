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

template <typename VertexType>
struct draw_vertices;

template <>
struct draw_vertices<vertex_flat>
{
	static void draw(const vertex_flat *verts, int num_verts, GLenum mode)
	{
		glEnableClientState(GL_VERTEX_ARRAY);

		glVertexPointer(2, GL_FLOAT, sizeof(vertex_flat), verts->pos);

		glDrawArrays(mode, 0, num_verts);

		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

template <>
struct draw_vertices<vertex_texuv>
{
	static void draw(const vertex_texuv *verts, int num_verts, GLenum mode)
	{
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, sizeof(vertex_texuv), verts->pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(vertex_texuv), verts->texuv);

		glDrawArrays(mode, 0, num_verts);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
};

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
	{ draw_vertices<Vertex>::draw(&verts_[0], verts_.size(), mode); }

	size_t get_num_verts() const
	{ return verts_.size(); }

private:
  	vertex_array(const vertex_array&) = delete;
	vertex_array& operator=(const vertex_array&) = delete;

	std::vector<Vertex> verts_;
};

} // gge
