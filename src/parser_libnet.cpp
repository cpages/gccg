/*
    Gccg - Generic collectible card game.
    Copyright (C) 2001,2002,2003,2004 Tommi Ronkainen

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program, in the file license.txt. If not, write
  to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA 02111-1307, USA.
*/

#include <list>
#include <vector>
#include <time.h>
#include <signal.h>
#include "SDL_net.h"
#include "SDL_thread.h"
#include "parser.h"

#define MAX_CONNECTIONS 1024

#ifdef WIN32
// Buffer size is way overkill and would cause stackoverflows on Windows.
// Check with Tommi if this it really needs a 1MB buffer...
#define BUFFER_SIZE 128*1024
#else
#define BUFFER_SIZE 1024*1024
#endif

typedef void (*sighandler_t)(int);

//
// TPC/IP Library
// --------------

namespace Evaluator
{
	namespace Libnet
	{
// Server variables
		static bool server_created=false; // Only one server needed.
		static IPaddress serverIP; // IP address of the server.
		static TCPsocket servsock = NULL; // Server socket.
		static SDLNet_SocketSet socketset = NULL; // Current socket set to listen.

		struct ClientData {
			SDL_mutex *lock; // Mutex to lock this entry.
			int number; // Connection number.
			TCPsocket sock; // Client socket.
			IPaddress peer; // Clint address.
			string read_buffer; // Buffer for reading.
			string write_buffer; // Buffer for writing.
			SDL_sem *wait; // This semphore signals (value 1) when there are events for writer thread.
			bool writing; // This flag is set when writer thread is writing to the socket.
			bool writer_eof; // Writer thread can exit when finished.
			bool writer_pipe; // Writer have encountered an error.
			bool closed; // Socket is going to close soon. Don't append data to write buffer anymore.
			SDL_Thread *writer_thread; // Thread performing writing.
		};
		
		static ClientData people[MAX_CONNECTIONS];
		
		static list<Data> server_event_buffer; // Events received from server sockets.

// Client variables
		static SDLNet_SocketSet client_socketset = NULL;
		static vector<TCPsocket> connections;
		static string client_buffer[MAX_CONNECTIONS];
		static list<Data> event_buffer;

// Support functions

		// Ignore SIG PIPE.
		static void IgnoreSignal(int s)
		{
			cerr << "Warning: SIG PIPE received" << endl;
		}

		void QuitSignal(int s)
		{
			cout << "Warning: Signal " << s << " received." << endl;
			Evaluator::quitsignal=true;
		}
		
		// Server writer thread
		static int writer_thread(void *data)
		{
			ClientData& client=*(ClientData*)(data);

			TCPsocket socket;
			bool eof;
			string writestr;
			int len;

			SDL_LockMutex(client.lock);
//			int number=client.number;
			socket=client.sock;
			SDL_UnlockMutex(client.lock);

//			cout << "Thread " << number << ": started" << endl;

			while(1)
			{
				SDL_SemWait(client.wait);
				
				SDL_LockMutex(client.lock);
				eof=client.writer_eof;
				writestr=client.write_buffer;
				client.write_buffer="";
				SDL_UnlockMutex(client.lock);

				if(writestr != "")
				{
//					cout << "Thread " << number << ": send " << writestr << endl;
					SDL_LockMutex(client.lock);
					client.writing=true;
					SDL_UnlockMutex(client.lock);
					
					len=SDLNet_TCP_Send(socket, (char *)writestr.c_str(), writestr.length());
					
					SDL_LockMutex(client.lock);
					client.writing=false;
					SDL_UnlockMutex(client.lock);

					if(len != (int)writestr.length())
					{
						SDL_LockMutex(client.lock);
						client.writer_pipe=true;
						SDL_UnlockMutex(client.lock);
					}
				}
				
				if(eof)
				{
					SDL_LockMutex(client.lock);
					SDLNet_TCP_Close(client.sock);
					client.sock = NULL;
					client.read_buffer="";
					SDL_UnlockMutex(client.lock);
					break;
				}				
			}

//			cout << "Thread " << number << ": finished" << endl;
			
			return 0;
		}
		
// Library functions
		/// net_create_server(p) - Initialize server on TCP-port
		/// $p$. Throw {\tt Parser::LangErr} if socket initialization
		/// fails.
		Data net_create_server(const Data& arg)
		{
			if(server_created)
				throw LangErr("net_create_server","server already created");
			if(!arg.IsInteger())
				throw LangErr("net_create_server","invalid port number");
			int port=arg.Integer();

			security.CreateSocket(port);
			
			/* Allocate the socket set */
			socketset = SDLNet_AllocSocketSet(MAX_CONNECTIONS+1);
			if ( socketset == NULL )
				throw LangErr("net_create_server","couldn't create socket set");
			/* Create the server socket */
			SDLNet_ResolveHost(&serverIP, NULL, port);
			servsock = SDLNet_TCP_Open(&serverIP);
			if (servsock == NULL)
				throw LangErr("net_create_server","couldn't create server socket");
			SDLNet_TCP_AddSocket(socketset, servsock);

			for(size_t i=0; i<MAX_CONNECTIONS; i++)
			{
				people[i].lock = SDL_CreateMutex();
				people[i].wait = SDL_CreateSemaphore(0);
				people[i].writer_thread = NULL;
			}
			
			server_created=true;
			
			return Null;
		}

