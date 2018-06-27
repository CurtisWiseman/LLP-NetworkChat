#include "stdafx.h"
#include <future>
#include <iostream>
#include <string>
#include "..\Common\MessageTypes.h"
#include <SFML\Network.hpp>

const sf::IpAddress SERVER_IP("127.0.0.1");
constexpr int SERVER_TCP_PORT(53000);

using TcpClient = sf::TcpSocket;
using TcpClientPtr = std::unique_ptr<TcpClient>;
using TcpClients = std::vector<TcpClientPtr>;


void client();
bool connect(TcpClient&);
void input(TcpClient&);

int main()
{
	client();
	return 0;
}

bool connect(TcpClient& socket)
{
	//Attempt to connect to server
	auto status = socket.connect(SERVER_IP, SERVER_TCP_PORT);
	if (status != sf::Socket::Done)
	{
		return false;
	}

	//If successful
	std::cout << "Connected to server: " << SERVER_IP;
	std::cout << std::endl;

	return true;
}

void client()
{
	TcpClient socket;
	if (!connect(socket))
	{
		return;
	}

	

	auto handle = std::async(std::launch::async, [&]
	{
		//Track the status of the socket
		sf::Socket::Status status;

		do
		{
			sf::Packet packet;
			status = socket.receive(packet);
			if (status == sf::Socket::Done)
			{
				int header = 0;
				packet >> header;

				NetMsg msg = static_cast<NetMsg>(header);
				if (msg == NetMsg::CHAT)
				{
					std::string str;
					packet >> str;
					std::cout << "Recieved message from server: " << str << std::endl;
				}
				else if (msg == NetMsg::PING)
				{
					std::cout << "Ping request recieved, sending pong" << std::endl;
					sf::Packet pingMsg;
					pingMsg << NetMsg::PONG;
					socket.send(pingMsg);
				}
				
			}
		} while (status != sf::Socket::Disconnected);
	}
	);

	return input(socket);
}

void input(TcpClient& socket)
{
	while (true)
	{
		std::string input;
		std::getline(std::cin, input);

		sf::Packet pack;
		pack << NetMsg::CHAT << input;
		socket.send(pack);
	}
}