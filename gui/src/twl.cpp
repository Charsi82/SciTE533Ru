// compile@ gcc -c -D_WINDOWS_ $(FileNameExt)
// TWL.CPP
/*
 * Main classes of YAWL
 * Steve Donovan, 2003
 * This is GPL'd software, and the usual disclaimers apply.
 * See LICENCE
 * Based on the Tiny, Terminal or Toy Windows Library
 *  Steve Donovan, 1998
 *  based on SWL, 1995.
*/
#define NO_STRICT
#include "twl.h"
#ifdef _WIN32
	#include <commctrl.h>
#else
	#define LOW_WORD WORD
	#define GWL_USERDATA 0
#endif

#include <string>
#include <stdlib.h>
#include <ctype.h>
#include "twl_cntrls.h"
#include <cassert> // for assert

constexpr TCHAR EW_CLASSNAME[] = L"EVNTWNDCLSS";

static int CmdShow;
static HANDLE hAccel = 0, hModeless = 0;
static Point g_mouse_pt, g_mouse_pt_right;

#include <fstream>
std::string UTF8FromString(const std::wstring& s);

#define LOG_ON

#ifdef LOG_ON
class Log
{
private:
	std::string txt_log;
public:
	Log() = default;
	~Log() {
		char pFilename[MAX_PATH]{};
		GetModuleFileNameA(NULL, pFilename, MAX_PATH);
		std::string path = pFilename;
		path.erase(path.find_last_of('\\') + 1);
		std::ofstream outf(path.append("log.txt").c_str());
		outf << txt_log;
		outf.flush();
		outf.close();
	}
	void add(const char* txt) {
		txt_log.append(txt).append("\n");
	}
	void clear()
	{
		txt_log.clear();
	}
};

Log g_log;

void log_add(const char* s, int val)
{
	if (s && *s) {
		char buf[512]{};
		sprintf_s(buf, s, val);
		g_log.add(buf);
	}
	else
	{
		g_log.clear();
	}
}
#else
void log_add(const char*, int) {}
#endif

typedef unsigned char byte;

//HINSTANCE ApplicationInstance() { return hInst; }

//void subclass_control(TControl* ctrl); // in twl_cntrls.cpp
//void remove_subclass_control(TControl* ctrl);

// Miscelaneous functions!!
COLORREF RGBF(float r, float g, float b)
{
	return RGB(byte(255 * r), byte(255 * g), byte(255 * b));
}

//int exec(pchar s, int mode)
//{
//	return (int)(size_t)(ShellExecute(0, L"open", s, NULL, NULL, mode)) > 31;
//}

Rect::Rect(const TEventWindow* pwin)
{
	pwin->get_client_rect(*this);
}

bool Rect::is_inside(const Point& p) const
{
	return PtInRect(static_cast<const RECT*>(this), (POINT)p);
}

Point Rect::corner(Corner idx) const
{
	int x, y;
	switch (idx) {
	case Corner::TOP_LEFT:		x = left;	y = top; break;
	case Corner::TOP_RIGHT:		x = right;	y = top; break;
	case Corner::BOTTOM_RIGHT:	x = right;	y = bottom; break;
	case Corner::BOTTOM_LEFT:	x = left;	y = bottom; break;
	default: x = y = 0; break;
	}
	return Point(x, y);
}

long Rect::width() const
{
	return right - left;
}

long Rect::height() const
{
	return bottom - top;
}

void Rect::offset_by(int dx, int dy)
{
	OffsetRect(static_cast<RECT*>(this), dx, dy);
}

/// TDC ///////////////

TDC::TDC()
{
	m_hdc = m_pen = m_font = m_brush = NULL;
	m_twin = NULL; m_flags = 0;
}

TDC::~TDC() = default;

void TDC::get(TWin * pw)
//---------------------
{
	if (!pw) pw = m_twin;
	m_hdc = GetDC(pw->handle());
}

void TDC::release(TWin * pw)
//--------------------------
{
	if (!pw) pw = m_twin;
	ReleaseDC(pw->handle(), m_hdc);
}

void TDC::kill()
//--------------
{
	DeleteDC(m_hdc);
}

Handle TDC::select(Handle obj)
//---------------------------
{
	return SelectObject(m_hdc, obj);
}

void TDC::select_stock(int val)
//----------------------------
{
	select(GetStockObject(val));
}

void TDC::xor_pen(bool on_off)
//-----------------
{
	SetROP2(m_hdc, !on_off ? R2_COPYPEN : R2_XORPEN);
}

// this changes both the _pen_ and the _text_ colour
void TDC::set_colour(float r, float g, float b)
//----------------------------
{
	COLORREF rgb = RGBF(r, g, b);
	get();
	SetTextColor(m_hdc, rgb);
	if (m_pen) DeleteObject(m_pen);
	m_pen = CreatePen(PS_SOLID, 0, rgb);
	select(m_pen);
	release();
}

void TDC::set_text_align(int flags)
//---------------------------------
{
	get();
	SetTextAlign(m_hdc, TA_UPDATECP | flags);
	release();
}

void TDC::get_text_extent(pchar text, int& w, int& h, TFont * font)
//----------------------------------------------------------------
{
	SIZE sz;
	HFONT oldfont = 0;
	get();
	if (font) oldfont = select(*font);
	GetTextExtentPoint32(m_hdc, text, wcslen(text), &sz);
	if (font) select(oldfont);
	release();
	w = sz.cx;
	h = sz.cy;
}

