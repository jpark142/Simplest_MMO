#pragma once
class OBJ : public Sprite
{
public:
	OBJ();
	OBJ(const string& textureFilePath);
	OBJ(const string& textureFilePath, const Vector2f& position);
	virtual ~OBJ() = default;

protected:

	bool isActive = true;

protected:

	Texture* texture = nullptr;

	// debugBox Vertices
	Vertex* vertices = nullptr;
	Color boxColor = Color::Red;
	bool debugBox = true;

public:

	virtual void Destroy();

	void SetDebugBoxActive(bool isActive);
	void SetActive(bool isActive);
	bool IsActive();

	void SetBoxColor(const Color& color);
	void SetBoxColor(const Uint8& r, const Uint8& g, const Uint8& b, const Uint8& a);


	virtual void Update(const float& deltaTime);
	virtual void Update(const Vector2f& mousePosition);

	virtual void Render(RenderTarget* target);
};

