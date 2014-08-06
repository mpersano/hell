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

static const int BORDER = 8;

static const float BODY_RADIUS = 3;

static const float GRAVITY = 1.;
static const float DAMPING = .75;
static const float FRICTION = .6;

static const int SPAWN_INTERVAL = 30;

struct body
{
	body(const vec2& position)
	: position(position)
	, prev_position(position)
	{ }

	void draw() const;

	vec2 position;
	vec2 prev_position;
	vec2 acceleration;
};

typedef std::shared_ptr<body> body_ptr;

struct spring
{
	spring(body_ptr b0, body_ptr b1)
	: b0(b0)
	, b1(b1)
	, rest_length((b0->position - b1->position).length())
	{ }

	void draw() const;

	body_ptr b0, b1;
	float rest_length;
};

struct rgb
{
	rgb(float r, float g, float b)
	: r(r), g(g), b(b)
	{ }

	float r, g, b;
};

struct triangle
{
	triangle(body_ptr b0, body_ptr b1, body_ptr b2, const rgb& color)
	: b0(b0)
	, b1(b1)
	, b2(b2)
	, color(color)
	{ }

	void draw() const;

	void initialize_bounding_circle();

	void shift(const vec2& offset)
	{
		b0->position += offset;
		b1->position += offset;
		b2->position += offset;
	}

	body_ptr b0, b1, b2;
	rgb color;
	vec2 center;
	float radius;
};

class triangle_intersection
{
public:
	triangle_intersection(const triangle& t0, const triangle& t1)
	: t0_(t0), t1_(t1)
	{ }

	bool operator()();

	const vec2& push_vector() const
	{ return push_vector_; }

private:
	template <bool First>
	bool separating_axis_test(const vec2& from, const vec2& to);

	const triangle &t0_, &t1_;
	vec2 push_vector_;
};

class world
{
public:
	world();

	void draw() const;
	void update();

	void on_mouse_button_down(int button, int x, int y);
	void on_mouse_button_up();
	void on_mouse_motion(int x, int y);

private:
	void spawn_piece(float x_origin, float y_origin, int type);

	void draw_walls() const;

	std::vector<body_ptr> bodies_;
	std::vector<triangle> triangles_;
	std::vector<spring> springs_;

	float left_wall_x_;
	float right_wall_x_;
	float top_wall_y_;
	float bottom_wall_y_;
	int spawn_tic_;
};

world::world()
: left_wall_x_(BORDER)
, right_wall_x_(WINDOW_WIDTH - BORDER)
, top_wall_y_(BORDER)
, bottom_wall_y_(WINDOW_HEIGHT - BORDER)
, spawn_tic_(SPAWN_INTERVAL)
{ }

