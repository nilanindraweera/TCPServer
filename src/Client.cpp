#include "Client.h"
#include <ws2tcpip.h>
#include <iostream>

#define BUF_SIZE  100

Client::Client(SOCKET socket, unsigned int id, const SocketCallbacks& callbacks)
	:m_socket(socket), m_id(id), m_callbacks(callbacks)
{
	m_canRunReader = true;
	m_readerThread = std::make_unique<std::thread>(&Client::read, this);

	m_canRunWriter = true;
	m_writerThread = std::make_unique<std::thread>(&Client::write, this, "Server Message_");
}

Client::~Client()
{
	stop();
}

void Client::write(const std::string& data)
{
	char buffer[BUF_SIZE];
	std::string msg = data;
	int i = 0;

	while (m_canRunWriter)
	{
		memset(buffer, 0, BUF_SIZE);
		auto newMsg = msg + std::to_string(i++);
		strcpy_s(buffer, BUF_SIZE, newMsg.c_str());

		auto writtenBytes = send(m_socket, buffer, newMsg.size(), 0);
		if (writtenBytes > 0)
		{
			std::cout << "Written " << std::to_string(writtenBytes) << " bytes" << std::endl;
		}
		else
		{
			std::cout << "Client " << std::to_string(m_id) <<" disconnected" << std::endl;
			dispatchOnClose();
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

void Client::read()
{
	char buffer[BUF_SIZE];

	while (m_canRunReader)
	{
		memset(buffer, 0, BUF_SIZE);
		auto readBytes = recv(m_socket, buffer, BUF_SIZE, 0);// MSG_WAITALL);
		if (readBytes > 0)
		{
			std::string newData(buffer, readBytes);
			std::cout << "Read " << std::to_string(readBytes) << " bytes : " << newData << std::endl;
			dispatchOnReceive(newData);
		}
		else
		{
			std::cout << "Client " << std::to_string(m_id) << " disconnected" << std::endl;
			dispatchOnClose();
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

void Client::stop()
{
	m_canRunWriter = false;
	m_canRunReader = false;

	m_readerThread->join();
	m_writerThread->join();
}

void Client::dispatchOnClose()
{
	if (m_callbacks.OnDisconnectFunc)
	{
		m_callbacks.OnDisconnectFunc(m_id);
	}
}

void Client::dispatchOnReceive(const std::string& data)
{
	if (m_callbacks.OnDataReceivedFunc)
	{
		m_callbacks.OnDataReceivedFunc(data);
	}
}
