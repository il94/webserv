/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilandols <ilandols@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/28 17:57:03 by halvarez          #+#    #+#             */
/*   Updated: 2023/05/18 12:28:48 by halvarez         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>

#include "Server.hpp"
#include "../http/request/Request.hpp"
#include "../http/response/Response.hpp"

#define MAX_EVENTS	16

// Constructors ============================================================= //
Server::Server(void) : _nbSrv( 0 ), /*_flags( ),*/ _names( ), _ports( ), _sockaddr( ), _eplevs( ),
	_srvfd( )//, _cliSocket(  )
{
	this->_eplfd = epoll_create( 1 );
	if ( this->_eplfd == -1 )
		this->_displayError( __func__, __LINE__, "Server/epoll_create" );
	if ( fcntl( this->_eplfd, F_SETFL, O_NONBLOCK) == -1 )
		this->_displayError( __func__, __LINE__, "Server/fcntl" );
	return;
}

Server::Server( const Server & srv ) : _nbSrv( srv._nbSrv ), /*_flags( ),*/ _names( srv._names ),
	_ports( srv._ports ), _sockaddr( srv._sockaddr ), _eplevs( srv._eplevs ), _srvfd(  )//, _cliSocket(  )
{
	size_t	i	= 0;

	while ( i < this->_getNbSrv() )
	{
		this->_srvfd.push_back( dup( srv._getSrvFd( i ) ) );
		this->_eplevs[i].data.fd = this->_getSrvFd( i );
		i++;
	}
	this->_eplfd = epoll_create( 1 );
	if ( this->_eplfd == -1 )
		this->_displayError( __func__, __LINE__, "Server/epoll_create" );
	if ( fcntl( this->_eplfd, F_SETFL, O_NONBLOCK) == -1 )
		this->_displayError( __func__, __LINE__, "Server/fcntl" );
	return;
}

// Destructor =============================================================== //
Server::~Server(void)
{
	size_t	i = 0;

	while ( i < this->_getNbSrv() )
	{
		if ( close( this->_getSrvFd( i ) ) == -1 )
			this->_displayError( __func__, __LINE__, "~Server/close" );
		i++;
	}
	if ( close( this->_getEplFd() ) == -1 )
		this->_displayError( __func__, __LINE__, "~Server/close" );
	return;
}

// Operators ================================================================ //
Server &	Server::operator=(const Server & srv __attribute__((unused)))
{
	size_t	i	= 0;

	if ( this->_srvfd.size() != 0 )
		for (size_t j = 0; j < this->_srvfd.size(); j++)
		{
			close( this->_getSrvFd( j ) );
		}
	this->_srvfd.clear();
	this->_names	= srv._names;
	this->_ports	= srv._ports;
	this->_sockaddr	= srv._sockaddr;
	this->_srvfd	= srv._srvfd;
	this->_nbSrv	= srv._ports.size();
	while ( i < this->_getNbSrv() )
	{
		this->_srvfd.push_back( dup( srv._getSrvFd( i ) ) );
		this->_eplevs[i].data.fd = this->_getSrvFd( i );
		i++;
	}
	this->_eplfd = epoll_create( 1 );
	if ( this->_eplfd == -1 )
		this->_displayError( __func__, __LINE__, "Operator=/epoll_create" );
	return ( *this );
}

// Testing functions ======================================================== ///
std::string	testing_data(void)
{
	// Testing data = will be removed ======================================= //
	std::string	hello = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: ";
	int hfd = open("./html/index.html", O_RDONLY );
	char b[30000];

	read( hfd, b, 30000);
	std::string html(b);

	std::ostringstream oss;
    oss << html.size();
	hello = hello + oss.str() + "\n" + b;
	// ====================================================================== //
	return ( hello );
}

// Public member functions ================================================== //

void	Server::display(void)
{
	std::cout << std::endl << "========================================" << std::endl << std::endl;

	std::cout << _getNbSrv() << " Instances" << std::endl << std::endl; 

	displayVector(_ports, "Ports alloues");

	std::cout << std::endl;	

	displayVector(_names, "Serveur concerne");

	std::cout << std::endl;

	std::cout << "========================================" << std::endl << std::endl;

	for (std::vector<Config>::iterator it = _configs.begin(); it != _configs.end(); it++)
	{
		std::cout << "Serveur [" << it->getName() << "]" << std::endl << std::endl;
		it->display();
		std::cout << "========================================" << std::endl << std::endl;
	}
}

