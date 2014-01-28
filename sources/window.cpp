#include "pch.h"
#include "window.h"
#include "stbi_image.h"

glm::vec2 uv_scale = glm::vec2(100, 100);

bool Triangle::compare(int index1, int index2, int index3) {
	bool cmp1 = indexes[0] == index1 || indexes[1] == index1 || indexes[2] == index1;
	bool cmp2 = indexes[0] == index2 || indexes[1] == index2 || indexes[2] == index2;
	bool cmp3 = indexes[0] == index3 || indexes[1] == index3 || indexes[2] == index3;
	return cmp1 && cmp2 && cmp3;
}

bool Triangle::contains(int index) {
	return indexes[0] == index || indexes[1] == index || indexes[2] == index;
}

glm::mat4 Editor::view_matrix() {
	float scale = editor_transform.scale / default_scale;
	glm::vec2 translate = editor_transform.position;
	glm::mat4 view(1.f);
	view = glm::translate(view, glm::vec3(translate.x, translate.y, 0.f));
	view = glm::scale(view, glm::vec3(scale, scale, 1.f));
	return view;
}

glm::vec2 Editor::mouse_global_to_local(float mouse_x, float mouse_y) {
	const glm::mat4 & view = view_matrix();
	glm::mat4 inverse = glm::inverse(view);
	glm::vec4 mouse = inverse * glm::vec4(mouse_x, mouse_y, 0.f, 1.f);
	return glm::vec2(mouse.x, mouse.y);
}

Editor * Window::get_editor(Uint32 windowID) {
	SDL_Window * target_window = SDL_GetWindowFromID(windowID);
	for (int i = 0; i < EditorTypeCount; i++) {
		if (editors[i].window == target_window) {
			return &editors[i];
		}
	}
	return 0;
}

Window::Window() {
	wheel_lock = false;
	mouse_down = false;
	moving_geometry = false;
	multiple_selection = false;
	multiple_selection_init = false;
	m_background.loaded = m_texture.loaded = false;
	m_background.size = m_texture.size = glm::vec2(100, 100);
	m_background_edit = false;
}

void Window::work() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glGenTextures(1, &m_texture.name);
	glGenTextures(1, &m_background.name);

	while (handle_events()) {
		render_geometry();
		render_texture();
		render_timeline();
	}
}

