#include "pch.h"
#include "window.h"
#include "stbi_image.h"

glm::vec2 ed_scale = glm::vec2(100, 100);

unsigned int frame_count = 50;

bool compare(const Vertex & v1, const Vertex & v2) {
	return v1.time < v2.time;
}

glm::vec2 MeshVertex::get_pos(float time) {
	if (vertexes.empty()) return glm::vec2();
	if (vertexes.size() == 1) return vertexes[0].position;
	for (unsigned int i = 1; i < vertexes.size(); i++) {
		Vertex & v1 = vertexes[i - 1];
		Vertex & v2 = vertexes[i];
		if (v1.time <= time && time <= v2.time) {
			float dt = (time - v1.time) / (v2.time - v1.time);
			return v1.position + (v2.position - v1.position) * dt;
		}
	}
	return vertexes.back().position;
}

glm::vec2 MeshVertex::get_uv(float time) {
	if (vertexes.empty()) return glm::vec2();
	if (vertexes.size() == 1) return vertexes[0].texture_uv;
	for (unsigned int i = 1; i < vertexes.size(); i++) {
		Vertex & v1 = vertexes[i - 1];
		Vertex & v2 = vertexes[i];
		if (v1.time <= time && time <= v2.time) {
			float dt = (time - v1.time) / (v2.time - v1.time);
			return v1.texture_uv + (v2.texture_uv - v1.texture_uv) * dt;
		}
	}
	return vertexes.back().texture_uv;
}

Vertex & MeshVertex::vertex(float time) {
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Vertex & v = vertexes[i];
		if (std::abs(v.time - time) <= (1.f / (float)frame_count)) return v;
	}
	time = int(time * frame_count) / float(frame_count);
	Vertex v;
	v.position = get_pos(time);		v.texture_uv = get_uv(time);	
	v.time = time;
	vertexes.push_back(v);			
	sort();
	return vertex(time);
}

void MeshVertex::sort() {
	std::sort(vertexes.begin(), vertexes.end(), compare);
}

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
	m_time = 0.f;
	m_play = false;
	m_show_trace = m_show_pivots = false;
}

