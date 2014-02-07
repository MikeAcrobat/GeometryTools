#ifndef ___render_primitives_h____
#define ___render_primitives_h____

class Color {
public:
	float r, g, b;
	Color(float _r_, float _g_, float _b_) {
		r = _r_;
		g = _g_;
		b = _b_;
	}
	static Color Red;
	static Color Green;
	static Color Blue;
	static Color Grey;
};

class Primitives {
public:
	static void draw_ruler(glm::vec2 size);
	static void draw_pivot(glm::vec2 position, Color color, float scale);

	static void draw_line(glm::vec2 p1, glm::vec2 p2, glm::vec2 scale, Color color1, Color color2, float opacity);
	static void draw_triangle(glm::vec2 p1, glm::vec2 uv1, glm::vec2 p2, glm::vec2 uv2, glm::vec2 p3, glm::vec2 uv3, GLuint texture, glm::vec2 scale, float opacity);
	static void draw_rect(glm::vec2 p1, glm::vec2 p2, Color color);
	static void draw_region(glm::vec2 p1, glm::vec2 p2, Color color, float alpha);
};

#endif // ___render_primitives_h____