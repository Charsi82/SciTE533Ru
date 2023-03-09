// twl_toolbar.cpp
// implements the following common controls
//   TImageList
//   TTabControl
//   TListView
//   TTreeView
#include "twl.h"
#include <commctrl.h>
#define IS_IMPLEMENTATION
#include "twl_menu.h"
#include "twl_imagelist.h"
#include "twl_listview.h"
#include "twl_tab.h"
#include "twl_treeview.h"
#include "twl_cntrls.h"
#include <stdio.h>
#include <io.h>
#include "utf.h"
#include <Uxtheme.h>

static size_t gID = 445560;

static HWND create_common_control(TWin* form, pchar winclass, DWORD style, int height = -1)
{
	int w = CW_USEDEFAULT, h = CW_USEDEFAULT;
	if (height != -1) { w = 100; h = height; }
	return CreateWindowEx(0L,   // No extended styles.
		winclass, L"", WS_CHILD | style,
		0, 0, w, h,
		(HWND)form->handle(),                  // Parent window of the control.
		(HMENU)(void*)gID++,
		hInst,             // Current instance.
		NULL);
}

///// TImageList class
static int icon_size(bool s)
{
	return s ? 16 : 32;
}

TImageList::TImageList(int cx, int cy)
{
	create(cx, cy);
	m_small_icons = cx == icon_size(true);
}

TImageList::TImageList(bool s /*= true*/)
	: m_small_icons(s)
{
	int cx = icon_size(s);
	int cy = cx;
	create(cx, cy);
}

Handle load_icon(pchar file, int idx, bool small_icon)
{
	HICON hIcon;
	if (small_icon)
		ExtractIconEx(file, idx, NULL, &hIcon, 1);
	else
		ExtractIconEx(file, idx, &hIcon, NULL, 1);
	return (Handle)hIcon;
}

Handle load_bitmap(pchar file)
{
	Handle res = LoadImage(NULL, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
	if (!res) {
		log_add("'load_bitmap':file not exist");
	}
	return res;
}

void TImageList::create(int cx, int cy)
{
	m_handle = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 0, 32);
}

int TImageList::add_icon(pchar iconfile)
{
	HICON hIcon = (HICON)load_icon(iconfile);
	if (!hIcon) return -1;  // can't find icon
	return ImageList_AddIcon(m_handle, hIcon);
}

int TImageList::add(pchar bitmapfile, COLORREF mask_clr)
{
	HBITMAP hBitmap = (HBITMAP)load_bitmap(bitmapfile);
	if (!hBitmap) return -1;  // can't find bitmap
	if (mask_clr != 1)
		return ImageList_AddMasked(m_handle, hBitmap, mask_clr);
	else
		return ImageList_Add(m_handle, hBitmap, NULL);
}

int TImageList::load_icons_from_module(pchar mod, bool icon_small)
{
	int res = ExtractIconEx(mod, -1, 0, 0, 1);
	if (!res) return 0;
	int icon_cnt = 0;
	while (HICON hIcon = (HICON)load_icon(mod, icon_cnt++, icon_small))
		ImageList_AddIcon(m_handle, hIcon);
	return icon_cnt - 1;
}

/*int TImageList::load_icons_from_module(pchar mod)
{
	HMODULE lib = LoadLibrary(mod);
	HINSTANCE hInst = GetModuleHandle(mod);
	HICON hIcon;
	int cx = icon_size(m_small_icons);
	int cy = cx;
	int i = 0;
	//while (hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(++i), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE))
	if (hIcon = (HICON)LoadImage(hInst, L"0", IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE))
		ImageList_AddIcon(m_handle, hIcon);
	while (hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(++i), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE))
		ImageList_AddIcon(m_handle, hIcon);
	if (lib) FreeLibrary(lib);
	return i-1;
}*/

void TImageList::set_back_colour(COLORREF clrRef)
{
	ImageList_SetBkColor(m_handle, clrRef);
}

void TImageList::load_shell_icons()
{
	load_icons_from_module(L"%windir%\\shell32.dll");
	set_back_colour(CLR_NONE);
}

////// TListView

