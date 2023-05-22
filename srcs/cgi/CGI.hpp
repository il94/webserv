#ifndef CGI_HPP
# define CGI_HPP

# include "../http/request/Request.hpp"
# include <vector>
# include <algorithm>
# include <unistd.h>
# include <string.h>
# include <sys/wait.h>
# include <stdlib.h>

# define	CGI_BUFSIZE 42424

class CGI
{
	public:
		CGI();
		CGI(Request &request);
		CGI(const CGI &src);
		~CGI();

		CGI	&	operator=(const CGI & src);

		void	setEnv();
		std::string	execCGI(std::string scriptName);
	private:
		Request			_request;
		char				**_env;
};

# endif