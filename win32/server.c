#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32 1
#endif

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <mstcpip.h>

#define close closesocket
#else
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
typedef int HANDLE;

#endif

#define LOGS_FULL(OPTION,ERRNO) \
		printf("(%s %d %s) %s errno:%d",__FILE__,__LINE__,__FUNCTION__,OPTION,ERRNO);

#define LOGS(OPTION,ERRNO) \
		printf("%s errno:%d",OPTION,ERRNO);

#define ERRORS(OPTION,ERRNO) LOGS(OPTION,ERRNO)

#ifdef WIN32
int initNetwork()
{
	WSADATA wsaData;
	int ret = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (ret != 0) return 1;
	return 0;
}
void setnonblocking(HANDLE sock)
{
	unsigned long ul=1;
	int ret=ioctlsocket(sock,FIONBIO,(unsigned long *)&ul);
	if(ret==SOCKET_ERROR)
	{
	}
}
int socket_keepalive(HANDLE socket)
{
    int keep_alive = 1;
    int ret = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&keep_alive, sizeof(keep_alive));
    if (ret == SOCKET_ERROR)
    {
        printf("SO_KEEPALIVE failed:%d\n", WSAGetLastError());
        return -1;
    }

    struct tcp_keepalive in_keep_alive = {0};
    unsigned long ul_in_len = sizeof(struct tcp_keepalive);
    struct tcp_keepalive out_keep_alive = {0};
    unsigned long ul_out_len = sizeof(struct tcp_keepalive);
    unsigned long ul_bytes_return = 0;

    in_keep_alive.onoff = 1;
    in_keep_alive.keepaliveinterval = 5000;
    in_keep_alive.keepalivetime = 1000;

    ret = WSAIoctl(socket, SIO_KEEPALIVE_VALS, (LPVOID)&in_keep_alive, ul_in_len,
                          (LPVOID)&out_keep_alive, ul_out_len, &ul_bytes_return, NULL, NULL);
    if (ret == SOCKET_ERROR)
    {
        printf("WSAIoctl KEEPALIVE failed:%d\n", WSAGetLastError());
        return -1;
    }
    return 0;
}
#else
void  setnonblocking(HANDLE sock)
{
	int  opts;
	opts = fcntl(sock,F_GETFL);
	if (opts < 0 )
	{
		printf( "fcntl(sock,GETFL) " );
		exit( 1 );
	}
	opts  =  opts | O_NONBLOCK;
	if (fcntl(sock,F_SETFL,opts) < 0 )
	{
		printf( "fcntl(sock,SETFL,opts) " );
		exit( 1 );
	}
}
int socket_keepalive(HANDLE socket)
{
    int optval;
    socklen_t optlen = sizeof(int);

    optval = 1;
    int  ret = setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
	if(ret != 0)
	{
		printf("SO_KEEPALIVE set failed:%d\n",errno);
		return -1;
	}
	//重复检查次数
    optval = 5;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
	if(ret != 0)
	{
		printf("SO_KEEPALIVE set failed:%d\n",errno);
		return -1;
	}
	//检查空闲
    optval = 1;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
	if(ret != 0)
	{
		printf("SO_KEEPALIVE set failed:%d\n",errno);
		return -1;
	}
	//检查间隔
    optval = 1;
    ret = setsockopt(socket, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
	if(ret != 0)
	{
		printf("SO_KEEPALIVE set failed:%d\n",errno);
		return -1;
	}
	return 0;
}
#endif

void init()
{
#ifdef WIN32
	initNetwork();
#endif
}

int main(int argc,char *argv[])
{
	init();

	HANDLE fd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server = {0};
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(argv[1]);
	server.sin_port = htons(atoi(argv[2]));

	int ret = bind(fd,&server,sizeof(struct sockaddr_in));
	if(ret < 0)
	{
		return -1;
	}
	ret = listen(fd,100);
	if(ret == -1)
	{
		close(fd);
		return -1;
	}

	int nfds = fd;
	fd_set readfds_cache;
	fd_set writefds_cache;

	FD_ZERO(&readfds_cache);
	FD_ZERO(&writefds_cache);
	FD_SET(fd,&readfds_cache);

	fd_set readfds;
	fd_set writefds;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 1000000;
	printf("begin run...\n");
	int fd_count = 1;
	int select_index = 0;

	int no_recv = 0;
	int no_send = 1;
	fd_set * ptrRecv = &readfds;
	fd_set * ptrSend = &writefds;
	while(1)
	{
		readfds = readfds_cache;
		writefds = writefds_cache;

		readfds = readfds_cache;
		writefds = writefds_cache;

		ptrRecv = &readfds;
		ptrSend = &writefds;
		if(no_recv == 1)
		{
			ptrRecv = NULL;
		}
		if(no_send == 1)
		{
			ptrSend = NULL;
		}

		int ret = select(nfds + 1,ptrRecv,ptrSend,NULL,&timeout);
		if(ret  == -1)
		{
			if (errno == EINTR)
                continue;
			printf("select error:%d\n",errno);
			break;
		}
		if(ret == 0)
		{
			// printf("select index:%d %d\n",select_index++,nfds);
			continue;
		}
		printf("select %d.\n",ret);

		if(ptrRecv != NULL && FD_ISSET(fd,ptrRecv))
		{
			printf("accept connection\n");
			struct sockaddr_in client = {0};
			socklen_t client_len = sizeof(struct sockaddr_in);
			HANDLE cfd = accept(fd,&client,&client_len);
			if(cfd == -1)
			{
				ERRORS("accept",errno);
			}else{
				printf("accept %d\n",cfd);
				setnonblocking(cfd);
				socket_keepalive(cfd);
				FD_SET(cfd,&readfds_cache);
				FD_SET(cfd,&writefds_cache);
				nfds = nfds>cfd?nfds:cfd;
				fd_count++;
				printf("accept end.\n");
			}
		}

		int i = 0;
		for (; i <= nfds; i++)
		{
			if( i == fd)
			{
				continue;
			}
			if(ptrRecv != NULL && no_recv == 0)
			{
				if (FD_ISSET(i, ptrRecv))
				{
					printf("recv check...\n");
					char peek;
					ret = recv(i,&peek,1,MSG_PEEK);
					if(ret != 1)
					{
						if(ret == 0)
						{
							FD_CLR(i,&readfds_cache);
							FD_CLR(i,&writefds_cache);
							close(i);
							fd_count--;
						}
						printf("recv check :%d errno:%d\n",ret,errno);
						continue;
					}
					printf("recv...\n");
					char buffer[65536];
					ret = recv(i,buffer,65535,0);
					if(ret == -1)
					{
						if(EAGAIN == errno)
						{
							continue;
						}
						printf("recv close.%d\n",errno);
						FD_CLR(i,&readfds_cache);
						FD_CLR(i,&writefds_cache);
						close(i);
						printf("close.");
						fd_count--;
						continue;
					}else if(ret == 0)
					{
						printf("recv close.\n");
						FD_CLR(i,&readfds_cache);
						FD_CLR(i,&writefds_cache);
						close(i);
						printf("close.");
						fd_count--;
						continue;
					}else{
						buffer[ret] = 0x00;
						// printf("%s\n",buffer);
					}
				}
			}
			if(ptrSend != NULL && no_send == 0)
			{
				if(FD_ISSET(i, &writefds))
				{
					printf("send...\n");
					ret = send(i,"client",6,0);
					if(ret == -1)
					{
						if(EAGAIN == errno)
						{
							continue;
						}
						printf("send close.%d\n",errno);
						FD_CLR(i,&readfds_cache);
						FD_CLR(i,&writefds_cache);
						close(i);
						printf("close.");
						fd_count--;
					}else{
						printf("send %d\n",ret);
					}
				}
			}
		}
	}
	return -1;
}
