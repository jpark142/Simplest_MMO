#include "framework.h"
#include "TileMap.h"
#include "WallManager.h"
#include "Mycharacter.h"

sf::TcpSocket socket;

constexpr auto BUF_SIZE = 256;
constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 32;
constexpr auto WINDOW_WIDTH = TILE_WIDTH * SCREEN_WIDTH + 10;   // size of window
constexpr auto WINDOW_HEIGHT = TILE_WIDTH * SCREEN_WIDTH + 10;
//constexpr auto BUF_SIZE = MAX_BUFFER;

//======================================
TileMap* my_map = nullptr;
View* gameView = nullptr;

WallManager* wallMgr = nullptr;

Clock clockk;

static float deltaTime = 0.f;
float elapsedTime = 0.f;
Clock* engineClock = nullptr;
unsigned int frame = 0;
unsigned int FPS = 0;


RectangleShape rect_shape;

//======================================

int g_myid;

MyCharacter* avatar = new MyCharacter;
MyCharacter players[MAX_USER + MAX_NPC];


//OBJECT white_tile;
//OBJECT black_tile;

//sf::Texture* board;
sf::Texture* player_pieces;
Texture* monster_pieces;
Texture* agro_monster_pieces;

void create_walls()
{
	wallMgr = new WallManager();

	WallObject* object;

	for (int i = 0; i < 3000.f; ++i) {
		float temp_x = rand() % 60000 + 1200;
		float temp_y = rand() % 60000 + 1200;
		object = new WallObject("Textures/Object/wall1.png", { temp_x, temp_y });
		wallMgr->SetWall(object);
	}
	
	object = new WallObject("Textures/Object/wall3.png", { 500.f, 500.f });
	wallMgr->SetWall(object);

	//포탈
	object = new WallObject("Textures/HUD/portal.png", { 500.f, 550.f });
	wallMgr->SetWall(object);

	//핫스팟 가두리 양식장
	for (int y = 30; y < 1200; y += 30)
	{
		for (int x = 30; x < 1200; x += 30)
		{
			if (x == 30 || x == 1170 || y == 30 || y == 1170)
			{
				object = new WallObject("Textures/Object/wall1.png", { (float)y, (float)x });
				wallMgr->SetWall(object);
			}
			else
				continue;
		}
	}
}


void client_initialize()
{
	player_pieces = new sf::Texture;
	monster_pieces = new Texture;
	agro_monster_pieces = new Texture;
	
	if (false == g_font.loadFromFile("Font/Maplestory Bold.ttf")) {
		cout << "Font Loading Error!\n";
		while (true);
	}

	player_pieces->loadFromFile("Textures/Character/pink_warrior/char2.png");
	monster_pieces->loadFromFile("Textures/Character/monster/monster.png");
	agro_monster_pieces->loadFromFile("Textures/Character/monster/agro.png");

	*avatar = MyCharacter{ *player_pieces, 0, 0, 32, 32 };
	for (auto& pl : players) {
		pl = MyCharacter{ *player_pieces, 0, 0, 32, 32 };
	}

	create_walls();

	//맵 초기화
	vector<int> levels;

	for (int i = 0; i < 2000 * 2000; ++i)
	{
		//56: 잔디
		levels.push_back(56);
	}
	my_map = new TileMap("Textures/Map/tileSet.png", { 32, 32 }, levels, { 2000, 2000 });
	

	FloatRect viewRect{ 0.f, 0.f, 1000.f, 1000.f };
	gameView = new View(viewRect);
}

