#include <GL/glew.h>

#include <map>
#include <set>
#include <algorithm>

#include <cmath>
#include <cassert>

#include "vec2.h"
#include "world.h"

static const float GRAVITY = 1.;
static const float DAMPING = .75;
static const float FRICTION = .6;

static const int SPAWN_INTERVAL = 30;

static const int BLOCK_SIZE = 20;

struct rgb
{
	rgb(float r, float g, float b)
	: r(r), g(g), b(b)
	{ }

	float r, g, b;
};

enum {
	MAX_PIECE_ROWS = 4,
	MAX_PIECE_COLS = 4,
};

static const struct piece_pattern {
	char pattern[MAX_PIECE_ROWS][MAX_PIECE_COLS + 1];
	rgb color;
} piece_patterns[] = {
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
	    rgb(1, 1, 1) },
};

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
	quad(vec2& p0, vec2& p1, vec2& p2, vec2& p3)
	: p0(p0)
	, p1(p1)
	, p2(p2)
	, p3(p3)
	{ }

	void draw() const;

	quad& operator+=(const vec2& v)
	{
		p0 += v;
		p1 += v;
		p2 += v;
		p3 += v;
		return *this;
	}

	quad& operator-=(const vec2& v)
	{
		p0 -= v;
		p1 -= v;
		p2 -= v;
		p3 -= v;
		return *this;
	}

	vec2 &p0, &p1, &p2, &p3;
};

class quad_intersection
{
public:
	quad_intersection(const quad& t0, const quad& t1)
	: t0_(t0), t1_(t1)
	{ }

	bool operator()();

	const vec2& push_vector() const
	{ return push_vector_; }

private:
	template <bool First>
	bool separating_axis_test(const vec2& from, const vec2& to);

	const quad &t0_, &t1_;
	vec2 push_vector_;
};

class piece
{
public:
	piece(const piece_pattern& pattern, float x_origin, float y_origin);

	void draw() const;

	void update_positions();
	void check_constraints(int width, int height);
	void collide(piece& other);

private:
	void update_bounding_box();

	rgb color_;
	std::vector<body> bodies_;
	std::vector<spring> springs_;
	std::vector<quad> quads_;
	vec2 min_pos_, max_pos_;
};

typedef std::shared_ptr<piece> piece_ptr;

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
//  q u a d
//

void
quad::draw() const
{
	glBegin(GL_QUADS);
	glVertex2f(p0.x, p0.y);
	glVertex2f(p1.x, p1.y);
	glVertex2f(p2.x, p2.y);
	glVertex2f(p3.x, p3.y);
	glEnd();
}


//
//  q u a d _ i n t e r s e c t i o n
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
quad_intersection::separating_axis_test(const vec2& from, const vec2& to)
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
quad_intersection::operator()()
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

	return true;
}


//
//  p i e c e
//

piece::piece(const piece_pattern& pattern, float x_origin, float y_origin)
: color_(pattern.color)
{
	std::map<std::pair<int, int>, size_t> body_map;
	std::set<std::pair<size_t, size_t>> spring_set;

	auto add_body = [&] (int i, int j) -> size_t {
		auto it = body_map.find(std::make_pair(i, j));

		if (it == body_map.end()) {
			bodies_.push_back(vec2(x_origin + j*BLOCK_SIZE, y_origin + i*BLOCK_SIZE));
			it = body_map.insert(it, std::make_pair(std::make_pair(i, j), bodies_.size() - 1));
		}

		return it->second;
	};

	auto add_spring = [&] (size_t b0, size_t b1) {
		if (spring_set.find(std::make_pair(b0, b1)) != spring_set.end())
			return;

		if (spring_set.find(std::make_pair(b1, b0)) != spring_set.end())
			return;

		springs_.push_back(spring(bodies_[b0].position, bodies_[b1].position));
		spring_set.insert(std::make_pair(b0, b1));
	};

	bodies_.reserve(32); // ;_;

	for (int i = 0; i < MAX_PIECE_ROWS; i++) {
		for (int j = 0; j < MAX_PIECE_COLS; j++) {
			if (pattern.pattern[i][j] == '#') {
				// bodies

				size_t b0 = add_body(i, j);
				size_t b1 = add_body(i, j + 1);
				size_t b2 = add_body(i + 1, j + 1);
				size_t b3 = add_body(i + 1, j);

				// springs

				add_spring(b0, b1);
				add_spring(b1, b2);
				add_spring(b2, b3);
				add_spring(b3, b0);

				add_spring(b0, b2);
				add_spring(b1, b3);

				// quads

				vec2& v0 = bodies_[b0].position;
				vec2& v1 = bodies_[b1].position;
				vec2& v2 = bodies_[b2].position;
				vec2& v3 = bodies_[b3].position;

				quads_.push_back(quad(v0, v1, v2, v3));
			}
		}
	}

	update_bounding_box();

	printf("%u bodies, %u springs, %u quads\n", bodies_.size(), springs_.size(), quads_.size());
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
			quad_intersection intersection(q0, q1);

			if (intersection()) {
				q0 -= intersection.push_vector();
				q1 += intersection.push_vector();
				collided = true;
			}
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

	for (auto& i : quads_)
		i.draw();
}

void
piece::update_bounding_box()
{
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
//  w o r l d
//

world_impl::world_impl(int width, int height)
: spawn_tic_(SPAWN_INTERVAL)
, width_(width)
, height_(height)
{ }

void
world_impl::draw() const
{
	draw_walls();

	for (auto& i : pieces_)
		i->draw();
}

void
world_impl::draw_walls() const
{
	glColor3f(1, 1, 1);

	glBegin(GL_LINE_LOOP);

	glVertex2f(0, height_);

	const float bowl_radius = .5*width_;

	const int NUM_SEGS = 20;
	
	float a = 0;
	const float da = M_PI/(NUM_SEGS - 1);

	for (int i = 0; i < NUM_SEGS; i++) {
		float x = bowl_radius*(1 - cosf(a));
		float y = bowl_radius*(1 - sinf(a));

		glVertex2f(x, y);
		a += da;
	}

	glVertex2f(width_, height_);

	glEnd();
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
		const float x = rand()%(width_ - BLOCK_SIZE*4);
		const float y = height_;
		const int type = rand()%(sizeof piece_patterns/sizeof *piece_patterns);

		pieces_.push_back(std::make_shared<piece>(piece_patterns[type], x, y));
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
