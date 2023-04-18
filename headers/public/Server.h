#pragma once
#include "DataTypes.h"
#include <memory>

class Server
{
public:
	explicit Server(const IpEndpoint& ipEndPoint, unsigned int maxCon = 20);
	~Server();

	bool start();
	void stop();
	
private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};