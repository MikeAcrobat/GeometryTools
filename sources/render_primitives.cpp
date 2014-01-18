#include "pch.h"
#include "render_primitives.h"

Color Color::Red	= Color(1.f, 0.f, 0.f);
Color Color::Green	= Color(0.f, 1.f, 0.f);
Color Color::Blue	= Color(0.f, 0.f, 1.f);

std::vector<glm::vec3> init_ruler_vertexes() {
	std::vector<glm::vec3> vertexes;
	vertexes.push_back(glm::vec3(-25.f, 0.f, 0.f));
	vertexes.push_back(glm::vec3(125.f, 0.f, 0.f));
	vertexes.push_back(glm::vec3(0.f, -25.f, 0.f));
	vertexes.push_back(glm::vec3(0.f, 125.f, 0.f));

	vertexes.push_back(glm::vec3(-25.f, 100.f, 0.f));
	vertexes.push_back(glm::vec3(125.f, 100.f, 0.f));
	vertexes.push_back(glm::vec3(100.f, -25.f, 0.f));
	vertexes.push_back(glm::vec3(100.f, 125.f, 0.f));
	
	int count = 20;
	float space = 100.f / count;
	for (int i = 0; i < count + 1; i++) {
		vertexes.push_back(glm::vec3( i * space, i % 2 == 1 ?  2.f :  4.f, 0.f));
		vertexes.push_back(glm::vec3( i * space, i % 2 == 1 ? -2.f : -4.f, 0.f));
	}

	for (int i = 0; i < count + 1; i++) {
		vertexes.push_back(glm::vec3( i % 2 == 1 ?  2.f :  4.f, i * space, 0.f));
		vertexes.push_back(glm::vec3( i % 2 == 1 ? -2.f : -4.f, i * space, 0.f));
	}

	return vertexes;
}

void Primitives::draw_ruler() {

	static std::vector<glm::vec3> & vertexes = init_ruler_vertexes();

	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.f, 0.f, 0.f);
	glVertexPointer(3, GL_FLOAT, 0, vertexes.data());
	glDrawArrays(GL_LINES, 0, vertexes.size());
	glEnableClientState(GL_COLOR_ARRAY);
}

void Primitives::draw_pivot(glm::vec2 position, Color color) {
	static glm::vec2 half_size(1, 1);
	glm::vec2 p1 = position - half_size;
	glm::vec2 p2 = position + half_size;
	float vertexes[] = {
		p1.x,	p1.y,	0.f,
		p2.x,	p1.y,	0.f,
		p1.x,	p2.y,	0.f,
		p2.x,	p2.y,	0.f,
	};

	float colors[] = {
		color.r, color.g, color.b,
		color.r, color.g, color.b,
		color.r, color.g, color.b,
		color.r, color.g, color.b,
	};

	glColorPointer(3, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Primitives::draw_line(glm::vec2 p1, glm::vec2 p2, Color color1, Color color2) {
	float vertexes[] = {
		p1.x,	p1.y,	0.f,
		p2.x,	p2.y,	0.f,
	};

	float colors[] = {
		color1.r, color1.g, color1.b,
		color2.r, color2.g, color2.b,
	};

	glColorPointer(3, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glDrawArrays(GL_LINES, 0, 2);
}

void Primitives::draw_triangle(Vertex p1, Vertex p2, Vertex p3, GLuint texture) {

	float vertexes[] = {
		p1.position.x,	p1.position.y,	0.f,
		p2.position.x,	p2.position.y,	0.f,
		p3.position.x,	p3.position.y,	0.f,
	};
	
	static float colors[] = {
		1.f,	1.f,	1.f,
		1.f,	1.f,	1.f,
		1.f,	1.f,	1.f,
	};

	float uvs[] = {
		p1.texture_uv.x,	p1.texture_uv.y,
		p2.texture_uv.x,	p2.texture_uv.y,
		p3.texture_uv.x,	p3.texture_uv.y,
	};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
	glColorPointer(3, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glTexCoordPointer(2, GL_FLOAT, 0, uvs);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void Primitives::draw_rect(glm::vec2 p1, glm::vec2 p2, Color color) {
	float vertexes[] = {
		p1.x,	p1.y,	0.f,
		p2.x,	p1.y,	0.f,
		p2.x,	p2.y,	0.f,
		p1.x,	p2.y,	0.f,
		p1.x,	p1.y,	0.f,
	};

	float colors[] = {
		color.r, color.g, color.b,
		color.r, color.g, color.b,
		color.r, color.g, color.b,
		color.r, color.g, color.b,
		color.r, color.g, color.b,
	};

	glColorPointer(3, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glDrawArrays(GL_LINE_STRIP, 0, 5);
}