std::vector<std::vector <std::string> >	Server::extractContent( const std::string &path )
{
	std::vector<std::vector <std::string> > result;

	std::vector<std::string>			fileContent = fileToVector(path);
	std::vector<std::string>::iterator	start;
	std::vector<std::string>::iterator	end;

	std::vector<std::string>			element;

	for (start = fileContent.begin(); start != fileContent.end(); start = end)
	{
		element.clear();
		end = start + 1;
		if (static_cast<int>(start->rfind("server", 0, sizeof("server") - 1)) != -1 and openBrace(*start, start->find("server", 0) + sizeof("server")))
		{
			while (end != fileContent.end() and (*end)[0] != '}')
			{
				element.push_back(*end);
				end++;
			}
			if (end != fileContent.end() and (*end)[0] == '}')
				result.push_back(element);
		}
	}
	return (result);
}

void	Server::setConfigs( char **av )
{
	std::string	src;
	if (av[0] and av[1])
		src = av[1];

	setContent(extractContent(src));

	bool	error = false;

	for (std::vector<std::vector <std::string> >::iterator it = _content.begin(); it != _content.end(); it++)
	{
		Config	tmp;

		tmp.setContent(tmp.extractContent(*it));

		tmp.setPort(tmp.extractPort());
		tmp.setHost(tmp.extractHost());
		tmp.setName(tmp.extractName());
		tmp.setMaxBodySize(tmp.extractMaxBodySize());

		tmp.setDefaultLocation(tmp.extractDefaultLocation());
		tmp.setErrorPages(tmp.extractErrorPages());
		tmp.setLocations(tmp.extractLocations(*it));

		if (tmp.getError() == false)
		{
			std::vector<int>	tmpPorts = tmp.getPort();

			_ports.insert(_ports.end(), tmpPorts.begin(), tmpPorts.end());
			for (size_t i = 0; i < tmpPorts.size(); i++)
				_names.push_back(tmp.getName());
				
			_configs.push_back(tmp);
		}
		else
			error = true;
	}
	if (_configs.empty() == true and error == false)
	{
		_configs.push_back(Config());
		_names.push_back(DEFAULT_NAME);
		_ports.push_back(DEFAULT_PORT);
	}
	for (size_t i = 0; i < _ports.size(); i++)
	{
		for (size_t j = i + 1; j < _ports.size(); j++)
		{
			while (_ports[j] == _ports[i])
			{
				_ports.erase(_ports.begin() + j);
				_names.erase(_names.begin() + j);
			}
		}
	}

	_nbSrv = _ports.size();
}

// to be delete ----------------------------------------------------------------
void	putInfile( std::string file, std::string content )
{
	std::ofstream ofs;

	ofs.open( file.c_str() );
	ofs << content;
	ofs.close();
	return;
}
// -----------------------------------------------------------------------------

void	Server::run(void)
{
	int				cliSocket	= -1;
	Client			client( this->_getEplFd() );
	int				nbEvents	= -1;
	t_epollEv		cliEvents[ MAX_EVENTS ];
	std::string		request;
	size_t			sizeRequest	= 1000000;

	std::cout << "Server log : " << std::endl;
	// Creates sockets if they don't exist
	if ( this->_srvfd.size() == 0 )
		this->_initSrv();
	while ( 1 )
	{
		nbEvents = epoll_wait( this->_getEplFd( ), cliEvents, MAX_EVENTS, -1);
		if ( nbEvents == -1 )
			this->_displayError( __func__, __LINE__, "run/epoll_wait" );
		for (int i = 0; i < nbEvents; i++)
		{
			cliSocket = -1;
			for ( size_t j = 0; j < client.size(); j++ )
			{
				if ( client.find( cliEvents[i].data.fd ) != -1 )
				{
					cliSocket = cliEvents[i].data.fd;
					if ( cliSocket != -1 && cliEvents[i].events & EPOLLOUT && client.getFlag( cliSocket ) == CONTENT )
					{
						// send response
						int		bytes = 0;
						size_t	sizeResp = client.responseSize( cliSocket );

						bytes = send( cliSocket,
								static_cast< const unsigned char* >( ( client.getResponse( cliSocket ) ).c_str() ),
								sizeResp,
								0 );
						if ( bytes == -1 || static_cast< size_t >( bytes ) != sizeResp )
						{
							client.remove( cliSocket );
							std::cerr << "\tError : send response to client failed, ";
							std::cerr << "the socket has been closed and cleared." << std::endl;
						}
					}
					if ( cliSocket != -1 && cliEvents[i].events & EPOLLIN )
					{
						request.clear();
						request.resize( sizeRequest );
						if ( cliSocket != -1 && cliEvents[i].events & EPOLLIN )
						{
							request = this->_readRequest( client, cliSocket, request );
							if ( cliSocket != -1 && request.size() > 0 )
							{
								//std::cout << YELLOW << request << END << std::endl;
								Request	req;
								req.parseHeader(request);
								if (req.getRet() == 200)
									req.parseBody();								
								Response	rep(req, _configs[0], client.getPort( cliSocket ), client.getName( cliSocket ) );

								rep.generate();
								client.newResponse( cliSocket, rep.getResponse() );
							}
						}
					}
				}
			}
			for (size_t k = 0; k < this->_getNbSrv(); k++)
			{
				if ( cliEvents[i].data.fd == this->_getSrvFd( k ) )
				{
					cliSocket = this->_acceptConnection( k, client );
					cliSocket = -1;
				}
			}
			//if ( cliSocket != -1 )
			//	client.remove( cliSocket );
		}
	}
	// ====================================================================== //
	return;
}