void
world::spawn_piece(float x_origin, float y_origin, int type)
{
	static const float SPACING = 20;

	enum {
		PIECE_WIDTH = 4,
		PIECE_HEIGHT = 4,
	};

	struct {
		char pattern[PIECE_HEIGHT][PIECE_WIDTH + 1];
		rgb color;
	} pieces[] {
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

	std::map<std::pair<int, int>, size_t> body_map;
	std::set<std::pair<size_t, size_t>> spring_set;

	auto add_body = [&] (int i, int j) -> size_t {
		auto it = body_map.find(std::make_pair(i, j));

		if (it == body_map.end()) {
			bodies_.push_back(std::make_shared<body>(vec2(x_origin + j*SPACING, y_origin + i*SPACING)));
			it = body_map.insert(it, std::make_pair(std::make_pair(i, j), bodies_.size() - 1));
		}

		return it->second;
	};

	auto add_spring = [&] (size_t b0, size_t b1) {
		if (spring_set.find(std::make_pair(b0, b1)) != spring_set.end())
			return;

		if (spring_set.find(std::make_pair(b1, b0)) != spring_set.end())
			return;

		springs_.push_back(spring(bodies_[b0], bodies_[b1]));
		spring_set.insert(std::make_pair(b0, b1));
	};

	for (int i = 0; i < PIECE_HEIGHT; i++) {
		for (int j = 0; j < PIECE_WIDTH; j++) {
			if (pieces[type].pattern[i][j] == '#') {
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

				// triangles

				triangles_.push_back(triangle(bodies_[b0], bodies_[b1], bodies_[b2], pieces[type].color));
				triangles_.push_back(triangle(bodies_[b2], bodies_[b3], bodies_[b0], pieces[type].color));
			}
		}
	}

	printf("%u bodies, %u springs, %u triangles\n", bodies_.size(), springs_.size(), triangles_.size());
}

void
world::draw() const
{
	draw_walls();

	for (auto& i : triangles_)
		i.draw();

	for (auto& i : bodies_)
		i->draw();

	for (auto& i : springs_)
		i.draw();
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
	// forces

	for (auto& i : bodies_) {
		vec2 speed = DAMPING*(i->position - i->prev_position);

		i->prev_position = i->position;
		i->position += speed + vec2(0, -GRAVITY);
	}

	static const int NUM_ITERATIONS = 16;

	for (int i = 0; i < NUM_ITERATIONS; i++) {
		// springs

		for (auto& i : springs_) {
			vec2& p0 = i.b0->position;
			vec2& p1 = i.b1->position;

			vec2 dir = p1 - p0;

			float l = dir.length();
			float f = .5*(l - i.rest_length)/l;

			p0 += f*dir;
			p1 -= f*dir;
		}

		// body-wall collisions

		for (auto& i : bodies_) {
			vec2& p = i->position;

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

		// triangle-triangle collisions

		if (triangles_.size()) {
			for (auto& t : triangles_)
				t.initialize_bounding_circle();

			for (size_t i = 0; i < triangles_.size() - 1; i++) {
				for (size_t j = i + 1; j < triangles_.size(); j++) {
					triangle& t0 = triangles_[i];
					triangle& t1 = triangles_[j];

					triangle_intersection intersection(t0, t1);

					if (intersection()) {
						vec2 push_vector = intersection.push_vector();

						t0.shift(-push_vector);
						t1.shift(push_vector);
					}
				}
			}
		}
	}

	if (!--spawn_tic_) {
		spawn_piece(2*BORDER + rand()%(WINDOW_WIDTH - 4*BORDER - 20*4), WINDOW_HEIGHT, rand()%7);
		spawn_tic_ = SPAWN_INTERVAL;
	}
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

void
triangle::draw() const
{
	glColor3f(color.r, color.g, color.b);

	glBegin(GL_TRIANGLES);
	glVertex2f(b0->position.x, b0->position.y);
	glVertex2f(b1->position.x, b1->position.y);
	glVertex2f(b2->position.x, b2->position.y);
	glEnd();
}

static std::pair<float, float>
project_triangle_to_axis(const vec2& dir, const triangle& t)
{
	float t0 = dir.dot(t.b0->position);
	float t1 = dir.dot(t.b1->position);
	float t2 = dir.dot(t.b2->position);

	return std::make_pair(std::min(t0, std::min(t1, t2)), std::max(t0, std::max(t1, t2)));
}

template <bool First>
bool
triangle_intersection::separating_axis_test(const vec2& from, const vec2& to)
{
	vec2 normal = vec2(-(to.y - from.y), to.x - from.x).normalize();

	std::pair<float, float> s0 = project_triangle_to_axis(normal, t0_);
	std::pair<float, float> s1 = project_triangle_to_axis(normal, t1_);

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

	if (First || push_length < push_vector_.length_squared())
		push_vector_ = normal*(push_length/normal.length());

	return false;
}

bool
triangle_intersection::operator()()
{
	if ((t0_.center - t1_.center).length_squared() > (t0_.radius + t1_.radius)*(t0_.radius + t1_.radius))
		return false;
	
	{
	const vec2& v0 = t0_.b0->position;
	const vec2& v1 = t0_.b1->position;
	const vec2& v2 = t0_.b2->position;

	if (separating_axis_test<true>(v0, v1))
		return false;

	if (separating_axis_test<false>(v1, v2))
		return false;

	if (separating_axis_test<false>(v2, v0))
		return false;
	}

	{
	const vec2& v0 = t1_.b0->position;
	const vec2& v1 = t1_.b1->position;
	const vec2& v2 = t1_.b2->position;

	if (separating_axis_test<false>(v0, v1))
		return false;

	if (separating_axis_test<false>(v1, v2))
		return false;

	if (separating_axis_test<false>(v2, v0))
		return false;
	}

	return true;
}

void
triangle::initialize_bounding_circle()
{
	const vec2& v0 = b0->position;
	const vec2& v1 = b1->position;
	const vec2& v2 = b2->position;

	center = (1./3.)*(v0 + v1 + v2);

	const float r0 = (v0 - center).length();
	const float r1 = (v1 - center).length();
	const float r2 = (v2 - center).length();

	radius = std::max(r0, std::max(r1, r2));
}

void
spring::draw() const
{
	glColor3f(1, 1, 1);

	glBegin(GL_LINES);
	glVertex2f(b0->position.x, b0->position.y);
	glVertex2f(b1->position.x, b1->position.y);
	glEnd();
}

void
world::on_mouse_button_down(int, int, int)
{ }

void
world::on_mouse_button_up()
{ }

void
world::on_mouse_motion(int, int)
{ }

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

			case SDL_MOUSEBUTTONDOWN:
				w.on_mouse_button_down(event.button.button, event.button.x, WINDOW_HEIGHT - event.button.y - 1);
				break;

			case SDL_MOUSEBUTTONUP:
				w.on_mouse_button_up();
				break;

			case SDL_MOUSEMOTION:
				w.on_mouse_motion(event.motion.x, WINDOW_HEIGHT - event.motion.y - 1);
				break;
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