bool Window::handle_events() {
	SDL_Event event;

	const Uint8 * keystate = SDL_GetKeyboardState(NULL);

	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) return false;
		if (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_ENTER || event.window.event == SDL_WINDOWEVENT_LEAVE)) {
			wheel_lock = mouse_down = moving_geometry = multiple_selection = multiple_selection_init = false;
		}
		if (event.type == SDL_DROPFILE) {
			std::string filepath = event.drop.file;
			std::string extension = filepath.substr(filepath.size() - 3);
			if (extension == "xml") {
				m_xml_animation_path = filepath;
				load_geometry();
			} else {
				if (m_background_edit) {
					load_texture(filepath, m_background);
				} else {
					load_texture(filepath, m_texture);
					float scale_x = m_texture.size.x / 100.f;
					float scale_y = m_texture.size.y / 100.f;
					float scale = std::max(scale_x, scale_y);
					scale_x /= scale;	scale_y /= scale;
					uv_scale.x = 100.f * scale_x;
					uv_scale.y = 100.f * scale_y;
				}
			}
		}

		// Navigation		
		{
			if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) {
				Editor * editor = get_editor(event.button.windowID);
				if (editor) {
					if (editor->type == Geometry || editor->type == Texture) {
						wheel_lock = true;
					}
				}
			}
			if (event.type == SDL_MOUSEMOTION && wheel_lock) {
				Editor * editor = get_editor(event.motion.windowID);
				if (editor) {
					if (editor->type == Geometry || editor->type == Texture) {
						if (m_background_edit) {
							m_background_offset += editor->mouse_global_to_local((float)event.motion.xrel, (float)event.motion.yrel) - editor->mouse_global_to_local(0, 0);
						} else {
							editor->editor_transform.position.x += event.motion.xrel;
							editor->editor_transform.position.y += event.motion.yrel;
						}
					}
				}
			}
			if (event.type == SDL_MOUSEWHEEL) {
				Editor * editor = get_editor(event.wheel.windowID);
				if (editor) {
					if (editor->type == Geometry || editor->type == Texture) {
						float deminator = editor->editor_transform.scale > 1 ? log(editor->editor_transform.scale) : 1.f;
						float new_scale = editor->editor_transform.scale + (event.wheel.y / deminator);
						editor->editor_transform.scale = std::max(new_scale, 1.f);
					}
				}
			}
			if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) {
				wheel_lock = false;
			}
		}

		// Mouse down left
		if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
			Editor * editor = get_editor(event.button.windowID);
			if (editor) {
				if (editor->type == Geometry || editor->type == Texture) {
					mouse_down = true;
					end_selection = start_selection = glm::vec2(event.button.x, event.button.y);
					if (!hit_test(editor->type, glm::vec2(event.button.x, event.button.y))) {
						multiple_selection_init = true;
						selection_editor = editor->type;
						addictive_selection = keystate[SDL_SCANCODE_LSHIFT] != 0;
					}
				}
			}
		}

		// mouse move
		if (event.type == SDL_MOUSEMOTION && mouse_down) {
			Editor * editor = get_editor(event.motion.windowID);
			if (editor) {
				if (editor->type == Geometry || editor->type == Texture) {
					if (multiple_selection_init) {
						push_state();
						multiple_selection_init = false;
						multiple_selection = true;				
					} else if (multiple_selection) {
						end_selection = glm::vec2(event.motion.x, event.motion.y);
					} else {
						if (moving_geometry) {
							glm::vec2 v = editor->mouse_global_to_local((float)event.motion.xrel, (float)event.motion.yrel) - editor->mouse_global_to_local(0, 0);
							move_selected_vertexes(editor->type, v);
						} else {
							moving_geometry = true;
							push_state();
						}
					}
				}
			}
		}

		// mouse left up
		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
			Editor * editor = get_editor(event.button.windowID);
			if (editor) {
				if (editor->type == Geometry || editor->type == Texture) {
					mouse_down = false;
					if (moving_geometry) {
						moving_geometry = false;
					} else {				
						if (multiple_selection) {
							multiple_selection = false;
							if ( glm::length(start_selection - end_selection) > 1 ) {
								select_vertex_range(editor->type, start_selection, end_selection, addictive_selection);
							}
						} else {
							select_vertex(editor->type, glm::vec2(event.button.x, event.button.y), keystate[SDL_SCANCODE_LSHIFT] != 0);
						}
					} 
				}
			}
		}

		// mouse right up
		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
			Editor * editor = get_editor(event.button.windowID);
			if (editor && editor->type == Geometry) add_vertex( editor->mouse_global_to_local((float)event.button.x, (float)event.button.y) );
		}

		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_R)		reset_views();
			if (event.key.keysym.scancode == SDL_SCANCODE_T)		m_background_offset = glm::vec2();
			if (event.key.keysym.scancode == SDL_SCANCODE_UP)		move_editor( 0,	-1 );
			if (event.key.keysym.scancode == SDL_SCANCODE_LEFT)		move_editor(-1,	 0 );
			if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT)	move_editor( 1,	 0 );
			if (event.key.keysym.scancode == SDL_SCANCODE_DOWN)		move_editor( 0,	 1 );
			if (event.key.keysym.scancode == SDL_SCANCODE_F)		add_triangle_from_selection();
			if (event.key.keysym.scancode == SDL_SCANCODE_D)		del_triangle_from_selection();
			if (event.key.keysym.scancode == SDL_SCANCODE_DELETE)	del_vertexes_from_selection();
			if (event.key.keysym.scancode == SDL_SCANCODE_Z && keystate[SDL_SCANCODE_LCTRL] != 0) {
				pop_state();
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_S && keystate[SDL_SCANCODE_LCTRL] != 0) {
				save_geometry();
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_TAB) {
				m_background_edit = !m_background_edit;
				SDL_SetWindowTitle(editors[Geometry].window, m_background_edit ? "Background Edit" : "Geometry");
			}
		}
	}
	return true;
}

