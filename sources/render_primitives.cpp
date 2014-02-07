#include "pch.h"
#include "render_primitives.h"

Color Color::Red	= Color(1.f, 0.f, 0.f);
Color Color::Green	= Color(0.f, 1.f, 0.f);
Color Color::Blue	= Color(0.f, 0.f, 1.f);
Color Color::Grey	= Color(.5f, .5f, .5f);

std::vector<glm::vec3> init_ruler_vertexes(glm::vec2 size) {
	std::vector<glm::vec3> vertexes;
	vertexes.push_back(glm::vec3(-25.f,			0.f,			0.f));
	vertexes.push_back(glm::vec3(size.x + 25.f, 0.f,			0.f));
	vertexes.push_back(glm::vec3(0.f,		    -25.f,			0.f));
	vertexes.push_back(glm::vec3(0.f,			size.y + 25.f,	0.f));

	vertexes.push_back(glm::vec3(-25.f,			size.y,			0.f));
	vertexes.push_back(glm::vec3(size.x + 25.f, size.y,			0.f));
	vertexes.push_back(glm::vec3(size.x,		-25.f,			0.f));
	vertexes.push_back(glm::vec3(size.x,		size.y + 25.f,	0.f));
	
	int count = 20;
	float space = 100.f;
	space = size.x / count;
	for (int i = 0; i < count + 1; i++) {
		vertexes.push_back(glm::vec3( i * space, i % 2 == 1 ?  2.f :  4.f, 0.f));
		vertexes.push_back(glm::vec3( i * space, i % 2 == 1 ? -2.f : -4.f, 0.f));
	}

	space = size.y / count;
	for (int i = 0; i < count + 1; i++) {
		vertexes.push_back(glm::vec3( i % 2 == 1 ?  2.f :  4.f, i * space, 0.f));
		vertexes.push_back(glm::vec3( i % 2 == 1 ? -2.f : -4.f, i * space, 0.f));
	}

	return vertexes;
}

void Primitives::draw_ruler(glm::vec2 size) {

	std::vector<glm::vec3> & vertexes = init_ruler_vertexes(size);

	glDisableClientState(GL_COLOR_ARRAY);
	glColor3f(0.f, 0.f, 0.f);
	glVertexPointer(3, GL_FLOAT, 0, vertexes.data());
	glDrawArrays(GL_LINES, 0, vertexes.size());
	glEnableClientState(GL_COLOR_ARRAY);
}

void Primitives::draw_pivot(glm::vec2 position, Color color, float scale) {
	static glm::vec2 half_size(32, 32);
	glm::vec2 p1 = position - half_size / scale;
	glm::vec2 p2 = position + half_size / scale;
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

void Primitives::draw_line(glm::vec2 p1, glm::vec2 p2, glm::vec2 scale, Color color1, Color color2, float opacity) {
	float vertexes[] = {
		p1.x * scale.x,	p1.y * scale.y,	0.f,
		p2.x * scale.x,	p2.y * scale.y,	0.f,
	};

	float colors[] = {
		color1.r, color1.g, color1.b, opacity,
		color2.r, color2.g, color2.b, opacity,
	};

	glColorPointer(4, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glDrawArrays(GL_LINES, 0, 2);
}

void Primitives::draw_triangle(glm::vec2 p1, glm::vec2 uv1, glm::vec2 p2, glm::vec2 uv2, glm::vec2 p3, glm::vec2 uv3, GLuint texture, glm::vec2 scale, float opacity) {

	float vertexes[] = {
		p1.x * scale.x,	p1.y * scale.y,	0.f,
		p2.x * scale.x,	p2.y * scale.y,	0.f,
		p3.x * scale.x,	p3.y * scale.y,	0.f,
	};
	
	float colors[] = {
		1.f,	1.f,	1.f,	opacity,
		1.f,	1.f,	1.f,	opacity,
		1.f,	1.f,	1.f,	opacity,
	};

	float uvs[] = {
		uv1.x,	uv1.y,
		uv2.x,	uv2.y,
		uv3.x,	uv3.y,
	};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
	glColorPointer(4, GL_FLOAT, 0, colors);
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

void Primitives::draw_region(glm::vec2 p1, glm::vec2 p2, Color color, float alpha) {
	float vertexes[] = {
		p1.x,	p1.y,	0.f,
		p2.x,	p1.y,	0.f,
		p2.x,	p2.y,	0.f,
		p1.x,	p1.y,	0.f,
		p1.x,	p2.y,	0.f,
		p2.x,	p2.y,	0.f,
	};

	float colors[] = {
		color.r, color.g, color.b, alpha,
		color.r, color.g, color.b, alpha,
		color.r, color.g, color.b, alpha,
		color.r, color.g, color.b, alpha,
		color.r, color.g, color.b, alpha,
		color.r, color.g, color.b, alpha,
	};

	glColorPointer(4, GL_FLOAT, 0, colors);
	glVertexPointer(3, GL_FLOAT, 0, vertexes);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}