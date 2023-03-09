/* TWL_CNTRLS.CPP
 * Steve Donovan, 2003
 * This is GPL'd software, and the usual disclaimers apply.
 * See LICENCE
 */

 //#define NO_STRICT
#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include "twl_cntrls.h"
//void *ApplicationInstance();

//////////////////////////////////
// TButton
TButtonBase::TButtonBase(TWin* parent, pchar caption, int id, ButtonStyle style)
	: TControl(parent, L"button", caption, id, (DWORD)style)
{
	//calc_size();
}

void TButtonBase::check(int state)
{
	switch (state)
	{
	case BST_CHECKED:
		send_msg(BM_SETCHECK, BST_CHECKED);
		break;
	case BST_INDETERMINATE:
		send_msg(BM_SETCHECK, BST_INDETERMINATE);
		break;
	//case BST_UNCHECKED:
	default:
		send_msg(BM_SETCHECK, BST_UNCHECKED);
	}
}

int TButtonBase::check() const
{
	return send_msg(BM_GETCHECK);
}

/////////////////////////////////////
/// Button
TButton::TButton(TWin* parent, pchar caption, int id, ButtonStyle style)
	:TButtonBase(parent, caption, id, style) {};

void TButton::calc_size()
{
	int cx, cy;
	gui_string tmp;
	get_text(tmp);
	m_parent->get_dc()->get_text_extent(tmp.c_str(), cx, cy, m_font);
	resize(cx + 10, cy + 10);
}

void TButton::set_icon(pchar mod, int icon_id)
{
	DWORD style = GetWindowLongPtr((HWND)handle(), GWL_STYLE);
	SetWindowLongPtr((HWND)handle(), GWL_STYLE, style | (DWORD)ButtonStyle::ICON);
	HICON hIcon = (HICON)load_icon(mod, icon_id, true);
	if (!hIcon) return log_add("set_icon >> can't load image");
	send_msg(BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
}

void TButton::set_bitmap(pchar file)
{
	DWORD style = GetWindowLongPtr((HWND)handle(), GWL_STYLE);
	SetWindowLongPtr((HWND)handle(), GWL_STYLE, style | (DWORD)ButtonStyle::ICON);
	HBITMAP image_handle = (HBITMAP)load_bitmap(file);
	if (!image_handle)
		return log_add("set_bitmap >> can't load image");
	send_msg(BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)image_handle);
}

//////////////////////////////
//  TCheckBox
TCheckBox::TCheckBox(TWin* parent, pchar caption, int id, bool is3state)
	: TButtonBase(parent, caption, id, is3state ? ButtonStyle::AUTO3STATE : ButtonStyle::AUTOCHECKBOX)
{
	if (caption && *caption)
		calc_size();
	else
		resize(20, 20);
}

void TCheckBox::calc_size()
{
	int cx, cy;
	TDC* dc = m_parent->get_dc();
	// If the parent was a TComboBox, then it won't have a DC ready...
	if (dc) {
		gui_string tmp;
		get_text(tmp);
		dc->get_text_extent(tmp.c_str(), cx, cy, m_font);
		resize(int(1.05 * cx) + 30, int(1.05 * cy));
	}
}

//////////////////////////////
//  TGroupBox
TGroupBox::TGroupBox(TWin* parent, pchar caption)
	:TControl(parent, L"button", caption, -1, (DWORD)TButtonBase::ButtonStyle::GROUPBOX)
{}

//////////////////////////////
//  TRadioButton
TRadioButton::TRadioButton(TWin* parent, pchar caption, int id, bool is_auto)
	:TButtonBase(parent, caption, id, is_auto ? ButtonStyle::AUTORADIOBUTTON : ButtonStyle::RADIOBUTTON)
{
	calc_size();
}

void TRadioButton::calc_size()
{
	int cx, cy;
	TDC* dc = m_parent->get_dc();
	// If the parent was a TComboBox, then it won't have a DC ready...
	if (dc) {
		gui_string tmp;
		get_text(tmp);
		dc->get_text_extent(tmp.c_str(), cx, cy, m_font);
		resize(int(1.05 * cx) + 30, int(1.05 * cy));
	}
}

//////////////////////////////
//  TListBox
TListBox::TListBox(TWin* parent, int id, bool is_sorted)
	: TControl(parent, L"listbox", L"", id,
		LBS_NOTIFY | WS_VSCROLL | WS_BORDER | (is_sorted ? LBS_SORT : 0))
{}

void TListBox::add(pchar str, int data)
{
	send_msg(LB_ADDSTRING, 0, (LPARAM)data);
	if (data) set_data(count() - 1, data);
}

void TListBox::set_data(int i, int data)
{
	send_msg(LB_SETITEMDATA, (WPARAM)i, (LPARAM)data);
}

int TListBox::get_data(int i)
{
	return send_msg(LB_GETITEMDATA, (WPARAM)i);
}

void TListBox::insert(int i, pchar str)
{
	send_msg(LB_INSERTSTRING, (WPARAM)i, (LPARAM)str);
}

void TListBox::remove(int i)
{
	send_msg(LB_DELETESTRING, (WPARAM)i);
}

void TListBox::clear()
{
	send_msg(LB_RESETCONTENT);
}

void TListBox::redraw(bool on)
{
	send_msg(WM_SETREDRAW, (WPARAM)(on ? TRUE : FALSE));
}

int  TListBox::count()
{
	return (int)send_msg(LB_GETCOUNT);
}

void TListBox::selected(int idx)
{
	send_msg(LB_SETCURSEL, (WPARAM)idx);
}