void Window::render_texture() {
	Editor & editor = editors[Texture];
	SDL_GL_MakeCurrent(editor.window, context);

	int width, height;
	SDL_GL_GetDrawableSize(editor.window, &width, &height);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 transform = glm::ortho<float>(0.f, static_cast<float>(width), static_cast<float>(height), 0.f);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(transform));
	
	const glm::mat4 & view = editor.view_matrix();
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(view));

	if (m_texture.loaded) {
		const glm::vec2 & size = uv_scale;
		Vertex texture_pivots[4] = {
			{ glm::vec2(0.f, 0.f), glm::vec2(0.f, 0.f), false }, 
			{ glm::vec2(1.f, 0.f), glm::vec2(1.f, 0.f), false }, 
			{ glm::vec2(0.f, 1.f), glm::vec2(0.f, 1.f), false }, 
			{ glm::vec2(1.f, 1.f), glm::vec2(1.f, 1.f), false }
		};
		Primitives::draw_triangle(texture_pivots[0], texture_pivots[1], texture_pivots[2], m_texture.name, size, 1.f);
		Primitives::draw_triangle(texture_pivots[1], texture_pivots[2], texture_pivots[3], m_texture.name, size, 1.f);
	}

	Primitives::draw_ruler(uv_scale);
	
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Primitives::draw_pivot(vertexes[i].texture_uv * uv_scale, vertexes[i].locked ? Color::Green : Color::Blue, editor.editor_transform.scale);
	}

	for (unsigned int i = 0; i < triangles.size(); i++) {
		Triangle & tri = triangles[i];
		Vertex & p0 = vertexes[tri.indexes[0]];
		Vertex & p1 = vertexes[tri.indexes[1]];
		Vertex & p2 = vertexes[tri.indexes[2]];
		Primitives::draw_line(p0.texture_uv, p1.texture_uv, uv_scale, p0.locked ? Color::Green : Color::Blue, p1.locked ? Color::Green : Color::Blue);
		Primitives::draw_line(p1.texture_uv, p2.texture_uv, uv_scale, p1.locked ? Color::Green : Color::Blue, p2.locked ? Color::Green : Color::Blue);
		Primitives::draw_line(p2.texture_uv, p0.texture_uv, uv_scale, p2.locked ? Color::Green : Color::Blue, p0.locked ? Color::Green : Color::Blue);
	}

	if (multiple_selection && selection_editor == Texture) {
		glm::vec2 start = editors[Texture].mouse_global_to_local(start_selection.x, start_selection.y);
		glm::vec2 finish = editors[Texture].mouse_global_to_local(end_selection.x, end_selection.y);
		Primitives::draw_rect(start, finish, Color::Green);
	}

	SDL_GL_SwapWindow(editor.window);
}

void Window::render_geometry() {
	Editor & editor = editors[Geometry];
	SDL_GL_MakeCurrent(editor.window, context);

	int width, height;
	SDL_GL_GetDrawableSize(editor.window, &width, &height);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 transform = glm::ortho<float>(0.f, static_cast<float>(width), static_cast<float>(height), 0.f);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(transform));
	
	const glm::mat4 & view = editor.view_matrix();
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(view));

	if (m_background.loaded) {
		const glm::vec2 & offset = m_background_offset / uv_scale;
		const glm::vec2 & size = m_background.size / m_texture.size;
		Vertex background_pivots[4] = {
			{ glm::vec2(offset.x,			offset.y),			glm::vec2(0.f, 0.f), false }, 
			{ glm::vec2(offset.x + size.x,	offset.y),			glm::vec2(1.f, 0.f), false }, 
			{ glm::vec2(offset.x,			offset.y + size.y),	glm::vec2(0.f, 1.f), false }, 
			{ glm::vec2(offset.x + size.x,  offset.y + size.y),	glm::vec2(1.f, 1.f), false }
		};
		Primitives::draw_triangle(background_pivots[0], background_pivots[1], background_pivots[2], m_background.name, uv_scale, 1.f);
		Primitives::draw_triangle(background_pivots[1], background_pivots[2], background_pivots[3], m_background.name, uv_scale, 1.f);
	}

	if (m_texture.loaded) {
		const glm::vec2 & size = uv_scale;
		Vertex texture_pivots[4] = {
			{ glm::vec2(0.f, 0.f), glm::vec2(0.f, 0.f), false }, 
			{ glm::vec2(1.f, 0.f), glm::vec2(1.f, 0.f), false }, 
			{ glm::vec2(0.f, 1.f), glm::vec2(0.f, 1.f), false }, 
			{ glm::vec2(1.f, 1.f), glm::vec2(1.f, 1.f), false }
		};
		Primitives::draw_triangle(texture_pivots[0], texture_pivots[1], texture_pivots[2], m_texture.name, size, .3f);
		Primitives::draw_triangle(texture_pivots[1], texture_pivots[2], texture_pivots[3], m_texture.name, size, .3f);

		for (unsigned int i = 0; i < triangles.size(); i++) {
			Triangle & tri = triangles[i];
			Vertex & p0 = vertexes[tri.indexes[0]];
			Vertex & p1 = vertexes[tri.indexes[1]];
			Vertex & p2 = vertexes[tri.indexes[2]];
			Primitives::draw_triangle(p0, p1, p2, m_texture.name, size, 1.f);
		}	
	} 
	Primitives::draw_ruler(uv_scale);

	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Primitives::draw_pivot(vertexes[i].position * uv_scale, vertexes[i].locked ? Color::Green : Color::Blue, editor.editor_transform.scale);
	}

	for (unsigned int i = 0; i < triangles.size(); i++) {
		Triangle & tri = triangles[i];
		Vertex & p0 = vertexes[tri.indexes[0]];
		Vertex & p1 = vertexes[tri.indexes[1]];
		Vertex & p2 = vertexes[tri.indexes[2]];
		Primitives::draw_line(p0.position, p1.position, uv_scale, p0.locked ? Color::Green : Color::Blue, p1.locked ? Color::Green : Color::Blue);
		Primitives::draw_line(p1.position, p2.position, uv_scale, p1.locked ? Color::Green : Color::Blue, p2.locked ? Color::Green : Color::Blue);
		Primitives::draw_line(p2.position, p0.position, uv_scale, p2.locked ? Color::Green : Color::Blue, p0.locked ? Color::Green : Color::Blue);
	}

	if (multiple_selection && selection_editor == Geometry) {
		glm::vec2 start = editors[Geometry].mouse_global_to_local(start_selection.x, start_selection.y);
		glm::vec2 finish = editors[Geometry].mouse_global_to_local(end_selection.x, end_selection.y);
		Primitives::draw_rect(start, finish, Color::Green);
	}

	SDL_GL_SwapWindow(editor.window);
}

