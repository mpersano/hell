#include <SDL.h>
#include <GL/glew.h>

#include <memory>
#include <map>
#include <set>
#include <algorithm>

#include <cmath>
#include <cassert>

#include "panic.h"
#include "vec2.h"

static bool running = false;

static const int WINDOW_WIDTH = 320;
static const int WINDOW_HEIGHT = 240;

static const float BODY_RADIUS = 3;

static const float GRAVITY = 1.;
static const float DAMPING = .75;
static const float FRICTION = .6;

static const int SPAWN_INTERVAL = 30;

static const int BORDER = 8;

static const float left_wall_x_ = BORDER;
static const float right_wall_x_ = WINDOW_WIDTH - BORDER;
static const float top_wall_y_ = BORDER;
static const float bottom_wall_y_ = WINDOW_HEIGHT - BORDER;

enum {
	MAX_PIECE_ROWS = 4,
	MAX_PIECE_COLS = 4,
};

struct rgb
{
	rgb(float r, float g, float b)
	: r(r), g(g), b(b)
	{ }

	float r, g, b;
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
	void draw() const;

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

	void draw() const;

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
	void check_constraints();
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

class world
{
public:
	world();

	void draw() const;
	void update();

private:
	void draw_walls() const;

	std::vector<piece_ptr> pieces_;

	int spawn_tic_;
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

void
body::draw() const
{
	static const int NUM_SIDES = 20;

	float a = 0;
	const float da = 2.*M_PI/NUM_SIDES;

	glColor3f(1, 1, 1);

	glBegin(GL_LINE_LOOP);

	for (int i = 0; i < NUM_SIDES; i++) {
		glVertex2f(position.x + sinf(a)*BODY_RADIUS, position.y + cosf(a)*BODY_RADIUS);
		a += da;
	}

	glEnd();
}


//
//  s p r i n g
//

void
spring::draw() const
{
	glColor3f(1, 1, 1);

	glBegin(GL_LINES);
	glVertex2f(p0.x, p0.y);
	glVertex2f(p1.x, p1.y);
	glEnd();
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
			static const float SPACING = 12;

			bodies_.push_back(vec2(x_origin + j*SPACING, y_origin + i*SPACING));
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
piece::check_constraints()
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

	for (auto& i : bodies_) {
		vec2& p = i.position;

		if (p.y < top_wall_y_)
			p.y += FRICTION*(top_wall_y_ - p.y);

#if 0
		if (p.y > bottom_wall_y_)
			p.y += FRICTION*(bottom_wall_y_ - p.y);
#endif

		if (p.x < left_wall_x_)
			p.x += FRICTION*(left_wall_x_ - p.x);

		if (p.x > right_wall_x_)
			p.x += FRICTION*(right_wall_x_ - p.x);
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

world::world()
: spawn_tic_(SPAWN_INTERVAL)
{ }

void
world::draw() const
{
	draw_walls();

	for (auto& i : pieces_)
		i->draw();
}

void
world::draw_walls() const
{
	glColor3f(1, 1, 1);

	glBegin(GL_LINE_LOOP);
	glVertex2f(left_wall_x_, top_wall_y_);
	glVertex2f(right_wall_x_, top_wall_y_);
	glVertex2f(right_wall_x_, bottom_wall_y_);
	glVertex2f(left_wall_x_, bottom_wall_y_);
	glEnd();
}

void
world::update()
{
	if (!pieces_.empty()) {
		for (auto& i : pieces_)
			i->update_positions();

		static const int NUM_ITERATIONS = 30;

		for (int i = 0; i < NUM_ITERATIONS; i++) {
			for (auto& i : pieces_)
				i->check_constraints();

			for (size_t j = 0; j < pieces_.size() - 1; j++)
				for (size_t k = j + 1; k < pieces_.size(); k++)
					pieces_[j]->collide(*pieces_[k]);
		}
	}

	if (!--spawn_tic_) {
		const float x = 2*BORDER + rand()%(WINDOW_WIDTH - 4*BORDER - 20*4);
		const float y = WINDOW_HEIGHT;
		const int type = rand()%(sizeof piece_patterns/sizeof *piece_patterns);

		pieces_.push_back(std::make_shared<piece>(piece_patterns[type], x, y));
		spawn_tic_ = SPAWN_INTERVAL;
	}
}

static void
init_sdl()
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		panic("SDL_Init: %s", SDL_GetError());

	if (SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 0, SDL_OPENGL) == 0)
		panic("SDL_SetVideoMode: %s", SDL_GetError());
}

static void
init_glew()
{
	GLenum rv;

	if ((rv = glewInit()) != GLEW_OK)
		panic("glewInit: %s", glewGetErrorString(rv));
}

static void
init_gl_state()
{
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	glClearColor(0, 0, 0, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glPolygonMode(GL_FRONT, GL_FILL);
}

static void
tear_down_sdl()
{
	SDL_Quit();
}

static void
init()
{
	init_sdl();
	init_glew();
	init_gl_state();
}

static void
tear_down()
{
	tear_down_sdl();
}

static world w;

static void
handle_events()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

#if 0
			case SDL_MOUSEBUTTONDOWN:
				w.on_mouse_button_down(event.button.button, event.button.x, WINDOW_HEIGHT - event.button.y - 1);
				break;

			case SDL_MOUSEBUTTONUP:
				w.on_mouse_button_up();
				break;

			case SDL_MOUSEMOTION:
				w.on_mouse_motion(event.motion.x, WINDOW_HEIGHT - event.motion.y - 1);
				break;
#endif
		}
	}
}

#ifdef DUMP_FRAMES
static void
dump_frame(int frame_num)
{
	static char buffer[3*WINDOW_WIDTH*WINDOW_HEIGHT];

	glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, buffer); 

	char path[80];
	sprintf(path, "%05d.ppm", frame_num);

	FILE *out = fopen(path, "wb");
	if (!out)
		return;

	fprintf(out, "P6\n%d %d\n255\n", WINDOW_WIDTH, WINDOW_HEIGHT);
	fwrite(buffer, sizeof buffer, 1, out);

	fclose(out);

	++frame_num;
}
#endif

void
game_loop()
{
#ifdef DUMP_FRAMES
	int cur_frame = 0;
#endif

	running = true;

	static const int FRAME_INTERVAL = 33;

	Uint32 next_frame = SDL_GetTicks() + FRAME_INTERVAL;

	while (running) {
		handle_events();

		w.update();
		w.update();

		glClear(GL_COLOR_BUFFER_BIT);
		w.draw();

#ifdef DUMP_FRAMES
		if (cur_frame%2 == 0)
			dump_frame(cur_frame/2);

		++cur_frame;
#endif

		SDL_GL_SwapBuffers();

		Uint32 now = SDL_GetTicks();

		if (now < next_frame)
			SDL_Delay(next_frame - now);

		next_frame += FRAME_INTERVAL;
	}
}

int
main(int argc, char *argv[])
{
	init();
	game_loop();
	tear_down();

	return 0;
}
