#ifndef __ANIMATION_HPP_
#define __ANIMATION_HPP_

#include "global.hpp"
#include "stdafx.h"
#include <windowsx.h>
#include "config.hpp"
#include "unit_frame.hpp"

class tanim_type
{
public:
	tanim_type(const std::string& id = null_str, const std::string& description = null_str)
		: id_(id)
		, description_(description)
		, variables_()
	{}

public:
	std::string id_;
	std::string description_;
	std::vector<std::string> variables_;
};

//
// animation section

// SLG Maker          kingdom
// ---------------------------
// tparticular <==>   particular
// tanim       <==>   unit_animation

class tparticular: public animated<unit_frame>
{
public:
	explicit tparticular(int start_time=0, const frame_builder &builder = frame_builder())
		: animated<unit_frame>(start_time)
		, parameters_(builder)
		{};
	explicit tparticular(const config& cfg, const std::string& frame_string = "frame");

	std::string description() const;
	void update_to_ui_particular_edit(HWND hdlgP, const std::string& frame_tag) const;
	void update_to_ui_frame_edit(HWND hdlgP, int n) const;

	void paramsters_update_to_ui_anim_edit(HWND hctl, HTREEITEM branch, const std::string& name, LPARAM param) const;
	void frame_update_to_ui_anim_edit(HWND hctl, HTREEITEM branch, const std::string& name, size_t n, size_t cookie_index) const;

public:
	// animation params that can be locally overridden by frames
	frame_parsed_parameters parameters_;
};

class tanim_
{
public:
	enum {PARAM_NONE = 0, PARAM_ANIM, PARAM_MODE, PARAM_FILTER, PARAM_TIME, PARAM_PARTICULAR, PARAM_FRAME = 100};

	tanim_() 
		: id_()
		, cfg_()
		, screen_mode_(false)
		, global_(false)
	{}

	bool operator==(const tanim_& that) const
	{
		if (id_ != that.id_) return false;
		if (cfg_ != that.cfg_) return false;
		if (screen_mode_ != that.screen_mode_) return false;
		if (global_ != that.global_) return false;
		return true;
	}
	bool operator!=(const tanim_& that) const { return !operator==(that); }

public:
	std::string id_;
	bool screen_mode_;
	bool global_;
	config cfg_;
};

class tanim: public tanim_
{
public:
	static std::vector<tanim_type> anim_types;
	static const tanim_type& anim_type(const std::string& id);
	static tanim_type null_anim_type;

	typedef enum { HIT, MISS, KILL, INVALID} hit_type;

	tanim()
		: tanim_()
		, anim_from_cfg_()
		, directions_()
		, primary_attack_filter_()
		, secondary_attack_filter_()
		, hits_()
		, unit_anim_()
		, sub_anims_()
	{}

	void from_config(const config& cfg, bool golbal);
	void from_ui(HWND hdlgP);
	void from_ui_anim_edit(HWND hdlgP);
	bool from_ui_particular_edit(HWND hdlgP, int n);
	bool from_ui_filter_edit(HWND hdlgP);
	bool from_ui_frame_edit(HWND hdlgP, tparticular& l, int n);
	void update_to_ui_anim_edit(HWND hdlgP);
	void update_to_ui_filter_edit(HWND hdlgP) const;
	std::string generate() const;

	void change_particular_name(const std::string& from, const std::string& to);

	void add_particular();
	void delete_particular(int n);

	void add_frame(tparticular& l, int n, bool front);
	void delete_frame(tparticular& l, int n);

	bool dirty() const;

	void parse(const config& cfg);
	int get_end_time() const;
	int get_begin_time() const;

	std::string filter_description() const;
	bool has_particular(const std::string& tag) const;
	std::string particular_frame_tag(const tparticular& l) const;

private:
	void filter_update_to_ui_anim_edit(HWND hctl, HTREEITEM branch) const;

public:
	tparticular unit_anim_;
	std::map<std::string, tparticular> sub_anims_;

	std::vector<std::pair<tparticular*, int> > frame_cookie_;

	// filter
	std::set<map_location::DIRECTION> directions_;
	std::set<hit_type> hits_;
	std::vector<config> primary_attack_filter_;
	std::vector<config> secondary_attack_filter_;

	tanim_ anim_from_cfg_;
};

#endif // __ANIMATION_HPP_