TListViewB::TListViewB(TEventWindow* form, bool large_icons, bool multiple_columns, bool single_select):TNotifyWin(form)
{
	DWORD style = WS_CHILD | LVS_SHOWSELALWAYS;
	if (large_icons) {
		style |= (LVS_ICON | LVS_AUTOARRANGE);
	}
	else {
		style |= LVS_REPORT;
		if (single_select) {
			style |= LVS_SINGLESEL;
		}
		if (!multiple_columns) {
			style |= LVS_NOCOLUMNHEADER;
			//add_column("*",1000);
		}
	}

	// Create the list view control.
	set(create_common_control(form, WC_LISTVIEW, style));
	m_custom_paint = false;
	m_last_col = 0;
	m_last_row = -1;
	m_bg = 0;
	m_fg = 0;
	send_msg(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT); // Set style
}

int TListViewB::load_icons(pchar path, bool small_size) {

	int icons_loaded = new_image_list(path, small_size);
	send_msg(LVM_SETIMAGELIST, small_size ? LVSIL_SMALL : LVSIL_NORMAL, (LPARAM)get_image_list());
	return icons_loaded;
}

//int TListViewB::load_icons(pchar path, bool small_size) {
//	TImageList il(small_size);
//	int icons_loaded = il.load_icons_from_module(path, small_size);
//	if (small_size) set_image_list(&il);
//	else set_image_list(NULL, &il);
//	return icons_loaded;
//}

//void TListViewB::set_image_list(TImageList* il_small, TImageList* il_large)
//{
//	if (il_small) send_msg(LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)il_small->handle());
//	if (il_large) send_msg(LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)il_large->handle());
//	m_has_images = true;
//}

void TListViewB::add_column(pchar label, int width)
{
	LVCOLUMN lvc{};
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;   // left-align, by default
	lvc.cx = width;
	lvc.pszText = (wchar_t*)label;
	lvc.iSubItem = m_last_col;

	ListView_InsertColumn((HWND)m_hwnd, m_last_col, &lvc);
	m_last_col++;
}

void TListViewB::set_foreground(COLORREF colour)
{
	send_msg(LVM_SETTEXTCOLOR, 0, (LPARAM)colour);
	m_fg = colour;
}

void TListViewB::set_background(COLORREF colour)
{
	send_msg(LVM_SETBKCOLOR, 0, (LPARAM)colour);
	m_bg = colour;
	m_custom_paint = true;
}

void TListViewB::set_theme(bool explorer)
{
	SetWindowTheme((HWND)handle(), explorer ? L"Explorer" : L"Normal", NULL);
}

unsigned int TListViewB::columns() const
{
	return m_last_col;
}

void TListViewB::autosize_column(int col, bool by_contents)
{
	ListView_SetColumnWidth((HWND)m_hwnd, col, by_contents ? LVSCW_AUTOSIZE : LVSCW_AUTOSIZE_USEHEADER);
}

void TListViewB::start_items()
{
	m_last_row = -1;
}

int TListViewB::add_item_at(int i, pchar text, int idx, int data)
{
	LVITEM lvi{};
	lvi.mask = LVIF_TEXT | LVIF_STATE;
	if (has_image())
	{
		lvi.mask |= LVIF_IMAGE;
		lvi.iImage = idx;                // image list index
	}
	lvi.state = 0;
	lvi.stateMask = 0;
	lvi.pszText = (wchar_t*)text;
	if (data)
	{
		lvi.mask |= LVIF_PARAM;
		lvi.lParam = (LPARAM)data;
	}
	lvi.iItem = i;
	lvi.iSubItem = 0;

	ListView_InsertItem((HWND)m_hwnd, &lvi);
	return i;
}

int TListViewB::add_item(pchar text, int idx, int data)
{
	m_last_row++;
	return add_item_at(m_last_row, text, idx, data);
}

void TListViewB::add_subitem(int i, pchar text, int idx)
{
	ListView_SetItemText((HWND)m_hwnd, i, idx, (wchar_t*)text);
}

void TListViewB::remove_item(int i)
{
	ListView_DeleteItem((HWND)m_hwnd, i);
}