		/// net_client_ip(c) - Return IP of the client $c$ as a
		/// string.
		Data net_client_ip(const Data& arg)
		{
			if(!arg.IsInteger())
				ArgumentError("net_client_ip",arg);
			
			int client=arg.Integer();
			
			if(client < 0 || client >=MAX_CONNECTIONS)
				throw LangErr("net_client_ip","invalid client number");

			SDL_LockMutex(people[client].lock);
			bool socket_exist=people[client].sock!=NULL;
			SDL_UnlockMutex(people[client].lock);
			if(!socket_exist)
				throw LangErr("net_client_ip","no such client");

			SDL_LockMutex(people[client].lock);
			IPaddress *ip=SDLNet_TCP_GetPeerAddress(people[client].sock);

			string s;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
			s=ToString(ip->host & 0xff);
			s+=".";
			s+=ToString((ip->host & 0xff00) >> 8);
			s+=".";
			s+=ToString((ip->host & 0xff0000) >> 16);
			s+=".";
			s+=ToString((ip->host & 0xff000000) >> 24);
#else
			s=ToString((ip->host & 0xff000000) >> 24);
			s+=".";
			s+=ToString((ip->host & 0xff0000) >> 16);
			s+=".";
			s+=ToString((ip->host & 0xff00) >> 8);
			s+=".";
			s+=ToString(ip->host & 0xff);
#endif
			SDL_UnlockMutex(people[client].lock);

			return s;
		}

		/// net_client_name(c) - Return canonical hostname of the
		/// client, which uses server connetion number $c$.
		Data net_client_name(const Data& arg)
		{
			if(!arg.IsInteger())
//				ArgumentError("net_client_name",arg);
				return Null;
			
			int client=arg.Integer();
			if(client < 0 || client >=MAX_CONNECTIONS || people[client].sock==NULL)
//				throw LangErr("net_client_name","invalid client number");
				return Null;

			SDL_LockMutex(people[client].lock);
			const char *ip=SDLNet_ResolveIP(SDLNet_TCP_GetPeerAddress(people[client].sock));
			SDL_UnlockMutex(people[client].lock);

			if(ip)
                            return string(ip);
			else
                            return Evaluator::Libnet::net_client_ip(arg);
		}
		
