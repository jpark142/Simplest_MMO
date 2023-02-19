#include "framework.h"
#include "WallManager.h"
#include "Mycharacter.h"

WallManager::WallManager(const size_t& WallCount)
{
	for (auto i = 0; i < WallCount; ++i)
	{
		walls.push_back(new WallObject("Textures/Object/wall3.png"));
	}
}

void WallManager::Destroy()
{
	for (auto& i : walls)
	{
		i->Destroy();
	}
}

void WallManager::SetWall(WallObject* object)
{
	if (object)
	{
		walls.push_back(object);
	}
}

vector<WallObject*>* WallManager::GetWalls()
{
	return &walls;
}

void WallManager::Update(const float& deltaTime)
{
	for (auto& i : walls)
	{
		i->Update(deltaTime);
	}
}

void WallManager::Update(const Vector2f& mousePosition)
{
	for (auto& i : walls)
	{
		i->Update(mousePosition);
	}
}

void WallManager::CollisionUpdate(OBJ* object)
{
	for (auto& wall : walls)
	{
		if (dynamic_cast<MyCharacter*>(object))
		{
			if (wall->getGlobalBounds().intersects(object->getGlobalBounds()))
			{
				cout << "Ãæµ¹" << endl;

			}
		}
	}
}

void WallManager::Render(RenderTarget* target)
{
	for (auto& i : walls)
	{
		i->Render(target);
	}
}
