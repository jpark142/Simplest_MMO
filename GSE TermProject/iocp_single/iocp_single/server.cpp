#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <array>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include <chrono>
#include <cmath>
#include <random>

#define UNICODE  
#include <sqlext.h>  
#define NAME_LEN 20  

#define HOTSPOT_SIZE 30

#include <string>

#include "protocol.h"
using namespace std;
#pragma comment (lib, "WS2_32.LIB")
#pragma comment (lib, "MSWSock.LIB")

//LUA
extern "C" {
#include "include\lua.h"
#include "include\lauxlib.h"
#include "include\lualib.h"
}

//DB
struct DB
{
	int id;
	short level;
	int x, y;
	char userName[20];
};
vector<DB> g_db;

vector<char*>login_players;

const int BUFSIZE = 256;
const int RANGE = 15; //시야처리 15x15
const int AGRO_RANGE = 11; //어그로 범위 11x11

HANDLE g_h_iocp;
SOCKET g_s_socket;

void show_error() {
	printf("error\n");
}

void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

char* DeleteSpace(char s[])
{
	char* str = (char*)malloc(sizeof(s));
	int i, k = 0;

	for (i = 0; i < strlen(s); i++)
		if (s[i] != ' ') str[k++] = s[i];

	str[k] = '\0';
	return str;
}

void sql_player_data_read()
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;
	SQLINTEGER p_id;
	SQLSMALLINT p_level;
	SQLWCHAR p_name[NAME_LEN];
	SQLSMALLINT p_x;
	SQLSMALLINT p_y;

	SQLLEN cbName = 0, cbP_ID = 0, cbP_Level = 0, cbP_X = 0, cbP_Y = 0;

	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));


	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					std::cout << "ODBC Connected.\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


					retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"EXEC select_highlevel 0", SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						cout << "Select Success.\n";

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &p_id, 100, &cbP_ID);
						retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, p_name, NAME_LEN, &cbName);
						retcode = SQLBindCol(hstmt, 3, SQL_C_SHORT, &p_level, 10, &cbP_Level);
						retcode = SQLBindCol(hstmt, 4, SQL_C_SHORT, &p_x, 100, &cbP_X);
						retcode = SQLBindCol(hstmt, 5, SQL_C_SHORT, &p_y, 100, &cbP_Y);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
								HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);

							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								wprintf(L"%d: %d '%s' %d %d %d\n", i + 1, p_id, p_name, p_level, p_x, p_y);
								char* temp;
								int nChars = WideCharToMultiByte(CP_ACP, 0, p_name, -1, NULL, 0, NULL, NULL);
								temp = new char[nChars];
								WideCharToMultiByte(CP_ACP, 0, p_name, -1, temp, nChars, 0, 0);

								DB data;
								data.id = p_id;
								data.level = p_level;
								data.x = p_x;
								data.y = p_y;

								strcpy_s(data.userName, DeleteSpace(temp));

								g_db.emplace_back(data);
								delete[]temp;

							}
							else
								break;
						}
					}
					else
					{
						//Error check
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void sql_player_data_save(wchar_t* name, int level, int x, int y)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	SQLWCHAR query[1000];

	wsprintf(query, L"UPDATE player_data SET player_level = %d, player_x = %d, player_y = %d WHERE player_name = '%s'", level, x, y, name);


	for (auto& d : g_db)
	{
		const WCHAR* temp;
		int nChars = MultiByteToWideChar(CP_ACP, 0, d.userName, -1, NULL, 0);
		temp = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, d.userName, -1, (LPWSTR)temp, nChars);
		if (wcsncmp(temp, name, sizeof(name)) == 0)
		{
			d.x = x;
			d.y = y;
			d.level = level;
		}

		delete[]temp;
	}

	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));


	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					std::cout << "ODBC Connected.\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


					retcode = SQLExecDirect(hstmt, query, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						char* temp2;
						int nChars = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
						temp2 = new char[nChars];
						WideCharToMultiByte(CP_ACP, 0, name, -1, temp2, nChars, 0, 0);

						//level, x, y 저장
						cout << "Save player_name = " << temp2 << ", player_level = " << level << ", player_x = " << x << ", player_y = " << y << "\n";

						delete[]temp2;
					}
					else {
						//Error check
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void sql_player_level_realtime_update(wchar_t* name, int level)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	SQLWCHAR query[1000];

	wsprintf(query, L"UPDATE player_data SET player_level = %d WHERE player_name = '%s'", level, name);


	for (auto& d : g_db)
	{
		const WCHAR* temp;
		int nChars = MultiByteToWideChar(CP_ACP, 0, d.userName, -1, NULL, 0);
		temp = new WCHAR[nChars];
		MultiByteToWideChar(CP_ACP, 0, d.userName, -1, (LPWSTR)temp, nChars);
		if (wcsncmp(temp, name, sizeof(name)) == 0)
		{

			d.level = level;
		}

		delete[]temp;
	}

	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));


	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					std::cout << "ODBC Connected.\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


					retcode = SQLExecDirect(hstmt, query, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						char* temp2;
						int nChars = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
						temp2 = new char[nChars];
						WideCharToMultiByte(CP_ACP, 0, name, -1, temp2, nChars, 0, 0);

						//level 실시간 업데이트
						cout << "Update " << temp2 << " level -> " << level << endl;

						delete[]temp2;
					}
					else {
						//Error check
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void sql_new_player_insert(int id, wchar_t* name, int level, int x, int y)
{
	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLRETURN retcode;

	SQLWCHAR query[1000];
	wsprintf(query, L"INSERT INTO player_data (player_id, player_name, player_level, player_x, player_y) VALUES (%d, '%s', %d, %d, %d)",
		id, name, level, x, y);


	setlocale(LC_ALL, "korean");
	std::wcout.imbue(std::locale("korean"));


	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					std::cout << "ODBC Connected.\n";
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);


					retcode = SQLExecDirect(hstmt, query, SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						char* temp2;
						int nChars = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
						temp2 = new char[nChars];
						WideCharToMultiByte(CP_ACP, 0, name, -1, temp2, nChars, 0, 0);

						//New 플레이어 Inesrt
						cout << "New Player Added!" << endl;

						DB data;
						data.id = id;
						data.level = level;
						data.x = x;
						data.y = y;
						strcpy_s(data.userName, DeleteSpace(temp2));

						g_db.emplace_back(data);
						delete[]temp2;
					}
					else {
						//Error check
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}
					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt);
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}


