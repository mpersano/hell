#include <GL/glew.h>

#include <map>
#include <set>
#include <algorithm>

#include <cmath>
#include <cassert>

#include "vec2.h"
#include "vertex_array.h"
#include "debug_font.h"
#include "piece_pattern.h"
#include "texture.h"
#include "world.h"

static const float GRAVITY = 1.;
static const float DAMPING = .75;
static const float FRICTION = .6;

static const int SPAWN_INTERVAL = 30;

static const int BLOCK_SIZE = 20;

struct body
{
	body(const vec2& position)
	: position(position)
	, prev_position(position)
	{ }

	void update_position();

	vec2 position;
	vec2 prev_position;
};

using body_ptr = std::shared_ptr<body>;

struct spring
{
	spring(vec2& p0, vec2& p1)
	: p0(p0)
	, p1(p1)
	, rest_length((p0 - p1).length())
	{ }

	vec2 &p0, &p1;
	float rest_length;
};

struct quad
{
	quad(vec2& p0, const vec2& uv0, vec2& p1, const vec2& uv1, vec2& p2, const vec2& uv2, vec2& p3, const vec2& uv3)
	: p0(p0), uv0(uv0)
	, p1(p1), uv1(uv1)
	, p2(p2), uv2(uv2)
	, p3(p3), uv3(uv3)
	{ }

	vec2 &p0, uv0;
	vec2 &p1, uv1;
	vec2 &p2, uv2;
	vec2 &p3, uv3;
};

class quad_collision
{
public:
	quad_collision(quad& t0, quad& t1)
	: t0_(t0), t1_(t1)
	{ }

	bool operator()();

	const vec2& push_vector() const
	{ return push_vector_; }

private:
	template <bool First>
	bool separating_axis_test(const vec2& from, const vec2& to);

	quad &t0_, &t1_;
	vec2 push_vector_;
};

class piece;
using piece_ptr = std::shared_ptr<piece>;

class piece
{
public:
	piece(const piece& other);
	piece(const piece_pattern& pattern);

	void draw() const;

	void update_positions();
	void check_constraints(int width, int height);
	void collide(piece& other);

	void move(const vec2& p);

private:
	void update_bounding_box();

	rgb color_;
	std::shared_ptr<gge::texture> texture_;
	std::vector<body> bodies_;
	std::vector<spring> springs_;
	std::vector<quad> quads_;
	vec2 min_pos_, max_pos_;
};

class piece_factory
{
public:
	static piece_factory& get_instance();

	piece_ptr make_piece(int type) const;

	size_t get_num_types() const
	{ return pieces_.size(); }

private:
	piece_factory();

	std::vector<piece_ptr> pieces_;

	piece_factory(const piece_factory&) = delete;
	piece_factory& operator=(const piece_factory&) = delete;
};

class world_impl
{
public:
	world_impl(int width, int height);

	void draw() const;
	void update();

private:
	void draw_walls() const;

	std::vector<piece_ptr> pieces_;

	int spawn_tic_;
	int width_;
	int height_;
	gge::vertex_array<gge::vertex_flat> wall_va_;
	gge::debug_font font_;
};


//
//  b o d y
//

void
body::update_position()
{
	vec2 speed = DAMPING*(position - prev_position);
	prev_position = position;
	position += speed + vec2(0, -GRAVITY);
}

//
//  q u a d _ c o l l i s i o n
//

static std::pair<float, float>
project_quad_to_axis(const vec2& dir, const quad& t)
{
	float t0 = dir.dot(t.p0);
	float t1 = dir.dot(t.p1);
	float t2 = dir.dot(t.p2);
	float t3 = dir.dot(t.p3);

	float min = std::min(t0, std::min(t1, std::min(t2, t3)));
	float max = std::max(t0, std::max(t1, std::max(t2, t3)));

	return std::make_pair(min, max);
}

