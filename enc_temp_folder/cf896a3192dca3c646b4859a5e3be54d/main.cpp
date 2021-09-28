#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>

enum dataType {
	Text,
	UserList,
	MessageStatus,
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

void server(int port) {
	const int maxClients = 8;
	int clientsConnected = 0;
	std::cout << "Server starting" << std::endl;

	sf::TcpListener listener;
	sf::SocketSelector selector;

	bool done = false;

	std::vector<sf::TcpSocket*> clients;
	std::vector<std::string> clientNames;

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
				if (clientsConnected < maxClients) {
					sf::TcpSocket* socket = new sf::TcpSocket;
					listener.accept(*socket);
					sf::Packet packet;
					std::string id;

					if (socket->receive(packet) == sf::Socket::Done) {
						packet >> id;
					}

					clientsConnected++;

					sf::Packet succesFullJoin;
					bool joinSuccesFull = true;
					succesFullJoin << joinSuccesFull;
					socket->send(succesFullJoin);


					sf::Packet newUserJoined;
					int i = -1;
					std::string newUserText = id + " had joined the server!";
					sf::Uint8 header = dataType::Text;

					newUserJoined << header << i << newUserText;
					for (sf::TcpSocket* client : clients) {
						client->send(newUserJoined);
					}

					newUserJoined.clear();
					std::string welcomeText = "Welcome to the server " + id + "!";
					newUserJoined << header << i << welcomeText;
					socket->send(newUserJoined);
					clients.push_back(socket);
					clientNames.push_back(id);
					selector.add(*socket);

					sf::Packet userListPacket;
					sf::Uint8 userListPacketHeader = dataType::UserList;
					userListPacket << userListPacketHeader << static_cast<int>(clients.size());
					for (std::string& username : clientNames) {
						userListPacket << username;
					}
					for (sf::TcpSocket* client : clients) {
						client->send(userListPacket);
					}

				}
				else {
					sf::TcpSocket tempSocket;
					listener.accept(tempSocket);
					sf::Packet succesFullJoin;
					bool joinSuccesFull = false;
					succesFullJoin << joinSuccesFull;
					tempSocket.send(succesFullJoin);
					tempSocket.disconnect();
				}
			}
			else {
				std::vector<int> clientsDisconnected;
				for (int i = 0; i < clients.size(); i++) {
					if (selector.isReady(*clients[i])) {
						sf::Packet packet, sendPacket, confirmPacket;
						if (clients[i]->receive(packet) == sf::Socket::Done) {
							sf::Uint8 recievedHeader;
							packet >> recievedHeader;
							switch (recievedHeader) {
							case dataType::Text:
								std::string text;
								packet >> text;
								sf::Uint8 header = dataType::Text;
								sendPacket << header << i << text;
								for (int j = 0; j < clients.size(); j++) {
									if (i != j) {
										clients[j]->send(sendPacket);
									}
								}
								sf::Uint8 headerCFM = dataType::MessageStatus;
								bool isSentToAll = true;
								confirmPacket << headerCFM << isSentToAll;
								clients[i]->send(confirmPacket);
								break;
							} 
						}
						else if (clients[i]->receive(packet) == sf::Socket::Disconnected) {
							clientsDisconnected.push_back(i);
							sf::Uint8 header = dataType::Text;
							std::string text = clientNames[i] +" disconnected!";
							sendPacket << header << -1 << text;
							for (int j = 0; j < clients.size(); j++) {
								if (i != j) {
									clients[j]->send(sendPacket);
								}
							}
						}
					}
				}
				for (int& clientId : clientsDisconnected) {
					clients.erase(clients.begin() + clientId);
					clientNames.erase(clientNames.begin() + clientId);
					clientsConnected--;
					sf::Packet userListPacket;
					sf::Uint8 userListPacketHeader = dataType::UserList;
					userListPacket << userListPacketHeader << static_cast<int>(clients.size());
					for (std::string& username : clientNames) {
						userListPacket << username;
					}
					for (sf::TcpSocket* client : clients) {
						client->send(userListPacket);
					}
				}
			}
		}
	}
}

