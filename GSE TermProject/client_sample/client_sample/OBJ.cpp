#include "framework.h"
#include "OBJ.h"

OBJ::OBJ()
{
	vertices = new Vertex[5];
}

OBJ::OBJ(const string& textureFilePath)
{
	this->texture = new Texture;

	if (this->texture->loadFromFile(textureFilePath))
	{
		setTexture(*this->texture);

		// GlobalBoundBox
		auto gbb = this->getGlobalBounds();

		setOrigin(gbb.width / 2.f, gbb.height / 2.f);
		vertices = new Vertex[5];
		vertices[0] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
		vertices[1] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top), boxColor));
		vertices[2] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top + gbb.height), boxColor));
		vertices[3] = (Vertex(Vector2f(gbb.left, gbb.top + gbb.height), boxColor));
		vertices[4] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
	}
}


OBJ::OBJ(const string& textureFilePath, const Vector2f& position)
	:OBJ(textureFilePath)
{
	setPosition(position);
}

void OBJ::Destroy()
{
	DELETE(texture);
	DELETE(vertices);
}

void OBJ::SetDebugBoxActive(bool isActive)
{
	this->debugBox = isActive;
}

void OBJ::SetActive(bool isActive)
{
	this->isActive = isActive;
}

bool OBJ::IsActive()
{
	return isActive;
}

void OBJ::SetBoxColor(const Color& color)
{
	this->boxColor = color;
}

void OBJ::SetBoxColor(const Uint8& r, const Uint8& g, const Uint8& b, const Uint8& a)
{
	SetBoxColor(Color(r, g, b, a));
}

void OBJ::Update(const float& deltaTime)
{
	auto gbb = this->getGlobalBounds();
	vertices[0] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
	vertices[1] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top), boxColor));
	vertices[2] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top + gbb.height), boxColor));
	vertices[3] = (Vertex(Vector2f(gbb.left, gbb.top + gbb.height), boxColor));
	vertices[4] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
}

void OBJ::Update(const Vector2f& mousePosition)
{
	auto gbb = this->getGlobalBounds();
	vertices[0] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
	vertices[1] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top), boxColor));
	vertices[2] = (Vertex(Vector2f(gbb.left + gbb.width, gbb.top + gbb.height), boxColor));
	vertices[3] = (Vertex(Vector2f(gbb.left, gbb.top + gbb.height), boxColor));
	vertices[4] = (Vertex(Vector2f(gbb.left, gbb.top), boxColor));
}

void OBJ::Render(RenderTarget* target)
{
	if (target && isActive)
	{
		target->draw(*this);

		if (vertices && debugBox)
		{
			target->draw(this->vertices, 5, LinesStrip);
		}
	}
}