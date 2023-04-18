#include "Server.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <unordered_map>

#include <iostream>
#include "Client.h"
#include <mutex>

#define BUF_SIZE  100

struct Server::Impl 
{
	Impl(const IpEndpoint& ipEndPoint, unsigned int maxCon /*= 20*/)
		:m_ipEndpoint(ipEndPoint), m_maxConnection(maxCon)
	{
		m_disconnectedClients.reserve(m_maxConnection);
	}

	~Impl()
	{
		stop();
	}

	bool start()
	{
		if (m_isInitialized)
		{
			std::cout << "Server already started" << std::endl;
			return true;
		}

		WSADATA wsaData;
		int wsaret = WSAStartup(0x101, &wsaData);
		if (wsaret != 0)
		{
			std::cout << "Win sock initialization failed" << std::endl;
			return false;
		}
		m_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (m_serverSocket == INVALID_SOCKET)
		{
			std::cout << "Server socket creation failed" << std::endl;
			return false;
		}

		auto localAddr = makeSockAddr();
		if (bind(m_serverSocket, (sockaddr*)&localAddr, sizeof(localAddr)) != 0)
		{
			std::cout << "Server failed to start" << std::endl;
			return false;
		}

		std::cout << "Server started successfully" << std::endl;

		m_isInitialized = true;
		m_canRunListenThread = true;
		m_listeningThread = std::make_unique<std::thread>(&Impl::Listen, this);
		return true;
	}

	void stop()
	{
		if (!m_isInitialized)
		{
			std::cout << "Server not started" << std::endl;
			return;
		}
		if (m_serverSocket != INVALID_SOCKET)
		{			
			closesocket(m_serverSocket);
		}
		WSACleanup();
		m_isInitialized = false;

		m_canRunListenThread = false;
		m_listeningThread->join();

		m_canRunCleaningThread = false;
		m_clientCleaningThread->join();

		std::cout << "Server stopped successfully" << std::endl;
	}

	void Listen()
	{
		std::cout << "Server listening..." << std::endl;
		if (listen(m_serverSocket, m_maxConnection) != 0)
		{
			std::cout << "Server listening failed" << std::endl;
			return;
		}

		sockaddr_in clientAddr;
		int clientAddrSize = sizeof(clientAddr);
		SOCKET newClient;

		char temp[BUF_SIZE];
		unsigned int clientid = 0;

		SocketCallbacks callbacks;
		callbacks.OnDataReceivedFunc = [&](const std::string& data) {};
		callbacks.OnDisconnectFunc = [&](unsigned int id) {onClientDisconnect(id); };

		m_canRunCleaningThread = true;
		m_clientCleaningThread = std::make_unique<std::thread>(&Impl::cleanClients, this);
		
		while (m_canRunListenThread)
		{
			newClient = accept(m_serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

			memset(temp, 0, BUF_SIZE);
			auto ipAddress = inet_ntop(AF_INET, &clientAddr.sin_addr, temp, INET_ADDRSTRLEN);
			std::cout << "client joined from " << ipAddress << std::endl;

			clientid++;			 
			m_clients.insert({ clientid, std::make_shared<Client>(newClient, clientid, callbacks)});
			std::this_thread::sleep_for(std::chrono::seconds(2));
		}
	}

	void onClientDisconnect(unsigned int id)
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		m_disconnectedClients.emplace_back(id);
	}

	void cleanClients()
	{
		while (m_canRunCleaningThread)
		{
			
			std::unique_lock<std::mutex> m_ulk(m_mutex);
			if (m_disconnectedClients.empty())
			{
				m_ulk.unlock();
				std::this_thread::sleep_for(std::chrono::seconds(1));
				continue;
			}
			auto disconnectedClientCopy = m_disconnectedClients;
			m_disconnectedClients.clear();
			m_ulk.unlock();

			for (auto id : disconnectedClientCopy)
			{
				auto itr = m_clients.find(id);
				if (itr == m_clients.end())
				{
					return;
				}
				m_clients.erase(itr);
			}			
		}
		
	}

	sockaddr_in makeSockAddr()
	{
		sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET; //Address family
		//serverAddr.sin_addr.s_addr = INADDR_ANY; //Wild card IP address
		serverAddr.sin_port = htons((u_short)m_ipEndpoint.Port); //port to use
		inet_pton(AF_INET, m_ipEndpoint.IpAddress.c_str(), &serverAddr.sin_addr);
		return serverAddr;
	}

	IpEndpoint m_ipEndpoint;
	unsigned int m_maxConnection;

	SOCKET m_serverSocket;

	std::atomic_bool m_canRunCleaningThread;
	std::unique_ptr<std::thread> m_clientCleaningThread;
	std::vector<unsigned int> m_disconnectedClients;
	std::mutex m_mutex;

	std::atomic_bool m_canRunListenThread;
	std::unique_ptr<std::thread> m_listeningThread;
	
	std::atomic_bool m_isInitialized;

	std::unordered_map<SOCKET, std::shared_ptr<Client>> m_clients;
	
};

Server::Server(const IpEndpoint& ipEndPoint, unsigned int maxCon /*= 20*/)
	:m_impl(std::make_unique<Impl>(ipEndPoint, maxCon))
{
}

Server::~Server()
{
}

bool Server::start()
{
	return m_impl->start();
}

void Server::stop()
{
	m_impl->stop();
}