#pragma once

class MyCharacter : public Sprite
{
public:

	MyCharacter();
	//MyCharacter(const string& textureFilePath);
	MyCharacter(Texture& t, int x, int y, int x2, int y2);

	virtual ~MyCharacter() = default;

private:

	bool m_showing;
	Sprite m_sprite;
	Text m_name;
	Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;

	Vertex* vertices = nullptr;
	bool debugBox = true;
public:


	void Update(const float& deltaTime);

	void Render(RenderTarget* target);


public:
	int m_x, m_y;
	int m_level;
	bool is_collide = false;
	char npc_type;

public:
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x) * 32.0f;
		float ry = (m_y) * 32.0f;
		//cout << rx << ", " << ry << endl;

		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx - 15, ry - 30);
			g_window->draw(m_name);
		}
		else {
			m_chat.setPosition(rx - 15, ry - 30);
			g_window->draw(m_chat);
		}
	}
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setCharacterSize(20);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}
	void set_chat(const char str[]) { //Hello 텍스트
		m_chat.setFont(g_font);
		m_chat.setString(str);
		if (isComeOn == true) {
			m_chat.setFillColor(sf::Color(255, 140, 0));
			isComeOn = false;
		}

		else
			m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setStyle(sf::Text::Bold);

		//3초 동안 그려줄 것이다.
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
};

