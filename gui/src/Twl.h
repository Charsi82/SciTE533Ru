/* TWL.H
 * Steve Donovan, 2003
 * This is GPL'd software, and the usual disclaimers apply.
 * See LICENCE
*/
#pragma once
#include <windows.h>
#include <memory>
typedef const TCHAR* pchar;
#include "list"
#include "utf.h"
class TWin;
typedef std::list<TWin*> ChildList;

// 4312 int to void*
// 4244 lua_integer to int
// 4267 size_t to int
#pragma warning(disable: 4312 4244 4267)

#define WM_USER_PLUS 0x9999

#ifndef __UNDERC__
// #define __declspec(dllexport)	--[http://forum.ru-board.com/topic.cgi?forum=2&topic=3339&start=400#20]
#define EXPORT
#define DLGFN BOOL __stdcall
#define WNDFN LRESULT __stdcall
#else
#define DLGFN __stdcall int
#define WNDFN __stdcall long
#endif
typedef void* Handle;

static HINSTANCE hInst;

struct Point : POINT {
	explicit Point(int xp = 0, int yp = 0) { x = xp; y = yp; }
	void set(int xp, int yp) { x = xp; y = yp; }
};

class TEventWindow;
struct Rect : RECT {
	enum class Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT };
	Rect() = default;
	explicit Rect(const TEventWindow* pwin);
	Rect(int x0, int y0, int x1, int y1)
	{
		left = x0; top = y0; right = x1;  bottom = y1;
	}
	bool is_inside(const Point& p) const;
	Point corner(Corner idx) const;
	long width() const;
	long height() const;
	void offset_by(int dx, int dy);
};

struct RectD {
	double left, top, right, bottom;
};

// warnings about 'int' to 'bool' conversions can be suppressed
#pragma warning (disable: 4800)
// returnig value ignored
#pragma warning (disable: 6031)

#define BUFSIZE     120
//#define ID_ABOUT    103

extern wchar_t obuff[BUFSIZE];

namespace
{
	int MSG_WARNING = 1;
	int MSG_ERROR = 2;
	int MSG_QUERY = 3;
}

#define PARENT_WND ((TWin*)0x1)

enum class Alignment { alNone, alTop, alBottom, alLeft, alRight, alClient };

// a basic wrapper for a HWND
class TWin {
public:
	explicit TWin(Handle hwnd = NULL) { set(hwnd); }
	void set(Handle hwnd)
	{
		m_hwnd = hwnd;
		m_align = Alignment::alNone;
	}
	TWin(TWin* parent, pchar winclss, pchar text, int id, DWORD styleEx = 0);
	std::unique_ptr<TWin> create_child(pchar winclss, pchar text, int id, DWORD styleEx = 0);
	virtual ~TWin();
	virtual void update();
	Handle handle() { return m_hwnd; }
	void  invalidate(Rect* lprt = NULL);
	void  get_client_rect(Rect& rt) const;
	void  get_rect(Rect& rt, bool use_client = false);
	int   width();
	int   height();
	virtual void  set_text(pchar str);
	void  get_text(gui_string& str);
	void  set_text(int id, pchar str);
	void  get_text(int id, gui_string& str);
	void  set_int(int id, int val);
	int   get_int(int id);
	int   get_ctrl_id();
	std::unique_ptr<TWin> get_twin(int id);
	std::unique_ptr<TWin> get_active_window();
	int   get_id();
	void  set_focus();
	void  mouse_capture(bool do_grab);
	void  resize(int x0, int y0, int w, int h);
	void  resize(const Rect& rt);
	void  resize(int w, int h);
	void  move(int x0, int y0);
	void  map_points(Point* pt, int n, TWin* target_wnd = PARENT_WND);
	void  on_top();
	void  to_foreground();
	virtual void  show(int how = SW_SHOW);
	virtual void  hide();
	bool  visible();
	void  set_parent(TWin* w);
	void  set_tooltip(int id, pchar tiptext, bool balloon);
	void  set_style(DWORD s);
	LRESULT send_msg(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0) const;
	void  close();
	int message(pchar msg, int type = 0);
	Alignment align() { return m_align; }
	void align(Alignment a, int size = 0);
	std::unique_ptr<TWin> get_foreground_window();
	bool set_enable(bool state);
protected:
	Handle m_hwnd;
	Alignment m_align;
};