void	Server::closeCliSocket(int cliSocket)
{
	if ( close( cliSocket ) == -1 )
		this->_displayError( __func__, __LINE__, "closeCliSocket/close" );
	return;
}

// Private member functions ================================================= //
void	Server::_initSrv(void)
{
	size_t			i			= 0;
	int				srvfd		= -1;
	int				opt			= 1;

	if ( this->_sockaddr.size() == 0 )
		this->_setSockaddr();
	if ( this->_eplevs.size() == 0 )
		this->_setEplevs();
	while ( i < this->_getNbSrv() )
	{
		srvfd = -1;
		srvfd = socket( AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0 ); 
		if ( srvfd == -1 )
			this->_displayError( __func__, __LINE__, "_initSrv/socket" );
		if ( setsockopt( srvfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) ) == -1 )
			this->_displayError( __func__, __LINE__, "_initSrv/setsockopt" );
		if ( bind( srvfd, &this->_getSockaddr( i ), sizeof( t_sockaddr ) ) == -1 )
		{
			this->_displayError( __func__, __LINE__, "_initSrv/bind" );
			this->_names.erase( this->_names.begin() + i );
			this->_ports.erase( this->_ports.begin() + i );
			this->_sockaddr.erase( this->_sockaddr.begin() + i );
			this->_eplevs.erase( this->_eplevs.begin() + i );
			this->_nbSrv--;
		}
		else
		{
			this->_setSrvFd( srvfd );
			if ( epoll_ctl( this->_getEplFd(  ), EPOLL_CTL_ADD, this->_getSrvFd( i ), &this->_eplevs[i] )== -1)
				this->_displayError( __func__, __LINE__, "_initSrv/epoll_ctl ADD" );
			try
			{
				if ( listen( this->_getSrvFd( i ), 100 ) == -1 )
				{
					this->_displayError( __func__, __LINE__, "_initSrv/listen" );
					if ( epoll_ctl( this->_getEplFd(), EPOLL_CTL_DEL, this->_getSrvFd( i ), &this->_eplevs[i] ) == -1 )
						this->_displayError( __func__, __LINE__, "_initSrv/epoll_ctl DEL" );
				}
				std::cout << "\t" << this->_getName( i ) << "[" << this->_getPort( i ) << "]\t: ";
				std::cout << "listening" << std::endl;
			}
			catch (std::exception & e)
			{
				std::cerr << e.what() << std::endl;
			}
			i++;
		}
	}
	return;
}

std::string &	Server::_readRequest(Client & client, int & cliSocket, std::string & request)
{
	int	bytes = 0;

	bytes = recv(cliSocket, reinterpret_cast<void*>(const_cast<char*>(request.data())), request.size() - 1, 0);
	if ( bytes == 0 || bytes == -1 )
	{
		this->_displayError( __func__, __LINE__, "_readRequest/recv" );
		client.remove( cliSocket );
		cliSocket = -1;
	}
	else
	{
		request[ bytes ] = '\0';
		request.resize( bytes );
	}
	std::cout << "------------------------- print request -------------------------" << std::endl;
	std::cout << request << std::endl;
	return ( request );
}

