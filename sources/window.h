#ifndef ___window___
#define ___window___

const float default_scale = 10.f;

enum EditorType {
	Geometry,
	Texture,
	Timeline,
	EditorTypeCount
};

struct EditorTransformation {
	glm::vec2 position;
	float scale;
};

struct Vertex {
	glm::vec2	position;
	glm::vec2	texture_uv;
	float		time;
};

struct MeshVertex {
	std::vector<Vertex> vertexes;
	bool locked;

	glm::vec2	get_pos(float time);
	glm::vec2	get_uv(float time);
	Vertex &	vertex(float time);
	void		sort();
};

struct Triangle {
	int indexes[3];

	bool compare(int index1, int index2, int index3);
	bool contains(int index);
};

struct Editor {
	EditorTransformation	editor_transform;
	SDL_Window *			window;
	EditorType				type;

	Editor() {
		window = nullptr;
	}

	glm::mat4 view_matrix();
	glm::vec2 mouse_global_to_local(float mouse_x, float mouse_y);
};

struct GeomState {
	std::vector<MeshVertex>	vertexes;
	std::vector<Triangle>	triangles;
};

class Window {
	std::string				m_xml_animation_path;

	struct Image {
		glm::vec2	size;
		GLuint		name;
		bool		loaded;
	};
	Image					m_texture;
	Image					m_background;
	bool					m_background_edit;
	glm::vec2				m_background_offset;

	bool					m_show_trace;
	bool					m_show_pivots;

	bool					m_play;
	float					m_time;

	bool					wheel_lock;
	bool					mouse_down;
	bool					moving_geometry;
	
	EditorType				selection_editor;
	bool					addictive_selection;
	bool					multiple_selection;
	bool					multiple_selection_init;
	glm::vec2				start_selection, end_selection;
	
	std::vector<MeshVertex>	vertexes;
	std::vector<Triangle>	triangles;

	std::deque<GeomState>	state_stack;

	Editor * get_editor(Uint32 windowID);
public:

	Editor editors[EditorTypeCount];
	SDL_GLContext context;

	Window();
	void work();
	bool handle_events();

	void render_texture();
	void render_geometry();
	void render_timeline();

	void load_texture(const std::string & path, Image & image);

	bool hit_test(EditorType type, glm::vec2 mouse);
	int  select_vertex(EditorType type, glm::vec2 mouse, bool add);
	void select_vertex_range(EditorType type, glm::vec2 start, glm::vec2 end, bool add);
	void add_vertex(glm::vec2 position);
	void add_triangle_from_selection();
	void del_triangle_from_selection();
	void del_vertexes_from_selection();

	void move_selected_vertexes(EditorType type, glm::vec2 vector);

	void push_state();
	void pop_state();

	void move_editor(int x_offset, int y_offset);
	void reset_views();

	void load_geometry();
	void save_geometry();
};

#endif // ___window___