/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: auzun <auzun@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/28 17:57:03 by halvarez          #+#    #+#             */
/*   Updated: 2023/04/05 18:47:19 by halvarez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <fstream>
//#include <string>
//#include <cstdio>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "Server.hpp"

#define PORT		8080
#define MAX_EVENTS	5

// Constructors ============================================================= //
Server::Server(void) : _srvfd( -1 ), _eplfd( -1 ), _address( NULL ), _eplev( NULL )
{
	this->_setSockAddr();
	this->_mkSrvSocket();
	this->_setEpollEvent();
	this->_mkEpoll();
	return;
}

Server::Server(const Server & srv) : _srvfd( -1 ), _eplfd( -1 ), _address( NULL ), _eplev( NULL )
{
	int	i = 0;

	if (this->_address == NULL)
		this->_setSockAddr();
	this->_srvfd				= srv._getFd( SRV );
	this->_eplfd				= srv._getFd( EPL );
	this->_address->sa_family	= ( srv._getSockAddr() )->sa_family;
	while (i < 14)
	{
		this->_address->sa_data[i] = ( srv._getSockAddr() )->sa_data[i];
		i++;
	}
	if (this->_eplev == NULL)
		this->_setEpollEvent();
	this->_eplev->events = srv._eplev->events;
	this->_eplev->data.fd = srv._getFd( SRV );
	return;
}

// Destructor =============================================================== //
Server::~Server(void)
{
	if ( this->_srvfd != -1 && close( this->_srvfd ) == -1 )
		this->_srvError(__func__, __LINE__, "accept");
	if ( this->_eplfd != -1 && close( this->_eplfd ) == -1 )
		this->_srvError(__func__, __LINE__, "accept");
	return;
}

// Operators ================================================================ //
Server &	Server::operator=(const Server & srv)
{
	int	i = 0;

	if (this->_address == NULL)
		this->_setSockAddr();
	this->_srvfd				= srv._getFd( SRV );
	this->_eplfd				= srv._getFd( EPL );
	this->_address->sa_family	= ( srv._getSockAddr() )->sa_family;
	while (i < 14)
	{
		this->_address->sa_data[i] = ( srv._getSockAddr() )->sa_data[i];
		i++;
	}
	if (this->_eplev == NULL)
		this->_setEpollEvent();
	this->_eplev->events = srv._eplev->events;
	this->_eplev->data.fd = srv._getFd( SRV );
	return ( *this );
}

// Public member functions ================================================== //
void	Server::run(void)
{
	int				cliSocket	__attribute__((unused)) = -1;
	int				nbEvents	__attribute__((unused)) = -1;
	int				i									= 0;
	int				bytes		__attribute__((unused))	= 0;
	char			buffer[1000] = {0};
	int				addrlen		__attribute__((unused));
	t_epoll_event	cliEvents[ MAX_EVENTS ] __attribute__((unused));

	// Testing data ========================================================= //
	std::string	hello = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: ";
	int hfd = open("./html/index.html", O_RDONLY );
	char b[30000];

	read( hfd, b, 30000);
	std::string html(b);

	std::ostringstream oss;
    oss << html.size();
	hello = hello + oss.str() + "\n" + b;
	
	//std::cout << hello  << std::endl;
	// ====================================================================== //

	// Connection management ================================================ //
	if ( listen( this->_getFd( SRV ), 100 ) == -1 )
		this->_srvError(__func__, __LINE__, "accept");
	while ( 1 )
	{
		std::cout << "========== waiting for connection ==========" << std::endl;
		nbEvents = epoll_wait( this->_getFd( EPL ), cliEvents, MAX_EVENTS, 30000);
		std::cout << "============================================" << std::endl;
			cliSocket = accept( this->_getFd( SRV ), this->_address, reinterpret_cast<socklen_t *>(&cliSocket) );
		if ( cliSocket == -1 )
			this->_srvError(__func__, __LINE__, "accept");

		i = 0;
		while ( i < nbEvents )
		{
			//bytes = recv();
			std::cout << "epoll_event[i].data.fd = " << cliEvents[i].data.fd << std::endl;
			recv( cliSocket, buffer, 1000, 0 );
			send( cliSocket, hello.c_str(), hello.size(), 0 );
			i++;
		}
		*buffer = '\0';
		//close( cliSocket );
	}
	// ====================================================================== //
	return;
}

// Private member functions ================================================= //
void	Server::_setSockAddr(void)
{
	static t_sockaddr_in	address;

	address.sin_family		= AF_INET;
	address.sin_addr.s_addr	= INADDR_ANY;
	address.sin_port		= htons( PORT );
	for (size_t i = 0; i < sizeof( address.sin_zero ); i++)
	{
		address.sin_zero[i] = '\0';
	}
	this->_address = reinterpret_cast<t_sockaddr *>(&address);
	return;
}

void	Server::_mkSrvSocket(void)
{
	int	fd	= -1;
	int	opt = 1;

	fd = socket( AF_INET, SOCK_STREAM /*| SOCK_NONBLOCK*/, 0 ); 
	if (fd == -1)
		this->_srvError(__func__, __LINE__, "accept");

	if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) ) == -1 )
		this->_srvError(__func__, __LINE__, "accept");

	if ( bind( fd, this->_getSockAddr(), sizeof( t_sockaddr ) ) == -1 )
		this->_srvError(__func__, __LINE__, "accept");

	this->_srvfd = fd;
	return;
}

void	Server::_setEpollEvent(void)
{
	static t_epoll_event	eplev;

	eplev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP;
	eplev.data.fd = this->_getFd( SRV );
	this->_eplev = &eplev;
	return;
}

void	Server::_mkEpoll(void)
{
	int	fd	= -1;

	fd = epoll_create( 1 );
	if ( fd == -1 )
		this->_srvError(__func__, __LINE__, "epoll_create");

	this->_eplfd = fd;
	return;
}

const int & 	Server::_getFd( const t_fd FD ) const
{
	switch ( FD )
	{
		case SRV :
			return ( this->_srvfd );
		case EPL :
			return ( this->_eplfd );
		default :
			throw;
	}
}

Server::t_sockaddr *	Server::_getSockAddr(void) const
{
	return ( this->_address );
}

Server::t_epoll_event *	Server::_getEpollEvent(void) const
{
	return ( this->_eplev );
}

void	Server::_srvError(const char *func, const int line, const char *msg) const
{
	std::cerr << func << ":" << line - 1 << ":";
	perror( msg );
	//exit( 1 );
	return;
}

/*
	//void				setFd(const t_fd FD, const int & fd);
	void	Server::setFd(const t_fd FD, const int & fd)
	{
		switch ( FD )
		{
			case SRV :
				this->_srvfd = fd;
				return;
			case EPL :
				this->_eplfd = fd;
				return;
			default :
				std::cerr << "Error: invalid fd value : " << fd << std::endl;
				exit( 1 );
				return;
		}
		return;
	}
*/