void Window::render_timeline() {
	Editor & editor = editors[Timeline];
	SDL_GL_MakeCurrent(editor.window, context);

	int width, height;
	SDL_GL_GetDrawableSize(editor.window, &width, &height);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);

	glm::mat4 transform = glm::ortho<float>(0.f, static_cast<float>(width), static_cast<float>(height), 0.f);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(glm::value_ptr(transform));
	
	const glm::mat4 & view = editor.view_matrix();
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(glm::value_ptr(view));

	SDL_GL_SwapWindow(editor.window);
}

void Window::load_texture(const std::string & path, Image & image) {

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image.name);

	int width, height, channels;
	unsigned char* ptr = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (ptr && width && height) {
		image.loaded = true;
		image.size = glm::vec2(width, height);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        stbi_image_free(ptr);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

bool Window::hit_test(EditorType type, glm::vec2 mouse) {
	Editor & editor = editors[type];
	mouse = editor.mouse_global_to_local(mouse.x, mouse.y);
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		if (type == Geometry && glm::length(vertex.position * uv_scale - mouse) < 2.f) return true;;
		if (type == Texture && glm::length(vertex.texture_uv * uv_scale - mouse) < 2.f) return true;
	}
	return false;
}

int Window::select_vertex(EditorType type, glm::vec2 mouse, bool add) {
	push_state();

	bool state_change = false;
	int selected_index = -1;

	Editor & editor = editors[type];
	mouse = editor.mouse_global_to_local(mouse.x, mouse.y);
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		bool pick = false;
		pick = pick || type == Geometry && glm::length(vertex.position * uv_scale - mouse) < 2.f;
		pick = pick || type == Texture && glm::length(vertex.texture_uv * uv_scale - mouse) < 2.f;
		bool prev_lock = vertex.locked;
		bool next_lock = add ? (pick ? !vertex.locked : vertex.locked) : pick;
		vertex.locked = next_lock;
		state_change = state_change || (prev_lock != next_lock);

		selected_index = pick ? i : selected_index;
	}

	if (!state_change) pop_state();

	return selected_index;
}

void Window::select_vertex_range(EditorType type, glm::vec2 start, glm::vec2 end, bool add) {
#define INSIDE(P, S, E)  (std::min(S.x, E.x) < P.x && P.x < std::max(S.x, E.x) && std::min(S.y, E.y) < P.y && P.y < std::max(S.y, E.y))
	//push_state();
	bool state_change = false;

	Editor & editor = editors[type];
	start = editor.mouse_global_to_local(start.x, start.y);
	end = editor.mouse_global_to_local(end.x, end.y);
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		bool pick = false;
		pick = pick || (type == Geometry && INSIDE((vertex.position * uv_scale), start, end));
		pick = pick || (type == Texture	&& INSIDE((vertex.texture_uv * uv_scale), start, end));
		bool prev_lock = vertex.locked;
		bool next_lock = add ? (pick ? !vertex.locked : vertex.locked) : pick;
		vertex.locked = next_lock;
		state_change = state_change || (prev_lock != next_lock);
	}

	if (!state_change) pop_state();
