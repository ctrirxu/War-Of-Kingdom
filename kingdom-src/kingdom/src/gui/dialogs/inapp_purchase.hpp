/* $Id: language_selection.hpp 48440 2011-02-07 20:57:31Z mordante $ */
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

#ifndef GUI_DIALOGS_INAPP_PURCHASE_HPP_INCLUDED
#define GUI_DIALOGS_INAPP_PURCHASE_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"
#include "sdl_utils.hpp"

namespace gui2 {

class tbutton;
class tlabel;
class tlistbox;

struct tinapp_item
{
	tinapp_item(int _index, const std::string& _short_id = null_str);

	int index;
	std::string short_id;
	std::string id;
	std::string name;
	std::string description;
	std::string icon;
	int price;
	bool purchased;
};

class tinapp_purchase : public tdialog
{
public:
	static tinapp_purchase* get_singleton() { return singleton_ ;}

	tinapp_purchase(bool browse);
	~tinapp_purchase();

	void refresh_list(bool set_purchase = false) const;

	tlabel& status() { return *status_; }
	tinapp_item& get_item(int index);

	void purchase_status(bool exit);
private:
	void restore(twindow& window);
	void purchase(twindow& window);
	void timer_handler();

	void item_selected(twindow& window);
	void refresh_tip(twindow& window, const tinapp_item& item);

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

private:
	std::vector<tinapp_item> items_;
	tbutton* restore_;
	tbutton* purchase_;
	tlabel* status_;
	tlistbox* list_;
	bool browse_;

	// timer
	unsigned long ing_timer_;
	int ing_ticks_;
	int ing_item_;

	static tinapp_purchase* singleton_;
};

} // namespace gui2

#endif