void Window::work() {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glGenTextures(1, &m_texture.name);
	glGenTextures(1, &m_background.name);

	while (handle_events()) {

		static unsigned int last_time = SDL_GetTicks();
		unsigned int current_time = SDL_GetTicks();
		float dt = (current_time - last_time) / 1000.f;
		last_time = current_time;

		if (m_play) {
			m_time += dt;
			if (m_time >= 1.f) m_time -= 1.f;
		}

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
					ed_scale.x = 100.f * scale_x;
					ed_scale.y = 100.f * scale_y;
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
				mouse_down = true;
				if (editor->type == Geometry || editor->type == Texture) {
					end_selection = start_selection = glm::vec2(event.button.x, event.button.y);
					if (!hit_test(editor->type, glm::vec2(event.button.x, event.button.y))) {
						multiple_selection_init = true;
						selection_editor = editor->type;
						addictive_selection = keystate[SDL_SCANCODE_LSHIFT] != 0;
					}
				} else {
					int w, h;
					SDL_GL_GetDrawableSize(editor->window, &w, &h);
					m_time = (float)event.motion.x * (frame_count + 1) / (float)w / (float)frame_count;
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
				} else {
					int w, h;
					SDL_GL_GetDrawableSize(editor->window, &w, &h);
					m_time = (float)event.motion.x * (frame_count + 1) / (float)w / (float)frame_count;
				}
			}
		}

		// mouse left up
		if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
			Editor * editor = get_editor(event.button.windowID);
			if (editor) {
				mouse_down = false;
				if (editor->type == Geometry || editor->type == Texture) {
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
				} else {
					int w, h;
					SDL_GL_GetDrawableSize(editor->window, &w, &h);
					m_time = (float)event.motion.x * (frame_count + 1) / (float)w / (float)frame_count;
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
				SDL_SetWindowTitle(editors[Geometry].window, m_background_edit ? "Background Edit" : "Geometry Edit");
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)	m_play = !m_play;
			if (event.key.keysym.scancode == SDL_SCANCODE_HOME)		m_time = 0;
			if (event.key.keysym.scancode == SDL_SCANCODE_END)		m_time = 1;
			if (event.key.keysym.scancode == SDL_SCANCODE_H)		m_show_trace = !m_show_trace;
			if (event.key.keysym.scancode == SDL_SCANCODE_J)		m_show_pivots = !m_show_pivots;
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
		const glm::vec2 & size = ed_scale;
		glm::vec2 v[8] = {
			glm::vec2(0.f, 0.f), glm::vec2(0.f, 0.f), 
			glm::vec2(1.f, 0.f), glm::vec2(1.f, 0.f), 
			glm::vec2(0.f, 1.f), glm::vec2(0.f, 1.f), 
			glm::vec2(1.f, 1.f), glm::vec2(1.f, 1.f),
		};
		Primitives::draw_triangle(v[0], v[1], v[2], v[3], v[4], v[5], m_texture.name, size, 1.f);
		Primitives::draw_triangle(v[2], v[3], v[4], v[5], v[6], v[7], m_texture.name, size, 1.f);
	}

	Primitives::draw_ruler(ed_scale);
	
	if (m_show_pivots) {
		for (int t = 0; t != 10; t++) {
			float time = t / 9.f;
			for (unsigned int i = 0; i < vertexes.size(); i++) {
				MeshVertex & vertes = vertexes[i];
				if (vertes.locked) {
					Primitives::draw_pivot(vertes.get_uv(time) * ed_scale, Color::Grey, editor.editor_transform.scale);
				}
			}
		}
	}

	if (m_show_trace) {
		for (int t = 0; t != 10; t++) {
			float time = t / 9.f;
			for (unsigned int i = 0; i < triangles.size(); i++) {
				Triangle & tri = triangles[i];
				MeshVertex & p0 = vertexes[tri.indexes[0]];	glm::vec2 uv0_pos = p0.get_uv(time);
				MeshVertex & p1 = vertexes[tri.indexes[1]]; glm::vec2 uv1_pos = p1.get_uv(time);
				MeshVertex & p2 = vertexes[tri.indexes[2]]; glm::vec2 uv2_pos = p2.get_uv(time);
				Primitives::draw_line(uv0_pos, uv1_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
				Primitives::draw_line(uv1_pos, uv2_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
				Primitives::draw_line(uv2_pos, uv0_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
			}
		}
	}

	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Primitives::draw_pivot(vertexes[i].get_uv(m_time) * ed_scale, vertexes[i].locked ? Color::Green : Color::Blue, editor.editor_transform.scale);
	}

	for (unsigned int i = 0; i < triangles.size(); i++) {
		Triangle & tri = triangles[i];
		MeshVertex & p0 = vertexes[tri.indexes[0]];	glm::vec2 p0_uv = p0.get_uv(m_time);
		MeshVertex & p1 = vertexes[tri.indexes[1]];	glm::vec2 p1_uv = p1.get_uv(m_time);
		MeshVertex & p2 = vertexes[tri.indexes[2]];	glm::vec2 p2_uv = p2.get_uv(m_time);
		Primitives::draw_line(p0_uv, p1_uv, ed_scale, p0.locked ? Color::Green : Color::Blue, p1.locked ? Color::Green : Color::Blue, 1.f);
		Primitives::draw_line(p1_uv, p2_uv, ed_scale, p1.locked ? Color::Green : Color::Blue, p2.locked ? Color::Green : Color::Blue, 1.f);
		Primitives::draw_line(p2_uv, p0_uv, ed_scale, p2.locked ? Color::Green : Color::Blue, p0.locked ? Color::Green : Color::Blue, 1.f);
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
		const glm::vec2 & offset = m_background_offset / ed_scale;
		const glm::vec2 & size = m_background.size / m_texture.size;
		glm::vec2 v[8] = {
			glm::vec2(offset.x,				offset.y),			glm::vec2(0.f, 0.f), 
			glm::vec2(offset.x + size.x,	offset.y),			glm::vec2(1.f, 0.f), 
			glm::vec2(offset.x,				offset.y + size.y),	glm::vec2(0.f, 1.f), 
			glm::vec2(offset.x + size.x,	offset.y + size.y),	glm::vec2(1.f, 1.f),
		};
		Primitives::draw_triangle(v[0], v[1], v[2], v[3], v[4], v[5], m_texture.name, size, 1.f);
		Primitives::draw_triangle(v[2], v[3], v[4], v[5], v[6], v[7], m_texture.name, size, 1.f);
	}

	if (m_show_trace) {
		for (int t = 0; t != 10; t++) {
			float time = t / 9.f;
			for (unsigned int i = 0; i < triangles.size(); i++) {
				Triangle & tri = triangles[i];
				MeshVertex & p0 = vertexes[tri.indexes[0]];	glm::vec2 p0_pos = p0.get_pos(time);
				MeshVertex & p1 = vertexes[tri.indexes[1]]; glm::vec2 p1_pos = p1.get_pos(time);
				MeshVertex & p2 = vertexes[tri.indexes[2]]; glm::vec2 p2_pos = p2.get_pos(time);
				Primitives::draw_line(p0_pos, p1_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
				Primitives::draw_line(p1_pos, p2_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
				Primitives::draw_line(p2_pos, p0_pos, ed_scale, Color::Grey, Color::Grey, 1.f);
			}
		}
	}

	if (m_show_pivots) {
		for (int t = 0; t != 10; t++) {
			float time = t / 9.f;
			for (unsigned int i = 0; i < vertexes.size(); i++) {
				MeshVertex & vertes = vertexes[i];
				if (vertes.locked) {
					Primitives::draw_pivot(vertes.get_pos(time) * ed_scale, Color::Grey, editor.editor_transform.scale);
				}
			}
		}
	}

	if (m_texture.loaded) {
		const glm::vec2 & size = ed_scale;
		static glm::vec2 v[8] = {
			glm::vec2(0.f, 0.f), glm::vec2(0.f, 0.f), 
			glm::vec2(1.f, 0.f), glm::vec2(1.f, 0.f), 
			glm::vec2(0.f, 1.f), glm::vec2(0.f, 1.f), 
			glm::vec2(1.f, 1.f), glm::vec2(1.f, 1.f),
		};
		Primitives::draw_triangle(v[0], v[1], v[2], v[3], v[4], v[5], m_texture.name, size, .3f);
		Primitives::draw_triangle(v[2], v[3], v[4], v[5], v[6], v[7], m_texture.name, size, .3f);	

		for (unsigned int i = 0; i < triangles.size(); i++) {
			Triangle & tri = triangles[i];
			MeshVertex & p0 = vertexes[tri.indexes[0]];
			MeshVertex & p1 = vertexes[tri.indexes[1]];
			MeshVertex & p2 = vertexes[tri.indexes[2]];
			Primitives::draw_triangle(p0.get_pos(m_time), p0.get_uv(m_time), p1.get_pos(m_time), p1.get_uv(m_time), p2.get_pos(m_time), p2.get_uv(m_time), m_texture.name, size, 1.f);
		}
	} 
	Primitives::draw_ruler(ed_scale);

	for (unsigned int i = 0; i < vertexes.size(); i++) {
		Primitives::draw_pivot(vertexes[i].get_pos(m_time) * ed_scale, vertexes[i].locked ? Color::Green : Color::Blue, editor.editor_transform.scale);
	}

	for (unsigned int i = 0; i < triangles.size(); i++) {
		Triangle & tri = triangles[i];
		MeshVertex & p0 = vertexes[tri.indexes[0]];	glm::vec2 p0_pos = p0.get_pos(m_time);
		MeshVertex & p1 = vertexes[tri.indexes[1]]; glm::vec2 p1_pos = p1.get_pos(m_time);
		MeshVertex & p2 = vertexes[tri.indexes[2]]; glm::vec2 p2_pos = p2.get_pos(m_time);
		Primitives::draw_line(p0_pos, p1_pos, ed_scale, p0.locked ? Color::Green : Color::Blue, p1.locked ? Color::Green : Color::Blue, 1.f);
		Primitives::draw_line(p1_pos, p2_pos, ed_scale, p1.locked ? Color::Green : Color::Blue, p2.locked ? Color::Green : Color::Blue, 1.f);
		Primitives::draw_line(p2_pos, p0_pos, ed_scale, p2.locked ? Color::Green : Color::Blue, p0.locked ? Color::Green : Color::Blue, 1.f);
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

	float frame_width = (float)width / (float)(frame_count + 1);
	float time_per_frame = 1.f / frame_count;

	for (unsigned int i = 0; i < vertexes.size(); i++) {
		MeshVertex & vertex = vertexes[i];
		if (vertex.locked) {
			for (unsigned int k = 0; k < vertex.vertexes.size(); k++) {
				int frame = int(vertex.vertexes[k].time / time_per_frame);
				float x1 = (frame + 0) * frame_width;
				float x2 = (frame + 1) * frame_width;
				Primitives::draw_region(glm::vec2(x1, 0), glm::vec2(x2, 100), Color::Green, 0.4f);
			}
		}
	}

	int frame = int(m_time / time_per_frame);
	float x1 = (frame + 0) * frame_width;
	float x2 = (frame + 1) * frame_width;
	Primitives::draw_rect(glm::vec2(x1, 0), glm::vec2(x2, 100), Color::Red);

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
		MeshVertex & vertex = vertexes[i];
		if (type == Geometry && glm::length(vertex.get_pos(m_time) * ed_scale - mouse) < 3.f) return true;
		if (type == Texture && glm::length(vertex.get_uv(m_time) * ed_scale - mouse) < 3.f) return true;
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
		MeshVertex & vertex = vertexes[i];
		bool pick = false;
		pick = pick || type == Geometry && glm::length(vertex.get_pos(m_time) * ed_scale - mouse) < 3.f;
		pick = pick || type == Texture && glm::length(vertex.get_uv(m_time) * ed_scale - mouse) < 3.f;
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
		MeshVertex & vertex = vertexes[i];
		bool pick = false;
		pick = pick || (type == Geometry && INSIDE((vertex.get_pos(m_time) * ed_scale), start, end));
		pick = pick || (type == Texture	&& INSIDE((vertex.get_uv(m_time) * ed_scale), start, end));
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

	MeshVertex mv;
	{
		Vertex & v = mv.vertex(m_time);
		v.position = v.texture_uv = position / ed_scale;
	}
	mv.locked = true;
	vertexes.push_back(mv);
}

void Window::add_triangle_from_selection() {
	std::vector<int> selected;
	for (unsigned int i = 0; i < vertexes.size(); i++) {
		MeshVertex & vertex = vertexes[i];
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
		MeshVertex & vertex = vertexes[i];
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
		MeshVertex & vertex = vertexes[i];
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
		MeshVertex & mv = vertexes[i];
		if (mv.locked) {
			Vertex & vertex = mv.vertex(m_time);
			if (type == Geometry)	vertex.position += vector / ed_scale;
			if (type == Texture)	vertex.texture_uv += vector / ed_scale;
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