enum EVENT_TYPE { EVENT_NPC_MOVE, EVENT_NPC_AGRO_CHASE, EVENT_NPC_AGRO_ATTACK};

struct timer_event {
	int obj_id;
	int target_id;
	chrono::system_clock::time_point	start_time;
	EVENT_TYPE ev;
	constexpr bool operator < (const timer_event& _Left) const
	{
		return (start_time > _Left.start_time);
	}

};

concurrency::concurrent_priority_queue <timer_event> timer_queue;
mutex t_lock;

void push_timer_event(int obj_id, int target_id, int sec, EVENT_TYPE ev_type)
{
	timer_event ev = { obj_id, target_id, chrono::system_clock::now() + chrono::seconds(sec), ev_type };
	t_lock.lock();

	timer_queue.push(ev);
	t_lock.unlock();
}

void error_display(int err_no);
void do_npc_move(int npc_id);

enum COMP_OP {
	OP_RECV, OP_SEND, OP_ACCEPT, OP_NPC_MOVE, 
	OP_NPC_AGRO_MOVE, OP_NPC_AGRO_ATTACK,
	OP_PLAYER_MOVE
};
class EXP_OVER {
public:
	WSAOVERLAPPED	_wsa_over;
	COMP_OP			_comp_op;
	WSABUF			_wsa_buf;
	unsigned char	_net_buf[BUFSIZE];
	int				_target;

public:
	EXP_OVER(COMP_OP comp_op, char num_bytes, void* mess) : _comp_op(comp_op)
	{
		ZeroMemory(&_wsa_over, sizeof(_wsa_over));
		_wsa_buf.buf = reinterpret_cast<char*>(_net_buf);
		_wsa_buf.len = num_bytes;
		memcpy(_net_buf, mess, num_bytes);
	}

	EXP_OVER(COMP_OP comp_op) : _comp_op(comp_op) {}

	EXP_OVER()
	{
		_comp_op = OP_RECV;
	}

	~EXP_OVER()
	{
	}
};

enum STATE { ST_FREE, ST_ACCEPT, ST_INGAME };
class CLIENT {
public:
	char name[MAX_NAME_SIZE];
	int	   _id;
	int	   _target_id;
	short  x, y;
	short  level;

	short  start_x = 10;
	short  start_y = 10;


	unordered_set   <int>  viewlist;
	mutex vl;

	lua_State* L;
	mutex lua_l;

	mutex state_lock;
	STATE _state;
	atomic_bool	_is_active;
	int		_type;   // 1.Player   2.NPC	

	EXP_OVER _recv_over;
	SOCKET  _socket;
	int		_prev_size;
	int		last_move_time;

	bool	is_moving = false;
	int move_count;

	//npc 종류
	char npc_type; // 0.Noraml  1. Agro

	bool login_success = false;
	bool is_p2n_attack = false;
	bool is_teleport = false;
	bool is_die = false;
	bool is_dummy = false;
public:
	CLIENT() : _state(ST_FREE), _prev_size(0)
	{
		x = 0;
		y = 0;
	}

	~CLIENT()
	{
		closesocket(_socket);
	}

	void do_recv()
	{
		DWORD recv_flag = 0;
		ZeroMemory(&_recv_over._wsa_over, sizeof(_recv_over._wsa_over));
		_recv_over._wsa_buf.buf = reinterpret_cast<char*>(_recv_over._net_buf + _prev_size);
		_recv_over._wsa_buf.len = sizeof(_recv_over._net_buf) - _prev_size;
		int ret = WSARecv(_socket, &_recv_over._wsa_buf, 1, 0, &recv_flag, &_recv_over._wsa_over, NULL);
		if (SOCKET_ERROR == ret) {
			int error_num = WSAGetLastError();
			if (ERROR_IO_PENDING != error_num)
				error_display(error_num);
		}
	}