void TListViewB::select_item(int i)
{
	if (i != -1)
	{
		ListView_SetItemState((HWND)m_hwnd, i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible((HWND)m_hwnd, i, true);
	}
	else
		ListView_SetItemState((HWND)m_hwnd, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
}

void TListViewB::get_item_text(int i, wchar_t* buff, int buffsize)
{
	ListView_GetItemText((HWND)m_hwnd, i, 0, buff, buffsize);
}

int TListViewB::get_item_data(int i)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.iSubItem = 0;
	ListView_GetItem((HWND)m_hwnd, &lvi);
	return lvi.lParam;
}

int TListViewB::selected_id()
{
	return (int)send_msg(LVM_GETNEXTITEM, (WPARAM)(-1), LVNI_FOCUSED);
}

int TListViewB::next_selected_id(int i)
{
	return (int)send_msg(LVM_GETNEXTITEM, i, LVNI_SELECTED);
}

int TListViewB::count()
{
	return (int)send_msg(LVM_GETITEMCOUNT);
}

int TListViewB::selected_count()
{
	return (int)send_msg(LVM_GETSELECTEDCOUNT);
}

void TListViewB::clear()
{
	send_msg(LVM_DELETEALLITEMS);
	m_last_row = -1;
}

static int list_custom_draw(void* lParam, COLORREF fg, COLORREF bg)
{
	LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	if (lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
		// Request prepaint notifications for each item.
		return CDRF_NOTIFYITEMDRAW;

	if (lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
		lplvcd->clrText = fg;
		lplvcd->clrTextBk = bg;
		return CDRF_NEWFONT;
	}
	return 0;
}

int TListViewB::handle_notify(void* lparam)
{
	LPNMHDR np = (LPNMHDR)lparam;
	int id = selected_id();
	switch (np->code) {
	case LVN_ITEMCHANGED:
		handle_select(id);
		return 1;
	case NM_DBLCLK:
	{
		LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lparam;
		LVHITTESTINFO pInfo;
		pInfo.pt = lpnmitem->ptAction;
		ListView_SubItemHitTest((HWND)handle(), &pInfo);

		int i = pInfo.iItem;
		int j = pInfo.iSubItem;
		wchar_t buffer[10]{};
		LVITEM item;
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = i;
		item.iSubItem = j;
		item.cchTextMax = sizeof(buffer) / sizeof(buffer[0]);
		item.pszText = buffer;
		ListView_GetItem((HWND)handle(), &item);
		if (i > -1)
			handle_double_click(i, j, UTF8FromString(std::wstring(buffer)).c_str());
		return 1;
	}
	case LVN_KEYDOWN:
		handle_onkey(((LPNMLVKEYDOWN)lparam)->wVKey);
		return 0;  // ignored, anyway
	case NM_RCLICK:
		//send_msg(WM_CHAR,VK_ESCAPE,0);
		return handle_rclick();
	case NM_SETFOCUS:
		handle_onfocus(true);
		return 1;
	case NM_KILLFOCUS:
		handle_onfocus(false);
		return 1;
	case NM_CUSTOMDRAW:
		if (m_custom_paint) {
			return list_custom_draw(lparam, m_fg, m_bg);
		}
		return 0;
	}
	return 0;
}

TListView::TListView(TEventWindow* form, bool large_icons, bool multiple_columns, bool single_select)
	: TListViewB(form, large_icons, multiple_columns, single_select),
	m_form(form), m_on_select(NULL), m_on_key(NULL), m_on_double_click(NULL)
{
}

void TListView::handle_select(intptr_t i)
{
	if (m_on_select) {
		(m_form->*m_on_select)(i);
	}
}

void TListView::handle_double_click(int row, int col, const char* s)
{
	if (m_on_double_click) {
		(m_form->*m_on_double_click)(row, col, s);
	}
}

void TListView::handle_onkey(int i)
{
	if (m_on_key) {
		(m_form->*m_on_key)(i);
	}
}

TTabControl::TTabControl(TEventWindow* form, bool multiline) :TNotifyWin(form), m_index(0), m_last_selected_idx(0)
{
	// Create the tab control.
	DWORD style = WS_CHILD | TCS_TOOLTIPS;
	if (multiline) style |= TCS_MULTILINE;
	set(create_common_control(form, WC_TABCONTROL, style, 25));
	send_msg(WM_SETFONT, (WPARAM)::GetStockObject(DEFAULT_GUI_FONT), (LPARAM)TRUE);
}

TTabControl::~TTabControl()
{
	for (int idx = 0; idx < m_index; ++idx)
		if (idx != m_last_selected_idx)
			if (TWin* p = panels[idx]) delete p;
	panels.clear();
}

void TTabControl::add(wchar_t* caption, void* data, int image_idx /*= -1*/)
{
	TCITEM item{};
	item.mask = TCIF_TEXT | TCIF_PARAM;
	item.pszText = caption;
	//item.lParam = (LPARAM)data;
	if (has_image())
	{
		item.mask |= TCIF_IMAGE;
		item.iImage = image_idx;
	}
	panels.push_back((TWin*)data);
	send_msg(TCM_INSERTITEM, m_index++, (LPARAM)&item);
}

void* TTabControl::get_data(int idx)
{
	if (idx == -1) idx = selected();
	//TCITEM item{};
	//item.mask = TCIF_PARAM;
	//send_msg(TCM_GETITEM, idx, (LPARAM)&item);
	//return (void*)item.lParam;
	if (idx >= panels.size()) return nullptr;
	return panels[idx];
}

void TTabControl::remove(int idx /*= -1*/)
{
	if (idx == -1)
	{
		TabCtrl_DeleteAllItems((HWND)handle());
		m_last_selected_idx = m_index = 0;
		for (TWin*& p : panels)
			delete p;
		panels.clear();
	}
	else
	{
		TabCtrl_DeleteItem((HWND)handle(), idx);
		delete panels[idx];
		panels.erase(std::remove(panels.begin(), panels.end(), panels[idx]));
		if (m_last_selected_idx && (idx <= m_last_selected_idx)) m_last_selected_idx--;
	}
}

int TTabControl::getRowCount() const {
	return send_msg(TCM_GETROWCOUNT);
}

void TTabControl::selected(int idx)
{
	send_msg(TCM_SETCURSEL, idx);
	NMHDR nmh{};
	nmh.code = (UINT)(TCN_SELCHANGE);
	nmh.idFrom = GetDlgCtrlID((HWND)handle());
	nmh.hwndFrom = (HWND)handle();
	SendMessage(GetParent((HWND)handle()), WM_NOTIFY, nmh.idFrom, (LPARAM)&nmh);
}

int TTabControl::selected()
{
	return m_last_selected_idx = send_msg(TCM_GETCURSEL);
}

int TTabControl::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	int id = selected();
	switch (np->code) {
	case TCN_SELCHANGE:
		handle_select(id);
		return 1;
	case NM_RCLICK:
		return handle_rclick(id);
	case TTN_NEEDTEXT:
		log_add("tabneed text");
		LPNMTTDISPINFO ttn = reinterpret_cast<LPNMTTDISPINFO>(p);
		TCHAR buf[MAX_PATH]{};
		TCITEM item{};
		item.mask = TCIF_TEXT;
		item.pszText = buf;
		item.cchTextMax = MAX_PATH;
		send_msg(TCM_GETITEM, np->idFrom, (LPARAM)&item);
		wcscpy_s(buf, item.pszText);
		ttn->lpszText = buf;
		return 1;
	}
	return 0;
}

int TTabControl::load_icons(pchar path, bool small_size) {
	int icons_loaded = new_image_list(path, small_size);
	TabCtrl_SetImageList((HWND)handle(), get_image_list());
	return icons_loaded;
}

TTreeView::TTreeView(TEventWindow* form, DWORD tree_style) :TNotifyWin(form)
{
	//DWORD style = TVS_HASLINES | TVS_LINESATROOT;
	set(create_common_control(form, WC_TREEVIEW, tree_style));
	ins_mode = TVI_LAST;
}

void TTreeView::set_theme(bool explorer)
{
	SetWindowTheme((HWND)handle(), explorer ? L"Explorer" : L"Normal", NULL);
}

void TTreeView::expand(Handle itm)
{
	send_msg(TVM_EXPAND, TVE_EXPAND, (LPARAM)itm);
}

void TTreeView::collapse(Handle itm)
{
	send_msg(TVM_EXPAND, TVE_COLLAPSE, (LPARAM)itm);
}

void TTreeView::makeLabelEditable(bool toBeEnabled)
{
	DWORD dwNewStyle = (DWORD)GetWindowLongPtr((HWND)handle(), GWL_STYLE);
	if (toBeEnabled)
		dwNewStyle |= TVS_EDITLABELS;
	else
		dwNewStyle &= ~TVS_EDITLABELS;
	::SetWindowLongPtr((HWND)handle(), GWL_STYLE, dwNewStyle);
}

void TTreeView::set_image_list(bool normal)
{
	send_msg(TVM_SETIMAGELIST, normal ? TVSIL_NORMAL : TVSIL_STATE, (LPARAM)get_image_list());
}

int TTreeView::load_icons(pchar path, bool small_size) {
	int icons_loaded = new_image_list(path, small_size);
	set_image_list();
	return icons_loaded;
}

void TTreeView::set_foreground(COLORREF clr)
{
	send_msg(TVM_SETTEXTCOLOR, 0, clr);
}

void TTreeView::set_background(COLORREF clr)
{
	send_msg(TVM_SETBKCOLOR, 0, clr);
}

Handle TTreeView::get_root()
{
	return TreeView_GetRoot((HWND)handle()); // send_msg(TVM_get, 0, clr);
}

Handle TTreeView::get_next(Handle itm)
{
	return TreeView_GetNextSibling((HWND)handle(), itm);
}

Handle TTreeView::get_prev(Handle itm)
{
	return TreeView_GetPrevSibling((HWND)handle(), itm);
}

Handle TTreeView::get_child(Handle itm)
{
	return TreeView_GetChild((HWND)handle(), itm);
}

void TTreeView::clean_subitems(Handle itm)
{
	for (Handle hItem = get_child(itm); hItem != NULL; hItem = get_next(hItem))
	{
		TVITEM tvItem;
		tvItem.hItem = (HTREEITEM)hItem;
		tvItem.mask = TVIF_PARAM;
		//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		TreeView_GetItem((HWND)handle(), &tvItem);
		if (int data = tvItem.lParam)
			clean_data(data);
		clean_subitems(hItem);
	}
	TVITEM tvItem;
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	TreeView_GetItem((HWND)handle(), &tvItem);
	if (int data = tvItem.lParam)
		clean_data(data);
}

const std::vector<int> TTreeView::iterate_item(Handle itm)
{
	std::vector<int> res;

	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
	TreeView_GetItem((HWND)handle(), &tvItem);
	int data = tvItem.lParam;
	res.push_back(data);

	for (Handle hItem = get_child(itm); hItem != NULL; hItem = get_next(hItem))
	{
		//TVITEM tvItem;
		tvItem.hItem = (HTREEITEM)hItem;
		tvItem.mask = TVIF_PARAM;
		//SendMessage(_hSelf, TVM_GETITEM, 0, reinterpret_cast<LPARAM>(&tvItem));
		TreeView_GetItem((HWND)handle(), &tvItem);
		int data = tvItem.lParam;
		res.push_back(data);
	}
	return res;
}

void TTreeView::iterate_childs(Handle itm)
{
	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)itm;
	tvItem.mask = TVIF_PARAM;
	TreeView_GetItem((HWND)handle(), &tvItem);
	clean_data(tvItem.lParam);

	for (Handle tvProj = get_child(itm);
		tvProj != NULL;
		tvProj = get_next(tvProj))
	{
		//TVITEM item_p{};
		//item_p.mask = TVIF_PARAM;
		//item_p.hItem = (HTREEITEM)tvProj;
		//TreeView_GetItem((HWND)handle(), &item_p);
		//clean_data(item_p.lParam);

		iterate_childs(tvProj);
	}
}