void client_finish()
{
	delete avatar;
	//delete board;
	delete player_pieces;
	delete monster_pieces;
	delete agro_monster_pieces;

	delete my_map;
	delete wallMgr;
	delete gameView;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_PACKET_LOGIN_OK:
	{
		sc_packet_login_ok* packet = reinterpret_cast<sc_packet_login_ok*>(ptr);
		g_myid = packet->id;
		avatar->m_x = packet->x;
		avatar->m_y = packet->y;
		avatar->m_level = packet->level;
		//g_x_origin = packet->x - (SCREEN_WIDTH / 2);
		//g_y_origin = packet->y - (SCREEN_WIDTH / 2);
		avatar->move(packet->x, packet->y);
		avatar->show();
		cout << "로그인 성공" << endl;

		break;

	}
	case SC_PACKET_LOGIN_FAIL:
	{
		sc_packet_login_fail* packet = reinterpret_cast<sc_packet_login_fail*>(ptr);
		
		if (packet->reason == 0) //아이디 중복
			cout << "아이디 중복!" << endl;
		cout << "로그인 실패!" << endl;


		break;
	}
	case SC_PACKET_PUT_OBJECT:
	{
		sc_packet_put_object* packet = reinterpret_cast<sc_packet_put_object*>(ptr);
		int id = packet->id;

		if (id < MAX_USER) { // PLAYER
			players[id].set_name(packet->name);

			players[id].move(packet->x, packet->y);
			players[id].show();
			//players[id] = OBJECT{ *player_pieces, 0, 0, 32, 32 };

		}
		else {  // NPC
			if (packet->npc_type == NPC_PEACE)
				players[id] = MyCharacter{ *monster_pieces, 0, 0, 32, 32 };
			else if (packet->npc_type == NPC_AGRO) {
				//cout << players[id].m_x << ", " << players[id].m_y << endl;
				players[id] = MyCharacter{ *agro_monster_pieces, 0, 0, 32, 32 };

			}
			else
				players[id] = MyCharacter{ *monster_pieces, 0, 0, 32, 32 };


			players[id].npc_type = packet->npc_type;
			players[id].set_name(packet->name);
			players[id].move(packet->x, packet->y);
			players[id].show();
		}
		
		break;
	}
	case SC_PACKET_MOVE:
	{
		sc_packet_move* packet = reinterpret_cast<sc_packet_move*>(ptr);
		int other_id = packet->id;
		if (other_id == g_myid) {
			avatar->move(packet->x, packet->y);
			//g_x_origin = my_packet->x - (SCREEN_WIDTH / 2);
			//g_y_origin = my_packet->y - (SCREEN_WIDTH / 2);
		}
		else if (other_id < MAX_USER) {
			players[other_id].move(packet->x, packet->y);

		}
		else {

			players[other_id].move(packet->x, packet->y);
		}

		break;
	}
	case SC_PACKET_STATUS_CHANGE:
	{
		sc_packet_status_change* packet = reinterpret_cast<sc_packet_status_change*> (ptr);

		avatar->m_level = packet->level;
		avatar->m_x = packet->x;
		avatar->m_y = packet->y;
		avatar->move(packet->x, packet->y);
		

		break;
	}

	case SC_PACKET_REMOVE_OBJECT:
	{
		sc_packet_remove_object* packet = reinterpret_cast<sc_packet_remove_object*>(ptr);
		int other_id = packet->id;
		if (other_id == g_myid) {
			avatar->hide();
		}
		else if (other_id < MAX_USER) {
			players[other_id].hide();
		}
		else {
			players[other_id].hide();
		}
		break;
	}

	case SC_PACKET_CHAT:
	{
		sc_packet_chat* packet = reinterpret_cast<sc_packet_chat*>(ptr);
		int other_id = packet->id;

		if (packet->chat_type == 1) {
			isComeOn = true;
		}

		if (other_id == g_myid) {
			avatar->set_chat(packet->message);
		}
		else if (other_id < MAX_USER) {
			players[other_id].set_chat(packet->message);
		}
		else {
			players[other_id].set_chat(packet->message);
		}
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

bool client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		while (true);
	}
	if (recv_result == sf::Socket::Disconnected)
	{
		wcout << L"서버 접속 종료.\n";
		return false;
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	
	//맵 그리기
	if (my_map)
	{
		g_window->draw(*my_map);
	}

	//장애물 그리기
	if (wallMgr)
	{
		wallMgr->Render(g_window);
	}

	avatar->draw();
	for (auto& pl : players) pl.draw(); //npc들 그리기

	//TEXT====================================================
	sf::Text pos_text, level_text;
	pos_text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar->m_x, avatar->m_y);
	pos_text.setString(buf);
	pos_text.setCharacterSize(15);
	pos_text.setPosition(avatar->m_x * 32.f - 15.f, avatar->m_y *32.f + 30.f);
	
	level_text.setFont(g_font);
	sprintf_s(buf, "Lv.: %d", avatar->m_level);
	level_text.setString(buf);
	level_text.setCharacterSize(15);
	level_text.setFillColor(sf::Color(224, 255, 255));
	level_text.setPosition(avatar->m_x *32.f + 32.f, avatar->m_y * 32.f);

	g_window->draw(pos_text);
	g_window->draw(level_text);
	//========================================================


	return true;
}


void send_teleport_packet()
{
	cs_packet_teleport packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_TELEPORT;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

void Update()
{

	deltaTime = clockk.getElapsedTime().asSeconds();
	clockk.restart();
	
	//장애물 업데이트
	if (wallMgr)
	{
		wallMgr->Update(deltaTime);

	}

	//텔레포트
	if ((avatar->m_x > 13 && avatar->m_x < 17)
		&& (avatar->m_y > 17 && avatar->m_y < 20))
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::T))
		{
			cout << "텔레포트" << endl;
			send_teleport_packet();
		}
	}

}

void send_move_packet(char dr)
{
	cs_packet_move packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_MOVE;
	packet.direction = dr;
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

void send_login_packet(char* name)
{
	cs_packet_login packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGIN;
	packet.is_dummy = 0; //0.사람 1.더미
	strcpy_s(packet.name, name);
	size_t sent = 0;
	socket.send(&packet, sizeof(packet), sent);
}

int main()
{
	wcout.imbue(locale("korean"));

	char ip_addr[20];
	cout << "ip주소를 입력하세요: ";
	cin >> ip_addr;
	sf::Socket::Status status = socket.connect(ip_addr, SERVER_PORT);


	socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		while (true);
	}

	client_initialize();
	
	char player_id[20];
	cout << "ID를 입력하세요: ";
	cin >> player_id;

	send_login_packet(player_id); //로그인 패킷


	avatar->set_name(player_id);
	sf::RenderWindow window(sf::VideoMode(1050, 1050), "Game Server Term Project");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					break;
				case sf::Keyboard::Right:
					direction = 3;
					break;
				case sf::Keyboard::Up:
					direction = 0;
					break;
				case sf::Keyboard::Down:
					direction = 1;
					break;
				case Keyboard::Space:
					direction = 99; //attack
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
				if (-1 != direction)
					send_move_packet(direction);
			}
			if (event.type == Event::KeyReleased)
			{
				int direction_released = -1;
				switch (event.key.code) {
				case Keyboard::Space:
					direction_released = 98; //attack stop
					break;
				}
				if (-1 != direction_released)
					send_move_packet(direction_released);
			}
		}
		Update();
	
		gameView->setCenter({ (float)avatar->m_x * 32.f, (float)avatar->m_y *32.f});
		window.setView(*gameView);

		window.clear();
		if (false == client_main())
			window.close();
		window.display();
	}
	client_finish();

	return 0;
}