		/// net_get() - Check if there are data available in some
		/// client connection.  Return {\tt NULL}, if not. Otherwise,
		/// return a pair $(n,s)$, where $n$ is the number of the
		/// client connection that originates data and $s$ is the data
		/// itself. Function returns {\tt ("close",$n$)\} if server closed
		/// connection $n$.
		Data net_get(const Data& arg)
		{
			if(!arg.IsNull())
				throw LangErr("net_get","invalid arguments "+tostr(arg).String());
			
			if(event_buffer.size()==0)
			{
				SDLNet_CheckSockets(client_socketset, 10);
				for(size_t i=0; i<connections.size(); i++)
					if (connections[i]!=NULL)
					{
						if(Evaluator::quitsignal)
						{
							SDLNet_TCP_DelSocket(client_socketset, connections[i]);
							SDLNet_TCP_Close(connections[i]);
							connections[i] = NULL;
							event_buffer.push_back(Data(Data("close"),Data(int(i))));
						}
						else if(SDLNet_SocketReady(connections[i]))
						{
							char data[BUFFER_SIZE+1];
							int len=SDLNet_TCP_Recv(connections[i], data, BUFFER_SIZE);

							if (len <=0)
							{
								SDLNet_TCP_DelSocket(client_socketset, connections[i]);
								SDLNet_TCP_Close(connections[i]);
								connections[i] = NULL;
								event_buffer.push_back(Data(Data("close"),Data(int(i))));
							}
							else
							{
								if(len>=BUFFER_SIZE)
									throw LangErr("net_get","buffer overflow");

								// Scan data and split it at each null-character.
								for(int j=0; j<len; j++)
								{
									if(data[j]=='\n')
									{
										event_buffer.push_back(Data(Data(int(i)),Data(client_buffer[i])));
										client_buffer[i]="";									
									}
									else if(data[j]!='\r')
										client_buffer[i]+=data[j];
								}
							}
						}
					}
			}

			if(event_buffer.size()==0)
				return Null;

			Data ret=event_buffer.front();
			event_buffer.pop_front();
			
			return ret;
		}
		
		/// net_server_get(t) - Wait for network events. If a client
		/// connects to the server, return a pair \{\tt("open",$n$)\} where
		/// $n$ is is a client number. Whenever a client disconnects,
		/// return a pair \{\tt ("close",$n$)\} respectively. Special event
		/// \{\tt ("quit",NULL)\} is returned when the server process is
		/// interrupted by the signal. If the client
		/// number $n$ sends data, return a pair $(n,s)$ where $s$ is
		/// a string containing data sent. Throw an exception if
		/// network error occurs. Parameter $t$ defines time out for
		/// listening process. If $t$ is {\tt NULL}, then the function
		/// does not return until there is an event available. If $t$
		/// is positive integer, process waits for $t$ms and
		/// returns {\tt NULL} if there are no events available.
		Data net_server_get(const Data& arg)
		{
			bool done=false;
			Uint32 timeleft,timeout=0;
			
			if(arg.IsNull())
				timeout=(~0);
			else if(arg.IsInteger())
				timeout=arg.Integer() > 0 ? arg.Integer() : 0;
			else
				throw LangErr("net_server_get","invalid arguments "+tostr(arg).String());
			
			if(!server_created)
				throw LangErr("net_server_get","server not created");

			while(server_event_buffer.size()==0)
			{
				/* Check for events */
				timeleft=timeout;

				while(!done)
				{
					if(Evaluator::quitsignal)
					{
						server_event_buffer.push_back(Data(Data("quit"),Null));
						done=true;
					}
					else if(SDLNet_CheckSockets(socketset, 500)>=1)
						break;

					if(timeleft <= 500)
						done=true;
					else if(timeleft!=Uint32(~0))
						timeleft-=500;					
				}

				if(done)
					break;
				
				/* Check for new connections */
				if ( SDLNet_SocketReady(servsock) )
				{
					TCPsocket newsock;
					newsock = SDLNet_TCP_Accept(servsock);
					if (newsock == NULL)
						throw LangErr("net_server_get","accept failed");
					
					/* Look for unconnected person slot */
					int which;
					bool isfree;
					for (which=0; which<MAX_CONNECTIONS; ++which )
					{
						SDL_LockMutex(people[which].lock);
						isfree=!people[which].sock;
						SDL_UnlockMutex(people[which].lock);
						
						if (isfree)
							break;
					}
					if(which >= MAX_CONNECTIONS)
						throw LangErr("net_server_get","too many connections");

					/* Initialize structures for new connection */
					SDL_LockMutex(people[which].lock);
					people[which].number = which;
					people[which].sock = newsock;
					people[which].peer = *SDLNet_TCP_GetPeerAddress(newsock);
					people[which].write_buffer="";
					people[which].read_buffer="";
					people[which].writing=false;
					people[which].writer_eof=false;
					people[which].writer_pipe=false;
					people[which].closed=false;
					if(people[which].writer_thread)
						SDL_WaitThread(people[which].writer_thread,0);
					people[which].writer_thread=SDL_CreateThread(writer_thread,&people[which]);
					if(people[which].writer_thread==NULL)
					{
						cout << "ERROR: Cannot create thread" << endl;
						SDLNet_TCP_Close(people[which].sock);
						people[which].sock = NULL;
					}
					else
					{
						SDLNet_TCP_AddSocket(socketset, people[which].sock);
						server_event_buffer.push_back(Data(Data("open"),Data(which)));
					}
					SDL_UnlockMutex(people[which].lock);
				}

				/* Check for events on existing clients */
				for (int i=0; i<MAX_CONNECTIONS; i++)
				{
					SDL_LockMutex(people[i].lock);

					if(people[i].sock)
					{						
						if (!people[i].writing && SDLNet_SocketReady(people[i].sock))
						{
							char data[BUFFER_SIZE+1];
							int len=SDLNet_TCP_Recv(people[i].sock, data, BUFFER_SIZE);

							if (len <=0)
							{
								if(people[i].read_buffer != "")
									server_event_buffer.push_back(Data(Data(i),Data(people[i].read_buffer)));
								people[i].closed=true;
								server_event_buffer.push_back(Data(Data("close"),Data(i)));
							}
							else
							{
								if(len>=BUFFER_SIZE)
									throw LangErr("net_server_get","buffer overflow");

								// Scan data and split it at each null-character.
								for(int j=0; j<len; j++)
								{
									if(data[j]=='\n')
									{
										server_event_buffer.push_back(Data(Data(i),Data(people[i].read_buffer)));
										people[i].read_buffer="";
									}
									else if(data[j]!='\r')
										people[i].read_buffer+=data[j];
								}
							}
						
						}

						if(people[i].writer_pipe)
						{
							people[i].closed=true;
							server_event_buffer.push_back(Data(Data("close"),Data(i)));
						}
						
					} // if(people[i].sock)

					SDL_UnlockMutex(people[i].lock);

				} // for (int i=0; i<MAX_CONNECTIONS; i++)
			}

			/* Take return value from event queue. */
			Data ret;

			if(server_event_buffer.size())
			{
				ret=server_event_buffer.front();
				server_event_buffer.pop_front();

				// Handle "close" event.
				if(ret[0].IsString() && ret[0].String()=="close")
				{
					int con=ret[1].Integer();

					SDL_LockMutex(people[con].lock);
					SDLNet_TCP_DelSocket(socketset, people[con].sock);
					people[con].writer_eof=true;
					if(SDL_SemPost(people[con].wait)!=0)
						cerr << "ERROR: SemPost failed" << endl;
					SDL_UnlockMutex(people[con].lock);
				}
			}
			
			return ret;
		}