	void do_send(int num_bytes, void* mess)
	{
		EXP_OVER* ex_over = new EXP_OVER(OP_SEND, num_bytes, mess);
		int ret = WSASend(_socket, &ex_over->_wsa_buf, 1, 0, 0, &ex_over->_wsa_over, NULL);
		if (SOCKET_ERROR == ret) {
			int error_num = WSAGetLastError();
			if (ERROR_IO_PENDING != error_num)
				error_display(error_num);
		}
	}
};

array <CLIENT, MAX_USER + MAX_NPC> clients;

// ID 에 영역지정
// 0 - (MAX_USER - 1) : 플레이어
// MAX_USER  - (MAX_USER + MAX_NPC) : NPC

void error_display(int err_no)
{
	WCHAR* lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, 0);
	wcout << lpMsgBuf << endl;
	//while (true);
	LocalFree(lpMsgBuf);
}

bool is_near(int a, int b)
{
	if (RANGE < abs(clients[a].x - clients[b].x)) return false;
	if (RANGE < abs(clients[a].y - clients[b].y)) return false;
	return true;
}

bool is_npc(int id)
{
	return (id >= NPC_ID_START) && (id <= NPC_ID_END);
}

bool is_player(int id)
{
	return (id >= 0) && (id < MAX_USER - 1);
}

bool is_agro_near(int npc_id, int player_id)
{
	if (AGRO_RANGE < abs(clients[npc_id].x - clients[player_id].x)) return false;
	if (AGRO_RANGE < abs(clients[npc_id].y - clients[player_id].y)) return false;
	return true;
}

int get_new_id()
{
	static int g_id = 0;

	for (int i = 0; i < MAX_USER; ++i) {
		clients[i].state_lock.lock();
		if (ST_FREE == clients[i]._state) {
			clients[i]._state = ST_ACCEPT;
			clients[i].state_lock.unlock();
			return i;
		}
		clients[i].state_lock.unlock();
	}
	cout << "Maximum Number of Clients Overflow!!\n";
	return -1;
}

