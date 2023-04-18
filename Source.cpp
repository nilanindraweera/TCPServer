#include <iostream>
#include "Server.h"
#include <thread>

int main(int argc, char** argv)
{
	IpEndpoint ipendpoint{ "192.168.100.5", 5000 };
	Server server{ ipendpoint };
	if (server.start())
	{
		std::cout << "Server IpEndpoint >> " << ipendpoint.IpAddress << ":" << std::to_string(ipendpoint.Port) << std::endl;
	}

	std::this_thread::sleep_for(std::chrono::seconds(2000));
	return 0;
}