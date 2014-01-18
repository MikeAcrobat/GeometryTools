#include "pch.h"

int main(int agrc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "failed to init SDL" << std::endl;
		return 0;
	}

	const int offset_x = 200;
	const int offset_y = 100;
	const int space = 10;
	const int window_size = 512;

	const char * g_title = "Geometry";
	const int g_w_x = offset_x;
	const int g_w_y = offset_y;
	const char * t_title = "Texture";
	const int t_w_x = offset_x + window_size + space;
	const int t_w_y = offset_y;
	const char * a_title = "Timeline";
	const int a_w_x = offset_x;
	const int a_w_y = offset_y + window_size + space * 3;

	SDL_GL_SetSwapInterval(1);

	SDL_Window * g_window = SDL_CreateWindow(g_title, g_w_x, g_w_y, window_size, window_size, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (g_window == NULL) {
		std::cout << "failed to create window" << std::endl;
		return 0;
	}

	SDL_Window * t_window = SDL_CreateWindow(t_title, t_w_x, t_w_y, window_size, window_size, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (t_window == NULL) {
		std::cout << "failed to create window" << std::endl;
		return 0;
	}

	SDL_Window * a_window = SDL_CreateWindow(a_title, a_w_x, a_w_y, window_size * 2 + space, 50, SDL_WINDOW_OPENGL);
	if (a_window == NULL) {
		std::cout << "failed to create window" << std::endl;
		return 0;
	}

	SDL_GLContext context = SDL_GL_CreateContext(g_window);
	if (context == NULL) {
		std::cout << "failed to create GL context" << std::endl;
		return 0;
	}

	if (glewInit() != GLEW_OK) {
		std::cout << "failed to init glew" << std::endl;
		return 0;
	}

	SDL_GL_SetSwapInterval(1);
	glClearColor(.7f, .7f, .7f, 1.f);

	Window editor(g_window, t_window, a_window, context);
	editor.work();

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(g_window);
	SDL_DestroyWindow(t_window);
	SDL_Quit();
	return 0;
}