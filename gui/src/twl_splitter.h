#pragma once
#include "twl.h"

class TSplitterB : public TEventWindow {
protected:
	int m_split, m_new_size;
	CursorType m_cursor;
	bool m_line_visible, m_down, m_vertical;
	HDC m_line_dc;
	TWin* m_control;
	TWin* m_form;
	static Point m_start;
public:
	TSplitterB(TWin* parent, TWin* control, int thick = 3);
	void update_size(short x, short y);
	void draw_line();

	// overrides
	void mouse_down(Point& pt) override;
	void mouse_move(Point& pt) override;
	void mouse_up(Point& pt) override;

	virtual void on_resize(const Rect& rt);
};

class TSplitter : public TSplitterB {
public:
	TSplitter(TEventWindow* parent, TWin* control);
	void on_resize(const Rect& rt) override;
};