void TTreeView::remove_item(Handle itm)
{
	iterate_childs(itm);
	TreeView_DeleteItem((HWND)handle(), itm);
}

void TTreeView::remove_childs(Handle itm)
{
	while (Handle tvProj = get_child(itm))
		TreeView_DeleteItem((HWND)handle(), tvProj);

	TVITEM item_p{};
	item_p.mask = TVIF_CHILDREN;
	item_p.hItem = (HTREEITEM)itm;
	item_p.cChildren = 0;
	TreeView_SetItem((HWND)handle(), &item_p);
}

void TTreeView::clear()
{
	iterate_childs(get_root());
	TreeView_DeleteAllItems((HWND)handle());
}

void TTreeView::insert_mode(Handle mode)
{
	ins_mode = mode;
}

void TTreeView::insert_mode(const char* mode)
{
	if (!strcmp(mode, "last"))
		ins_mode = TVI_LAST;
	else if (!strcmp(mode, "first"))
		ins_mode = TVI_FIRST;
	else if (!strcmp(mode, "sort"))
		ins_mode = TVI_SORT;
	else if (!strcmp(mode, "root"))
		ins_mode = TVI_ROOT;
	else
		ins_mode = TVI_LAST;
}

Handle TTreeView::add_item(pchar caption, Handle parent, int idx1, int idx2, int data)
{
	TVITEM item{};
	item.mask = TVIF_TEXT | TVIF_CHILDREN;
	if (has_image()) {
		item.mask |= (TVIF_IMAGE | TVIF_SELECTEDIMAGE);
		if (idx2 == -1) idx2 = idx1;
		item.iImage = idx1;
		item.iSelectedImage = idx2;
	}
	item.pszText = (wchar_t*)caption;
	item.cchTextMax = MAX_PATH;
	item.cChildren = 0;
	if (data)
	{
		item.mask |= TVIF_PARAM;
		item.lParam = data;
	}

	TVINSERTSTRUCT tvsi{};
	tvsi.item = item;
	tvsi.hInsertAfter = (HTREEITEM)ins_mode; //(HTREEITEM)parent;

	if (parent)
	{
		tvsi.hParent = (HTREEITEM)parent;

		// для родителького элемента указываем что он родительский 
		TVITEM item_p{};
		item_p.mask = TVIF_CHILDREN;
		item_p.hItem = (HTREEITEM)parent;
		item_p.cChildren = 1;
		TreeView_SetItem((HWND)handle(), &item_p);
	}
	else
	{
		tvsi.hParent = TVI_ROOT;
	}
	return TreeView_InsertItem((HWND)handle(), &tvsi);
}