sf::Packet interactWindow(std::vector<sf::Text>& chat, sf::RenderWindow& window, std::string& text, sf::Font& font) {
	sf::Event event;
	sf::Packet textPacket;

	while (window.pollEvent(event)) {
		switch (event.type) {
		case sf::Event::KeyPressed:
			if (event.key.code == sf::Keyboard::Escape || event.Closed) {
				window.close();
			}
			else if (event.key.code == sf::Keyboard::Return) {
				sf::Uint8 header = dataType::Text;
				textPacket << header << text;
				sf::Text displayText(text, font, 20);
				displayText.setFillColor(sf::Color::Red);
				chat.push_back(displayText);
				text = "";
			}
			break;
		case sf::Event::TextEntered:
			if (event.text.unicode != '\b') {
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
	return textPacket;
}

void drawChat(std::vector<sf::Text>& chat, sf::RenderWindow& window, std::string& text, sf::Font& font, int textLine) {
	for (sf::Text chatLine : chat) {
		chatLine.setPosition(48, (textLine * 20)+ 490);
		chatLine.setOutlineColor(sf::Color::Black);
		chatLine.setOutlineThickness(2);
		window.draw(chatLine);
		textLine++;
	}

	sf::Text drawText(text, font, 20);
	drawText.setFillColor(sf::Color::Green);
	drawText.setOutlineColor(sf::Color::Black);
	drawText.setOutlineThickness(2);
	drawText.setPosition(48, 1010);
	window.draw(drawText);

	window.display();
}

void client(bool selfHosted, int port) {
	std::vector<sf::Text>chat;
	std::string ipString;
	std::string id;
	std::string text = "";
	std::vector<std::string> userList;

	if (selfHosted) {
		ipString = "127.0.0.1";
	}
	else {	
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		std::cout << "Enter a port to connect to" << std::endl;
		std::cin >> port;
	}

	std::cout << "Enter nickname" << std::endl;
	std::cin >> id;

	sf::IpAddress ip = sf::IpAddress(ipString);
	sf::TcpSocket socket;

	while(socket.connect(ip, port) == sf::Socket::Error) {
		std::cout << "Unable to connect!" << std::endl;
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		std::cout << "Enter a port to connect to" << std::endl;
		std::cin >> port;
	}

	sf::Packet packet;
	packet << id;
	socket.send(packet);

	socket.setBlocking(true);
	bool succesfullJoin;
	sf::Packet joinPacket;
	socket.receive(joinPacket);
	joinPacket >> succesfullJoin;
	if (!succesfullJoin) {
		std::cout << "Could not join server, because the sever is full!" << std::endl;
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		std::cout << "Enter a port to connect to" << std::endl;
		std::cin >> port;
		while (socket.connect(ip, port) == sf::Socket::Error) {
			std::cout << "Unable to connect!" << std::endl;
			std::cout << "Enter ip of the server to connect to" << std::endl;
			std::cin >> ipString;
			std::cout << "Enter a port to connect to" << std::endl;
			std::cin >> port;
		}
	}

	sf::RenderWindow window(sf::VideoMode(1920, 1080, 32), "ChatApp");

	socket.setBlocking(false);

	sf::Font font;
	font.loadFromFile("fonts/SatellaRegular.ttf");

	sf::Texture splashScreenTexture;
	sf::Sprite splashScreenSprite;

	splashScreenTexture.loadFromFile("textures/SplashScreen.png");
	splashScreenSprite.setTexture(splashScreenTexture);
	splashScreenSprite.setPosition(0, 0);
	sf::RectangleShape overlay, chatWindow, playerWindow;
	overlay.setFillColor(sf::Color(95, 205, 228, 102));
	overlay.setSize(sf::Vector2f(1920, 1080));
	overlay.setPosition(0, 0);
	chatWindow.setFillColor(sf::Color(46, 99, 110, 204));
	chatWindow.setSize(sf::Vector2f(1200,545));
	chatWindow.setPosition(45,490);
	playerWindow.setFillColor(sf::Color(46, 99, 110, 204));
	playerWindow.setSize(sf::Vector2f(1200, 400));
	playerWindow.setPosition(45,45);

	
	while (window.isOpen()) {
		unsigned textLine = 0;
		window.clear(sf::Color::White);
		window.draw(splashScreenSprite);
		window.draw(overlay);
		window.draw(chatWindow);
		window.draw(playerWindow);
		sf::Packet potentialPacket = interactWindow(chat, window, text, font);
		if (potentialPacket.getDataSize() != 0) {
			socket.send(potentialPacket);
		}
		potentialPacket.clear();

		//do network stuff here
		sf::Packet recievePacket;
		sf::Socket::Status statusOfSocket = socket.receive(recievePacket);
		if (statusOfSocket == sf::Socket::Disconnected) {
			//complain
			sf::Text drawText("CONNECTION LOST!!!", font, 20);
			drawText.setFillColor(sf::Color::Red);
			drawText.setOutlineColor(sf::Color::Black);
			drawText.setOutlineThickness(2);
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
		else if (statusOfSocket == sf::Socket::Done) {
			sf::Uint8 recievedHeader;
			recievePacket >> recievedHeader;
			switch (recievedHeader) {
				case dataType::Text:
				{
					std::string recievedText;
					int userId;
					recievePacket >> userId;
					recievePacket >> recievedText;
					
					if (userId == -1) {
						sf::Text displayText("*** " + recievedText, font, 20);
						displayText.setFillColor(sf::Color::Yellow);
						chat.push_back(displayText);
					}
					else {
						sf::Text displayText(userList[userId] + ": " + recievedText, font, 20);
						displayText.setFillColor(sf::Color::Blue);
						chat.push_back(displayText);
					}
					break;
				}
				case dataType::MessageStatus:
					bool relayed;
					recievePacket >> relayed;
					if (relayed) {
						chat.back().setFillColor(sf::Color::White);
					}
					break;
				case dataType::UserList:
					userList.clear();
					int numberOfUsers;
					recievePacket >> numberOfUsers;
					for (int i = 0; i < numberOfUsers; i++) {
						std::string tempString;
						recievePacket >> tempString;
						userList.push_back(tempString);
					}
					break;
			}
		}

		sf::Text playerWindowText("Players in lobby:", font, 20);
		playerWindowText.setFillColor(sf::Color::White);
		playerWindowText.setOutlineColor(sf::Color::Black);
		playerWindowText.setOutlineThickness(2);
		playerWindowText.setPosition(48, 48);
		window.draw(playerWindowText);
		int playerCounter = 0;
		for (std::string& player : userList) {
			sf::Text playerWindowText(player, font, 20);
			playerWindowText.setFillColor(teamColors[playerCounter]);
			playerWindowText.setOutlineColor(sf::Color::Black);
			playerWindowText.setOutlineThickness(2);
			playerWindowText.setPosition(48, 68+(20*playerCounter));
			window.draw(playerWindowText);
			playerCounter++;
		}


		drawChat(chat, window, text, font, textLine);
	}
}

int main()
{
	std::string whutDo;
	std::cout << "Start as (S)erver or (C)lient?" <<std::endl;
	std::cin >> whutDo;

	if (whutDo == "s" || whutDo == "S") {
		std::cout << "Enter a port for listening" << std::endl;
		int port;
		std::cin >> port;
		sf::Thread serverThread(&server, port);
		serverThread.launch();
		client(true, port);
	}
	else if (whutDo == "c" || whutDo == "C") {
		client(false, 0);
	}
}