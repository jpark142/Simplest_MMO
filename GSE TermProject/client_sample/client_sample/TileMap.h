#pragma once
class OBJ;

class TileMap :public Drawable, public Transformable
{
public:
	TileMap() = default;

	//&: �������� �ʰ� �״�� �����ڴ�. const�� �ٴ´�-> ���� ������ ���� ���̴�.(�ջ� �� ��x)
	TileMap(const string& tileSetFilePath, const Vector2u& tileSize, const vector<int>& tiles, const Vector2u& mapSize);
	TileMap(const TileMap&) = delete;
	TileMap& operator=(const TileMap&) = delete;
	~TileMap() = default;

private:


	Texture* tileSet = nullptr; // TileSet : Ÿ�� �ؽ�ó �̹���
	VertexArray vertices; //Ÿ�ϵ��� ������ ��Ƴ��� �����̳�

	Vector2u tileSize;
	Vector2u mapSize;


	vector<int> tiles; //Ÿ���� ��ȣ�� ��Ƴ��� �����̳�

	IntRect imageRect;

private:

	void InitVertices();

public:

	virtual void draw(RenderTarget& target, RenderStates states) const;


};