// wrappers around common graphics calls
void TDC::draw_text(pchar msg)
//---------------------------------------------------
{
	TextOut(m_hdc, 0, 0, msg, wcslen(msg));
}

void TDC::move_to(int x, int y)
//----------------------------
{
	MoveToEx(m_hdc, x, y, NULL);
}

void TDC::line_to(int x, int y)
//-----------------------------
{
	LineTo(m_hdc, x, y/*,NULL*/);
}

void TDC::rectangle(const Rect & rt)
//---------------------------------
{
	Rectangle(m_hdc, rt.left, rt.top, rt.right, rt.bottom);
}

void TDC::polyline(Point * pts, int npoints)
{
	Polyline(m_hdc, pts, npoints);
}

void TDC::draw_focus_rect(const Rect & rt)
{
	DrawFocusRect(m_hdc, (const RECT*)&rt);
}

void TDC::draw_line(const Point & p1, const Point & p2)
{
	POINT pts[] = { {p1.x,p1.y},{p2.x,p2.y} };
	Polyline(m_hdc, pts, 2);
}

//// TGDIObj

void TGDIObj::destroy()
{
	if (m_hand) DeleteObject(m_hand); m_hand = NULL;
}

//// TFont ////////
#define PLF ((LOGFONT *)m_pfont)

TFont::TFont()
{
	m_pfont = /*(void *)*/ new LOGFONT;
}

TFont::TFont(const TFont & f)
{
	m_pfont =/* (void *)*/ new LOGFONT;
	memcpy(m_pfont, f.m_pfont, sizeof(LOGFONT));
};

TFont::~TFont()
{
	delete /*(LOGFONT *)*/ m_pfont;
}

void TFont::set(pchar spec, int sz, int ftype)
//-------------------------------------------
{
	LOGFONT& lf = */*(LOGFONT *)*/m_pfont;  // define an alias...
	int wt = FW_NORMAL;
	lf.lfHeight = sz;
	if (ftype & BOLD)  wt = FW_BOLD;
	lf.lfWeight = wt;
	lf.lfItalic = (ftype & ITALIC) ? TRUE : FALSE;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfQuality = PROOF_QUALITY;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	wcscpy_s(lf.lfFaceName, spec);
	create();
}

/// copy constructor
TFont& TFont::operator = (const TFont & f)
//--------------------------------------
{
	memcpy(m_pfont, f.m_pfont, sizeof(LOGFONT));
	create();
	return *this;
}

void TFont::create()
{
	destroy();
	m_hand = CreateFontIndirect(/*(LOGFONT *)*/m_pfont);
}

void  TFont::set_name(pchar name)
{
	wcscpy_s(PLF->lfFaceName, name); create();
}

void  TFont::set_size(int pts)
{
	PLF->lfHeight = pts;      create();
}

void  TFont::set_bold()
{
	PLF->lfWeight = FW_BOLD;  create();
}

void  TFont::set_italic()
{
	PLF->lfItalic = TRUE;     create();
}

////// TWin ///////////
TWin::TWin(TWin * parent, pchar winclss, pchar text, int id, DWORD styleEx)
//------------------------------------------------------------------------
{
	DWORD err;
	DWORD style = WS_CHILD | WS_VISIBLE | styleEx;
	HWND hwndChild = CreateWindowEx(WS_EX_LEFT, winclss, text, style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		parent->m_hwnd, (HMENU)id, hInst, NULL);
	if (hwndChild == NULL)
	{
		err = GetLastError();
		log_add("TWin::TWin can't create window error:%d", err);
	}
	set(hwndChild);
}

TWin::~TWin()
{
	//log_add("~TWin");
}

void TWin::update()
//-----------------
{
	UpdateWindow(m_hwnd);
}

void TWin::invalidate(Rect* lprt)
{
	InvalidateRect(m_hwnd, (LPRECT)lprt, TRUE);
}

void TWin::align(Alignment a, int size)
{
	m_align = a;
	if (size > 0 && a != Alignment::alNone) {
		if (a == Alignment::alRight || a == Alignment::alLeft)
			resize(size, 0);
		else
			resize(0, size);
	}
}

void TWin::get_client_rect(Rect & rt) const
{
	GetClientRect(m_hwnd, (LPRECT)&rt);
}

void TWin::get_rect(Rect & rt, bool use_parent_client)
//--------------------------------------------------
{
	GetWindowRect(m_hwnd, (LPRECT)&rt);
	if (use_parent_client) {
		HWND hp = GetParent(m_hwnd);
		MapWindowPoints(NULL, hp, (LPPOINT)&rt, 2);
	}
}

void TWin::map_points(Point * pt, int n, TWin * target_wnd /*= PARENT_WND*/)
{
	HWND hwndTo = (target_wnd == PARENT_WND) ? GetParent(m_hwnd) : target_wnd->handle();
	MapWindowPoints(m_hwnd, hwndTo, (LPPOINT)pt, n);
}

int TWin::width()
//---------------
{
	Rect rt; get_client_rect(rt);
	return rt.right - rt.left;
}

int TWin::height()
//----------------
{
	Rect rt; get_client_rect(rt);
	return rt.bottom - rt.top;
}

void TWin::set_text(pchar str)
//---------------------------
{
	SetWindowText(m_hwnd, str);
}

void TWin::get_text(gui_string& str)
//-------------------------------------
{
	int len = GetWindowTextLength(m_hwnd);
	if (len)
	{
		++len;
		str.resize(len);
		GetWindowText(m_hwnd, &str[0], len);
	}
}

