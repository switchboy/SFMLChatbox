#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include "lobby.h"
#include "connection.h"
#include "connectionSetupScreen.h"

sf::RenderWindow window;
sf::Thread serverThread(&server);

namespace networkStuff {
	int port;
}

int main()
{
	window.create(sf::VideoMode(1920, 1080, 32), "ChatApp");
	window.setFramerateLimit(60);
	connectionSetupScreen connectionSetup;
	connectionSetup.showConnectionSetupScreen();
	lobbyWindow lobby;
	lobby.showLobby();
}