#include "framework.h"
#include "TileMap.h"
#include "OBJ.h"

TileMap::TileMap(const string& tileSetFilePath, const Vector2u& tileSize, const vector<int>& tiles, const Vector2u& mapSize)
	:tiles(tiles), tileSize(tileSize), mapSize(mapSize)
{
	tileSet = new Texture;
	tileSet->loadFromFile(tileSetFilePath);

	// 타일은 사각형으로 만들 것이기 때문에 Quads
	vertices.setPrimitiveType(Quads);

	//정점의 개수를 미리 정해준다.
	vertices.resize(mapSize.x * mapSize.y * 4);

	InitVertices();
}

void TileMap::InitVertices()
{
	for (unsigned int i = 0; i < mapSize.x; ++i)
	{
		for (unsigned int j = 0; j < mapSize.y; ++j)
		{
			int tileNumber = tiles.data()[i + j * mapSize.x];

			Vertex* quad = &vertices[(i + j * mapSize.x) * 4]; //vertex 4개 당 quad 하나

			float tileX = (float)tileSize.x;
			float tileY = (float)tileSize.y;

			int texU = tileNumber % (tileSet->getSize().x / tileSize.x);
			int texV = tileNumber / (tileSet->getSize().x / tileSize.x);

			quad[0].position = Vector2f(i * tileX, j * tileY); //좌상
			quad[1].position = Vector2f((i + 1) * tileX, j * tileY); //우상
			quad[2].position = Vector2f((i + 1) * tileX, (j + 1) * tileY); //우하
			quad[3].position = Vector2f(i * tileX, (j + 1) * tileY); //좌하

			//텍스처에 대한 텍스코드를 정해준다.
			quad[0].texCoords = Vector2f(texU * tileX, texV * tileY); //좌상
			quad[1].texCoords = Vector2f((texU + 1) * tileX, texV * tileY); //우상
			quad[2].texCoords = Vector2f((texU + 1) * tileX, (texV + 1) * tileY); //우하
			quad[3].texCoords = Vector2f(texU * tileX, (texV + 1) * tileY); //좌하

		}
	}
}

void TileMap::draw(RenderTarget& target, RenderStates states) const
{
	states.transform *= getTransform();
	states.texture = tileSet;

	target.draw(vertices, states);

}
