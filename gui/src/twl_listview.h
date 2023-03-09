#pragma once
#include "twl_imagelist.h"

class TListViewB : public TNotifyWin, public THasIconWin {
	unsigned int m_last_col;
	int m_last_row;
	bool m_custom_paint;
	COLORREF m_fg, m_bg;

public:
	TListViewB(TEventWindow* form, bool large_icons = false, bool multiple_columns = false, bool single_select = true);
	int load_icons(pchar path, bool small_size = true) override;
	void add_column(pchar label, int width);
	void autosize_column(int col, bool by_contents);
	void start_items();
	int add_item_at(int i, pchar text, int idx = 0, int data = 0);
	int add_item(pchar text, int idx = 0, int data = 0);
	void add_subitem(int i, pchar text, int sub_idx);
	virtual void remove_item(int i);
	void select_item(int i);
	void get_item_text(int i, wchar_t* buff, int buffsize);
	int  get_item_data(int i);
	int  selected_id();
	int  next_selected_id(int i);
	int  count();
	int  selected_count();
	unsigned int columns() const;
	virtual void clear();
	void set_foreground(COLORREF colour);
	void set_background(COLORREF colour);
	void set_theme(bool explorer);

	virtual void handle_select(intptr_t id) = 0;
	virtual void handle_double_click(int id, int j, const char* s) = 0;
	virtual void handle_onkey(int id) = 0;
	virtual int handle_rclick() = 0;
	virtual void handle_onfocus(bool yes) = 0;

	// override
	int handle_notify(void* p) override;
};

class TListView : public TListViewB {
	TEventWindow* m_form;
	SelectionHandler m_on_select, m_on_key;
	SelectionHandler3 m_on_double_click;
public:
	explicit TListView(TEventWindow* form, bool large_icons = false, bool multiple_columns = false, bool single_select = true);
	void on_select(SelectionHandler handler)
	{
		m_on_select = handler;
	}
	void on_double_click(SelectionHandler3 handler)
	{
		m_on_double_click = handler;
	}
	void on_key(SelectionHandler handler)
	{
		m_on_key = handler;
	}

	// implement
	void handle_select(intptr_t id) override;
	void handle_double_click(int row, int col, const char* s) override;
	void handle_onkey(int id) override;
};