// These guys work with the specified _child_ of the window

void TWin::set_text(int id, pchar str)
//-------------------------------------
{
	SetDlgItemText(m_hwnd, id, str);
}

void TWin::set_int(int id, int val)
//-------------------------------------
{
	SetDlgItemInt(m_hwnd, id, val, TRUE);
}

void TWin::get_text(int id, gui_string& str)
//--------------------------------------------
{
	int len = GetWindowTextLength(GetDlgItem(m_hwnd, id));
	if (len)
	{
		++len;
		str.resize(len);
		GetDlgItemText(m_hwnd, id, &str[0], len);
	}
}

int TWin::get_ctrl_id()
//-----------------------
{
	return GetDlgCtrlID(m_hwnd);
}

int TWin::get_int(int id)
//-----------------------
{
	BOOL success;
	return (int)GetDlgItemInt(m_hwnd, id, &success, TRUE);
}

std::unique_ptr<TWin> TWin::get_twin(int id)
//--------------------
{
	HWND hwnd = GetDlgItem(m_hwnd, id);
	if (hwnd) {
		// Extract the 'this' pointer, if it exists
		TWin* pwin = (TWin*)GetWindowLongPtr(hwnd, 0);
		// if not, then just wrap up the handle
		if (!pwin) pwin = new TWin(hwnd);
		return std::make_unique<TWin>(pwin);
	}
	return nullptr;
}

std::unique_ptr<TWin> TWin::get_active_window()
{
	return std::make_unique<TWin>(GetActiveWindow());
}

std::unique_ptr<TWin> TWin::get_foreground_window()
{
	return std::make_unique<TWin>(GetForegroundWindow());
}

bool TWin::set_enable(bool state)
{
	return EnableWindow(m_hwnd, state);
}

void TWin::to_foreground()
{
	SetForegroundWindow(m_hwnd);
}

void TWin::set_focus()
{
	SetFocus((HWND)m_hwnd);
}

void TWin::mouse_capture(bool do_grab)
{
	if (do_grab) SetCapture(m_hwnd);
	else ReleaseCapture();
}

void TWin::close()
{
	send_msg(WM_CLOSE);
}

int TWin::get_id()
//------------
{
	return (int)GetWindowLongPtr(m_hwnd, GWL_ID);
}


void TWin::resize(int x0, int y0, int w, int h)
//--------------------------------------------
{
	MoveWindow(m_hwnd, x0, y0, w, h, TRUE);
}

void TWin::resize(const Rect & rt)
//--------------------------------------------
{
	MoveWindow(m_hwnd, rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top, TRUE);
}


void TWin::resize(int w, int h)
//------------------------------
{
	SetWindowPos(handle(), NULL, 0, 0, w, h, SWP_NOMOVE);
}

void TWin::move(int x0, int y0)
//------------------------------
{
	SetWindowPos(handle(), NULL, x0, y0, 0, 0, SWP_NOSIZE);
}

void TWin::show(int how)
//--------------------------------
{
	ShowWindow(m_hwnd, how);
}

void TWin::hide()
{
	ShowWindow(m_hwnd, SW_HIDE);
}

bool TWin::visible()
{
	return IsWindowVisible(m_hwnd);
}

void TWin::set_parent(TWin * w)
{
	SetParent(m_hwnd, w ? w->handle() : NULL);
}

void TEventWindow::set_statusbar(int parts, int* widths)
{
	HWND st_wnd = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_TOOLTIPS/*| SBARS_SIZEGRIP*/, L"", m_hwnd, 255);
	statusbar_id = GetDlgCtrlID(st_wnd);
	SendDlgItemMessage(m_hwnd, statusbar_id, SB_SETPARTS, parts, (LPARAM)widths);
}

void TEventWindow::set_statusbar_text(int part_id, pchar str)
{
	if (statusbar_id)
		SendDlgItemMessage(m_hwnd, statusbar_id, SB_SETTEXT, part_id, (LPARAM)str);
}

void TWin::set_tooltip(int id, pchar tiptext, bool balloon)
{
	TOOLINFO ti{};
	DWORD style = WS_POPUP | TTS_ALWAYSTIP;
	if (balloon) style |= TTS_BALLOON;
	HWND hwTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	ti.hwnd = m_hwnd;
	ti.uId = (UINT_PTR)GetDlgItem(m_hwnd, id);
	ti.hinst = hInst;
	ti.lpszText = (LPWSTR)tiptext;
	SendMessage(hwTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);

	SendMessage(hwTooltip, TTM_ACTIVATE, TRUE, 0);
	const DWORD ttl = 10000; // lifetime in 10 seconds
	SendMessage(hwTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP| TTDT_AUTOMATIC, MAKELPARAM((ttl), (0)));
}

void TWin::set_style(DWORD s)
//---------------------------
{
	SetWindowLongPtr(m_hwnd, GWL_STYLE, s);
}

LRESULT TWin::send_msg(UINT msg, WPARAM wparam, LPARAM lparam) const
//---------------------------------------------------
{
	return SendMessage(m_hwnd, msg, wparam, lparam);
}

std::unique_ptr<TWin> TWin::create_child(pchar winclss, pchar text, int id, DWORD styleEx)
//------------------------------------------------------------------------
{
	return std::make_unique<TWin>(this, winclss, text, id, styleEx);
}