void send_login_ok_packet(int c_id)
{
	sc_packet_login_ok packet;
	packet.id = c_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_OK;
	packet.x = clients[c_id].x;
	packet.y = clients[c_id].y;
	packet.level = clients[c_id].level;
	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_login_fail_packet(int c_id, int reason)
{
	sc_packet_login_fail packet;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_LOGIN_FAIL;
	packet.reason = reason;

	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_move_packet(int c_id, int mover)
{
	sc_packet_move packet;
	packet.id = mover;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_MOVE;
	packet.x = clients[mover].x;
	packet.y = clients[mover].y;
	packet.move_time = clients[mover].last_move_time;
	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_remove_object(int c_id, int victim)
{
	sc_packet_remove_object packet;
	packet.id = victim;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_REMOVE_OBJECT;
	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_put_object(int c_id, int target)
{
	sc_packet_put_object packet;
	packet.id = target;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_PUT_OBJECT;
	packet.x = clients[target].x;
	packet.y = clients[target].y;
	strcpy_s(packet.name, clients[target].name);
	packet.object_type = 0;

	packet.npc_type = clients[target].npc_type;

	clients[c_id].do_send(sizeof(packet), &packet);
}

void send_chat_packet(int user_id, int my_id, char* mess, int chat_type)
{
	sc_packet_chat packet;
	packet.id = my_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_CHAT;
	packet.chat_type = chat_type;
	strcpy_s(packet.message, mess);
	clients[user_id].do_send(sizeof(packet), &packet);
}

void send_changed_status(int c_id)
{
	sc_packet_status_change packet;
	//packet.id = others_id;
	packet.size = sizeof(packet);
	packet.type = SC_PACKET_STATUS_CHANGE;
	packet.level = clients[c_id].level;
	packet.x = clients[c_id].x;
	packet.y = clients[c_id].y;
	clients[c_id].do_send(sizeof(packet), &packet);
}

void Disconnect(int c_id)
{
	CLIENT& cl = clients[c_id];
	cl.vl.lock();
	unordered_set <int> my_vl = cl.viewlist;
	cl.vl.unlock();
	for (auto& other : my_vl) {
		CLIENT& target = clients[other];
		if (true == is_npc(target._id)) continue;
		if (ST_INGAME != target._state)
			continue;
		target.vl.lock();
		if (0 != target.viewlist.count(c_id)) {
			target.viewlist.erase(c_id);
			target.vl.unlock();
			send_remove_object(other, c_id);
		}
		else target.vl.unlock();
	}
	clients[c_id].state_lock.lock();
	closesocket(clients[c_id]._socket);
	clients[c_id]._state = ST_FREE;
	clients[c_id].state_lock.unlock();
	
	//메모리 초기화
	//vector<char*>().swap(login_players);
}

void Activate_Player_Move_Event(int target, int player_id)
{
	EXP_OVER* exp_over = new EXP_OVER;
	exp_over->_comp_op = OP_PLAYER_MOVE;
	exp_over->_target = player_id;
	PostQueuedCompletionStatus(g_h_iocp, 1, target, &exp_over->_wsa_over);
}

void process_packet(int client_id, unsigned char* p)
{
	unsigned char packet_type = p[1];
	CLIENT& cl = clients[client_id];

	switch (packet_type) {
	case CS_PACKET_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);

		bool insert_once = false;
		bool skip = false;
		if (packet->is_dummy == 0) //더미 클라가 아니면(사람이면)
		{
			for (auto& d : g_db)
			{
				//만약 클라가 입력한 아이디와 db에 저장된 아이디가 같다면
				if (strcmp(packet->name, d.userName) == 0)
				{
					if (login_players.size() == 0)
					{
						//login_players.emplace_back(packet->name);
						cl.login_success = true;
						cl.x = d.x;
						cl.y = d.y;
						cl.level = d.level;
						skip = true;
						break;
					}
					else
					{
						for (auto& lp : login_players) //로그인 한 플레이어들의 아이디와 비교 후
						{
							if (strcmp(lp, packet->name) == 0) //같으면
							{
								//이미 동일한 아이디로 로그인 한 유저가 있음.
								cout << "이미 동일한 아이디로 로그인 한 유저가 있음." << endl;

								//로그인 실패(아이디 중복)
								send_login_fail_packet(client_id, 0);
								Disconnect(client_id);
								skip = true;
								break;
							}
							else //다르면 로그인 성공
							{
								
								cl.login_success = true;
								//저장된 플레이어의 좌표값
								cl.x = d.x;
								cl.y = d.y;
								cl.level = d.level;
								skip = true;
								break;
							}
						}
					}
				}
				//다 돌았는데 같은 아이디가 없다.
			}
			
			//새로운 플레이어 정보insert
			if (insert_once == false && skip != true)
			{
				WCHAR* temp;
				int nChars = MultiByteToWideChar(CP_ACP, 0, packet->name, -1, NULL, 0);
				temp = new WCHAR[nChars];
				MultiByteToWideChar(CP_ACP, 0, packet->name, -1, (LPWSTR)temp, nChars);

				int new_id = g_db.back().id + 1;
				sql_new_player_insert(new_id, temp, 1, 10, 10);

				insert_once = true;

				cl.login_success = true;
				cl.x = 10;
				cl.y = 10;
				cl.level = 1;
			}
		}
		else { //더미 클라이면 그냥 로그인success
			cl.login_success = true;
			cl.is_dummy = true;
		}
		
		strcpy_s(cl.name, packet->name);
		if (cl.login_success)
		{
			login_players.emplace_back(cl.name); //로그인 성공하면 login_players 벡터에 집어넣기
			send_login_ok_packet(client_id);

			CLIENT& cl = clients[client_id];
			cl.state_lock.lock();
			cl._state = ST_INGAME;
			cl.state_lock.unlock();

			//cout << client_id << "번째 플레이어 접속!" << endl;
			//cout << clients[client_id].x << ", " << clients[client_id].y << endl;

			// 새로 접속한 플레이어의 정보를 주위 플레이어에게 보낸다
			for (auto& other : clients) {
				if (true == is_npc(other._id)) continue;
				if (other._id == client_id) continue;
				other.state_lock.lock();
				if (ST_INGAME != other._state) {
					other.state_lock.unlock();
					continue;
				}
				other.state_lock.unlock();

				if (false == is_near(other._id, client_id))
					continue;
				else {
					if (true == is_npc(other._id)) {
						if (clients[other._id]._is_active == true) continue;
						clients[other._id]._is_active = true;
						push_timer_event(other._id, 0, 1, EVENT_NPC_MOVE);

					}
				}


				other.vl.lock();
				other.viewlist.insert(client_id);
				other.vl.unlock();
				sc_packet_put_object packet;
				packet.id = client_id;
				strcpy_s(packet.name, cl.name);
				packet.object_type = 0;
				packet.size = sizeof(packet);
				packet.type = SC_PACKET_PUT_OBJECT;
				packet.x = cl.x;
				packet.y = cl.y;
				other.do_send(sizeof(packet), &packet);
			}

			// 새로 접속한 플레이어에게 주위 객체 정보를 보낸다
			for (auto& other : clients) {

				if (other._id == client_id) continue;
				other.state_lock.lock();
				if (ST_INGAME != other._state) {
					other.state_lock.unlock();
					continue;
				}
				other.state_lock.unlock();

				if (false == is_near(other._id, client_id))
					continue;
				else {
					if (true == is_npc(other._id)) {
						if (clients[other._id]._is_active == true) continue;
						clients[other._id]._is_active = true;
						push_timer_event(other._id, 0, 1, EVENT_NPC_MOVE);
					}
				}

				clients[client_id].vl.lock();
				clients[client_id].viewlist.insert(other._id);
				clients[client_id].vl.unlock();

				sc_packet_put_object packet;
				packet.id = other._id;
				strcpy_s(packet.name, other.name);
				packet.object_type = 0;
				packet.size = sizeof(packet);
				packet.type = SC_PACKET_PUT_OBJECT;
				packet.x = other.x;
				packet.y = other.y;
				cl.do_send(sizeof(packet), &packet);
			}
		}
		else
		{
			//로그인 실패
			send_login_fail_packet(client_id, 0);
			Disconnect(client_id);
		}

		break;
	}
	case CS_PACKET_MOVE:
	{
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
		cl.last_move_time = packet->move_time;
		int x = cl.x;
		int y = cl.y;
		switch (packet->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
		case 99: if (cl.is_p2n_attack == false) cl.is_p2n_attack = true; break;
		case 98: if (cl.is_p2n_attack == true) cl.is_p2n_attack = false; break;
		default:
			cout << "Invalid move in client " << client_id << endl;
			exit(-1);
		}
		cl.x = x;
		cl.y = y;

		unordered_set <int> nearlist;
		for (auto& other : clients) {
			if (other._id == client_id)
				continue;
			if (ST_INGAME != other._state)
				continue;
			if (false == is_near(client_id, other._id))
				continue;
			if (true == is_npc(other._id)) {
				Activate_Player_Move_Event(other._id, cl._id);
			}
			nearlist.insert(other._id);
		}

		send_move_packet(cl._id, cl._id);

		cl.vl.lock();
		unordered_set <int> my_vl{ cl.viewlist };
		cl.vl.unlock();

		// 새로시야에 들어온 플레이어 처리
		for (auto other : nearlist) {
			if (0 == my_vl.count(other)) {
				cl.vl.lock();
				cl.viewlist.insert(other);
				cl.vl.unlock();
				send_put_object(cl._id, other);

				//if (true == is_npc(other)) continue;
				if (true == is_npc(other)) {
					if (clients[other]._is_active == true) continue;
					clients[other]._is_active = true;
					push_timer_event(other, 0, 1, EVENT_NPC_MOVE);

				}

				if (true != is_player(other)) continue;

				clients[other].vl.lock();
				if (0 == clients[other].viewlist.count(cl._id)) {
					clients[other].viewlist.insert(cl._id);
					clients[other].vl.unlock();
					send_put_object(other, cl._id);
				}
				else {
					clients[other].vl.unlock();
					send_move_packet(other, cl._id);
				}
			}
			// 계속 시야에 존재하는 플레이어 처리
			else {
				if (true == is_npc(other)) continue;
				clients[other].vl.lock();
				if (0 != clients[other].viewlist.count(cl._id)) {
					clients[other].vl.unlock();
					send_move_packet(other, cl._id);
				}
				else {
					clients[other].viewlist.insert(cl._id);
					clients[other].vl.unlock();
					send_put_object(other, cl._id);
				}
			}
		}
		// 시야에서 사라진 플레이어 처리
		for (auto other : my_vl) {
			if (0 == nearlist.count(other)) {
				cl.vl.lock();
				cl.viewlist.erase(other);
				cl.vl.unlock();
				send_remove_object(cl._id, other);

				if (true == is_npc(other)) continue;
				/*if (true == is_npc(other)) {
					if (clients[other]._is_active == false) continue;
					clients[other]._is_active = false;
				}

				if (true != is_player(other)) continue;*/

				clients[other].vl.lock();
				if (0 != clients[other].viewlist.count(cl._id)) {
					clients[other].viewlist.erase(cl._id);
					clients[other].vl.unlock();

					send_remove_object(other, cl._id);
				}
				else clients[other].vl.unlock();
			}

		}
		break;
	}
	case CS_PACKET_TELEPORT:
	{
		cs_packet_teleport* packet = reinterpret_cast<cs_packet_teleport*>(p);
		cl.is_teleport = true;

		cout << "텔레포트 패킷 받음" << endl;
		//위치 순간이동
		cl.x = rand() % WORLD_WIDTH + 50;
		cl.y = rand() % WORLD_HEIGHT + 50;

		if (cl.is_teleport == true)
		{
			sc_packet_status_change packet;
			packet.size = sizeof(packet);
			packet.type = SC_PACKET_STATUS_CHANGE;
			packet.x = cl.x;
			packet.y = cl.y;
			packet.level = cl.level;
			cl.do_send(sizeof(packet), &packet);
			cl.is_teleport = false;
		}
		break;
	}

	}
}


//어그로 몬스터를 만났을 시 이 함수를 불러준다.
void NpcChasingUpdate(int npc_id, int player_id)
{

	int p_x = clients[player_id].x;
	int p_y = clients[player_id].y;
	int n_x = clients[npc_id].x;
	int n_y = clients[npc_id].y;

	int distance_x = abs(p_x - n_x);
	int distance_y = abs(p_y - n_y);


	//1초마다 한칸 씩 플레이어 쪽으로 간다.
	if (distance_x > distance_y)
	{
		if (n_x > p_x)
			clients[npc_id].x--;
		else if (n_x < p_x)
			clients[npc_id].x++;
	}
	else if (distance_x < distance_y)
	{
		if (n_y > p_y)
			clients[npc_id].y--;
		else if (n_y < p_y)
			clients[npc_id].y++;
	}
	else if (distance_x == distance_y)
	{
		if (n_x > p_x)
		{
			clients[npc_id].x--;
			clients[npc_id].y--;
		}
		else if (n_x < p_x)
		{
			clients[npc_id].x++;
			clients[npc_id].y++;
		}
	}

	//agro_npc랑 만남
	if (distance_x <= 1 && distance_y <= 1)
	{
		clients[npc_id].x = n_x;
		clients[npc_id].y = n_y;

		cout << "잡았다!" << endl;

		//TODO
		push_timer_event(npc_id, player_id, 1, EVENT_NPC_AGRO_ATTACK);
	}
}

void worker()
{
	for (;;) {
		DWORD num_byte;
		LONG64 iocp_key;
		WSAOVERLAPPED* p_over;
		BOOL ret = GetQueuedCompletionStatus(g_h_iocp, &num_byte, (PULONG_PTR)&iocp_key, &p_over, INFINITE);
		//cout << "GQCS returned.\n";
		int client_id = static_cast<int>(iocp_key);
		EXP_OVER* exp_over = reinterpret_cast<EXP_OVER*>(p_over);
		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			cout << "GQCS Error : ";
			error_display(err_no);
			cout << endl;
			Disconnect(client_id);

			WCHAR* temp;
			int nChars = MultiByteToWideChar(CP_ACP, 0, clients[client_id].name, -1, NULL, 0);
			temp = new WCHAR[nChars];
			MultiByteToWideChar(CP_ACP, 0, clients[client_id].name, -1, (LPWSTR)temp, nChars);

			//종료 하기 직전 플레이어의 정보를 저장하자!
			sql_player_data_save(temp, clients[client_id].level, clients[client_id].x, clients[client_id].y);

			if (exp_over->_comp_op == OP_SEND)
				delete exp_over;
			continue;
		}

		switch (exp_over->_comp_op) {
		case OP_RECV: {
			if (num_byte == 0) {
				Disconnect(client_id);
				continue;
			}
			CLIENT& cl = clients[client_id];
			int remain_data = num_byte + cl._prev_size;
			unsigned char* packet_start = exp_over->_net_buf;
			int packet_size = packet_start[0];

			while (packet_size <= remain_data) {
				process_packet(client_id, packet_start);
				remain_data -= packet_size;
				packet_start += packet_size;
				if (remain_data > 0) packet_size = packet_start[0];
				else break;
			}

			if (0 < remain_data) {
				cl._prev_size = remain_data;
				memcpy(&exp_over->_net_buf, packet_start, remain_data);
			}
			cl.do_recv();
			break;
		}
		case OP_SEND: {
			if (num_byte != exp_over->_wsa_buf.len) {
				Disconnect(client_id);
			}
			delete exp_over;
			break;
		}
		case OP_ACCEPT: {
			cout << "Accept Completed.\n";
			SOCKET c_socket = *(reinterpret_cast<SOCKET*>(exp_over->_net_buf));
			int new_id = get_new_id();
			if (-1 == new_id) {
				cout << "Maxmum user overflow. Accept aborted.\n";
			}
			else {
				CLIENT& cl = clients[new_id];
				//시작하면 핫스팟(stress test)에 가둬놓기
				//cl.x = rand() % HOTSPOT_SIZE;
				//cl.y = rand() % HOTSPOT_SIZE;
				cl.x = rand() % WORLD_WIDTH;
				cl.y = rand() % WORLD_HEIGHT;
				cl._id = new_id;
				cl._prev_size = 0;
				cl._recv_over._comp_op = OP_RECV;
				cl._recv_over._wsa_buf.buf = reinterpret_cast<char*>(cl._recv_over._net_buf);
				cl._recv_over._wsa_buf.len = sizeof(cl._recv_over._net_buf);
				ZeroMemory(&cl._recv_over._wsa_over, sizeof(cl._recv_over._wsa_over));
				cl._socket = c_socket;

				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_h_iocp, new_id, 0);
				cl.do_recv();
			}

			ZeroMemory(&exp_over->_wsa_over, sizeof(exp_over->_wsa_over));
			c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
			*(reinterpret_cast<SOCKET*>(exp_over->_net_buf)) = c_socket;
			AcceptEx(g_s_socket, c_socket, exp_over->_net_buf + 8, 0, sizeof(SOCKADDR_IN) + 16,
				sizeof(SOCKADDR_IN) + 16, NULL, &exp_over->_wsa_over);
			break;

		}
		case OP_NPC_MOVE: {
			delete exp_over;
			if (clients[client_id]._is_active == true) {
				do_npc_move(client_id);
			}

			break;
		}
		case OP_NPC_AGRO_MOVE:
		{
			//cout << "어그로 몬스터 move 발동!" << endl;
			if (clients[client_id]._is_active == true)
			{
				NpcChasingUpdate(client_id, clients[client_id]._target_id);
				//cout << "npc_id: " << client_id << "player_id: " << clients[client_id]._id << endl;
			}

			break;
		}
		case OP_NPC_AGRO_ATTACK:
		{
			//CLIENT& cl = clients[client_id]._target_id;
			cout << clients[client_id]._id << "번째 클라 공격당함!" << endl;

			if (false == is_npc(client_id))
			{
				clients[client_id].level--;

				if (clients[client_id].level < 1) //죽으면 집에서 리스폰
				{
					if (clients[client_id].is_dummy == true)
					{
						clients[client_id].x = rand() % WORLD_WIDTH;
						clients[client_id].y = rand() % WORLD_HEIGHT;
					}
					else
					{
						clients[client_id].level = 1;
						clients[client_id].x = 20;
						clients[client_id].y = 20;
					}
				}

				WCHAR* temp;
				int nChars = MultiByteToWideChar(CP_ACP, 0, clients[client_id].name, -1, NULL, 0);
				temp = new WCHAR[nChars];
				MultiByteToWideChar(CP_ACP, 0, clients[client_id].name, -1, (LPWSTR)temp, nChars);
				sql_player_level_realtime_update(temp, clients[client_id].level);

				send_changed_status(clients[client_id]._id);
			}
			
			break;
		}
		case OP_PLAYER_MOVE: 
		{ //플레이어가 움직이면..

			clients[client_id].lua_l.lock();

			lua_State* L = clients[client_id].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, exp_over->_target);
			int err = lua_pcall(L, 1, 0, 0);
			if (0 != err)
			{
				cout << lua_tostring(L, -1) << endl;
			}
			clients[client_id].lua_l.unlock();
		
			delete exp_over;

			break;
		}

		}
	}
}