int  TListBox::selected() const
{
	return (int)send_msg(LB_GETCURSEL);
}

void TListBox::get_text(int idx, wchar_t* buff)
{
	send_msg(LB_GETTEXT, idx, (LPARAM)buff);
}

size_t TListBox::get_textlen(int idx)
{
	return send_msg(LB_GETTEXTLEN, idx, 0);
}

//////////////////////////////////////////
// TTrackBar
TTrackBar::TTrackBar(TWin* parent, DWORD style, int id)
	: TControl(parent, TRACKBAR_CLASS, L"", id, TBS_AUTOTICKS | TBS_TOOLTIPS | style), m_redraw(true)
{}

void TTrackBar::selection(int lMin, int lMax)
{
	send_msg(TBM_SETSEL, m_redraw, MAKELONG(lMin, lMax));
}

void TTrackBar::sel_start(int lStart)
{
	send_msg(TBM_SETSELSTART, m_redraw, lStart);
}

int TTrackBar::sel_start() // returns starting pos of current selection
{
	return (int)send_msg(TBM_GETSELSTART);
}

int TTrackBar::sel_end() // returns end pos
{
	return (int)send_msg(TBM_GETSELEND);
}

void TTrackBar::sel_clear()
{
	send_msg(TBM_CLEARSEL, m_redraw, 0);
}

int TTrackBar::pos()
{
	return (int)send_msg(TBM_GETPOS);
}

void TTrackBar::pos(int lPos)
{
	send_msg(TBM_SETPOS, TRUE, lPos);
}

void TTrackBar::range(int lMin, int lMax)
{
	send_msg(TBM_SETRANGE, m_redraw, MAKELONG(lMin, lMax));
}

// Subclassing the controls!!
//const int MIN_ID = 300;
//LRESULT FormWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
//
//void subclass_control(TControl* ctrl)
//{
//	ctrl->m_wnd_proc = SetWindowLongPtr((HWND)ctrl->handle(), GWLP_WNDPROC, (LONG_PTR)FormWndProc);
//}
//
//void remove_subclass_control(TControl* ctrl)
//{
//	if (ctrl->m_wnd_proc)
//		SetWindowLongPtr((HWND)ctrl->handle(), GWLP_WNDPROC, ctrl->m_wnd_proc);
//}

/*
LRESULT ProcessCustomDraw(LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	switch (lplvcd->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT: //Before the paint cycle begins
		//request notifications for individual listview items
		return CDRF_NOTIFYITEMDRAW;

	case CDDS_ITEMPREPAINT: //Before an item is drawn
		if (((int)lplvcd->nmcd.dwItemSpec % 2) == 0)
		{
			//customize item appearance
			lplvcd->clrText = RGB(255, 0, 0);
			lplvcd->clrTextBk = RGB(200, 200, 200);
			return CDRF_NEWFONT;
		}
		else {
			lplvcd->clrText = RGB(0, 0, 255);
			lplvcd->clrTextBk = RGB(255, 255, 255);

			return CDRF_NEWFONT;
		}
		break;

		//Before a subitem is drawn
	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		if (iSelect == (int)lplvcd->nmcd.dwItemSpec)
		{
			if (0 == lplvcd->iSubItem)
			{
				//customize subitem appearance for column 0
				lplvcd->clrText = RGB(255, 0, 0);
				lplvcd->clrTextBk = RGB(255, 255, 255);

				//To set a custom font:
				//SelectObject(lplvcd->nmcd.hdc,
				//    <your custom HFONT>);

				return CDRF_NEWFONT;
			}
			else if (1 == lplvcd->iSubItem)
			{
				//customize subitem appearance for columns 1..n
				//Note: setting for column i
				//carries over to columnn i+1 unless
				//      it is explicitly reset
				lplvcd->clrTextBk = RGB(255, 0, 0);
				lplvcd->clrTextBk = RGB(255, 255, 255);

				return CDRF_NEWFONT;
			}
		}
	}
	return CDRF_DODEFAULT;
}*/

//LRESULT FormWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	TControl* ctrl = TControl::user_data(hwnd);
//	HWND hNext, hDlg = GetParent(hwnd);
//	TEventWindow* ew = reinterpret_cast<TEventWindow*>(GetWindowLongPtr(hDlg, 0));
//	switch (msg) {
//	case WM_KEYDOWN:
//		if (wParam == VK_TAB) {
//			bool shift_down = GetKeyState(VK_SHIFT) < 0;
//			int id = (int)GetWindowLongPtr(hwnd, GWL_ID);
//			if (shift_down) {
//				if (id > MIN_ID)id--;
//				hNext = GetDlgItem(hDlg, id);
//			}
//			else {
//				id++;
//				hNext = GetDlgItem(hDlg, id);
//				if (!hNext) hNext = GetDlgItem(hDlg, MIN_ID);
//			}
//			SetFocus(hNext);
//		}
//		else if (wParam == VK_RETURN) {
//			PostMessage(hDlg, WM_COMMAND, IDOK, 0);
//			return 0;
//		}
//		else if (wParam == VK_ESCAPE) {
//			PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
//			return 0;
//		}
//		break;
//	case WM_SETFOCUS:
//		if (ctrl->is_type(L"TEdit"))
//			ctrl->send_msg(EM_SETSEL, 0, -1);
//		break;
//	}
//	auto ret = CallWindowProc(WNDPROC(ctrl->m_wnd_proc), hwnd, msg, wParam, lParam);
//	if (ew && ew->child_messages()) {
//		if (msg == WM_KEYDOWN || msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK)
//			ew->send_msg(msg, wParam, lParam);
//	}
//	return ret;
//}
