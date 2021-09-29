#include "main.h"
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <string>

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

void server(int port) {
	const int maxClients = 8;
	int clientsConnected = 0;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	bool done = false;
	std::vector<connectedPlayers> clients;
	sf::Time serverTime;
	sf::Int32 lastPingSent = 0;
	sf::Packet pingPacket;
	sf::Uint8 pingPacketHeader = dataType::Ping;
	pingPacket << pingPacketHeader;

	while(listener.listen(port) != sf::Socket::Done) {
		std::cout << "Failed to open port " << port << std::endl;
		std::cout << "Enter a port for listening" << std::endl;
		std::cin >> port;
		std::cout << "Server starting" << std::endl;
	}
	selector.add(listener);

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
					for (connectedPlayers& client : clients) {
						client.playerSocket->send(newUserJoined);
					}

					newUserJoined.clear();
					std::string welcomeText = "Welcome to the server " + id + "!";
					newUserJoined << header << i << welcomeText;
					socket->send(newUserJoined);
					clients.push_back({ socket, id, socket->getRemoteAddress() , 0, serverTime.asMilliseconds(), false });
					socket->send(pingPacket);
					selector.add(*socket);

					{
						sf::Packet userListPacket;
						sf::Uint8 userListPacketHeader = dataType::UserList;
						userListPacket << userListPacketHeader << static_cast<int>(clients.size());
						for (connectedPlayers& client : clients) {
							userListPacket << client.name;
						}
						for (connectedPlayers& client : clients) {
							client.playerSocket->send(userListPacket);
						}
					}

					{
						sf::Packet sendPacket;
						sf::Uint8 header = dataType::playerReady;
						sendPacket << header;
						for (int i = 0; i < clients.size(); i++) {
							sendPacket << clients[i].isReady;
						}
						for (int j = 0; j < clients.size(); j++) {
							clients[j].playerSocket->send(sendPacket);
						}
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
					if (selector.isReady(*clients[i].playerSocket)) {
						sf::Packet packet, sendPacket, confirmPacket;
						if (clients[i].playerSocket->receive(packet) == sf::Socket::Done) {
							sf::Uint8 recievedHeader;
							packet >> recievedHeader;
							switch (recievedHeader) {
							case dataType::Text:
							{
								std::string text;
								packet >> text;
								sf::Uint8 header = dataType::Text;
								sendPacket << header << i << text;
								for (int j = 0; j < clients.size(); j++) {
									clients[j].playerSocket->send(sendPacket);
								}
								sf::Uint8 headerCFM = dataType::MessageStatus;
								bool isSentToAll = true;
								confirmPacket << headerCFM << isSentToAll;
								clients[i].playerSocket->send(confirmPacket);
								break;
							}

							case dataType::Ping:
								clients[i].lastPing = serverTime.asMilliseconds() - clients[i].lastPingPacketSend;
								break;
							
							case dataType::playerReady:
								packet >> clients[i].isReady;
								sf::Uint8 header = dataType::playerReady;
								sendPacket << header;
								for (int i = 0; i < clients.size(); i++) {
									sendPacket << clients[i].isReady;
								}
								for (int j = 0; j < clients.size(); j++) {
									clients[j].playerSocket->send(sendPacket);
								}
								break;
							} 
						}
						else if (clients[i].playerSocket->receive(packet) == sf::Socket::Disconnected) {
							clientsDisconnected.push_back(i);
							sf::Uint8 header = dataType::Text;
							std::string text = clients[i].name +" disconnected!";
							sendPacket << header << -1 << text;
							for (int j = 0; j < clients.size(); j++) {
								if (i != j) {
									clients[j].playerSocket->send(sendPacket);
								}
							}
						}
					}
				}
				for (int& clientId : clientsDisconnected) {
					clients.erase(clients.begin() + clientId);
					clientsConnected--;
					sf::Packet userListPacket;
					sf::Uint8 userListPacketHeader = dataType::UserList;
					userListPacket << userListPacketHeader << static_cast<int>(clients.size());
					for (connectedPlayers& client : clients) {
						userListPacket << client.name;
					}
					for (connectedPlayers& client : clients) {
						client.playerSocket->send(userListPacket);
					}
					sf::Packet sendPacket;
					sf::Uint8 header = dataType::playerReady;
					sendPacket << header;
					for (int i = 0; i < clients.size(); i++) {
						sendPacket << clients[i].isReady;
					}
					for (int j = 0; j < clients.size(); j++) {
						clients[j].playerSocket->send(sendPacket);
					}
				}

				if (serverTime.asMilliseconds() > lastPingSent + 100) {

					pingPacket.clear();
					pingPacket << pingPacketHeader;
					for (int i = 0; i < clients.size(); i++) {
						pingPacket << clients[i].lastPing;
					}
					for (int j = 0; j < clients.size(); j++) {
						clients[j].playerSocket->send(pingPacket);
					}
				}

			}
		}
	}
}

