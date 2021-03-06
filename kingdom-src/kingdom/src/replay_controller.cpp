/* $Id: replay_controller.cpp 47511 2010-11-07 21:14:49Z shadowmaster $ */
/*
   Copyright (C) 2006 - 2010 by Joerg Hinrichs <joerg.hinrichs@alice-dsl.de>
   wesnoth playlevel Copyright (C) 2003 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "game_end_exceptions.hpp"
#include "game_events.hpp"
#include "gettext.hpp"
#include "log.hpp"
#include "map_label.hpp"
#include "replay_controller.hpp"
#include "resources.hpp"
#include "savegame.hpp"
#include "unit_display.hpp"
#include "artifical.hpp"
#include "replay.hpp"

#include <boost/foreach.hpp>

static lg::log_domain log_engine("engine");
#define DBG_NG LOG_STREAM(debug, log_engine)

static lg::log_domain log_replay("replay");
#define DBG_REPLAY LOG_STREAM(debug, log_replay)
#define LOG_REPLAY LOG_STREAM(info, log_replay)

LEVEL_RESULT play_replay_level(const config& game_config,
		const config* level, CVideo& video, game_state& state_of_game, hero_map& heros, hero_map& heros_start, 
		card_map& cards)
{
	try {
		const int ticks = SDL_GetTicks();
		int num_turns = (*level)["turns"].to_int();
		DBG_NG << "creating objects... " << (SDL_GetTicks() - ticks) << "\n";
		// !!equal 
		heros = heros_start; 
		replay_controller replaycontroller(*level, state_of_game, heros, heros_start, cards, ticks, num_turns, game_config, video);
		DBG_NG << "created objects... " << (SDL_GetTicks() - replaycontroller.get_ticks()) << "\n";
		const events::command_disabler disable_commands;

		//replay event-loop
		for (;;){
			replaycontroller.play_slice();
		}
	}
	catch(end_level_exception&){
		DBG_NG << "play_replay_level: end_level_exception\n";
	}

	return VICTORY;
}

replay_controller::replay_controller(const config& level,
		game_state& state_of_game, hero_map& heros, hero_map& heros_start, 
		card_map& cards, 
		const int ticks, const int num_turns,
		const config& game_config, CVideo& video) :
	play_controller(level, state_of_game, heros, heros_start, cards, ticks, num_turns, game_config, video, false, true),
	teams_start_(),
	gamestate_start_(gamestate_),
	units_start_(),
	tod_manager_start_(level, num_turns, &state_of_game),
	current_turn_(1),
	delay_(0),
	is_playing_(false),
	show_everything_(false),
	show_team_(state_of_game.classification().campaign_type == "multiplayer" ? 0 : 1)
{
	// units_start_ = units_;
	// teams_start_ = teams_;

	init();

	show_context_menu(NULL, *gui_);
	// reset_replay();
}

replay_controller::~replay_controller()
{
	//YogiHH
	//not absolutely sure if this is needed, but it makes me feel a lot better ;-)
	//feel free to delete this if it is not necessary
	gui_->get_theme().theme_reset().detach_handler(this);
}

void replay_controller::init(){
	DBG_REPLAY << "in replay_controller::init()...\n";

	//guarantee the cursor goes back to 'normal' at the end of the level
	const cursor::setter cursor_setter(cursor::NORMAL);
	init_replay_display();

	teams_start_ = teams_;
	for (std::vector<team>::iterator itor = teams_start_.begin(); itor != teams_start_.end(); ++ itor) {
		std::vector<artifical*>& holded_cities = itor->holded_cities();
		holded_cities.clear();
		itor->clear_troop();
	}

	city_map::affect_team_ = false;
	//!! units_start_ = units_; rpg::humans duplicated
	city_map::affect_team_ = true;

	gamestate_start_ = gamestate_;

	fire_prestart(true);

	init_gui();
	statistics::fresh_stats();
	set_victory_when_enemy_no_city(
		utils::string_bool(level_["victory_when_enemy_no_city"], true));

	DBG_REPLAY << "first_time..." << (recorder.is_skipping() ? "skipping" : "no skip") << "\n";

	fire_start(!loading_game_);
	update_gui();
}

void replay_controller::init_gui(){
	DBG_NG << "Initializing GUI... " << (SDL_GetTicks() - ticks_) << "\n";
	play_controller::init_gui();

	if (show_team_)
		gui_->set_team(show_team_ - 1, show_everything_);
	else
		gui_->set_team(0, show_everything_);

	gui_->scroll_to_leader(units_, player_number_, display::WARP);
	update_locker lock_display((*gui_).video(),false);
	for(std::vector<team>::iterator t = teams_.begin(); t != teams_.end(); ++t) {
		t->reset_objectives_changed();
	}
}

void replay_controller::init_replay_display(){
	DBG_REPLAY << "initializing replay-display... " << (SDL_GetTicks() - ticks_) << "\n";

	rebuild_replay_theme();
	gui_->get_theme().theme_reset().attach_handler(this);
	DBG_REPLAY << "done initializing replay-display... " << (SDL_GetTicks() - ticks_) << "\n";
}

void replay_controller::rebuild_replay_theme()
{
	const config &theme_cfg = get_theme(game_config_, level_["theme"]);
	// if (const config &res = theme_cfg.child("resolution"))
	BOOST_FOREACH (const config &res, theme_cfg.child_range("resolution")) {
		int width = res["width"];
		int height = res["height"];
		if (game_config::tiny_gui && (width != 640 || height != 480)) {
			continue;
		}
		if (!game_config::tiny_gui && (width != 1024 || height != 768)) {
			continue;
		}
	
		if (const config &replay_theme_cfg = res.child("replay"))
			gui_->get_theme().modify(replay_theme_cfg);
		gui_->get_theme().modify_label("time-icon", _ ("current local time"));
		//Make sure we get notified if the theme is redrawn completely. That way we have
		//a chance to restore the replay controls of the theme as well.
		gui_->invalidate_theme();
		break;
	}
}

void replay_controller::reset_replay(){
	gui::button* b = gui_->find_button("button-playreplay");
	if (b != NULL) { b->release(); }
	b = gui_->find_button("button-stopreplay");
	if (b != NULL) { b->release(); }
	gui_->clear_chat_messages();
	is_playing_ = false;
	player_number_ = 1;
	current_turn_ = 1;
	previous_turn_ = 0;
	tod_manager_= tod_manager_start_;
	recorder.start_replay();
	gamestate_ = gamestate_start_;
	if (events_manager_ ){
		// NOTE: this double reset is required so that the new
		// instance of game_events::manager isn't created before the
		// old manager is actually destroyed (triggering an assertion
		// failure)
		events_manager_.reset();
		events_manager_.reset(new game_events::manager(level_));
	}

	gui_->labels().read(level_);

	statistics::fresh_stats();
	set_victory_when_enemy_no_city(level_["victory_when_enemy_no_city"].to_bool(true));

	// Add era events for MP game.
	if (const config &era_cfg = level_.child("era")) {
		game_events::add_events(era_cfg.child_range("event"), "era_events");
	}

	teams_ = teams_start_;
	//!! units_ = units_start_; rpg::humans duplicated

	// hero_map A要被B替换, 原先指向A的hero指针要无效, 需根据number_重新定位到B
	// 需要被替换hero指针
	//  1. unit中的master_, second_, third_
	//  2. artifical中的fresh_heros_, finish_heors_
	//  3. team中的leader_
	// units_start_也要被重新定位, 否则下一次reset时, hero要出错
	units_start_.extract_heros_number();
	units_.extract_heros_number();
	// 以下将执行替换
	heros_ = heros_start_;
	units_start_.recalculate_heros_pointer();
	units_.recalculate_heros_pointer();

	fire_prestart(true);
	init_gui();
	fire_start(true);
	update_gui();

	b = gui_->find_button("button-resetreplay");
	if (b != NULL) { b->release(); }
}

void replay_controller::stop_replay(){
	is_playing_ = false;
	gui::button* b = gui_->find_button("button-playreplay");
	if (b != NULL) { b->release(); }
}

void replay_controller::replay_next_turn(){
	is_playing_ = true;
	play_turn();

 	if (!skip_replay_){
		gui_->scroll_to_leader(units_, player_number_,game_display::ONSCREEN,false);
	}
	is_playing_ = false;
	gui::button* b = gui_->find_button("button-nextturn");
	if (b != NULL) { b->release(); }
}

void replay_controller::replay_next_side(){
	is_playing_ = true;
	play_side(player_number_ - 1, false);

	if (static_cast<size_t>(player_number_) > teams_.size()) {
		player_number_ = 1;
		current_turn_++;
	}

	if (!skip_replay_) {
		gui_->scroll_to_leader(units_, player_number_,game_display::ONSCREEN,false);
	}

	is_playing_ = false;
	gui::button* b = gui_->find_button("button-nextside");
	if (b != NULL) { b->release(); }
}

void replay_controller::process_oos(const std::string& msg) const
{
	std::stringstream message;
	message << _("The replay is corrupt/out of sync. It might not make much sense to continue. Do you want to save the game?");
	message << "\n\n" << _("Error details:") << "\n\n" << msg;

	config snapshot;
	to_config(snapshot);
	savegame::oos_savegame save(heros_, heros_start_, snapshot);
	save.save_game_interactive(resources::screen->video(), message.str(), gui::YES_NO); // can throw end_level_exception
}

void replay_controller::replay_show_everything(){
	show_everything_ = true;
	show_team_ = 0;
	update_teams();
	update_gui();
}

void replay_controller::replay_show_each(){
	show_everything_ = false;
	show_team_ = 0;
	update_teams();
	update_gui();
}

void replay_controller::replay_show_team1(){
	show_everything_ = false;
	show_team_ = 1;
	update_teams();
	update_gui();
}

void replay_controller::replay_skip_animation(){
	unit_display::player_number_ = (unit_display::player_number_ >= 0)? -1: 0;
}

void replay_controller::play_replay(){
	gui::button* b = gui_->find_button("button-stopreplay");
	if (b != NULL) { b->release(); }
	if (recorder.at_end()){
		return;
	}

	try{
		is_playing_ = true;

		DBG_REPLAY << "starting main loop\n" << (SDL_GetTicks() - ticks_) << "\n";
		if (unit_display::player_number_ > 0) {
			unit_display::player_number_ = 0;
		}
		for(; !recorder.at_end() && is_playing_; first_player_ = 1) {
			play_turn();
		} //end for loop
		is_playing_ = false;
	}
	catch(end_level_exception& e){
		if (e.result == QUIT) throw;
	}
}

void replay_controller::play_turn(){
	if (recorder.at_end()){
		return;
	}

	LOG_REPLAY << "turn: " << current_turn_ << "\n";

	gui_->new_turn();
	gui_->invalidate_game_status();
	events::raise_draw_event();

	bool last_team = false;

	while ( (!last_team) && (!recorder.at_end()) && is_playing_ ){
		last_team = static_cast<size_t>(player_number_) == teams_.size();
		play_side(player_number_ - 1, false);
		play_slice();
	}
}

void replay_controller::play_side(const unsigned int /*team_index*/, bool)
{
	if (recorder.at_end()){
		return;
	}

	DBG_REPLAY << "Status turn number: " << turn() << "\n";
	DBG_REPLAY << "Replay_Controller turn number: " << current_turn_ << "\n";
	DBG_REPLAY << "Player number: " << player_number_ << "\n";

	try{
		// If a side is empty skip over it.
		if (!current_team().is_empty()) {
			statistics::reset_turn_stats(current_team().save_id());

			play_controller::init_side(player_number_ - 1, true);

			DBG_REPLAY << "doing replay " << player_number_ << "\n";
			do_replay(player_number_);

			finish_side_turn();
/*
			// This is necessary for replays in order to show possible movements.
			BOOST_FOREACH (unit &u, units_) {
				if (u.side() != player_number_) {
					u.new_turn();
				}
			}
*/
		}

		player_number_++;

		if (static_cast<size_t>(player_number_) > teams_.size()) {
			tod_manager_.next_turn();
			finish_turn();
			player_number_ = 1;
			current_turn_++;
		}

		update_teams();
		update_gui();
	}
	catch (end_level_exception& e){
		//VICTORY/DEFEAT end_level_exception shall not return to title screen
		get_end_level_data().result = e.result;
		linger();
		if (e.result == VICTORY || e.result == DEFEAT) {
			return;
		}
		throw;
	}
}

