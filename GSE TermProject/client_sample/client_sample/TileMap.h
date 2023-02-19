#pragma once
class OBJ;

class TileMap :public Drawable, public Transformable
{
public:
	TileMap() = default;

	//&: 복사하지 않고 그대로 들고오겠다. const가 붙는다-> 절대 변하지 않을 것이다.(손상 될 일x)
	TileMap(const string& tileSetFilePath, const Vector2u& tileSize, const vector<int>& tiles, const Vector2u& mapSize);
	TileMap(const TileMap&) = delete;
	TileMap& operator=(const TileMap&) = delete;
	~TileMap() = default;

private:


	Texture* tileSet = nullptr; // TileSet : 타일 텍스처 이미지
	VertexArray vertices; //타일들의 정점을 모아놓은 컨테이너

	Vector2u tileSize;
	Vector2u mapSize;


	vector<int> tiles; //타일의 번호를 모아놓은 컨테이너

	IntRect imageRect;

private:

	void InitVertices();

public:

	virtual void draw(RenderTarget& target, RenderStates states) const;


};