		/// net_connect(s,p) - Connect to the port $p$ of server
		/// named $s$. Return a client connection number or {\tt NULL}
		/// if socket creation fails. Other network errors throws an
		/// exception.
		Data net_connect(const Data& arg)
		{
			if(!arg.IsList(2) || !arg[1].IsInteger() || !arg[0].IsString())
				throw LangErr("net_connect","invalid (host,port): "+tostr(arg).String());
			if(connections.size() >= MAX_CONNECTIONS)
				throw LangErr("net_connect","too many connections");

			string hostname=arg[0].String().c_str();
			int port=arg[1].Integer();

			security.Connect(hostname,port);
			
			IPaddress serverIP;
			SDLNet_ResolveHost(&serverIP, (char *)hostname.c_str() , port);
			if (serverIP.host == INADDR_NONE )
                            return Null;

			TCPsocket tcpsock = SDLNet_TCP_Open(&serverIP);
			if ( tcpsock == NULL )
				return Null;

			int con=-1;
			for(size_t i=0; i<connections.size(); i++)
				if(connections[i]==NULL)
				{
					con=i;
					break;
				}

			SDLNet_TCP_AddSocket(client_socketset, tcpsock);

			if(con==-1)
			{
				connections.push_back(tcpsock);
				return int(connections.size()-1);
			}
			else
			{
				connections[con]=tcpsock;
				return con;
			}
		}

