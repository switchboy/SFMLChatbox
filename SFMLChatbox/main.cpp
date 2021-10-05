#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>
#include "lobby.h"
#include "server.h"
#include "connection.h"

sf::RenderWindow window;

void client(bool selfHosted, int port) {
	


}

int main()
{
	lobbyWindow lobby;
	std::string whutDo;
	std::cout << "Start as (S)erver or (C)lient?" <<std::endl;
	std::cin >> whutDo;
	int port = 8756;
	sf::Thread serverThread(&server, port);

	if (whutDo == "s" || whutDo == "S") {
		serverThread.launch();
		std::string ipString = "127.0.0.1";
		sf::IpAddress ip = sf::IpAddress(ipString);
		std::cout << "Enter a nickname" << std::endl;
		std::string id;
		std::cin >> id;
		currentConnection.connect(ip, port, id);
	}
	else if (whutDo == "c" || whutDo == "C") {
		std::string ipString;
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		sf::IpAddress ip = sf::IpAddress(ipString);
		std::cout << "Enter a nickname" << std::endl;
		std::string id;
		std::cin >> id;
		currentConnection.connect(ip, port, id);
	}


	window.create(sf::VideoMode(1920, 1080, 32), "ChatApp");
	window.setFramerateLimit(60);
	while (!lobby.isDone()) {
		lobby.interact(*currentConnection.getTcpSocket());
		lobby.pollConnectionAndGetUpdate(*currentConnection.getTcpSocket());
		lobby.drawLobby();
	}

}