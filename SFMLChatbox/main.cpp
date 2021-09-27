#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>

void server() {
	std::cout << "Enter a port for listening" << std::endl;
	int port;
	std::cin >> port;
	std::cout << "Server starting" << std::endl;

	sf::TcpListener listener;
	sf::SocketSelector selector;

	bool done = false;

	std::vector<sf::TcpSocket*> clients;

	while(listener.listen(port) != sf::Socket::Done) {
		std::cout << "Failed to open port " << port << std::endl;
		std::cout << "Enter a port for listening" << std::endl;
		std::cin >> port;
		std::cout << "Server starting" << std::endl;
	}


	std::cout << "listening on port: " << port << std::endl;

	selector.add(listener);
	std::cout << "Waiting for incoming connections" << std::endl;

	while (!done) {
		if (selector.wait()) {
			if (selector.isReady(listener)) {
				sf::TcpSocket* socket = new sf::TcpSocket;
				listener.accept(*socket);
				sf::Packet packet;
				std::string id;

				if (socket->receive(packet) == sf::Socket::Done) {
					packet >> id;
				}

				std::cout << id << " from "<< socket->getRemoteAddress() <<" has connected!" << std::endl;
				clients.push_back(socket);
				selector.add(*socket);
			}
			else {
				std::vector<int> clientsDisconnected;
				for (int i = 0; i < clients.size(); i++) {
					if (selector.isReady(*clients[i])) {
						sf::Packet packet, sendPacket, confirmPacket;
						if (clients[i]->receive(packet) == sf::Socket::Done) {
							std::string text;
							packet >> text;
							sendPacket << text;
							std::cout << text << std::endl;
							for (int j = 0; j < clients.size(); j++) {
								if (i != j) {
									clients[j]->send(sendPacket);
								}
							}
							std::string confirmMsg = "ServerConfirmed123";
							confirmPacket << confirmMsg;
							clients[i]->send(confirmPacket);
						}
						else if (clients[i]->receive(packet) == sf::Socket::Disconnected) {
							clientsDisconnected.push_back(i);
							std::cout << clients[i]->getRemoteAddress() << " disconnected!" << std::endl;
						}
					}
				}
				for (int& clientId : clientsDisconnected) {
					//delete clients[clientId];
					clients.erase(clients.begin() + clientId);
				}
			}
		}
	}
}

void client() {
	std::cout << "Enter ip of the server to connect to" << std::endl;
	std::string ipString;
	std::cin >> ipString;

	int port;
	std::cout << "Enter a port to connect to" << std::endl;
	std::cin >> port;

	sf::IpAddress ip = sf::IpAddress(ipString);
	sf::TcpSocket socket;
	
	std::string id;
	std::string text = "";

	std::cout << "Enter nickname" << std::endl;
	std::cin >> id;

	while(socket.connect(ip, port) == sf::Socket::Error) {
		std::cout << "Unable to connect!" << std::endl;
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		std::cout << "Enter a port to connect to" << std::endl;
		std::cin >> port;
	}

	sf::RenderWindow window(sf::VideoMode(800, 600, 32), "ChatApp");
	std::vector<sf::Text>chat;

	sf::Packet packet;
	packet << id;
	socket.send(packet);
	socket.setBlocking(false);

	sf::Font font;
	font.loadFromFile("fonts/SatellaRegular.ttf");

	while (window.isOpen()) {
		window.clear();
		sf::Event event;

		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Escape || event.Closed) {
					window.close();
				}
				else if (event.key.code == sf::Keyboard::Return) {
					sf::Packet textPacket;
					textPacket << id + ": " + text;
					socket.send(textPacket);
					sf::Text displayText(text, font, 20);
					displayText.setFillColor(sf::Color::Red);
					chat.push_back(displayText);
					text = "";
				}
				break;
			case sf::Event::TextEntered:
				if (event.text.unicode != '\b' ) {
					if (event.text.unicode != '\r') {
						text += event.text.unicode;
					}
				}
				else {
					if (!text.empty()) {
						text.erase(text.size() - 1, 1);
					}
				}
				break;
			}
		}

		unsigned textLine = 0;
		sf::Packet recievePacket;
		if (socket.receive(recievePacket) == sf::Socket::Disconnected) {
			//complain
			sf::Text drawText("CONNECTION LOST!!!", font, 20);
			drawText.setFillColor(sf::Color::Red);
			drawText.setPosition(3, textLine);
			window.draw(drawText);
			textLine++;
			//do something about it
			socket.connect(ip, port);
			sf::Packet reconnectPacket;
			reconnectPacket << id;
			socket.send(reconnectPacket);
			socket.setBlocking(false);
		}

		std::string recievedText;
		bool recievedAndRelayed;
		if (recievePacket >> recievedText) {
			if (recievedText == "ServerConfirmed123") {
				chat.back().setFillColor(sf::Color::White);
			}
			else {
				sf::Text displayText(recievedText, font, 20);
				displayText.setFillColor(sf::Color::Blue);
				chat.push_back(displayText);
			}
		}



		
		for (sf::Text chatLine : chat) {
			chatLine.setPosition(3, textLine * 20);
			window.draw(chatLine);
			textLine++;
		}

		sf::Text drawText(text, font, 20);
		drawText.setFillColor(sf::Color::Green);
		drawText.setPosition(3, textLine * 20);
		window.draw(drawText);

		window.display();
	}
}

int main()
{
	std::string whutDo;
	std::cout << "Start as (S)erver or (C)lient?" <<std::endl;
	std::cin >> whutDo;

	if (whutDo == "s" || whutDo == "S") {
		server();
	}
	else if (whutDo == "c" || whutDo == "C") {
		client();
	}

}