int TTreeView::get_item_data(Handle pn)
{
	//if (pn == NULL) pn = selected();
	TVITEM item{};
	item.mask = TVIF_PARAM;
	item.hItem = (HTREEITEM)pn;
	item.lParam = 0;
	send_msg(TVM_GETITEM, 0, (LPARAM)&item);
	return (int)item.lParam;
}

void TTreeView::select(Handle p)
{
	send_msg(TVM_SELECTITEM, TVGN_CARET, (LPARAM)p);
}

Handle TTreeView::get_item_by_name(pchar caption, Handle parent_item)
{

	wchar_t wchLabel[MAX_PATH]{};
	TVITEM tvItem{};
	tvItem.hItem = (HTREEITEM)(parent_item ? get_child(parent_item) : get_root());
	tvItem.mask = TVIF_TEXT;
	tvItem.pszText = wchLabel;
	tvItem.cchTextMax = MAX_PATH;
	//int idx = 0;
	while (tvItem.hItem)
	{
		if (TreeView_GetItem((HWND)handle(), &tvItem))
		{
			//log_add("item[%d]", idx);
			//log_add( UTF8FromString(tvItem.pszText).c_str());
			if (!wcscmp(tvItem.pszText, caption))
				return tvItem.hItem;
		}
		tvItem.hItem = (HTREEITEM)get_next(tvItem.hItem);
	}
	return NULL;
}