		/// net_send(n,e) - Send the value of expression $e$ as a
		/// newline terminated string using the client connection
		/// number $n$. Return 1 if successful and 0 on SIGPIPE.
		Data net_send(const Data& arg)
		{
			static char null_char[1]={'\n'};

			if(!arg.IsList(2) || !arg[0].IsInteger())
				throw LangErr("net_send","invalid arguments "+tostr(arg).String());

			int client=arg[0].Integer();
			if(client < 0 || (size_t) client >=connections.size())
				throw LangErr("net_send","invalid client number");
			if(connections[client]==NULL)
				throw LangErr("net_send","socket is closed");			

			string data=tostr(arg[1]).String();

			SDLNet_TCP_Send(connections[client], (char *)data.c_str(), data.length());
			SDLNet_TCP_Send(connections[client], &null_char, 1);

			return 1;
		}

		/// net_server_isopen(n) - Check if server's connection number
		/// $n$ is open. Return NULL if invalid connection number.
		Data net_server_isopen(const Data& arg)
		{
			if(!arg.IsInteger())
//				ArgumentError("net_server_isopen",arg);
				return Null;

			int client=arg.Integer();
			if(client < 0 || client >=MAX_CONNECTIONS)
//				throw LangErr("net_server_isopen","invalid client number");
				return Null;

			SDL_LockMutex(people[client].lock);
			bool socket_open=(people[client].sock!=NULL && !people[client].closed);
			SDL_UnlockMutex(people[client].lock);

			return ((int)socket_open);
		}

		/// net_isopen(n) - Check if connection number $n$ is open. Return NULL if invalid connection number.
		Data net_isopen(const Data& arg)
		{
			if(!arg.IsInteger())
//				ArgumentError("net_isopen",arg);
				return Null;

			int con=arg.Integer();
			if(con < 0 || (size_t)con >=connections.size())
//				throw LangErr("net_isopen","invalid connections number");
				return Null;

			return ((int)(connections[con] != NULL));
		}

		/// net_server_send(n,s) - Send a string $s$ to the client
		/// number $n$. Return 1 if successful and 0 on SIGPIPE.
		Data net_server_send(const Data& arg)
		{
			if(!arg.IsList(2) || !arg[0].IsInteger())
				throw LangErr("net_server_send","invalid arguments "+tostr(arg).String());

			int client=arg[0].Integer();
			if(client < 0 || client >=MAX_CONNECTIONS)
				throw LangErr("net_server_send","invalid client number");

			SDL_LockMutex(people[client].lock);
			bool socket_open=people[client].sock!=NULL;
			SDL_UnlockMutex(people[client].lock);

			if(!socket_open)
				throw LangErr("net_server_send","socket is closed");
				
			string data=tostr(arg[1]).String();

			SDL_LockMutex(people[client].lock);
			if(!people[client].closed)
			{
				people[client].write_buffer+=data+"\n";
				if(SDL_SemPost(people[client].wait)!=0)
					cerr << "ERROR: SemPost failed" << endl;
			}
			SDL_UnlockMutex(people[client].lock);
						
			return 1;
		}

		/// net_server_close(n) - Close server's connection number
		/// $n$.
		Data net_server_close(const Data& arg)
		{
			if(!arg.IsInteger())
//				ArgumentError("net_server_close",arg);
				return Null;
			
			int client=arg.Integer();
 			if(client < 0 || client >=MAX_CONNECTIONS)
// 				throw LangErr("net_server_close","invalid client number");
				return Null;

			SDL_LockMutex(people[client].lock);
			bool socket_open=people[client].sock!=NULL;
			SDL_UnlockMutex(people[client].lock);

			if(!socket_open)
//				throw LangErr("net_server_close","socket is already closed");
				return Null;

			SDL_LockMutex(people[client].lock);
			people[client].closed=true;
			SDL_UnlockMutex(people[client].lock);
			
			server_event_buffer.push_back(Data(Data("close"),Data(client)));

			return Null;
		}

		/// net_close(n) - Close clients's connection number $n$.
		Data net_close(const Data& arg)
		{
			if(!arg.IsInteger())
//				ArgumentError("net_close",arg);
				return Null;
			
			int c=arg.Integer();
 			if(c < 0 || c >=(int)connections.size())
// 				throw LangErr("net_close","invalid client number");
				return Null;

			SDLNet_TCP_DelSocket(client_socketset, connections[c]);
			SDLNet_TCP_Close(connections[c]);
			connections[c]=NULL;

			return Null;
		}