class TGDIObj {
public:
	TGDIObj() { m_hand = NULL; };
	//~TGDIObj();
	void  set_handle(Handle hand) { m_hand = hand; }
	Handle handle() { return m_hand; }
	void destroy();
protected:
	Handle m_hand;
};

enum { NORMAL, BOLD = 2, ITALIC = 4 };

class TFont : public TGDIObj {
public:
	TFont();
	TFont(const TFont& f);
	~TFont();
	void create();
	// TFont& operator = (pchar spec);
	TFont& operator = (const TFont& f);
	void  set(pchar name, int sz, int ftype = NORMAL);
	void  set_name(pchar name);
	void  set_size(int pts);
	void  set_bold();
	void  set_italic();
protected:
	LOGFONT* m_pfont;
};


///// Wrapping up the Windows Device Context
class TDC {
public:
	TDC();
	~TDC();
	void set_hdc(Handle hdc) { m_hdc = hdc;	}
	Handle get_hdc() { return m_hdc; }
	void set_twin(TWin* w) { m_twin = w; }
	void get(TWin* pw = nullptr);
	void release(TWin* pw = nullptr);
	void kill();
	
	Handle select(Handle obj);
	Handle select(TGDIObj& obj)	{ return select(obj.handle()); }
	void select_stock(int val);

	void xor_pen(bool on_off);
	// this changes both the _pen_ and the _text_ colour
	void set_colour(float r, float g, float b);
	// wrappers around common graphics calls
	void set_text_align(int flags);
	void get_text_extent(pchar text, int& w, int& h, TFont* font = nullptr);
	void draw_text(pchar msg);
	void move_to(int x, int y);
	void line_to(int x, int y);
	void rectangle(const Rect& rt);
	void polyline(Point* pts, int npoints);
	void draw_focus_rect(const Rect& rt);
	void draw_line(const Point& p1, const Point& p2);

protected:
	Handle m_hdc, m_pen, m_font, m_brush;
	int m_flags;
	TWin* m_twin;
};

class TClientDC : public TDC {
public:
	explicit TClientDC(TWin* w)
	{
		set_twin(w);
		get();
	}

	~TClientDC()
	{
		release();
	}
};

class AbstractMessageHandler;
class TNotifyWin;
class THasIconWin
{
	Handle m_il_handle{};
public:
	bool has_image() const { return m_il_handle; }
	int new_image_list(pchar path, bool small_size);
	Handle get_image_list() { return m_il_handle; }
	virtual int load_icons(pchar path, bool small_size) = 0;
};

enum class CursorType { RESTORE, ARROW, HOURGLASS, SIZE_VERT, SIZE_HORZ, CROSS, HAND, UPARROW };

class TEventWindow : public TWin
{
	friend WNDFN WndProc(Handle, UINT, WPARAM, LPARAM);
	bool m_do_resize;
	POINT m_fixed_size;
	AbstractMessageHandler* m_dispatcher;
	Handle m_accel;
	HMENU m_hpopup_menu;
	TDC* m_dc;
	bool m_child_messages;
	DWORD m_style_extra;
	Handle m_old_cursor;
	ChildList* m_children;
	TWin* m_tool_bar;
	UINT statusbar_id;
public:

	enum {
		TM_CHAR_HEIGHT = 1, TM_CHAR_WIDTH,
		TM_CAPTION_HEIGHT, TM_MENU_HEIGHT, TM_CLIENT_EXTRA,
		TM_SCREEN_WIDTH, TM_SCREEN_HEIGHT,
		TM_END
	};

