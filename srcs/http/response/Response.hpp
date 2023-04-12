/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: auzun <auzun@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/31 13:44:30 by auzun             #+#    #+#             */
/*   Updated: 2023/04/02 23:17:58 by auzun            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef	RESPONSE_HPP
# define RESPONSE_HPP

#include <iostream>
#include <sys/stat.h>
#include <fstream>
#include "../../utils/utils.hpp"
#include "../request/Request.hpp"

class Response
{
	public :
		Response(void);
		Response(Request request);
		~Response(void);

		/*Methods*/
		void	GET(void);
		/*-------*/

		std::string	getResponse();

		/*Response Utils*/
		int	readContent(void);
		int	writeContent(std::string content);
		int	fileExist(std::string path);
		/*---------------*/

		/*Header*/
		void	setContentType(std::string path);
		void	setCode(int	code);
		void	setContentLength(size_t size);
		std::string	writeHeader(void);
		std::string	getHeader(size_t size, std::string path, int code);
		/*------*/
	
	private :

		/*Header*/
		std::string	_contentLength;
		std::string	_contentType;
		std::string	_code;
		/*------*/
		
		Request		_request;
		std::string	_response;
};


#endif