int API_SendComeOn(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);
	send_chat_packet(user_id, my_id, mess, 1);

	

	return 0;
}

int API_SendMessage(lua_State* L)
{
	cout << "메시지 보냄!" << endl;
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	send_chat_packet(user_id, my_id, mess, 0);


	//플레이어가 normal npc를 공격했으면..
	if (NPC_PEACE == clients[my_id].npc_type)
	{
		if (clients[user_id].is_p2n_attack == true)
		{
			//npc 없애기
			clients[my_id]._is_active = false;
			send_remove_object(user_id, my_id); 

			clients[my_id]._state = ST_FREE;

			clients[user_id].level++; //레벨 업

			WCHAR* temp;
			int nChars = MultiByteToWideChar(CP_ACP, 0, clients[user_id].name, -1, NULL, 0);
			temp = new WCHAR[nChars];
			MultiByteToWideChar(CP_ACP, 0, clients[user_id].name, -1, (LPWSTR)temp, nChars);
			sql_player_level_realtime_update(temp, clients[user_id].level);

			//state change
			send_changed_status(clients[user_id]._id);
		}
	}


	return 0; //리턴 값 개수 0개
}

int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = clients[user_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = clients[user_id].y;
	lua_pushnumber(L, y);
	return 1;
}



void Initialize_NPC()
{
	for (int i = NPC_ID_START; i <= NPC_ID_END; ++i) {
		sprintf_s(clients[i].name, "N%d", i);
		clients[i].x = rand() % WORLD_WIDTH + 40;
		clients[i].y = rand() % WORLD_HEIGHT + 40;
		clients[i]._id = i;
		clients[i]._state = ST_INGAME;
		clients[i]._type = 2;
		clients[i]._is_active = false;

		random_device rd;
		default_random_engine eng(rd());
		geometric_distribution<int> dist(0.7);

		//clients[i].npc_type = rand() % 2;
		clients[i].npc_type = (rand() % 100 < 70) ? 0 : 1;

		//LUA
		lua_State* L = clients[i].L = luaL_newstate(); //루아를 연다.
		luaL_openlibs(L); //루아 표준 라이브러리를 연다.
		int error = luaL_loadfile(L, "monster.lua") || //에러 체크
			lua_pcall(L, 0, 0, 0);
		if (0 != error)
		{
			cout << lua_tostring(L, -1) << endl;
		}
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0); //실행

		lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendComeOn", API_SendComeOn);
		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
	}
}