struct playersClient {
	std::string name;
	sf::Int32 lastPing;
	bool isReady;
};

void client(bool selfHosted, int port) {
	bool isReady;
	std::vector<sf::Text>chat;
	std::string ipString;
	std::string id;
	std::string text = "";
	std::vector<playersClient> playerList;
	sf::Texture splashScreenTexture;
	sf::Sprite splashScreenSprite;
	splashScreenTexture.loadFromFile("textures/SplashScreen.png");
	splashScreenSprite.setTexture(splashScreenTexture);
	splashScreenSprite.setPosition(0, 0);
	sf::RectangleShape overlay, chatWindow, playerWindow, sendTextBackground, scrollBar, sendButtonRect, readyButtonRect, readyIcon;
	overlay.setFillColor(sf::Color(95, 205, 228, 102));
	overlay.setSize(sf::Vector2f(1920, 1080));
	overlay.setPosition(0, 0);
	chatWindow.setFillColor(sf::Color(46, 99, 110, 204));
	chatWindow.setSize(sf::Vector2f(1200, 545));
	chatWindow.setPosition(45, 490);
	chatWindow.setOutlineThickness(2);
	playerWindow.setFillColor(sf::Color(46, 99, 110, 204));
	playerWindow.setSize(sf::Vector2f(1200, 400));
	playerWindow.setPosition(45, 45);
	playerWindow.setOutlineThickness(2);
	scrollBar.setFillColor(sf::Color(46, 99, 110, 255));
	scrollBar.setSize(sf::Vector2f(25, 25));
	scrollBar.setOutlineThickness(2);
	scrollBar.setOutlineColor(sf::Color::White);
	int minScrollPosition = 516;
	int maxScrollPosition = 957;
	int scrollSpace = 441;
	int drawableLines = 25;
	int lineOffSetByScrolling = 0;
	sf::CircleShape up(14, 3);
	up.setFillColor(sf::Color(46, 99, 110, 255));
	up.setPosition(1217, 492);
	up.setOutlineThickness(2);
	up.setOutlineColor(sf::Color::White);
	sf::CircleShape down(14, 3);
	down.setFillColor(sf::Color(46, 99, 110, 255));
	down.setPosition(1245, 1006);
	down.setOutlineThickness(2);
	down.setOutlineColor(sf::Color::White);
	down.setRotation(180);
	sf::Packet pingPacket;
	sf::Uint8 pingPacketHeader = dataType::Ping;
	pingPacket << pingPacketHeader;
	sf::Font font;
	font.loadFromFile("fonts/SatellaRegular.ttf");
	sf::Text sendButton("Send", font, 20);
	sendButton.setOutlineColor(sf::Color::Black);
	sendButton.setOutlineThickness(2);
	sendButton.setFillColor(sf::Color::White);
	sendButton.setPosition(1245 - sendButton.getLocalBounds().width, 1010);
	sendTextBackground.setFillColor(sf::Color(46, 99, 110, 204));
	sendTextBackground.setSize(sf::Vector2f(1195 - sendButton.getLocalBounds().width, 25));
	sendTextBackground.setOutlineThickness(2);
	sendTextBackground.setOutlineColor(sf::Color::White);
	sendTextBackground.setPosition(45, 1010);
	sendButtonRect.setFillColor(sf::Color(46, 99, 110, 204));
	sendButtonRect.setSize(sf::Vector2f(sendButton.getLocalBounds().width+1, 25));
	sendButtonRect.setOutlineThickness(2);
	sendButtonRect.setOutlineColor(sf::Color::White);
	sendButtonRect.setPosition(1243 - sendButton.getLocalBounds().width, 1010);
	bool scrollBarClicked = false;
	sf::Text readyButton("Ready", font, 45);
	readyButton.setOutlineColor(sf::Color::Black);
	readyButton.setOutlineThickness(2);
	readyButton.setFillColor(sf::Color::White);
	readyButtonRect.setFillColor(sf::Color(46, 99, 110, 204));
	readyButtonRect.setSize({(readyButton.getGlobalBounds().width + 15), 65 });
	readyButtonRect.setOutlineThickness(2);
	readyButtonRect.setOutlineColor(sf::Color::White);
	readyButtonRect.setPosition( 1530, 280);
	readyButton.setPosition(readyButtonRect.getPosition().x+9, readyButtonRect.getPosition().y+3);
	bool playerReady = false;
	readyIcon.setSize(sf::Vector2f(10, 10));
	readyIcon.setOutlineThickness(2);
	readyIcon.setOutlineColor(sf::Color::White);
	bool textAreaSelected = false;

	if (selfHosted) {
		ipString = "127.0.0.1";
	}
	else {	
		std::cout << "Enter ip of the server to connect to" << std::endl;
		std::cin >> ipString;
		//std::cout << "Enter a port to connect to" << std::endl;
		//std::cin >> port;
	}

	std::cout << "Enter a nickname" << std::endl;
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
		//std::cout << "Enter a port to connect to" << std::endl;
		//std::cin >> port;
		while (socket.connect(ip, port) == sf::Socket::Error) {
			std::cout << "Unable to connect!" << std::endl;
			std::cout << "Enter ip of the server to connect to" << std::endl;
			std::cin >> ipString;
			//std::cout << "Enter a port to connect to" << std::endl;
			//std::cin >> port;
		}
	}
	socket.setBlocking(false);

	sf::RenderWindow window(sf::VideoMode(1920, 1080, 32), "ChatApp");
	window.setFramerateLimit(60);
	sf::Vector2i lastMousePosition = { -1,-1 };
	while (window.isOpen()) {
		unsigned textLine = 0;
		window.clear(sf::Color::White);
		window.draw(splashScreenSprite);
		window.draw(overlay);
		window.draw(chatWindow);
		window.draw(playerWindow);
		sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
		if (up.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
			up.setFillColor(sf::Color::Yellow);
			down.setFillColor(sf::Color(46, 99, 110, 255));
		}
		else if (down.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
			up.setFillColor(sf::Color(46, 99, 110, 255));
			down.setFillColor(sf::Color::Yellow);
		}
		else {
			up.setFillColor(sf::Color(46, 99, 110, 255));
			down.setFillColor(sf::Color(46, 99, 110, 255));
		}
		if (textAreaSelected) {
			sendTextBackground.setOutlineColor(sf::Color::Yellow);
		}
		else {
			if (sendTextBackground.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
				sendTextBackground.setOutlineColor(sf::Color::Red);
			}
			else {
				sendTextBackground.setOutlineColor(sf::Color::White);
			}
		}

		if (sendButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
			sendButton.setFillColor(sf::Color::Yellow);
			sendButtonRect.setOutlineColor(sf::Color::Yellow);
		}
		else {
			sendButton.setFillColor(sf::Color::White);
			sendButtonRect.setOutlineColor(sf::Color::White);
		}

		if (scrollBar.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition)) && !scrollBarClicked) {
			scrollBar.setOutlineColor(sf::Color::Yellow);
		}
		else {
			scrollBar.setOutlineColor(sf::Color::White);
		}
		sf::Event event;

		if (scrollBarClicked) {
			int newPosition = mousePosition.y - 12;
			if (newPosition < minScrollPosition) {
				newPosition = minScrollPosition;
				lineOffSetByScrolling = chat.size() - 24;
			}
			else if (newPosition > maxScrollPosition) {
				newPosition = maxScrollPosition;
				lineOffSetByScrolling = 0;
			} else{
				scrollBar.setPosition(scrollBar.getPosition().x, newPosition);
				float line = (static_cast<float>(chat.size()) - 25.f) * (static_cast<float>(newPosition) - static_cast<float>(minScrollPosition)) / (static_cast<float>(maxScrollPosition) - static_cast<float>(minScrollPosition));
				lineOffSetByScrolling = chat.size() - 24 - line;
			}
		}

		if (playerReady) {
			readyButton.setFillColor(sf::Color::Green);
		}
		else {
			readyButton.setFillColor(sf::Color::Red);
		}

		while (window.pollEvent(event)) {
			switch (event.type) {

			case sf::Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Escape || event.Closed) {
					window.close();
				}
				else if (event.key.code == sf::Keyboard::Return && textAreaSelected) {
					sf::Packet textPacket;
					sf::Uint8 header = dataType::Text;
					textPacket << header << text;
					socket.send(textPacket);
					text = "";
				}
				else if (event.key.code == sf::Keyboard::Up) {
					if (chat.size() >= 25) {
						lineOffSetByScrolling++;
						if (lineOffSetByScrolling > chat.size()-25) {
							lineOffSetByScrolling = chat.size()-24;
						}
					}
				}if (event.key.code == sf::Keyboard::Down) {
					if (chat.size() >= 25) {
						lineOffSetByScrolling--;
						if (lineOffSetByScrolling < 0) {
							lineOffSetByScrolling = 0;
						}
					}
				}
				break;

			case event.MouseButtonPressed:
				if (event.mouseButton.button == sf::Mouse::Left) {
					if (scrollBar.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
						scrollBar.setFillColor(sf::Color::Yellow);
						scrollBarClicked = true;
					}
				}
				break;

			case event.MouseButtonReleased:
					if (event.mouseButton.button == sf::Mouse::Left) {
						if (sendTextBackground.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
							textAreaSelected = true;
						} else if (up.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
							if (chat.size() >= 25) {
								lineOffSetByScrolling++;
								if (lineOffSetByScrolling > chat.size() - 25) {
									lineOffSetByScrolling = chat.size() - 24;
								}
							}
							textAreaSelected = false;
						}
						else if (down.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
							if (chat.size() >= 25) {
								lineOffSetByScrolling--;
								if (lineOffSetByScrolling < 0) {
									lineOffSetByScrolling = 0;
								}
							}
							textAreaSelected = false;
						}
						else if(sendButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
							sf::Packet textPacket;
							sf::Uint8 header = dataType::Text;
							textPacket << header << text;
							socket.send(textPacket);
							text = "";
							textAreaSelected = false;
						}
						else if(readyButtonRect.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
							sf::Packet readyPacket;
							playerReady = !playerReady;
							sf::Uint8 header = dataType::playerReady;
							readyPacket << header << playerReady;
							socket.send(readyPacket);
							textAreaSelected = false;
						}
						else {
							textAreaSelected = false;
						}
						scrollBar.setFillColor(sf::Color(46, 99, 110, 255));
						scrollBarClicked = false;
					}
					break;
					
			case sf::Event::MouseWheelMoved:
				if (chat.size() > 25) {
					if (event.mouseWheel.delta > 0) {
						lineOffSetByScrolling++;
						if (lineOffSetByScrolling > chat.size() - 25) {
							lineOffSetByScrolling = chat.size() - 24;
						}
					}
					else {
						lineOffSetByScrolling--;
						if (lineOffSetByScrolling < 0) {
							lineOffSetByScrolling = 0;
						}
					}
				}
				break;

			case sf::Event::TextEntered:
				if (textAreaSelected) {
					if (event.text.unicode != '\b') {
						if (event.text.unicode != '\r') {
							if(sf::Text(id+": "+text, font, 20).getLocalBounds().width < sendTextBackground.getLocalBounds().width)
							text += event.text.unicode;
						}
					}
					else {
						if (!text.empty()) {
							text.erase(text.size() - 1, 1);
						}
					}
				}
				break;

			}
		}

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
						sf::Text displayText(playerList[userId].name + ": " + recievedText, font, 20);
						displayText.setFillColor(teamColors[userId]);
						chat.push_back(displayText);
					}
					break;
				}
				case dataType::MessageStatus:
					bool relayed;
					recievePacket >> relayed;
					if (relayed) {
						//chat.back().setFillColor(sf::Color::White);
					}
					break;
				case dataType::UserList:
					playerList.clear();
					int numberOfUsers;
					recievePacket >> numberOfUsers;
					for (int i = 0; i < numberOfUsers; i++) {
						std::string tempString;
						recievePacket >> tempString;
						playerList.push_back({ tempString, 0 });
					}
					break;
				case dataType::Ping:
					socket.send(pingPacket);
					for (playersClient& player : playerList) {
						recievePacket >> player.lastPing;
					}
					break;
				case dataType::playerReady:
					for (playersClient& player : playerList) {
						recievePacket >> player.isReady;
					}
			}
		}

		window.draw(sendTextBackground);
		sf::Text playerWindowText("Players in lobby:", font, 20);
		playerWindowText.setFillColor(sf::Color::White);
		playerWindowText.setOutlineColor(sf::Color::Black);
		playerWindowText.setOutlineThickness(2);
		playerWindowText.setPosition(48, 48);
		window.draw(playerWindowText);
		sf::Text playerPingText("Ping:", font, 20);
		playerPingText.setFillColor(sf::Color::White);
		playerPingText.setOutlineColor(sf::Color::Black);
		playerPingText.setOutlineThickness(2);
		playerPingText.setPosition(1136, 48);
		window.draw(playerPingText);
		int playerCounter = 0;
		for (playersClient& player : playerList) {
			readyIcon.setPosition(52, 8+68 + (20 * playerCounter));
			if (player.isReady) {
				readyIcon.setFillColor(sf::Color::Green);
			}
			else {
				readyIcon.setFillColor(sf::Color::Red);
			}
			window.draw(readyIcon);
			sf::Text playerWindowText(player.name, font, 20);
			playerWindowText.setFillColor(teamColors[playerCounter]);
			playerWindowText.setOutlineColor(sf::Color::Black);
			playerWindowText.setOutlineThickness(2);
			playerWindowText.setPosition(72, 68+(20*playerCounter));
			window.draw(playerWindowText);
			sf::Text playerPingText(std::to_string(player.lastPing)+" ms", font, 20);
			playerPingText.setFillColor(teamColors[playerCounter]);
			playerPingText.setOutlineColor(sf::Color::Black);
			playerPingText.setOutlineThickness(2);
			playerPingText.setPosition(1136, 68 + (20 * playerCounter));
			window.draw(playerPingText);
			playerCounter++;
		}

		int lineCounter = 0;
		for (sf::Text chatLine : chat) {
			if (chat.size() < 25 || chat.size() - lineCounter - lineOffSetByScrolling < 25) {
				if (textLine < 25) {
					chatLine.setPosition(48, (textLine * 20) + 490);
					chatLine.setOutlineColor(sf::Color::Black);
					chatLine.setOutlineThickness(2);
					window.draw(chatLine);
					textLine++;
				}
			}
			lineCounter++;
		}

		sf::Text drawText(text, font, 20);
		drawText.setFillColor(sf::Color::Green);
		drawText.setOutlineColor(sf::Color::Black);
		drawText.setOutlineThickness(2);
		drawText.setPosition(48, 1010);
		window.draw(drawText);
		window.draw(sendButtonRect);
		window.draw(sendButton);
		window.draw(readyButtonRect);
		window.draw(readyButton);
		if (chat.size() > 25) {
			float percentageOfScollMovement = static_cast<float>(lineOffSetByScrolling) / (static_cast<float>(chat.size()) - 25.f);
			float yOffsetScrollbar = static_cast<float>(maxScrollPosition) - (percentageOfScollMovement * static_cast<float>(scrollSpace));
			if (yOffsetScrollbar < minScrollPosition) {
				yOffsetScrollbar = minScrollPosition;
			}
			scrollBar.setPosition(1218, floor(yOffsetScrollbar));
			window.draw(scrollBar);
			window.draw(up);
			window.draw(down);
		}
		window.display();
	}
}

int main()
{
	std::string whutDo;
	std::cout << "Start as (S)erver or (C)lient?" <<std::endl;
	std::cin >> whutDo;
	int port = 8756;

	if (whutDo == "s" || whutDo == "S") {
		sf::Thread serverThread(&server, port);
		serverThread.launch();
		client(true, port);
	}
	else if (whutDo == "c" || whutDo == "C") {
		client(false, port);
	}
}