#pragma once
#include <string>
#include <functional>

struct IpEndpoint
{
	std::string IpAddress;
	unsigned int Port;
};

struct SocketCallbacks 
{
	using OnDisconnectFuncDef = std::function<void(int)>;
	using OnDataReceivedFuncDef = std::function<void(const std::string&)>;

	OnDisconnectFuncDef OnDisconnectFunc;
	OnDataReceivedFuncDef OnDataReceivedFunc;
};