#include "framework.h"
#include "WallObject.h"

WallObject::WallObject(const string& textureFilePath)
	:OBJ(textureFilePath)
{
}

WallObject::WallObject(const string& textureFilePath, const Vector2f& position)
	: OBJ(textureFilePath, position)
{
}

void WallObject::Destroy()
{
	OBJ::Destroy();
}

void WallObject::Update(const float& deltaTime)
{
	OBJ::Update(deltaTime);
}

void WallObject::Update(const Vector2f& mousePosition)
{
	OBJ::Update(mousePosition);
}

void WallObject::Render(RenderTarget* target)
{
	OBJ::Render(target);
}