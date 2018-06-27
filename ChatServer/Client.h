#pragma once

#include <SFML\Network.hpp>
#include <memory>
#include <chrono>

using namespace std::chrono_literals;
using s_clock = std::chrono::steady_clock;
using microSecs = std::chrono::microseconds;

class Client
{
public:
	//Construction and assignment
	Client() = default;
	Client(sf::TcpSocket* _socket);
	Client(Client&& _other);
	Client& operator=(Client&& rhs);

	//Comparison
	bool operator==(const Client& rhs) { return this->id == rhs.id; }

	//Getters
	sf::TcpSocket& getSocket()		{ return *socket; }
	int getClientID()			const { return this->id; }
	const auto& getLatency()	const { return this->latency; }
	const auto& getPingTime()	const { return timestamp; }
	int getFailedPings()			{ return failed_pings; }
	bool isConnected()				{ return connected; }

	//Setters
	void setLatency(microSecs _ms)	{ latency = _ms; }
	void setConnected(bool _con)	{ connected = _con; }

	//Functions
	void ping();
	void pingFailed() { failed_pings++; }
	void pingSuccess() { failed_pings = 0; }
	void pong();
	void disconnect();

private:
	std::unique_ptr<sf::TcpSocket> socket = nullptr;
	static unsigned int next_id;
	unsigned int id = next_id++;
	microSecs latency = 100us;
	s_clock::time_point timestamp = s_clock::now();
	int failed_pings = 0;
	bool connected = false;
};