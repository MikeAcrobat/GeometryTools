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
};

class Primitives {
public:
	static void draw_ruler();
	static void draw_pivot(glm::vec2 position, Color color);

	static void draw_line(glm::vec2 p1, glm::vec2 p2, Color color1, Color color2);
	static void draw_triangle(Vertex p1, Vertex p2, Vertex p3, GLuint texture);
	static void draw_rect(glm::vec2 p1, glm::vec2 p2, Color color);
};

#endif // ___render_primitives_h____