	POINT fixed_size();
	void enable_resize(bool do_resize, int w = 0, int h = 0);
	bool cant_resize() const;
	TDC* get_dc() { return m_dc; }
	bool child_messages() { return m_child_messages; }
	void view_child_messages(bool yesno) { m_child_messages = yesno; }
	virtual void client_resize(int cwidth, int cheight);
	//TEventWindow() { set_defaults(); }
	explicit TEventWindow(pchar caption, TWin* parent = 0, DWORD style_extra = 0, int is_child = false, DWORD style_override = -1);
	virtual ~TEventWindow();

	void set_statusbar(int parts, int* widths);
	void set_statusbar_text(int part_id, pchar str);
	void add(TWin* win);
	void remove(TWin* win);
	TWin* get_client() { return m_client; }
	//void set_client(TWin* cli, bool do_subclass = false);
	void set_client(TWin* cli);

	void set_defaults();
	void set_window();
	void add_accelerator(Handle accel);
	void set_icon(pchar file);
	void set_icon_from_window(TWin* win);

	bool check_notify(LPARAM lParam, int& ret);
	void create_window(pchar caption, TWin* parent, bool is_child);
	void create_timer(int msec);
	void kill_timer();
	void set_menu(pchar res);
	void set_menu(HANDLE menu);
	void set_popup_menu(HANDLE menu);
	void last_mouse_pos(int& x, int& y);
	HMENU get_menu() { return m_hmenu; }
	void check_menu(int id, bool check);
	void set_toolbar(TWin* tb, TNotifyWin* tth);
	TWin* toolbar() { return m_tool_bar; }
	void add_handler(AbstractMessageHandler* m_hand);
	AbstractMessageHandler* get_handler() { return m_dispatcher; }
	//void update_data();
	//void update_controls();
	void quit(int retcode = 0);
	int metrics(int ntype);
	void cursor(CursorType curs);
	int run();

	//-----------Event Handling-------------------
	  //virtual void show() {}
	virtual void size();
	virtual void paint(TDC&);
	virtual void ncpaint(TDC&);
	virtual bool query_close() { return true; }
	virtual void on_close() { }
	virtual void on_showhide(bool show) { }
	virtual bool activate(bool yes) { return true; }
	virtual void on_select(Rect& rt) { }

	// input
	virtual void keydown(int vkey);

	// mouse messages
	virtual void mouse_down(Point& pt);
	virtual void mouse_move(Point& pt);
	virtual void mouse_up(Point& pt);
	virtual void right_mouse_down(Point& pt);

	// scrolling
	// virtual void hscroll(int code, int posn);
	// virtual void vscroll(int code, int posn);
	virtual void scroll(int code, int posn);

	// command
	virtual bool command(int id, int code);
	virtual bool sys_command(int id);
	virtual int  notify(int id, void* ph);
	virtual int  handle_user(WPARAM wparam, LPARAM lparam);

	// other
	virtual void timer();
	virtual void focus();
	virtual void destroy();
	void show(int how = 0) override;

	void set_background(float r, float g, float b); // how to spec. colour??

protected:
	HBRUSH m_bkgnd_brush;//to do
	HMENU m_hmenu;
	DWORD m_style;
	UINT_PTR m_timer;
	COLORREF m_bk_color;
	TWin* m_client;
};