Handle TTreeView::get_item_parent(Handle item)
{
	return TreeView_GetParent((HWND)handle(), item);
}

Handle TTreeView::get_selected()
{
	return TreeView_GetSelection((HWND)handle());
}

void TTreeView::set_item_text(void* itm, pchar str) {
	TVITEM tvi{};
	tvi.pszText = (LPWSTR)str;
	tvi.cchTextMax = MAX_PATH;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_SETITEM, 0, (LPARAM)&tvi);
}

gui_string TTreeView::get_item_text(Handle itm) {
	TCHAR buffer[MAX_PATH]{};
	TVITEM tvi{};
	tvi.pszText = buffer;
	tvi.cchTextMax = MAX_PATH;
	tvi.mask = TVIF_TEXT;
	tvi.hItem = (HTREEITEM)itm;
	send_msg(TVM_GETITEM, 0, (LPARAM)&tvi);
	return buffer;
}

int TTreeView::handle_notify(void* p)
{
	LPNMTREEVIEW np = reinterpret_cast<LPNMTREEVIEW>(p);
	switch (np->hdr.code) {
	case TVN_KEYDOWN:
	{
		LPNMTVKEYDOWN ptvkd = reinterpret_cast<LPNMTVKEYDOWN>(p);
		handle_onkey(ptvkd->wVKey);
		break;
	}
	case NM_RCLICK:
	{
		Point ptCursor;
		GetCursorPos(&ptCursor);
		ScreenToClient((HWND)handle(), &ptCursor);
		TVHITTESTINFO hitTestInfo{};
		hitTestInfo.pt.x = ptCursor.x;
		hitTestInfo.pt.y = ptCursor.y;
		HTREEITEM targetItem = reinterpret_cast<HTREEITEM>(send_msg(TVM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTestInfo)));
		if (targetItem)
		{
			TreeView_Select((HWND)handle(), targetItem, TVGN_CARET);
			handle_rclick(); // show popup if cursor over item
		}
		return 0;
	}
	case NM_DBLCLK:
	{
		handle_dbclick(get_selected());
		break;
	}
	case TVN_SELCHANGED:
	{
		//if (GetActiveWindow() == (HWND)handle()) return 0;
		handle_select(get_selected());
		break;
	}

	case TVN_GETINFOTIP:
	{
		LPNMTVGETINFOTIP lpGetInfoTip = (LPNMTVGETINFOTIP)p;
		static TCHAR tips[MAX_PATH]{};
		size_t len = handle_ontip(lpGetInfoTip->hItem, tips);
		if (!len)
		{
			const gui_string tip = get_item_text(lpGetInfoTip->hItem);
			wcscpy_s(tips, MAX_PATH, tip.data());
		}
		lpGetInfoTip->pszText = tips;
		lpGetInfoTip->cchTextMax = MAX_PATH;
		break;
	}

	//case TVN_BEGINLABELEDIT:
	//{
	//	auto hEdit = TreeView_GetEditControl((HWND)handle());
	//	SetFocus(hEdit);
	//	break;
	//}

	case TVN_ENDLABELEDIT:
	{
		LPNMTVDISPINFO lp = reinterpret_cast<LPNMTVDISPINFO>(p);
		if (lp->item.pszText && wcslen(lp->item.pszText))
			return 1;
		break;
	}
	}
	return 0;
}

