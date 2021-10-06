#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include "server.h"

enum dataType {
	Text,
	UserList,
	MessageStatus,
	Ping,
	playerReady
};

const sf::Color teamColors[] =
{
	{0, 0, 255},
	{0, 255, 0},
	{255, 0, 0},
	{255, 255, 0},
	{0, 255, 255},
	{255, 0, 255},
	{255, 127, 0},
	{127, 127, 127}
};

struct connectedPlayers {
	sf::TcpSocket* playerSocket;
	std::string name;
	sf::IpAddress remoteIp;
	sf::Int32 lastPing;
	sf::Int32 lastPingPacketSend;
	bool isReady;
};

struct playersClient {
	std::string name;
	sf::Int32 lastPing;
	bool isReady;
};

extern sf::RenderWindow window;
extern sf::Thread serverThread;
namespace networkStuff {
	extern int port;
}