class TNotifyWin : public TWin {
	Handle m_hpopup_menu;
	TEventWindow* m_form;
public:
	TNotifyWin(TEventWindow* form) : m_hpopup_menu(NULL), m_form(form) { }
	virtual int handle_notify(void* p) = 0;
	TEventWindow* get_parent_win() { return m_form; };
	void set_popup_menu(Handle menu)
	{
		m_hpopup_menu = menu;
	}
	void show_popup()
	{
		if (m_hpopup_menu) {
			POINT p;
			GetCursorPos(&p);
			HWND hwnd = (HWND)(get_parent_win()->handle());
			TrackPopupMenu((HMENU)m_hpopup_menu, TPM_LEFTALIGN | TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
		}
	}
};

class AbstractMessageHandler {
protected:
	TEventWindow* m_form;
public:
	AbstractMessageHandler(TEventWindow* form)
		: m_form(form) { }
	virtual ~AbstractMessageHandler() { }
	void set_form(TEventWindow* form) { m_form = form; }
	virtual bool dispatch(UINT id, int notify, Handle ctrl) = 0;
	virtual void add_handler(AbstractMessageHandler*) = 0;
	virtual void write() = 0;
	virtual void read() = 0;
};

//class TFrameWindow : public TEventWindow {
//public:
//	TFrameWindow(pchar caption = L"twl", bool has_status = false, TWin* cli = nullptr);
//	virtual ~TFrameWindow();
//
//	// overrides
//	void destroy() override;
//	void client_resize(int cwidth, int cheight) override;
//
//	void set_status_fields(int* parts = NULL, int n = 0);
//	void set_status_text(int id, pchar txt);
//
//protected:
//	TWin* m_status;
//};

typedef TEventWindow TEW;  //...common alias for EventWindow!

//////// Wrappers around Windows Controls //////

class TControl : public TWin {
public:
	LONG_PTR m_wnd_proc;  //*TEMP*
	TControl(TWin* parent, pchar classname, pchar text, int id = -1, DWORD style = 0);
	virtual pchar type_name() const = 0;
	virtual void calc_size();
	bool is_type(pchar tname) const;
	void set_font(TFont* fnt);
	TEventWindow* get_parent_win() const { return m_parent; }
	void parent(TEventWindow* ew) { m_parent = ew; }
	void data(void* data) { m_data = data; }
	void* data() { return m_data; }

	static TControl* user_data(Handle handle);
protected:
	TFont* m_font;
	TEventWindow* m_parent;
	void* m_data;
};

class THasIconCtrl
{
public:
	virtual void set_icon(pchar mod, int icon_id) = 0;
	virtual void set_bitmap(pchar file) = 0;
};

class TLabel : public TControl, public THasIconCtrl {
public:
	TLabel(TWin* parent, pchar text, DWORD style = 0);
	pchar type_name() const override { return L"TLabel"; }
	void set_text(pchar caption)
	{
		TWin::set_text(caption);
		calc_size();
		invalidate();
	}
	void set_icon(pchar file, int icon_idx) override;
	void set_bitmap(pchar file) override;
};

class TEdit : public TControl {
public:
	TEdit(TWin* parent, pchar text, int id = -1, DWORD style = 0);
	pchar type_name() const override { return L"TEdit"; }
	void set_selection(int start, int finish);
};

class TProgressControl : public TControl {
public:
	TProgressControl(TWin* parent, int id, bool vertical, bool hasborder, bool smooth, bool smoothrevers);
	pchar type_name() const override { return L"Progress"; }
	void set_range(int from, int to);
	void set_pos(int to);
	void set_step(int step);
	void go();
	int get_range(int& hi);
};

class TComboBox : public TControl {
public:
	TComboBox(TWin* parent, int id, DWORD style);
	pchar type_name() const override { return L"TComboBox"; }
	void reset();
	int add_string(pchar);
	void ins_string(int, pchar);
	void set_data(int idx, int data);
	int get_data(int idx);
	void set_height(int sz);
	void set_cursel(int id);
	int get_cursel();
	int count();
};

// add text to log file
void log_add(const char* s, int val = 0);

// Loading icons from .ico and .dll
Handle load_icon(pchar file, int idx = 0, bool small_icon = true);

//Loading icons from .ico and .dll, bitmaps from .bmp files
Handle load_bitmap(pchar file);
