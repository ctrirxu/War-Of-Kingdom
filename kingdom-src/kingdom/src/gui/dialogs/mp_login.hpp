/* $Id: mp_login.hpp 48879 2011-03-13 07:49:40Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_MP_LOGIN_HPP_INCLUDED
#define GUI_DIALOGS_MP_LOGIN_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

class game_display;

namespace gui2 {

class tmp_login : public tdialog
{
public:
	tmp_login(game_display& disp, const std::string& label);

private:

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

	void register1(twindow& window);

	std::string text_box_str(twindow& window, const std::string& id, const std::string& name, int min, int max);
	bool create(twindow& window, bool close);

private:
	game_display& disp_;
};

} // namespace gui2

#endif