void replay_controller::linger()
{
	browse_ = true;
	linger_ = true;
	// If we need to set the status depending on the completion state
	// we're needed here.
	gui_->set_game_mode(game_display::LINGER_MP);

	gui_->invalidate_theme();
	gui_->redraw_everything();

	start_pass_scenario_anim(get_end_level_data().result);
}

void replay_controller::update_teams(){
	int next_team = player_number_;
	if(static_cast<size_t>(next_team) > teams_.size()) {
		next_team = 1;
	}

	if (!show_team_)
		gui_->set_team(next_team - 1, show_everything_);

	::clear_shroud(next_team);
	gui_->set_playing_team(next_team - 1);
	gui_->invalidate_all();
}

void replay_controller::update_gui(){
	(*gui_).recalculate_minimap();
	(*gui_).redraw_minimap();
	(*gui_).invalidate_all();
	events::raise_draw_event();
	(*gui_).draw();
}

void replay_controller::preferences(){
	play_controller::preferences();
	update_gui();
}

void replay_controller::show_statistics(){
	menu_handler_.show_statistics(gui_->playing_team()+1);
}

void replay_controller::handle_generic_event(const std::string& /*name*/){
	rebuild_replay_theme();
}

bool replay_controller::can_execute_command(hotkey::HOTKEY_COMMAND command, int index) const
{
	bool result = play_controller::can_execute_command(command,index);

	switch(command) {

	//commands we can always do
	case hotkey::HOTKEY_PLAY_REPLAY:
	case hotkey::HOTKEY_RESET_REPLAY:
	case hotkey::HOTKEY_STOP_REPLAY:
	case hotkey::HOTKEY_REPLAY_NEXT_TURN:
	case hotkey::HOTKEY_REPLAY_NEXT_SIDE:
	case hotkey::HOTKEY_REPLAY_SHOW_EVERYTHING:
	case hotkey::HOTKEY_REPLAY_SHOW_EACH:
	case hotkey::HOTKEY_REPLAY_SHOW_TEAM1:
	case hotkey::HOTKEY_REPLAY_SKIP_ANIMATION:
	case hotkey::HOTKEY_CHAT_LOG:
		return true;

	default:
		return result;
	}
}
