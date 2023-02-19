#include "framework.h"
#include "Mycharacter.h"

MyCharacter::MyCharacter()
{
	m_showing = false;
}

MyCharacter::MyCharacter(Texture& t, int x, int y, int x2, int y2)
{
	m_showing = false;
	m_sprite.setTexture(t);
	m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	//set_name("NONAME");
	m_mess_end_time = chrono::system_clock::now();


}


