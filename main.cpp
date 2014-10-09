#include <SDL.h>
#include <GL/glew.h>

#include "panic.h"
#include "world.h"

static const int WINDOW_WIDTH = 240;
static const int WINDOW_HEIGHT = 320;
static const int BORDER = 8;

static bool running = false;

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

static void
handle_events()
{
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					running = false;
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
	world w(WINDOW_WIDTH - 2*BORDER, WINDOW_HEIGHT - 2*BORDER);

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

		glPushMatrix();
		glTranslatef(BORDER, BORDER, 0);

		w.draw();

		glPopMatrix();

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
