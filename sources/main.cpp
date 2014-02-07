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

	const char * g_title = "Geometry Edit";
	const int g_w_x = offset_x;
	const int g_w_y = offset_y;
	const char * t_title = "Texture Edit";
	const int t_w_x = offset_x + window_size + space;
	const int t_w_y = offset_y;
	const char * a_title = "Timeline";
	const int a_w_x = offset_x;
	const int a_w_y = offset_y + window_size + space * 3;

	SDL_GL_SetSwapInterval(1);

	Window editor;

	Editor geometry_editor;
	{
		geometry_editor.type = Geometry;
		geometry_editor.window = SDL_CreateWindow(g_title, g_w_x, g_w_y, window_size, window_size, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		geometry_editor.editor_transform.scale		= default_scale * 4;
		geometry_editor.editor_transform.position	= glm::vec2(55, 55);
		editor.editors[Geometry] = geometry_editor;
	}

	Editor texture_editor;
	{
		texture_editor.type = Texture;
		texture_editor.window = SDL_CreateWindow(t_title, t_w_x, t_w_y, window_size, window_size, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
		texture_editor.editor_transform.scale		= default_scale * 4;
		texture_editor.editor_transform.position	= glm::vec2(55, 55);
		editor.editors[Texture]  = texture_editor;
	}

	Editor timeline;
	{
		timeline.type = Timeline;
		timeline.window = SDL_CreateWindow(a_title, a_w_x, a_w_y, window_size * 2 + space, 50, SDL_WINDOW_OPENGL);
		timeline.editor_transform.scale				= default_scale;
		editor.editors[Timeline]					= timeline;
	}

	editor.context = SDL_GL_CreateContext(editor.editors[Geometry].window);

	if (glewInit() != GLEW_OK) {
		std::cout << "failed to init glew" << std::endl;
		return 0;
	}

	SDL_GL_SetSwapInterval(1);
	glClearColor(.7f, .7f, .7f, 1.f);

	editor.work();

	SDL_GL_DeleteContext(editor.context);
	SDL_DestroyWindow(editor.editors[Geometry].window);
	SDL_DestroyWindow(editor.editors[Texture].window);
	SDL_DestroyWindow(editor.editors[Timeline].window);
	SDL_Quit();
	return 0;
}