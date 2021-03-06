/* $Id: mapgen_dialog.hpp 46186 2010-09-01 21:12:38Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef MAPGEN_DIALOG_HPP_INCLUDED
#define MAPGEN_DIALOG_HPP_INCLUDED

#include "config.hpp"
#include "mapgen.hpp"

namespace gui2 {

class tmap_generator;

}

class default_map_generator : public map_generator
{
public:
	default_map_generator(const config &game_config);

	bool allow_user_config() const;
	bool user_config(display& disp, const config& cfg, int max_players = -1, int min_w = -1, int max_w = -1, int min_h = -1, int max_h = -1);

	std::string name() const;

	std::string config_name() const;

	std::string create_map(const std::vector<std::string>& args);
	config create_scenario(const std::vector<std::string>& args);

private:
	friend class gui2::tmap_generator;

	std::string generate_map(const std::vector<std::string>& args, std::map<map_location,std::string>* labels=NULL);
	config economy_area_;

	size_t default_width_, default_height_, width_, height_, island_size_, iterations_, hill_size_, max_lakes_, nvillages_, castle_size_, nplayers_;
	bool link_castles_;
	config cfg_;
};

#endif