		/// net_server_send_all(s) - Send a string $s$ to all
		/// clients.
		Data net_server_send_all(const Data& arg)
		{
			string data=tostr(arg).String();

			for(int i=0; i<MAX_CONNECTIONS; i++)
			{
				SDL_LockMutex(people[i].lock);
				if(people[i].sock != NULL && !people[i].closed)
				{
					people[i].write_buffer+=data+"\n";
					if(SDL_SemPost(people[i].wait)!=0)
						cerr << "ERROR: SemPost failed" << endl;
				}
				SDL_UnlockMutex(people[i].lock);
			}
			
			return Null;
		}
		
// Cleanup code
		void cleanup()
		{
			TCPsocket socket;
			int status;
			
			for(size_t i=0; i<MAX_CONNECTIONS; i++)
			{
				SDL_LockMutex(people[i].lock);
				socket=people[i].sock;
				SDL_UnlockMutex(people[i].lock);
				if(socket!=NULL)
				{
					SDL_LockMutex(people[i].lock);
					people[i].writer_eof=true;
					if(SDL_SemPost(people[i].wait)!=0)
						cerr << "ERROR: SemPost failed" << endl;
					SDL_UnlockMutex(people[i].lock);
					SDL_WaitThread(people[i].writer_thread,&status);
				}
			}
					
			for(size_t i=0; i<connections.size(); i++)
			{
				SDLNet_TCP_Close(connections[i]);
//				cout << "SDLNet_TCP_Close(connections[" << i << "])" << endl;
			}
			if ( servsock != NULL )
			{
				SDLNet_TCP_Close(servsock);
//				cout << "SDLNet_TCP_Close(servsock)" << endl;
				servsock = NULL;
			}
			if ( client_socketset != NULL )
			{
				SDLNet_FreeSocketSet(client_socketset);
				client_socketset = NULL;
			}
			if ( socketset != NULL )
			{
				SDLNet_FreeSocketSet(socketset);
				socketset = NULL;
			}

			SDLNet_Quit();
			SDL_Quit();
		}

// Initialization code
	}

	void InitializeLibnet()
	{
		external_function["net_client_ip"]=&Libnet::net_client_ip;
		external_function["net_client_name"]=&Libnet::net_client_name;
		external_function["net_close"]=&Libnet::net_close;
		external_function["net_connect"]=&Libnet::net_connect;
		external_function["net_create_server"]=&Libnet::net_create_server;
		external_function["net_get"]=&Libnet::net_get;
		external_function["net_isopen"]=&Libnet::net_isopen;
		external_function["net_send"]=&Libnet::net_send;
		external_function["net_server_close"]=&Libnet::net_server_close;
		external_function["net_server_get"]=&Libnet::net_server_get;
		external_function["net_server_isopen"]=&Libnet::net_server_isopen;
		external_function["net_server_send"]=&Libnet::net_server_send;
		external_function["net_server_send_all"]=&Libnet::net_server_send_all;

		if (SDL_Init(SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE|SDL_INIT_EVENTTHREAD) < 0)
			throw Error::IO("LibraryInitializer::LibraryInitializer()","Couldn't initialize SDL");
		if (SDLNet_Init() < 0)
			throw Error::IO("LibraryInitializer::LibraryInitializer()","Couldn't initialize SDL_net");
				
		for(int i=0; i<MAX_CONNECTIONS; i++ )
		{
			Libnet::people[i].sock = NULL;
		}
		Libnet::client_socketset = SDLNet_AllocSocketSet(MAX_CONNECTIONS+1);

#ifndef WIN32
		signal(SIGPIPE,Evaluator::Libnet::IgnoreSignal);
		signal(SIGQUIT,Evaluator::Libnet::QuitSignal);
		signal(SIGHUP,Evaluator::Libnet::QuitSignal);
		signal(SIGINT,Evaluator::Libnet::QuitSignal);
		signal(SIGABRT,Evaluator::Libnet::QuitSignal);
		signal(SIGTERM,Evaluator::Libnet::QuitSignal);
#endif

		atexit(Libnet::cleanup);
	}

}