//About NPC 움직임
void do_npc_move(int npc_id)
{
	unordered_set <int> old_viewlist;
	unordered_set <int> new_viewlist;

	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (false == is_near(npc_id, obj._id))
			continue;
		old_viewlist.insert(obj._id);
	}

	auto& x = clients[npc_id].x;
	auto& y = clients[npc_id].y;
	switch (rand() % 4) {
	case 0: if (y > 0) y--; break;
	case 1: if (y < (WORLD_HEIGHT - 1)) y++; break;
	case 2: if (x > 0) x--; break;
	case 3: if (x < (WORLD_WIDTH - 1)) x++; break;
	}
	for (auto& obj : clients) {
		if (obj._state != ST_INGAME)
			continue;
		if (false == is_player(obj._id))
			continue;
		if (false == is_near(npc_id, obj._id))
			continue;
		new_viewlist.insert(obj._id);
	}

	clients[npc_id]._is_active = true;
	push_timer_event(npc_id, 0, 1, EVENT_NPC_MOVE);

	// 새로 시야에 들어온 플레이어
	for (auto pl : new_viewlist) {
		if (0 == old_viewlist.count(pl)) {

			clients[pl].vl.lock();
			clients[pl].viewlist.insert(npc_id);
			clients[pl].vl.unlock();

			send_put_object(pl, npc_id);
		}
		else {

			send_move_packet(pl, npc_id);
		}

		//어그로 몬스터이면
		if (NPC_AGRO == clients[npc_id].npc_type)
		{
			//cout << "어그로 몬스터다!" << endl;
			unordered_set<int> agro_list;
			for (int i = 0; i < NPC_ID_START; ++i) //플레이어들에 대해서
			{
				if (false == clients[npc_id]._is_active) continue;
				if (true == is_agro_near(clients[npc_id]._id, i)) {
					clients[npc_id].vl.lock();
					agro_list.insert(i);
					clients[npc_id].vl.unlock();

					//Come one! lua 스크립트
					clients[npc_id].lua_l.lock();
					lua_State* LL = clients[npc_id].L;
					lua_getglobal(LL, "npc_find_player");
					lua_pushnumber(LL, pl);
					int err = lua_pcall(LL, 1, 0, 0); //실행
					if (0 != err)
					{
						cout << lua_tostring(LL, -1) << endl;
					}
					clients[npc_id].lua_l.unlock();
				}
				else
				{
					clients[npc_id].vl.lock();
					agro_list.erase(i);
					clients[npc_id].vl.unlock();
				}
			}
			if (!agro_list.empty())
			{
				int target_id = -1;
				for (auto p_id : agro_list)
				{
					target_id = p_id;
				}
				clients[npc_id]._target_id = target_id;

				if (true == is_near(target_id, npc_id))
					push_timer_event(npc_id, 0, 2, EVENT_NPC_AGRO_CHASE);
				continue;
			}
		}
	}
	// 시야에서 사라지는 경우
	for (auto pl : old_viewlist) {

		if (0 == new_viewlist.count(pl))
		{
			clients[pl].vl.lock();
			clients[pl].viewlist.erase(npc_id);
			clients[pl].vl.unlock();

			if (true == is_player(pl))
			{

				if (clients[npc_id]._is_active == false) continue;
				clients[npc_id]._is_active = false;

			}

			send_remove_object(pl, npc_id);
		}
	}



}

