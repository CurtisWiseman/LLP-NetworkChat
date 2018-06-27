// ChatServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <SFML\Network.hpp>
#include "..\Common\MessageTypes.h" 
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "Client.h"

constexpr int SERVER_TCP_PORT(53000);
constexpr int SERVER_UDP_PORT(53001);

using TcpClient = sf::TcpSocket;
using TcpClientPtr = std::unique_ptr<TcpClient>;
using TcpClients = std::vector<Client>;

bool bindServerPort(sf::TcpListener&);
void listen(sf::TcpListener&, sf::SocketSelector&, TcpClients&);
void connect(sf::TcpListener&, sf::SocketSelector&, TcpClients&);
void recieveMsg(TcpClients&, sf::SocketSelector&);
void ping(TcpClients&);
void runServer();

int main()
{
	runServer();
    return 0;
}

bool bindServerPort(sf::TcpListener& listener)
{
	if (listener.listen(SERVER_TCP_PORT) != sf::Socket::Done)
	{
		std::cout << "Could not bind server port";
		std::cout << std::endl << "Port: " << SERVER_TCP_PORT;
		std::cout << std::endl;
		return false;
	}

	std::cout << "Server launched on port: " << SERVER_TCP_PORT << std::endl;;
	std::cout << "Scanning for messages..." << std::endl;
	return true;
}

void listen(sf::TcpListener& listener, sf::SocketSelector& selector, TcpClients& clients)
{
	while (true)
	{
		const sf::Time timeout = sf::Time(sf::milliseconds(250));
		if (selector.wait(timeout))
		{
			if (selector.isReady(listener))
			{
				connect(listener, selector, clients);
			}
			else
			{
				recieveMsg(clients, selector);
			}
		}
		else
		{
			ping(clients);
		}
	}
}

void connect(sf::TcpListener& listener, sf::SocketSelector& selector, TcpClients& clients)
{
	auto client_ptr = new sf::TcpSocket;
	auto& client_ref = *client_ptr;
	if (listener.accept(client_ref) == sf::Socket::Done)
	{
		selector.add(client_ref);

		auto client = Client(client_ptr);
		clients.push_back(std::move(client));
		std::cout << "Connected to client on port " << SERVER_TCP_PORT 
			<< " with ID" << client.getClientID() << std::endl;
		
	}
}

void recieveMsg(TcpClients& senders, sf::SocketSelector& selector)
{
	for (auto& sender : senders)
	{
		auto& sender_ref = sender.getSocket();
		if (selector.isReady(sender_ref) && sender.isConnected())
		{
			sf::Packet packet;
			sender_ref.receive(packet);
			int header = 0;
			packet >> header;

			NetMsg msg = static_cast<NetMsg>(header);
			if (msg == NetMsg::CHAT)
			{
				std::string string;
				packet >> string;

				std::cout << "Message recieved: '" << string << "' from client ID" << sender.getClientID() << std::endl;
				std::cout << "Latency: " << sender.getLatency().count() << "us" << std::endl;

				for (auto& client : senders)
				{
					client.getSocket().send(packet);
				}
			}
			else if (msg == NetMsg::PONG)
			{
				sender.pong();
			}
			else if (msg == NetMsg::INVALID || msg == NetMsg::DISCONNECT)
			{
				std::cout << "Invalid/Disconnect message recieved from client ID" 
					<< sender.getClientID() << ", disconnecting..." << std::endl;
				packet.clear();
				//DC
				sender.disconnect();
			}
		}
	}
}

void ping(TcpClients& tcp_clients)
{
	constexpr auto timeout = 10s;
	for (auto& client : tcp_clients)
	{
		if (client.isConnected())
		{
			const auto& timestamp = client.getPingTime();
			const auto now = std::chrono::steady_clock::now();
			auto delta = now - timestamp;
			if (delta > timeout)
			{
				client.pingFailed();
				std::cout << "Client ping timeout: ID" << client.getClientID() << std::endl;
				std::cout << "Ping request sent to client ID" << client.getClientID() << std::endl;
				client.ping();
			}
			else
			{
				client.pingSuccess();
			}
			const int ping_out = 5;
			if (client.getFailedPings() > ping_out)
			{
				std::cout << "No response from client ID" << client.getClientID()
					<< " after " << ping_out << " pings. Disconnecting..." << std::endl;
				//Disconnect
				client.disconnect();
			}
		}
	}
}

void runServer()
{
	sf::TcpListener listener;
	if (!bindServerPort(listener))
	{
		return;
	}

	sf::SocketSelector selector;
	selector.add(listener);

	TcpClients clients;
	return listen(listener, selector, clients);
}