template <bool First>
bool
quad_collision::separating_axis_test(const vec2& from, const vec2& to)
{
	vec2 normal = vec2(-(to.y - from.y), to.x - from.x).normalize();

	std::pair<float, float> s0 = project_quad_to_axis(normal, t0_);
	std::pair<float, float> s1 = project_quad_to_axis(normal, t1_);

	static const float EPSILON = 1e-5;

	if (s0.second < s1.first + EPSILON || s1.second < s0.first + EPSILON)
		return true;

	// update push_vector_

	float push_length;

	if (s0.second - s1.first < s1.second - s0.first) {
		push_length = s0.second - s1.first;
	} else {
		normal = -normal;
		push_length = s1.second - s0.first;
	}

	push_length *= .5*FRICTION;

	if (First || push_length*push_length < push_vector_.length_squared())
		push_vector_ = normal*(push_length/normal.length());

	return false;
}

bool
quad_collision::operator()()
{
	if (separating_axis_test<true>(t0_.p0, t0_.p1))
		return false;

	if (separating_axis_test<false>(t0_.p1, t0_.p2))
		return false;

	if (separating_axis_test<false>(t0_.p2, t0_.p3))
		return false;

	if (separating_axis_test<false>(t0_.p3, t0_.p0))
		return false;

	if (separating_axis_test<false>(t1_.p0, t1_.p1))
		return false;

	if (separating_axis_test<false>(t1_.p1, t1_.p2))
		return false;

	if (separating_axis_test<false>(t1_.p2, t1_.p3))
		return false;

	if (separating_axis_test<false>(t1_.p3, t1_.p0))
		return false;

	t0_.p0 -= push_vector_;
	t0_.p1 -= push_vector_;
	t0_.p2 -= push_vector_;
	t0_.p3 -= push_vector_;

	t1_.p0 += push_vector_;
	t1_.p1 += push_vector_;
	t1_.p2 += push_vector_;
	t1_.p3 += push_vector_;

	return true;
}


//
//  p i e c e
//

piece::piece(const piece_pattern& pattern)
: color_(pattern.color)
, texture_(pattern.make_texture())
{
	texture_->set_wrap_s(GL_CLAMP);
	texture_->set_wrap_t(GL_CLAMP);

	texture_->set_mag_filter(GL_LINEAR);
	texture_->set_min_filter(GL_LINEAR);

	texture_->set_env_mode(GL_MODULATE);

	std::map<std::pair<int, int>, body *> body_map;
	std::set<std::pair<vec2 *, vec2 *>> spring_set;

	auto add_body = [&] (int i, int j) -> body * {
		auto it = body_map.find(std::make_pair(i, j));

		if (it == body_map.end()) {
			bodies_.push_back(body(vec2(j*BLOCK_SIZE, i*BLOCK_SIZE)));
			it = body_map.insert(it, std::make_pair(std::make_pair(i, j), &bodies_.back()));
		}

		return it->second;
	};

	auto add_spring = [&] (vec2& v0, vec2& v1) {
		if (spring_set.find(std::make_pair(&v0, &v1)) != spring_set.end())
			return;

		if (spring_set.find(std::make_pair(&v1, &v0)) != spring_set.end())
			return;

		springs_.push_back(spring(v0, v1));
		spring_set.insert(std::make_pair(&v0, &v1));
	};

	bodies_.reserve((MAX_PIECE_ROWS + 1)*(MAX_PIECE_COLS + 1));

	const float du = static_cast<float>(texture_->get_orig_width())/texture_->get_width()/MAX_PIECE_COLS;
	const float dv = static_cast<float>(texture_->get_orig_height())/texture_->get_height()/MAX_PIECE_ROWS;

	for (int i = 0; i < MAX_PIECE_ROWS; i++) {
		for (int j = 0; j < MAX_PIECE_COLS; j++) {
			if (pattern.pattern[i][j] != '#')
				continue;

			// bodies

			body *b0 = add_body(i, j);
			body *b1 = add_body(i, j + 1);
			body *b2 = add_body(i + 1, j + 1);
			body *b3 = add_body(i + 1, j);

			// springs

			vec2& v0 = b0->position;
			vec2& v1 = b1->position;
			vec2& v2 = b2->position;
			vec2& v3 = b3->position;

			add_spring(v0, v1);
			add_spring(v1, v2);
			add_spring(v2, v3);
			add_spring(v3, v0);

			add_spring(v0, v2);
			add_spring(v1, v3);

			// quads

			const float u = du*j;
			const float v = dv*i;

			quads_.push_back(quad(v0, vec2(u, v), v1, vec2(u + du, v), v2, vec2(u + du, v + dv), v3, vec2(u, v + dv)));
		}
	}

	update_bounding_box();

	printf("%u bodies, %u springs, %u quads\n", bodies_.size(), springs_.size(), quads_.size());
}