void do_ai()
{
	while (true) {
		auto start_t = chrono::system_clock::now();
		for (auto& npc : clients) {
			if (false == is_npc(npc._id)) continue;
			EXP_OVER* ex_over = new EXP_OVER;
			ex_over->_comp_op = OP_NPC_MOVE;
			PostQueuedCompletionStatus(g_h_iocp, 1, npc._id, &ex_over->_wsa_over);
			// do_npc_move(npc._id);
		}
		auto end_t = chrono::system_clock::now();
		if (end_t - start_t < 1s) {
			this_thread::sleep_for(1s - (end_t - start_t));
			cout << "npc_mode in " <<
				chrono::duration_cast<chrono::milliseconds>(end_t - start_t).count()
				<< "ms.\n";
		}
		else
			cout << "AI_THREAD exec_time overflow!\n";
	}
}

void do_timer() {

	while (true) {
		while (true) {
			timer_event ev;
			if (timer_queue.size() == 0) {
				continue;
			}
			timer_queue.try_pop(ev);

			if (ev.start_time <= chrono::system_clock::now()) {
				// exec_event;
				if (false == is_npc(ev.obj_id)) continue;
				if (EVENT_NPC_MOVE == ev.ev) {
					EXP_OVER* ex_over = new EXP_OVER;

					ex_over->_comp_op = OP_NPC_MOVE;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &ex_over->_wsa_over);

				}
				if (EVENT_NPC_AGRO_CHASE == ev.ev) {
					EXP_OVER* ex_over = new EXP_OVER;

					ex_over->_comp_op = OP_NPC_AGRO_MOVE;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.obj_id, &ex_over->_wsa_over);
				}
				if (EVENT_NPC_AGRO_ATTACK == ev.ev) {
					EXP_OVER* ex_over = new EXP_OVER;

					ex_over->_comp_op = OP_NPC_AGRO_ATTACK;
					PostQueuedCompletionStatus(g_h_iocp, 1, ev.target_id, &ex_over->_wsa_over);
				}

			}
			else {
				timer_queue.push(ev);	// timer_queue에 넣지 않고 최적화 필요
				break;
			}
		}
		this_thread::sleep_for(10ms);
	}
}