//////////////////////////////////////////
// TMemo
TMemo::TMemo(TEventWindow* form, int id, bool do_scroll, bool plain) :TNotifyWin(form), m_pfmt(NULL), m_file_name(NULL)
//: TControl(parent, plain ? L"edit" : L"RichEdit", L"", id,
//	(do_scroll ? WS_HSCROLL | WS_VSCROLL : 0) |
//	WS_BORDER |  ES_AUTOVSCROLL | ES_LEFT
//), 
{
	DWORD style = WS_CHILD | WS_BORDER | ES_AUTOVSCROLL | ES_LEFT;
	if (do_scroll) style |= WS_HSCROLL | WS_VSCROLL;
	set(create_common_control(form, plain ? L"edit" : L"RichEdit", style));
	if (!plain) {
		m_pfmt = new CHARFORMAT{};
		m_pfmt->cbSize = sizeof(CHARFORMAT);
		m_pfmt->dwMask = 0;
		m_pfmt->dwEffects = 0;
	}
	send_msg(EM_SETEVENTMASK, 0, ENM_KEYEVENTS | ENM_MOUSEEVENTS);
}

TMemo::~TMemo()
{
	if (m_pfmt) delete m_pfmt;
}

void TMemo::set_font(pchar facename, int size, int flags, bool selection)
{
	m_pfmt->dwMask = CFM_FACE | CFM_BOLD | CFM_ITALIC;
	wcscpy_s(m_pfmt->szFaceName, facename);
	m_pfmt->dwEffects = 0;
	if (flags & BOLD) m_pfmt->dwEffects = CFE_BOLD;
	if (flags & ITALIC) m_pfmt->dwEffects |= CFE_ITALIC;
	send_char_format();
	m_pfmt->dwMask = 0;
	m_pfmt->dwEffects = 0;
	send_msg(EM_SETMARGINS, EC_LEFTMARGIN | EC_USEFONTINFO, 5);
}

pchar TMemo::file_name()
{
	return m_file_name;
}

void TMemo::cut()
{
	send_msg(WM_CUT);
}

void TMemo::copy()
{
	send_msg(WM_COPY);
}

void TMemo::clear()
{
	send_msg(WM_CLEAR);
}

void TMemo::paste()
{
	send_msg(WM_PASTE);
}

void TMemo::undo()
{
	send_msg(EM_UNDO);
}

int TMemo::text_size()
{
	return (int)send_msg(WM_GETTEXTLENGTH);
}

void TMemo::replace_selection(pchar str)
{
	send_msg(EM_REPLACESEL, TRUE, (LPARAM)str);
}