piece::piece(const piece& other)
: color_(other.color_)
, texture_(other.texture_)
, bodies_(other.bodies_)
, min_pos_(other.min_pos_)
, max_pos_(other.max_pos_)
{
	std::map<const vec2 *, size_t> pos_body;

	for (size_t i = 0; i < other.bodies_.size(); i++)
		pos_body[&other.bodies_[i].position] = i;

	for (auto& i : other.springs_) {
		vec2& p0 = bodies_[pos_body[&i.p0]].position;
		vec2& p1 = bodies_[pos_body[&i.p1]].position;
		springs_.push_back(spring(p0, p1));
	}

	for (auto& i : other.quads_) {
		vec2& p0 = bodies_[pos_body[&i.p0]].position;
		vec2& p1 = bodies_[pos_body[&i.p1]].position;
		vec2& p2 = bodies_[pos_body[&i.p2]].position;
		vec2& p3 = bodies_[pos_body[&i.p3]].position;
		quads_.push_back(quad(p0, i.uv0, p1, i.uv1, p2, i.uv2, p3, i.uv3));
	}
}

void
piece::update_positions()
{
	for (auto& i : bodies_)
		i.update_position();

	update_bounding_box();
}

void
piece::check_constraints(int width, int height)
{
	// springs

	for (auto& i : springs_) {
		vec2& p0 = i.p0;
		vec2& p1 = i.p1;

		vec2 dir = p1 - p0;

		float l = dir.length();
		float f = .5*(l - i.rest_length)/l;

		p0 += f*dir;
		p1 -= f*dir;
	}

	// body-wall collisions

	const float bowl_radius = .5*width;

	for (auto& i : bodies_) {
		vec2& p = i.position;

		if (p.y > bowl_radius) {
			if (p.x < 0)
				p.x += FRICTION*(-p.x);

			if (p.x > width)
				p.x += FRICTION*(width - p.x);

		} else {
			vec2 d = p - vec2(bowl_radius, bowl_radius);

			float r = d.length();

			if (r > bowl_radius)
				p -= d*(FRICTION*(r - bowl_radius)/r);
		}
	}

	update_bounding_box();
}

void
piece::move(const vec2& p)
{
	for (auto& i : bodies_) {
		i.position += p;
		i.prev_position = i.position;
	}
}

void
piece::collide(piece& other)
{
	// bounding box

	if (max_pos_.x < other.min_pos_.x)
		return;

	if (other.max_pos_.x < min_pos_.x)
		return;

	if (max_pos_.y < other.min_pos_.y)
		return;

	if (other.max_pos_.y < min_pos_.y)
		return;

	// quads

	bool collided = false;

	for (auto& q0 : quads_) {
		for (auto& q1 : other.quads_) {
			if (quad_collision(q0, q1)())
				collided = true;
		}
	}

	if (collided) {
		update_bounding_box();
		other.update_bounding_box();
	}
}

void
piece::draw() const
{
	glColor3f(color_.r, color_.g, color_.b);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_TEXTURE_2D);
	texture_->bind();

	gge::vertex_array<gge::vertex_texuv> va;

	for (auto& i : quads_) {
		va.add_vertex(i.p0.x, i.p0.y, i.uv0.x, i.uv0.y);
		va.add_vertex(i.p1.x, i.p1.y, i.uv1.x, i.uv1.y);
		va.add_vertex(i.p2.x, i.p2.y, i.uv2.x, i.uv2.y);
		va.add_vertex(i.p3.x, i.p3.y, i.uv3.x, i.uv3.y);
	}

	va.draw(GL_QUADS);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void
