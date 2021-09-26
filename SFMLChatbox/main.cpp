#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>

void server() {
	std::cout << "Server Running" << std::endl;

	sf::TcpListener listener;
	sf::SocketSelector selector;

	bool done = false;

	std::vector<sf::TcpSocket*> clients;

	listener.listen(2000);
	selector.add(listener);

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
				for (sf::TcpSocket* client : clients) {
					if (selector.isReady(*client)) {
						sf::Packet packet, sendPacket;
						if (client->receive(packet) == sf::Socket::Done) {
							std::string text;
							packet >> text;
							sendPacket << text;
							std::cout << text << std::endl;
							for (sf::TcpSocket* recievingClient : clients) {
								if (recievingClient != client) {
									client->send(sendPacket);
								}
							}
						}
					}
				}
				/*
				clients.erase(std::remove_if(clients.begin(), clients.end(),
					[](sf::TcpSocket* client) {
						if (client->Disconnected) {
							std::cout << "Client "<< client->getRemoteAddress() <<" closed connection remotely!" << std::endl;
						}
						return client->Disconnected;
					}
				), clients.end());
				*/
			}
		}
	}
}

void client() {
	sf::IpAddress ip = sf::IpAddress("127.0.0.1");
	sf::TcpSocket socket;
	
	std::string id;
	std::string text = "";

	std::cout << "Enter nickname" << std::endl;
	std::cin >> id;

	socket.connect(ip, 2000);

	sf::RenderWindow window(sf::VideoMode(800, 600, 32), "ChatApp");
	std::vector<sf::Text>chat;

	sf::Packet packet;
	packet << id;
	socket.send(packet);
	socket.setBlocking(false);

	sf::Font font;
	font.loadFromFile("fonts/SatellaRegular.ttf");

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			switch (event.type) {
			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Escape) {
					window.close();
				}
				else if (event.key.code == sf::Keyboard::Return) {
					packet.clear();
					packet << id + ": " + text;
					socket.send(packet);
					sf::Text displayText(text, font, 20);
					displayText.setFillColor(sf::Color::Red);
					chat.push_back(displayText);
					text = "";
				}
				break;
			case sf::Event::TextEntered:
				text += event.text.unicode;
				break;
			}
		}

		packet.clear();
		socket.receive(packet);

		std::string recievedText;
		if (packet >> recievedText) {
			sf::Text displayText(recievedText, font, 20);
			displayText.setFillColor(sf::Color::Blue);
			chat.push_back(displayText);
		}

		unsigned i = 0;
		for (sf::Text chatLine : chat) {
			chatLine.setPosition(0, i * 20);
			window.draw(chatLine);
			i++;
		}

		sf::Text drawText(text, font, 20);
		drawText.setFillColor(sf::Color::Green);
		drawText.setPosition(0, i * 20);
		window.draw(drawText);

		window.display();
	}
}

int main()
{
	std::string whutDo;
	std::cout << "Start as (S)erver or (C)lient?";
	std::cin >> whutDo;

	if (whutDo == "s") {
		server();
	}
	else if (whutDo == "c") {
		client();
	}

}