int TWin::message(pchar msg, int type)
//-------------------------------------
{
	int flags;
	const wchar_t* title;
	if (type == MSG_ERROR) { flags = MB_ICONERROR | MB_OK; title = L"Error"; }
	else if (type == MSG_WARNING) { flags = MB_ICONEXCLAMATION | MB_OKCANCEL; title = L"Warning"; }
	else if (type == MSG_QUERY) { flags = MB_YESNO; title = L"Query"; }
	else { flags = MB_OK; title = L"Message"; }
	int retval = (type == MSG_QUERY) ? IDYES : IDOK;
	return MessageBox(m_hwnd, msg, title, flags) == retval;
}

void TWin::on_top()  // *add 0.9.4
{
	SetWindowPos(handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

//----TEventWindow class member definitions---------------------
TEventWindow::TEventWindow(pchar caption, TWin* parent, DWORD style_extra, int is_child, DWORD style_override)
//-------------------------------------------------------------------------------------------------------
{
	m_children = new ChildList();
	m_client = NULL;
	m_tool_bar = NULL;
	m_style_extra = style_extra;
	set_defaults();
	if (style_override != -1) {
		m_style = style_override;
	}
	create_window(caption, parent, is_child);
	//m_dc->set_text_align(0);
	enable_resize(true);
	cursor(CursorType::ARROW);
	statusbar_id = 0;
}

void TEventWindow::create_window(pchar caption, TWin* parent, bool is_child)
//-------------------------------------------------------------------------
{
	HWND hParent= NULL;
	void* CreatParms[]{this, nullptr};
	m_dc = new TDC;
	//CreatParms[0] = (void*)this;
	if (parent) {
		hParent = parent->handle();
		if (is_child) m_style = WS_CHILD;
	}
	m_hwnd = CreateWindowEx(m_style_extra, EW_CLASSNAME, caption, m_style,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hParent, NULL, hInst, CreatParms);
	set_window();
}

void TEventWindow::set_defaults()
//-------------------------------
{
	m_style = WS_OVERLAPPEDWINDOW;
	//m_bkgnd_brush =  GetStockObject (GRAY_BRUSH);
	m_bk_color = GetSysColor(COLOR_BTNFACE);
	m_bkgnd_brush = CreateSolidBrush(m_bk_color);
	m_hmenu = NULL;
	m_hpopup_menu = NULL;
	m_timer = 0;
	m_dispatcher = NULL;
	m_child_messages = false;
}

void TEventWindow::add_handler(AbstractMessageHandler* m_hand)
{
	if (!m_dispatcher)
		m_dispatcher = m_hand;
	else
		m_dispatcher->add_handler(m_hand);
}

void TEventWindow::add_accelerator(Handle accel)
{
	hAccel = accel;
}

//void TEventWindow::update_data()
//{
//	get_handler()->write();
//}
//
//void TEventWindow::update_controls()
//{
//	get_handler()->read();
//}

bool TEventWindow::cant_resize() const
{
	return !m_do_resize;
}

int TEventWindow::metrics(int ntype)
//----------------------------------
// Encapsulates what we need from GetSystemMetrics() and GetTextMetrics()
{
	if (ntype < TM_CAPTION_HEIGHT) { // text metrics
		TEXTMETRIC tm;
		GetTextMetrics(get_dc()->get_hdc(), &tm);
		if (ntype == TM_CHAR_HEIGHT) return tm.tmHeight; else
			if (ntype == TM_CHAR_WIDTH)  return tm.tmMaxCharWidth;
	}
	else {
		switch (ntype) {
		case TM_CAPTION_HEIGHT: return GetSystemMetrics(SM_CYMINIMIZED);
		case TM_MENU_HEIGHT: return GetSystemMetrics(SM_CYMENU);
		case TM_CLIENT_EXTRA:
			if (m_style_extra & WS_EX_PALETTEWINDOW) {
				return GetSystemMetrics(SM_CYSMCAPTION);
			}
			else {
				return metrics(TM_CAPTION_HEIGHT) + ((m_hmenu != NULL) ? metrics(TM_MENU_HEIGHT) : 0);
			}
		case TM_SCREEN_WIDTH:  return GetSystemMetrics(SM_CXMAXIMIZED);
		case TM_SCREEN_HEIGHT: return GetSystemMetrics(SM_CYMAXIMIZED);
		default: return 0;
		}
	}
	return 0;
}

void TEventWindow::client_resize(int cwidth, int cheight)
{
	// *SJD* This allows for menus etc
	int sz = 0;
	if (!(m_style & WS_CHILD))
		sz = metrics(TM_CLIENT_EXTRA);
	resize(cwidth, cheight + sz);
}

void TEventWindow::enable_resize(bool do_resize, int w, int h)
{
	m_do_resize = do_resize;
	m_fixed_size.x = w;
	m_fixed_size.y = h;
}

POINT TEventWindow::fixed_size()
{
	return m_fixed_size;
}

//*1 new cursor types
void TEventWindow::cursor(CursorType curs)
{
	HCURSOR new_cursor = 0;
	if (curs == CursorType::RESTORE) new_cursor = m_old_cursor;
	else {
		m_old_cursor = GetCursor();
		switch (curs) {
		case CursorType::ARROW: new_cursor = LoadCursor(NULL, IDC_ARROW); break;
		case CursorType::HOURGLASS: new_cursor = LoadCursor(NULL, IDC_WAIT); break;
		case CursorType::SIZE_VERT: new_cursor = LoadCursor(NULL, IDC_SIZENS); break;
		case CursorType::SIZE_HORZ: new_cursor = LoadCursor(NULL, IDC_SIZEWE); break;
		case CursorType::CROSS: new_cursor = LoadCursor(NULL, IDC_CROSS); break;
		//case CursorType::HAND: new_cursor = LoadCursor(NULL,IDC_HAND); break;
		case CursorType::UPARROW: new_cursor = LoadCursor(NULL, IDC_UPARROW); break;
		}
	}
	SetCursor(new_cursor);
}

//*6 The toolbar goes into the window list; it isn't autosized
// because it has no alignment.
//+ We add a separate notify handler for the tooltip control
void TEventWindow::set_toolbar(TWin * tb, TNotifyWin * tth)
{
	m_tool_bar = tb;
	add(tth);
}

bool TEventWindow::check_notify(LPARAM lParam, int& ret)
{
	LPNMHDR ph = (LPNMHDR)lParam;
	for (TWin* w : *m_children) {
		if (ph->hwndFrom == w->handle())
			if (TNotifyWin* pnw = dynamic_cast<TNotifyWin*>(w)) {
				ret = pnw->handle_notify(ph);
				return ret;
			}
			//else if (TMemo* m = dynamic_cast<TMemo*>(w)) {
			//	ret = m->handle_notify(ph);
			//	return ret;
			//}
	}
	return false;
}

void TEventWindow::set_window()
//-----------------------------
{
	set(m_hwnd);
}

void TEventWindow::set_background(float r, float g, float b)
//-----------------------------------------
{
	COLORREF rgb = RGBF(r, g, b);
	m_bkgnd_brush = CreateSolidBrush(rgb);
	get_dc()->get(this);
	SetBkColor(get_dc()->get_hdc(), rgb);
	get_dc()->select(m_bkgnd_brush);
	m_bk_color = rgb;
	get_dc()->release(this);
	invalidate();
}

void TEventWindow::set_menu(pchar res)
//------------------------------------
{
	if (m_hmenu) DestroyMenu(m_hmenu);
	set_menu(LoadMenu(hInst, res));
}

void TEventWindow::set_menu(HANDLE menu)
//------------------------------------
{
	m_hmenu = menu;
	SetMenu(m_hwnd, menu);
}

void TEventWindow::set_popup_menu(HANDLE menu)
//--------------------------------------------
{
	m_hpopup_menu = menu;
}

void TEventWindow::last_mouse_pos(int& x, int& y)
//-----------------------------------------------
{
	POINT pt{ g_mouse_pt_right.x, g_mouse_pt_right.y };
	//pt.x = g_mouse_pt_right.x;
	//pt.y = g_mouse_pt_right.y;
	ScreenToClient(m_hwnd, &pt);
	x = pt.x;
	y = pt.y;
}

void TEventWindow::check_menu(int id, bool check)
{
	CheckMenuItem(m_hmenu, id, MF_BYCOMMAND | (check ? MF_CHECKED : MF_UNCHECKED));
}

void TEventWindow::show(int how)
//-----------------------------------
{
	// default:  use the 'nCmdShow' we were _originally_ passed
	if (how == 0) how = CmdShow;
	ShowWindow(m_hwnd, how);
}

enum { uIDtimer = 100 };

void TEventWindow::create_timer(int msec)
//---------------------------------------
{
	if (m_timer) kill_timer();
	m_timer = SetTimer(m_hwnd, uIDtimer, msec, NULL);
}

void TEventWindow::kill_timer()
//-----------------------------
{
	KillTimer(m_hwnd, uIDtimer);
}

constexpr int WM_QUIT_LOOP = 0x999;

/*
int _TranslateAccelerator(HWND h, HACCEL acc, MSG* msg)
{
	int ret = TranslateAccelerator(h,acc,msg);
	if (ret)
		ret = 1;
	return ret;
}
*/

// Message Loop!
// *NB* this loop shd cover ALL cases, controlled by global
// variables like mdi_sysaccell,  accell, hModelessDlg, etc.
int TEventWindow::run()
//---------------------
{
	BOOL bRet;
	MSG msg;
	while (bRet = GetMessage(&msg, NULL, 0, 0)) {
		if (bRet == -1) {
			// handle the error and possibly exit
		}
		else {
			if (msg.message == WM_QUIT_LOOP) return int(msg.wParam);
			if (!hAccel || !TranslateAccelerator(m_hwnd, hAccel, &msg)) {
				if (!hModeless || !IsDialogMessage(hModeless, &msg)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	return int(msg.wParam);
}

void TEventWindow::quit(int retcode)
{
	PostMessage(m_hwnd, WM_QUIT_LOOP, retcode, 0);
}

// Place holder functions - no functionality offered at this level!
TEventWindow::~TEventWindow()
{
	//log_add("~TEventWindow");
	if (m_timer) kill_timer();
	/*if (m_dc)*/ delete m_dc;
	int i = 0;
	for (TWin*& p : *m_children)
	{
		//log_add("del list item %d", ++i);
		delete p;
	}
	m_children->clear();
	delete m_children;
	DestroyWindow((HWND)m_hwnd);
}

void TEventWindow::set_icon(pchar file)
{
	HANDLE hIcon = LoadImage(hInst, file, IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SetClassLongPtr(m_hwnd, GCLP_HICON, (LPARAM)hIcon);
}

void TEventWindow::set_icon_from_window(TWin* win)
{
	HICON hIcon = (HICON)GetClassLongPtr((HWND)win->handle(), GCLP_HICON);
	SetClassLongPtr(m_hwnd, GCLP_HICON, (LPARAM)hIcon);
}

// *change 0.6.0 support for VCL-style alignment of windows
void TEventWindow::size()
{
	if (statusbar_id)
		SendDlgItemMessage(m_hwnd, statusbar_id, WM_SIZE, 0, 0);

	if (m_children->size() == 0) return;
	int n = (int)m_children->size();
	Rect m;
	get_client_rect(m);
	if (m_tool_bar) m.top += m_tool_bar->height();  //*6
	// we will only be resizing _visible_ windows with explicit alignments.
	for (const auto& win : *m_children) {
		if (win->align() == Alignment::alNone || !win->visible()) n--;
	}
	if (n == 0) return;
	HDWP hdwp = BeginDeferWindowPos(n);
	for (const auto& win : *m_children) {
		if (!win->visible()) continue; //*new
		Rect wrt;
		//win->get_client_rect(wrt);
		win->get_rect(wrt, true);
		int left = m.left, top = m.top, w = wrt.width(), h = wrt.height();
		int width_m = m.width(), height_m = m.height();
		switch (win->align()) {
		case Alignment::alTop:
			w = width_m;
			m.top += h;
			break;
		case Alignment::alBottom:
			m.bottom -= h;
			top = m.bottom;
			w = width_m;
			break;
		case Alignment::alLeft:
			h = height_m;
			m.left += w;
			break;
		case Alignment::alRight:
			m.right -= w;
			left = m.right;
			h = height_m;
			break;
		case Alignment::alClient:
			h = height_m;
			w = width_m;
			break;
		case Alignment::alNone: continue;  // don't try to resize anything w/ no alignment.
		} // switch(...)
		DeferWindowPos(hdwp, win->handle(), NULL, left, top, w, h, SWP_NOZORDER);
	} // for(...)
	EndDeferWindowPos(hdwp);
}

// *add 0.6.0 can add a control to the child list directly
void TEventWindow::add(TWin * win)
{
	if (m_client)
		m_children->push_front(win);
	else
		m_children->push_back(win);
	win->show();
}

void TEventWindow::remove(TWin * win)
{
	m_children->remove(win);
	win->hide();
	size();
}

// *change 0.6.0 set_client(), focus() has moved up from TFrameWindow
//*5 Note the special way that m_client is managed w.r.t the child list.
//void TEventWindow::set_client(TWin * cli, bool do_subclass)
void TEventWindow::set_client(TWin* cli)
{
	if (m_client) {
		//if (do_subclass) {
			// *NOTE* this is very dubious code, not all client windows are controls!
			//remove_subclass_control((TControl*)m_client);
		//}
		m_client->hide();
		m_client = NULL;
	}
	if (cli) {
		// client window also goes into the child list; there can _only_
		// be one such, and it must be at the end of the list.
		if (m_children->size() > 0 && m_children->back()->align() == Alignment::alClient)
			m_children->pop_back();
		add(cli);
		m_client = cli;
		m_client->align(Alignment::alClient);
		focus();
		//if (do_subclass) subclass_control((TControl*)cli);
	} // else m_children->pop_back();
}

void TEventWindow::focus()
{
	if (m_client) m_client->set_focus();
}

bool TEventWindow::command(int, int) { return true; }
bool TEventWindow::sys_command(int) { return false; }
void TEventWindow::paint(TDC&) {}
void TEventWindow::ncpaint(TDC&) {}
void TEventWindow::mouse_down(Point&) {}
void TEventWindow::mouse_up(Point&) { }
void TEventWindow::right_mouse_down(Point&) {}
void TEventWindow::mouse_move(Point&) { }
void TEventWindow::keydown(int) { }
void TEventWindow::destroy() { }
void TEventWindow::timer() { }
int  TEventWindow::notify(int id, void* ph) { return 0; }
int  TEventWindow::handle_user(WPARAM wparam, LPARAM lparam) { return 0; }
void TEventWindow::scroll(int code, int posn) { };

/// Windows controls - TControl ////////////
TControl::TControl(TWin* parent, pchar classname, pchar text, int id, DWORD style)
	:TWin(parent, classname, text, id, style), m_wnd_proc(NULL), m_data(NULL)
{
	m_font = NULL;
	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LPARAM)this);
	m_parent = (TEventWindow*)parent;
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

TControl* TControl::user_data(Handle handle)
{
	return (TControl*)GetWindowLongPtr(handle, GWLP_USERDATA);
}

void TControl::calc_size()
{
	int cx, cy;
	std::wstring tmp;
	get_text(tmp);
	m_parent->get_dc()->get_text_extent(tmp.c_str(), cx, cy, m_font);
	resize(int(1.05 * cx), int(1.05 * cy));
}

bool TControl::is_type(pchar tname) const
{
	return wcscmp(type_name(), tname) == 0;
}

void TControl::set_font(TFont * fnt)
{
	m_font = fnt;
	calc_size();
	if (m_font)
		send_msg(WM_SETFONT, (WPARAM)m_font->handle(), (LPARAM)TRUE);
}

TLabel::TLabel(TWin * parent, pchar text, DWORD style)
//--------------------------------------
	: TControl(parent, L"STATIC", text, -1, style)
{ }

void TLabel::set_icon(pchar file, int icon_idx)
{
	HICON icon_handle = (HICON)load_icon(file, icon_idx);
	if (!icon_handle)
		return log_add("TLabel::set_icon >> can't load icon");
	send_msg(STM_SETIMAGE, IMAGE_ICON, (LPARAM)icon_handle);
}

void TLabel::set_bitmap(pchar file)
{
	HBITMAP image_handle = (HBITMAP)load_bitmap(file);
	if (!image_handle)
		return log_add("TLabel::set_bitmap >> can't load image");
	send_msg(STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image_handle);
}

TEdit::TEdit(TWin * parent, pchar text, int id, DWORD style)
//-----------------------------------------------
	: TControl(parent, L"EDIT", text, id, style | WS_BORDER | ES_AUTOHSCROLL)
{ }

void TEdit::set_selection(int start, int finish)
{
	send_msg(EM_SETSEL, start, finish);
}

//////////////////////////////
// ProgressControl
TProgressControl::TProgressControl(TWin* parent, int id, bool vertical, bool hasborder, bool smooth, bool smoothrevers)
	: TControl(parent, PROGRESS_CLASS, NULL, id,
		WS_TABSTOP |
		(vertical ? PBS_VERTICAL : 0) |
		(hasborder ? WS_BORDER : 0) |
		(smooth ? PBS_SMOOTH : 0) |
		(smoothrevers ? PBS_SMOOTHREVERSE : 0)
	)
{}

void TProgressControl::set_range(int from, int to)
{
	send_msg(PBM_SETRANGE32, to, from);
}

void TProgressControl::set_step(int step)
{
	send_msg(PBM_SETSTEP, step, 0);
}

void TProgressControl::go()
{
	send_msg(PBM_STEPIT, 0, 0);
}

int TProgressControl::get_range(int& hi)
{
	PBRANGE rng;
	send_msg(PBM_GETRANGE, 1, (LPARAM)&rng);
	hi = rng.iHigh;
	return rng.iLow;
}

void TProgressControl::set_pos(int to)
{
	send_msg(PBM_SETPOS, to, 0);
}

//---------------------------------------------
TComboBox::TComboBox(TWin * parent, int id, DWORD style)
	: TControl(parent, WC_COMBOBOX, L"", id, style | WS_VSCROLL | WS_TABSTOP)
{ }

//#include "windowsx.h"
void TComboBox::reset()
{
	send_msg(CB_RESETCONTENT);
}

int TComboBox::add_string(pchar str)
{
	return send_msg(CB_ADDSTRING, 0, (LPARAM)str);
}

void TComboBox::ins_string(int id, pchar str)
{
	send_msg(CB_INSERTSTRING, id, (LPARAM)str);
}

void TComboBox::set_data(int idx, int data)
{
	send_msg(CB_SETITEMDATA, idx, (LPARAM)data);
}

int TComboBox::get_data(int idx)
{
	return send_msg(CB_GETITEMDATA, idx);
}

void TComboBox::set_cursel(int id)
{
	send_msg(CB_SETCURSEL, id);
}

int TComboBox::get_cursel()
{
	return send_msg(CB_GETCURSEL);
}

void TComboBox::set_height(int sz)
{
	send_msg(CB_SETITEMHEIGHT, 0, (LPARAM)sz);
}

int TComboBox::count()
{
	return send_msg(CB_GETCOUNT);
}

WNDFN WndProc(Handle hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


//----------------Window Registration----------------------

static bool been_registered = false;

void RegisterEventWindow(HANDLE hIcon = 0, HANDLE hCurs = 0)
//------------------------------------------------------
{
	WNDCLASS    wndclass{};

	wndclass.style =
		CS_HREDRAW | //перерисовывать окно при изменении вертикальных размеров
		CS_VREDRAW | //перерисовывать всё окно при изменении ширины 
		CS_OWNDC | //у каждого окна уникальный контекст устройства
		DS_LOCALEDIT;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(void*);
	wndclass.hInstance = hInst;
	wndclass.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = hCurs ? hCurs : NULL;
	wndclass.hbrBackground = NULL; //GetStockObject(LTGRAY_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = EW_CLASSNAME;

	RegisterClass(&wndclass);

	been_registered = true;
}

void UnregisterEventWindow()
{
	if (been_registered) {
		UnregisterClass(EW_CLASSNAME, hInst);
		been_registered = false;
	}
}

static HMODULE m_hRichEditDll = NULL;
void on_destroy();
void on_attach();
extern "C"  // inhibit C++ name mangling
BOOL APIENTRY DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved   // reserved
)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		m_hRichEditDll = LoadLibrary(L"riched32.dll");
		hInst = hinstDLL;
		RegisterEventWindow();
		on_attach();
		CmdShow = SW_SHOW;
		break;
	case DLL_PROCESS_DETACH:
		UnregisterEventWindow();  // though it is important only on NT platform...
		FreeLibrary(m_hRichEditDll);
		on_destroy();
		break;
	};
	return TRUE;
}

//--------------Default Window Proc for EventWindow-----------------

WNDFN WndProc(Handle hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//----------------------------------------------------------------------------
{
	TEventWindow* This = (TEventWindow*)GetWindowLongPtr(hwnd, 0);

	switch (msg)
	{
	case WM_CREATE:
		{
			LPCREATESTRUCT lpCreat = reinterpret_cast<LPCREATESTRUCT>(lParam);
			PVOID* lpUser = reinterpret_cast<PVOID*>(lpCreat->lpCreateParams);

			//..... 'This' pointer passed as first part of creation parms
			This = reinterpret_cast<TEventWindow*>(lpUser[0]);
			SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(This));
			This->get_dc()->set_twin(This);
		}
		return 0;

	case WM_SIZE:

		This->size();
		return 0;

	case WM_GETMINMAXINFO:
		if (This && This->cant_resize()) {
			LPMINMAXINFO pSizeInfo = (LPMINMAXINFO)lParam;
			//pSizeInfo->ptMaxTrackSize = This->fixed_size();
			//pSizeInfo->ptMinTrackSize = This->fixed_size();
			pSizeInfo->ptMaxTrackSize = pSizeInfo->ptMinTrackSize = This->fixed_size();
		}
		return 0;

	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) != WM_RBUTTONDOWN) break;
		g_mouse_pt_right.set(LOWORD(lParam), HIWORD(lParam));
		ClientToScreen(hwnd, (POINT*)&g_mouse_pt_right);
		lParam = 0;
		// pass through.....
		[[fallthrough]];
	case WM_CONTEXTMENU:
	{
		if (This->m_hpopup_menu == NULL) break;
		if (lParam != 0) {
			g_mouse_pt_right.set(LOWORD(lParam), HIWORD(lParam));
		}
		HWND wnd = WindowFromPoint(g_mouse_pt_right);
		if (wnd != This->handle()) break;
		TrackPopupMenu(This->m_hpopup_menu, TPM_LEFTALIGN | TPM_TOPALIGN,
			g_mouse_pt_right.x, g_mouse_pt_right.y, 0, hwnd, NULL);
		return 0;
	}
	case WM_NOTIFY:
	{
		int ret, id = (int)wParam;
		if (!This->check_notify(lParam, ret))
			return This->notify(id, (void*)lParam);
		else return ret;
	}

	case WM_COMMAND:
		if (This->m_dispatcher) {
			if (This->m_dispatcher->dispatch(LOWORD(wParam), HIWORD(wParam), (Handle)lParam))
				return 0;
		}
		if (This->command(LOWORD(wParam), HIWORD(wParam))) return 0;
		else break;

	case WM_USER_PLUS:
		return This->handle_user(wParam, lParam);

	case WM_KEYDOWN:
		This->keydown(LOWORD(wParam));
		return 0;

	case WM_HSCROLL:
	case WM_VSCROLL:
		if (This->m_dispatcher) {
			int id = (int)GetWindowLongPtr((HWND)lParam, GWL_ID);
			This->m_dispatcher->dispatch(id, LOWORD(wParam), (Handle)lParam);
		}
		if (lParam)
		{
			int id = (int)GetWindowLongPtr((HWND)lParam, GWL_ID);
			switch (LOWORD(wParam))
			{
			//case SB_THUMBTRACK: // change position
			case SB_THUMBPOSITION: // stop changing
				This->scroll(id, HIWORD(wParam));
				break;
			case SB_ENDSCROLL: // end of scroll or click on scrollbar, wParam is 0
			{
				int pos = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				This->scroll(id, pos);
			}
			}
		}
		return 0;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		TDC& dc = *This->m_dc;
		dc.set_hdc(BeginPaint(hwnd, &ps));
		This->paint(dc);
		dc.set_hdc(NULL);
		EndPaint(hwnd, &ps);
	}
	return 0;

	// Mouse messages....
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	{
		g_mouse_pt.set(LOWORD(lParam), HIWORD(lParam));
		//pt.to_logical(*This);
		switch (msg) {
		case WM_LBUTTONDOWN:
			This->mouse_down(g_mouse_pt);
			break;
		case WM_LBUTTONUP:
			This->mouse_up(g_mouse_pt);
			break;
		case WM_RBUTTONDOWN:
			This->right_mouse_down(g_mouse_pt);
			break;
		}
	}
	return 0;

	case WM_MOUSEMOVE:  // needs different treatment??
	{
		g_mouse_pt.set(LOWORD(lParam), HIWORD(lParam));
		This->mouse_move(g_mouse_pt);
	}
	return 0;

	case WM_ERASEBKGND:
	{
		RECT rt;
		GetClientRect(hwnd, &rt);
		FillRect((HDC)wParam, (LPRECT)&rt, This->m_bkgnd_brush);
	}
	return 0;

	// suspect this causes trouble
 // case WM_CTLCOLORSTATIC:
	//{
	// TControl *ctl = (TControl *)GetWindowLongPtr((HWND)lParam,GWLP_USERDATA);
	// SetBkColor((HDC)wParam, This->m_bk_color);
	// SetTextColor((HDC)wParam, ctl->get_colour());
	//}
	//return (LRESULT)This->m_bkgnd_brush;

	case WM_SETCURSOR:
		if (This) This->cursor(CursorType::ARROW);
		//return 0;
		break;

	case WM_SETFOCUS:
		if (This) This->focus();
		return 0;

	case WM_ACTIVATE:
		if (This && !This->activate(wParam != WA_INACTIVE)) return 0;
		break;

	case WM_SYSCOMMAND:
		if (This->sys_command(LOWORD(wParam))) return 0; //?
		else break;

	case WM_TIMER:
		This->timer();
		return 0;

	case WM_CLOSE:
		This->on_close();
		if (!This->query_close()) return 0;
		break; // let DefWindowProc handle this...

	case WM_SHOWWINDOW:
		This->on_showhide(!IsWindowVisible(hwnd));
		break; // let DefWindowProc handle this...

	case WM_DESTROY:
		This->destroy();
		//if (This->m_hmenu) DestroyMenu(This->m_hmenu);  // but why here?
		return 0;

	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
