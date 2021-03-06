#include <SDL.h>
#include <SDL_net.h>
#include <iostream>
#include <vector>
#include <cstring>

#define SDL_reinterpret_cast(type, expression)  reinterpret_cast<type>(expression)

struct data {
	TCPsocket socket;
	Uint32 timeout;
	int id; // player/client ID
	data(TCPsocket sock, Uint32 t, int i) :socket(sock), timeout(t), id(i) {}
};

int main(int argc, char ** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDLNet_Init();
	int curid = 0;
	int playernum = 0;

	SDL_Event event;

	IPaddress ip;
	// The server itself
	SDLNet_ResolveHost(&ip, NULL, 5555);

	std::vector<data> socketvector;

	char tmp[1400];
	bool running = true;

	//create a socket set that will be able to watch up to a given amount of sockets.
	SDLNet_SocketSet sockets = SDLNet_AllocSocketSet(30);

	//To create a listening TCP socket on port 1234 (server):
	TCPsocket server = SDLNet_TCP_Open(&ip);

	SDL_Window *screen = SDL_CreateWindow("Server", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	SDL_Renderer *m_pRender = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);

	while (running)
	{
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
				running = false;
		}
		//Accept an incoming connection on the server TCPsocket.

		// The input parameter "server" is the server TCPsocket which was previously created by SDLNet_TCP_Open.

		// "server" socket is never connected to a remote host. What you get back is a new "TCPsocket" that is connected to the remote host.

		//This is a non-blocking call, so if no connections are there to be accepted, you will get a NULL TCPsocket and the program will continue going.

		TCPsocket tmpsocket = SDLNet_TCP_Accept(server);
		if (tmpsocket)
		{
			if (playernum < 30)
			{
				SDLNet_TCP_AddSocket(sockets, tmpsocket);
				socketvector.push_back(data(tmpsocket, SDL_GetTicks(), curid));
				playernum++;
				sprintf_s(tmp, "0 %d \n", curid);
				curid++;
				std::cout << "New connection: " << curid << '\n';
			}
			else {
				sprintf_s(tmp, "3 \n");
			}
			SDLNet_TCP_Send(tmpsocket, tmp, strlen(tmp) + 1);
		}
		//check for incoming data
		// Check all sockets in the socket set for activity. If a non-zero timeout is given then this function will wait for activity, or else it will wait for timeout milliseconds.
		while (SDLNet_CheckSockets(sockets, 0) > 0)
		{
			for (int i = 0; i < socketvector.size(); i++)
			{
				//Check whether a socket has been marked as active.

				if (SDLNet_SocketReady(socketvector[i].socket))
				{
					socketvector[i].timeout = SDL_GetTicks();
					memset(tmp, 0, sizeof(tmp));
					SDLNet_TCP_Recv(socketvector[i].socket, tmp, 1400);

					int num = tmp[0] - '0';
					int j = 1;
					while (tmp[j] >= '0' && tmp[j] <= '9')
					{
						num *= 10;
						num += tmp[j] - '0';
						j++;
					}

					if (num == 1)
					{
						std::cout << "Message Type 1: " << socketvector[i].id << '\n';
						for (int k = 0; k < socketvector.size(); k++)
						{
							if (k == i)
								continue;
							SDLNet_TCP_Send(socketvector[k].socket, tmp, strlen(tmp) + 1);
						}
					}
					else if (num == 2) {
						std::cout << "Message Type 2: " << socketvector[i].id << '\n';
						//disconnect
						for (int k = 0; k < socketvector.size(); k++)
						{
							if (k == i)
								continue;
							SDLNet_TCP_Send(socketvector[k].socket, tmp, strlen(tmp) + 1);
						}
						SDLNet_TCP_DelSocket(sockets, socketvector[i].socket);
						SDLNet_TCP_Close(socketvector[i].socket);
						socketvector.erase(socketvector.begin() + i);
						playernum--;
					}
					else if (num == 3) {
						std::cout << "Message Type 3: " << socketvector[i].id << '\n';
						//one player shot another one
						int tmpvar;
						sscanf_s(tmp, "3 %d", &tmpvar);
						std::cout << tmp << std::endl;
						for (int k = 0; k < socketvector.size(); k++)
						{
							if (socketvector[k].id == tmpvar)
							{
								SDLNet_TCP_Send(socketvector[k].socket, tmp, strlen(tmp) + 1);
								break;
							}
						}
					}
				}
			}
		}
		// disconnect, timeout
		for (int j = 0; j < socketvector.size(); j++)
			if (SDL_GetTicks() - socketvector[j].timeout > 100000)
			{
				sprintf_s(tmp, "2 %d \n", socketvector[j].id);
				for (int k = 0; k < socketvector.size(); k++)
				{
					SDLNet_TCP_Send(socketvector[k].socket, tmp, strlen(tmp) + 1);
				}
				SDLNet_TCP_DelSocket(sockets, socketvector[j].socket);
				SDLNet_TCP_Close(socketvector[j].socket);
				socketvector.erase(socketvector.begin() + j);
				playernum--;
			}
		SDL_Delay(1);
	}
	for (int i = 0; i < socketvector.size(); i++)
		SDLNet_TCP_Close(socketvector[i].socket);
	SDLNet_FreeSocketSet(sockets);
	SDLNet_TCP_Close(server);
	SDLNet_Quit();
	SDL_Quit();
	return 0;
}