bool TMemo::modified()
{
	return !!send_msg(EM_GETMODIFY);
}

void TMemo::modified(bool yesno)
{
	send_msg(EM_SETMODIFY, yesno ? TRUE : FALSE);
}

int TMemo::line_count()
{
	return (int)send_msg(EM_GETLINECOUNT);
}

int TMemo::line_offset(int line)
{
	return (int)send_msg(EM_LINEINDEX, line);
}

int TMemo::line_size(int line)
{
	return (int)send_msg(EM_LINELENGTH, line);
}

int TMemo::get_line_text(int line, char* buff, int sz)
{
	*(short*)(void*)buff = (short)sz;
	int len = (int)send_msg(EM_GETLINE, line, (LPARAM)buff);
	buff[len] = '\0';
	return len;
}

void TMemo::get_selection(int& start, int& finish)
{
	send_msg(EM_GETSEL, (WPARAM)&start, (LPARAM)&finish);
}

void TMemo::set_selection(int start, int finish)
{
	send_msg(EM_SETSEL, start, finish);
}

void TMemo::select_all()
{
	set_selection(0, text_size());
}

void TMemo::go_to_end()
{
	set_selection(text_size(), text_size());
}

void TMemo::scroll_line(int line)
{
	send_msg(EM_LINESCROLL, 0, line);
}

int TMemo::line_from_pos(int pos)
{
	return (int)send_msg(EM_LINEFROMCHAR, pos, 0);
}

void TMemo::scroll_caret()
{
	send_msg(EM_SCROLLCARET);
}

void TMemo::auto_url_detect(bool yn)
{
	send_msg(EM_AUTOURLDETECT, (WPARAM)yn, 0);
}

void TMemo::send_char_format()
{
	send_msg(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)m_pfmt);
}

void TMemo::find_char_format()
{
	send_msg(EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)m_pfmt);
}

COLORREF TMemo::get_text_colour()
{
	m_pfmt->dwMask = CFM_COLOR;
	find_char_format();
	m_pfmt->dwMask = 0;
	m_pfmt->dwEffects = 0;
	return m_pfmt->crTextColor;
}

void TMemo::set_text_colour(COLORREF colour)
{
	m_pfmt->dwMask = CFM_COLOR;
	m_pfmt->crTextColor = colour;
	send_char_format();
	m_pfmt->dwMask ^= CFM_COLOR;
}

void TMemo::set_background_colour(COLORREF colour)
{
	send_msg(EM_SETBKGNDCOLOR, 0, colour);
}

void TMemo::go_to(int idx1, int idx2, int nscroll)
{
	if (idx2 == -1) idx2 = idx1;
	set_focus();
	set_selection(idx1, idx2);
	scroll_caret();
	scroll_line(nscroll); //*SJD* Should have an estimate of the page size!!
}

int TMemo::current_pos()
{
	int start = 0, finish = 0;
	get_selection(start, finish);
	return start;
}

int TMemo::current_line()
{
	return line_from_pos(current_pos()) + 1;
}

void TMemo::go_to_line(int line)
{
	int ofs = line_offset(line - 1);
	go_to(ofs, -1, current_line() > line ? -10 : +10);
}

COLORREF TMemo::get_line_colour(int l)
{
	int offs = line_offset(l);
	set_selection(offs, offs);
	return get_text_colour();
}

void TMemo::set_line_colour(int line, COLORREF colour)
{
	int old = current_pos();
	int ofs1 = line_offset(line - 1), ofs2 = line_offset(line);
	set_selection(ofs1, ofs2);
	set_text_colour(colour);
	set_selection(old, old);
}

int TMemo::handle_notify(void* p)
{
	LPNMHDR np = reinterpret_cast<LPNMHDR>(p);
	switch (np->code) {
	case EN_MSGFILTER:
		MSGFILTER* msf = reinterpret_cast<MSGFILTER*>(p);
		switch (msf->msg) {
		case WM_RBUTTONDOWN: // For future use
			return handle_rclick();
		case WM_KEYDOWN:
			return handle_onkey((int)msf->wParam);
		}
	}
	return 0;
}

int THasIconWin::new_image_list(pchar path, bool small_size)
{
	TImageList il(small_size);
	int icons_loaded = il.load_icons_from_module(path, small_size);
	m_il_handle = il.handle();
	return icons_loaded;
}

