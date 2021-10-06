#include "connectionSetupScreen.h"
#include "main.h"
#include "connection.h"

void connectionSetupScreen::showConnectionSetupScreen()
{
	while (!this->connectionEstablished) {
		this->interact();
		this->drawConnectionSetupScreen();
	}
}

void connectionSetupScreen::interact()
{
	currentTime = clock.getElapsedTime();
	sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
	detectMouseOvers(mousePosition);

	sf::Event event;
	while (window.pollEvent(event)) {
		switch (event.type) {

		case sf::Event::KeyPressed:
			if (event.key.code == sf::Keyboard::Escape || event.Closed) {
				window.close();
			}
			else if (event.key.code == sf::Keyboard::Return) {

			}
			break;

		case event.MouseButtonPressed:
			if (event.mouseButton.button == sf::Mouse::Left) {

			}
			break;

		case event.MouseButtonReleased:
			if (event.mouseButton.button == sf::Mouse::Left) {
				currentTextBoxSelected = textBoxSelected::None;
				if (ipAdressInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
					currentTextBoxSelected = textBoxSelected::ipAdress;
					ipAdressInputField.setOutlineColor(sf::Color::Yellow);
				}
				else if (portNumberInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
					currentTextBoxSelected = textBoxSelected::portNumber;
					portNumberInputField.setOutlineColor(sf::Color::Yellow);
				}
				else if (playerNameInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
					currentTextBoxSelected = textBoxSelected::playerName;
					playerNameInputField.setOutlineColor(sf::Color::Yellow);
				}
				else if (hostButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
					if (!portNumberString.empty()) {
						if (!playerNameString.empty()) {
							networkStuff::port = std::stoi(portNumberString);
							serverThread.launch();
							std::string ipString = "127.0.0.1";
							sf::IpAddress ip = sf::IpAddress(ipString);
							this->connectionEstablished = currentConnection.connect(ip, std::stoi(portNumberString), playerNameString);
							if (!connectionEstablished) {
								errorMessages.push_back({ currentTime.asMilliseconds(), "Can't connect to myself, this could be due to a firewall or inability to open the specified port!" });
							}
						}
						else {
							errorMessages.push_back({ currentTime.asMilliseconds(), "Please enter a nanme first!" });
						}
					}
					else {
						errorMessages.push_back({ currentTime.asMilliseconds(), "Please enter a port number first!" });
					}
				}
				else if (joinButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
					if (!ipAdressString.empty()) {
						if (!portNumberString.empty()) {
							if (!playerNameString.empty()) {
								this->connectionEstablished = currentConnection.connect(ipAdressString, std::stoi(portNumberString), playerNameString);
								if (!connectionEstablished) {
									errorMessages.push_back({ currentTime.asMilliseconds(), "Can't connect to remote server!!! Check IP, Port and firewall/NAT settings" });
								}
							}
							else {
								errorMessages.push_back({ currentTime.asMilliseconds(), "Please enter a nanme first!" });
							}
						}
						else {
							errorMessages.push_back({ currentTime.asMilliseconds(), "Please enter a port number first!" });
						}
					}
					else {
						errorMessages.push_back({ currentTime.asMilliseconds(), "Please enter a IPv4 adress first!" });
					}
				}
			}
			break;

		case sf::Event::TextEntered:
			switch(currentTextBoxSelected) {
			case textBoxSelected::ipAdress:
				if (event.text.unicode != '\b') {
					if (event.text.unicode != '\r') {
						if (sf::Text(ipAdressString, font, 20).getLocalBounds().width < ipAdressInputField.getLocalBounds().width) {
							ipAdressString += event.text.unicode;
						}
					}
				}
				else {
					if (!ipAdressString.empty()) {
						ipAdressString.erase(ipAdressString.size() - 1, 1);
					}
				}
				break;
			case textBoxSelected::playerName:
				if (event.text.unicode != '\b') {
					if (event.text.unicode != '\r') {
						if (sf::Text(playerNameString, font, 20).getLocalBounds().width < playerNameInputField.getLocalBounds().width) {
							playerNameString += event.text.unicode;
						}
					}
				}
				else {
					if (!playerNameString.empty()) {
						playerNameString.erase(playerNameString.size() - 1, 1);
					}
				}
				break;
			case textBoxSelected::portNumber:
				if (event.text.unicode != '\b') {
					if (event.text.unicode != '\r') {
						if (sf::Text(portNumberString, font, 20).getLocalBounds().width < portNumberInputField.getLocalBounds().width) {
							portNumberString += event.text.unicode;
						}
					}
				}
				else {
					if (!portNumberString.empty()) {
						portNumberString.erase(portNumberString.size() - 1, 1);
					}
				}
				break;
			}
			break;
		}
	}

	errorMessages.erase(std::remove_if(
		errorMessages.begin(), errorMessages.end(),
		[this](const errorMessage& mess) {
			if (mess.timeAdded + 10000 < currentTime.asMilliseconds()) {
				return true;
			}
			else {
				return false;
			}
		}), errorMessages.end());

		
}

void connectionSetupScreen::detectMouseOvers(sf::Vector2i& mousePosition)
{
	hostButton.setOutlineColor(sf::Color::White);
	joinButton.setOutlineColor(sf::Color::White);
	if (currentTextBoxSelected != textBoxSelected::ipAdress) {
		ipAdressInputField.setOutlineColor(sf::Color::White);
	}
	if (currentTextBoxSelected != textBoxSelected::portNumber) {
		portNumberInputField.setOutlineColor(sf::Color::White);
	}
	if (currentTextBoxSelected != textBoxSelected::playerName) {
		playerNameInputField.setOutlineColor(sf::Color::White);
	}
	
	if (ipAdressInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
		if (currentTextBoxSelected != textBoxSelected::ipAdress) {
			ipAdressInputField.setOutlineColor(sf::Color::Red);
		}

	}
	else if (portNumberInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
		if (currentTextBoxSelected != textBoxSelected::portNumber) {
			portNumberInputField.setOutlineColor(sf::Color::Red);
		}

	}
	else if (playerNameInputField.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
		if (currentTextBoxSelected != textBoxSelected::playerName) {
			playerNameInputField.setOutlineColor(sf::Color::Red);
		}
	} 
	else if (hostButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
		hostButton.setOutlineColor(sf::Color::Yellow);
	}
	else if (joinButton.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePosition))) {
		joinButton.setOutlineColor(sf::Color::Yellow);
	}

}

void connectionSetupScreen::drawConnectionSetupScreen()
{
	ipAdressText.setString(ipAdressString);
	portNumberText.setString(portNumberString);
	playerNameText.setString(playerNameString);
	window.clear();
	window.draw(splashScreenSprite);
	window.draw(overlay);
	window.draw(ipAdressLabel);
	window.draw(portNumberLabel);
	window.draw(playerNameLabel);
	window.draw(ipAdressInputField);
	window.draw(portNumberInputField);
	window.draw(playerNameInputField);
	window.draw(hostButton);
	window.draw(joinButton);
	window.draw(ipAdressText);
	window.draw(portNumberText);
	window.draw(playerNameText);
	window.draw(hostLabel);
	window.draw(joinLabel);

	for (errorMessage& mess : errorMessages) {
		errorText.setString(mess.error);
		window.draw(errorText);
		errorText.move(0, 30);
	}
	errorText.setPosition(48, 550);

	window.display();
}