int main()
{
	wcout.imbue(locale("korean"));
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	sql_player_data_read();
	cout << "player data loaded \n";

	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);

	g_h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), g_h_iocp, 0, 0);

	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	char	accept_buf[sizeof(SOCKADDR_IN) * 2 + 32 + 100];
	EXP_OVER	accept_ex;
	*(reinterpret_cast<SOCKET*>(&accept_ex._net_buf)) = c_socket;
	ZeroMemory(&accept_ex._wsa_over, sizeof(accept_ex._wsa_over));
	accept_ex._comp_op = OP_ACCEPT;

	AcceptEx(g_s_socket, c_socket, accept_buf, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &accept_ex._wsa_over);
	cout << "Accept Called\n";

	for (int i = 0; i < MAX_USER; ++i)
		clients[i]._id = i;

	cout << "Creating Worker Threads\n";

	Initialize_NPC();

	vector <thread > worker_threads;
	//thread ai_thread{ do_ai };
	thread timer_thread{ do_timer };
	for (int i = 0; i < 6; ++i)
		worker_threads.emplace_back(worker);
	for (auto& th : worker_threads) {
		th.join();
	}

	//ai_thread.join();
	timer_thread.join();
	for (auto& cl : clients) {
		if (ST_INGAME == cl._state)
			Disconnect(cl._id);
	}
	worker_threads.clear();
	closesocket(g_s_socket);
	WSACleanup();
}


