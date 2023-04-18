#pragma once
#include <WinSock2.h>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include "DataTypes.h"


class Client
{
public:
	Client(SOCKET socket, unsigned int id, const SocketCallbacks& callbacks);
	~Client();

	void write(const std::string& data);

private:
	void read();
	void stop();
	void dispatchOnClose();
	void dispatchOnReceive(const std::string& data);

private:
	SOCKET m_socket;
	unsigned int m_id;
	SocketCallbacks m_callbacks;

	std::unique_ptr<std::thread> m_readerThread;
	std::unique_ptr<std::thread> m_writerThread;
	std::atomic_bool m_canRunReader;
	std::atomic_bool m_canRunWriter;
};