piece::update_bounding_box()
{
	assert(!bodies_.empty());

	min_pos_ = max_pos_ = bodies_[0].position;

	std::for_each(
		bodies_.begin() + 1,
		bodies_.end(),
		[this] (const body& b) {
			min_pos_.x = std::min(min_pos_.x, b.position.x);
			max_pos_.x = std::max(max_pos_.x, b.position.x);

			min_pos_.y = std::min(min_pos_.y, b.position.y);
			max_pos_.y = std::max(max_pos_.y, b.position.y);
		});
}

//
//  p i e c e _ f a c t o r y
//

piece_factory&
piece_factory::get_instance()
{
	static piece_factory instance;
	return instance;
}

piece_factory::piece_factory()
{
	static const piece_pattern patterns[]
		{
		{ { "    ",
		    " ## ",
		    " ## ",
		    "    "  },
		    rgb(0, 0, 1) },
	
		{ { " #  ",
		    " #  ",
		    " #  ",
		    " #  "  },
		    rgb(0, 1, 0) },
	
		{ { " #  ",
		    " #  ",
		    " ## ",
		    "    "  },
		    rgb(0, 1, 1) },
	
		{ { "  # ",
		    "  # ",
		    " ## ",
		    "    "  },
		    rgb(1, 0, 0) },
	
		{ { " #  ",
		    " ## ",
		    " #  ",
		    "    "  },
		    rgb(1, 0, 1) },
	
		{ { " #  ",
		    " ## ",
		    "  # ",
		    "    "  },
		    rgb(1, 1, 0) },
	
		{ { "  # ",
		    " ## ",
		    " #  ",
		    "    "  },
		    rgb(1, 1, 1) } };

	for (auto& i : patterns)
		pieces_.push_back(std::make_shared<piece>(i));
}

piece_ptr
piece_factory::make_piece(int type) const
{
	return std::make_shared<piece>(*pieces_[type]);
}

//
//  w o r l d
//

world_impl::world_impl(int width, int height)
: spawn_tic_(SPAWN_INTERVAL)
, width_(width)
, height_(height)
{
	wall_va_.add_vertex(0, height_);

	const float bowl_radius = .5*width_;

	const int NUM_SEGS = 20;
	
	float a = 0;
	const float da = M_PI/(NUM_SEGS - 1);

	for (int i = 0; i < NUM_SEGS; i++) {
		float x = bowl_radius*(1 - cosf(a));
		float y = bowl_radius*(1 - sinf(a));

		wall_va_.add_vertex(x, y);
		a += da;
	}

	wall_va_.add_vertex(width_, height_);
}

void
world_impl::draw() const
{
	draw_walls();

	for (auto& i : pieces_)
		i->draw();

	glColor3f(1, 1, 1);
	font_.draw_string_f(8, 8, "%d", pieces_.size());
}

void
world_impl::draw_walls() const
{
	glColor3f(1, 1, 1);
	wall_va_.draw(GL_LINE_LOOP);
}

void
world_impl::update()
{
	if (!pieces_.empty()) {
		for (auto& i : pieces_)
			i->update_positions();

		static const int NUM_ITERATIONS = 30;

		for (int i = 0; i < NUM_ITERATIONS; i++) {
			for (auto& i : pieces_)
				i->check_constraints(width_, height_);

			for (size_t j = 0; j < pieces_.size() - 1; j++)
				for (size_t k = j + 1; k < pieces_.size(); k++)
					pieces_[j]->collide(*pieces_[k]);
		}
	}

	if (!--spawn_tic_) {
		piece_factory& factory = piece_factory::get_instance();

		piece_ptr piece = factory.make_piece(rand()%factory.get_num_types());
		piece->move(vec2(rand()%(width_ - BLOCK_SIZE*MAX_PIECE_COLS), height_));
		pieces_.push_back(piece);

		spawn_tic_ = SPAWN_INTERVAL;
	}
}

world::world(int width, int height)
: impl_(new world_impl(width, height))
{ }

world::~world() = default;

void
world::draw() const
{
	impl_->draw();
}

void
world::update()
{
	impl_->update();
}
