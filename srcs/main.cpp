/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ilandols <ilandols@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/03/28 14:29:41 by halvarez          #+#    #+#             */
/*   Updated: 2023/04/14 01:41:19 by ilandols         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

#include <string>
#include <vector>

#include <algorithm>

#include "server/Server.hpp"
#include "cgi/CGI.hpp"
#include "utils/utils.hpp"

int	main(int ac, char **av)
{
	(void)ac;
	
	if (av[0] and av[1])
	{
		std::cout << "Config File mode" << std::endl;
		std::vector<std::vector <std::string> > configs = splitFileConfig(av[1]);

		std::vector< Server >	servers(configs.size());

		std::cout << "=============================" << std::endl;
		for (size_t	i = 0; i < servers.size(); i++)
		{
			servers[i].setConfig(configs[i]);
			std::cout << "=============================" << std::endl;
		}
		servers[0].run();
	}
	else
	{
		std::cout << "Default mode" << std::endl;
		Server	server("default");
	}

	// std::for_each(fileContent.begin(), fileContent.end(), displayString<std::string>);
	
	return 0;
}