#undef INSIDE
}

void Window::add_vertex(glm::vec2 position) {
	push_state();

	Vertex v;
	v.position = v.texture_uv = position / uv_scale;
	v.locked = true;
	vertexes.push_back(v);
}

void Window::add_triangle_from_selection() {
	std::vector<int> selected;
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		if (vertex.locked) {
			selected.push_back(i);
		}
	}

	if (selected.size() != 3) return;

	for (unsigned int i = 0; i < triangles.size(); i++) {
		if (triangles[i].compare( selected[0], selected[1], selected[2] )) return;
	}

	push_state();

	Triangle t;
	t.indexes[0] = selected[0];
	t.indexes[1] = selected[1];
	t.indexes[2] = selected[2];
	triangles.push_back(t);
}

void Window::del_triangle_from_selection() {
	std::vector<int> selected;
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		if (vertex.locked) {
			selected.push_back(i);
		}
	}

	if (selected.size() != 3) return;
	
	push_state();

	for (unsigned int i = 0; i < triangles.size(); i++) {
		if (triangles[i].compare( selected[0], selected[1], selected[2] )) {
			triangles.erase(triangles.begin() + i);
			return;
		}		
	}
}

void Window::del_vertexes_from_selection() {

	std::vector<int> selected;
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		if (vertex.locked) {
			selected.push_back(i);			
		}
	}

	if (selected.size() == 0) return;

	push_state();

	std::sort(selected.begin(), selected.end());
	std::reverse(selected.begin(), selected.end());

	for (unsigned int s = 0; s < selected.size(); s++) {
		for (unsigned int i = 0; i < triangles.size(); i++) {		
			if (triangles[i].contains( selected[s] )) {
				triangles.erase(triangles.begin() + i); i--;
			}
		}
	}

	for (unsigned int s = 0; s < selected.size(); s++) {
		for (unsigned int i = 0; i < triangles.size(); i++) {		
			Triangle & tri = triangles[i];
			if (tri.indexes[0] > selected[s]) tri.indexes[0] -= 1;
			if (tri.indexes[1] > selected[s]) tri.indexes[1] -= 1;
			if (tri.indexes[2] > selected[s]) tri.indexes[2] -= 1;
		}
	}
	
	for (unsigned int s = 0; s < selected.size(); s++) {
		vertexes.erase(vertexes.begin() + selected[s]);
	}
}

void Window::move_selected_vertexes(EditorType type, glm::vec2 vector) {
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & vertex = vertexes[i];
		if (vertex.locked) {
			if (type == Geometry)	vertex.position += vector;
			if (type == Texture)	vertex.texture_uv += vector / uv_scale;
		}
	}
}

void Window::push_state() {
	std::cout << "Push state" << std::endl;
	GeomState state;
	state.triangles = triangles;
	state.vertexes = vertexes;
	state_stack.push_back(state);

	if (state_stack.size() > 40) {
		state_stack.pop_front();
	}
}

void Window::pop_state() {
	std::cout << "Pop state" << std::endl;
	if (state_stack.size() > 0) {
		GeomState & state = state_stack.back();
		triangles = state.triangles;
		vertexes = state.vertexes;
		state_stack.pop_back();
	}
}

void Window::move_editor(int x_offset, int y_offset) {
	for (int i = 0; i < EditorTypeCount; i++) {
		int x, y;
		SDL_GetWindowPosition(editors[i].window, &x, &y);
		x += x_offset;	y += y_offset;
		SDL_SetWindowPosition(editors[i].window,  x,  y);					
	}
}

void Window::reset_views() {
	editors[Geometry].editor_transform.scale	= default_scale * 4;
	editors[Geometry].editor_transform.position	= glm::vec2(55, 55);
	editors[Texture].editor_transform.scale		= default_scale * 4;
	editors[Texture].editor_transform.position	= glm::vec2(55, 55);
}

void Window::load_geometry() {
}

void Window::save_geometry() {
}