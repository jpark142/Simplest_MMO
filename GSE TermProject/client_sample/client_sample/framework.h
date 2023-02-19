#define _CRT_SECURE_NO_WARNINGS

#define SFML_STATIC 1

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include <sstream>
#include <random>
using namespace std;
using namespace sf;

//static int g_x_origin;
//static int g_y_origin;
static RenderWindow* g_window;
static Font g_font;
static bool isComeOn = false;

#define DELETE(x) if(x) delete (x); (x) = nullptr

#ifdef _DEBUG
#pragma comment (lib, "lib/sfml-graphics-s-d.lib")
#pragma comment (lib, "lib/sfml-window-s-d.lib")
#pragma comment (lib, "lib/sfml-system-s-d.lib")
#pragma comment (lib, "lib/sfml-network-s-d.lib")
#else
#pragma comment (lib, "lib/sfml-graphics-s.lib")
#pragma comment (lib, "lib/sfml-window-s.lib")
#pragma comment (lib, "lib/sfml-system-s.lib")
#pragma comment (lib, "lib/sfml-network-s.lib")
#endif
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "ws2_32.lib")

//#include "..\..\iocp_single\iocp_single\protocol.h"
#include "protocol.h"