int	Server::_acceptConnection(const int & j, Client & client __attribute__((unused)) )
{
	int			cliSocket;
	socklen_t	addrlen		= sizeof( t_sockaddr );

	cliSocket = accept( this->_getSrvFd( j ), const_cast< t_sockaddr* >( &this->_getSockaddr( j ) ), &addrlen );
	if ( cliSocket == -1 )
		this->_displayError( __func__, __LINE__, "_acceptConnection/accept" );
	else
	{
		if ( client.add( cliSocket, this->_getPort( j ), this->_getName( j ) ) == false )
		{
			std::cerr << "Error: adding socket bind on " << this->_getPort( j ) << " failed." <<std::endl;
			if ( close( cliSocket ) == -1 )
				this->_displayError( __func__, __LINE__, "_acceptConnection/close" );
			return ( -1 );
		}
	}
	return( cliSocket );
}

// log function ============================================================= //
void	Server::_displayError(const char *func, const int line, const char *msg) 
{
	std::cerr << "\t" << func << ":" << line - 1 << ":";
	perror(msg);
	return;
}

// Getters ================================================================== //
size_t Server::_getNbSrv(void) const
{
	return ( this->_nbSrv );
}

const int & Server::_getEplFd(void) const
{
	return ( this->_eplfd );
}

const std::string &	Server::_getName(const size_t & i) const
{
	if ( /*i < 0 ||*/ i >= this->_getNbSrv() )
		throw WrongSize();
	return ( this->_names[i] );
}

size_t 	Server::_getPort(const size_t & i) const
{
	if ( /*i < 0 ||*/ i >= this->_getNbSrv() || i >= this->_ports.size() )
		throw WrongSize();
	return ( this->_ports[i] );
}

const int &	Server::_getSrvFd(const size_t & i) const
{
	if ( /*i < 0 ||*/ i >= this->_getNbSrv() || i >= this->_srvfd.size() )
		throw WrongSize();
	return ( this->_srvfd[i] );
}

const Server::t_sockaddr &	Server::_getSockaddr(const size_t & i) const
{
	if ( /*i < 0 ||*/ i >= this->_getNbSrv() || i >= this->_sockaddr.size() )
		throw WrongSize();
	return ( this->_sockaddr[i] );
}

const Server::t_epollEv &	Server::_getEpollEv(const size_t & i) const
{
	if ( /*i < 0 ||*/ i >= this->_getNbSrv() || i >= this->_eplevs.size() )
		throw WrongSize();
	return ( this->_eplevs[i] );
}

std::vector<std::vector <std::string> >	Server::getContent( void ) const {
	return (_content);
}


// Setters ================================================================== //
void	Server::_setName(const std::string & name)
{
	size_t	i	= 0;

	while ( i < this->_names.size() && i < this->_getNbSrv() )
	{
		if ( name == this->_names[i] )
			throw Duplicate();
		i++;
	}
	this->_names.push_back( name );
	return;
}

void	Server::_setPort(const int & port)
{
	size_t	i	= 0;

	while ( i < this->_ports.size() && i < this->_getNbSrv() )
	{
		if ( port == this->_ports[i] )
			throw Duplicate();
		i++;
	}
	this->_ports.push_back( port );
	return;
}

void	Server::_setSrvFd(const int & fd)
{
	size_t	i	= 0;

	while ( i < this->_srvfd.size() && i < this->_getNbSrv() )
	{
		if ( fd == this->_srvfd[i] )
			throw Duplicate();
		i++;
	}
	this->_eplevs[i].data.fd = fd;
	this->_srvfd.push_back( fd );
	return;
}

void	Server::_setSockaddr(void)
{
	size_t			i		= 0;
	t_sockaddr_in	addrIn;
	t_sockaddr		*addr;

	addrIn.sin_family		= AF_INET;
	addrIn.sin_addr.s_addr	= INADDR_ANY;
	for (size_t j = 0; j < sizeof( addrIn.sin_zero ); j++)
	{
		addrIn.sin_zero[j] = '\0';
	}
	while ( i < this->_getNbSrv() )
	{
		addrIn.sin_port = htons( this->_getPort( i ) );
		addr = reinterpret_cast< t_sockaddr * >( &addrIn );
		this->_sockaddr.push_back( *addr );
		i++;
	}
	return;
}

void	Server::_setEplevs(void)
{
	size_t		i		= 0;
	t_epollEv	eplev;

	eplev.events	= EPOLLIN | EPOLLOUT;
	eplev.data.fd	= -1;
	while ( i < this->_getNbSrv() )
	{
		this->_eplevs.push_back( eplev );
		i++;
	}
	return;
}

void	Server::setContent( const std::vector<std::vector <std::string> > &src )
{
	_content = src;
}
