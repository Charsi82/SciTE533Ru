/* gui_ext.cpp
This is a simple set of predefined GUI windows for SciTE,
built using the YAWL library.
Steve Donovan, 2007.
  */

#include "twl.h"
#include <string>
#include <vector>
#include <io.h>
#include <direct.h>
#include "lua.hpp"
#include "twl_dialogs.h"
#include "twl_menu.h"
#include "twl_cntrls.h"
#include "twl_listview.h"
#include "twl_tab.h"
#include "twl_splitter.h"
#include "twl_treeview.h"

#define output(x) lua_pushstring(L, (x)); OutputMessage(L);
#define lua_pushwstring(L, str) lua_pushstring((L), UTF8FromString(str).c_str())
  //void msgbox(const TCHAR* txt) { MessageBox(NULL, txt, L"dbg", 0); };

  // vector of wide strings
typedef std::vector<gui_string> vecws;
#define EQ(s1,s2) (!wcscmp((s1),(s2)))

static const char* LIB_VERSION = "0.1.2";
static const char* DEFAULT_ICONLIB = "toolbar\\cool.dll";
static const char* WINDOW_CLASS = "WINDOW*";

static TWin* s_parent = nullptr;
static TWin* s_last_parent = nullptr;
static HWND hSciTE = nullptr, hContent = nullptr, hCode = nullptr;
static WNDPROC old_scite_proc, old_scintilla_proc, old_content_proc;
static TWin* code_window = nullptr;
static TWin* extra_window = nullptr;
static TWin* content_window = nullptr;
static TWin* extra_window_splitter = nullptr;
static bool forced_resize = false;
static Rect m, cwb, extra;

class PaletteWindow;

void OutputMessage(lua_State* L);
void dump_stack(lua_State* L)
{
	if (!L) return;
	output("\r\n==== dump start ====");
	int sz = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= sz; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_pcall(L, 1, 1, 0);
		const char* str = lua_tolstring(L, -1, 0);
		if (str) {
			output(str);
		}
		else {
			output(lua_typename(L, lua_type(L, i)));
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	output("\r\n===== dump end ====");
}

#define DS dump_stack(L);

void dump_stack_to_log(lua_State* L)
{
	if (!L) return;
	log_add("\r\n==== dump start ====");
	int sz = lua_gettop(L);
	lua_getglobal(L, "tostring");
	for (int i = 1; i <= sz; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_pcall(L, 1, 1, 0);
		const char* str = lua_tolstring(L, -1, 0);
		if (str) {
			log_add(str);
		}
		else {
			log_add(lua_typename(L, lua_type(L, i)));
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	log_add("\r\n===== dump end ====");
}

//#define DSL dump_stack_to_log(L);
#define DSL

static inline bool optboolean(lua_State* L, int idx, bool res = false)
{
	return lua_isnoneornil(L, idx) ? res : lua_toboolean(L, idx);
}

TWin* get_parent()
{
	return s_parent;
}

TWin* get_last_parent()
{
	return s_last_parent;
}

TWin* get_desktop_window()
{
	 static std::unique_ptr<TWin> desk = std::make_unique<TWin>(GetDesktopWindow());
	 return desk.get();
}

//  print version 
static int do_version(lua_State* L)
{
	lua_getglobal(L, "print");
	lua_pushstring(L, LIB_VERSION);
	lua_pcall(L, 1, 0, 0);
	return 0;
}

// show a message on the SciTE output window
void OutputMessage(lua_State* L)
{
	if (lua_isstring(L, -1)) {
		lua_getglobal(L, "print");
		lua_insert(L, -2);
		lua_pcall(L, 1, 0, 0);
	}
}

static const void* gui_st_name = "_GUI_ST_NAME";
namespace
{
	void reinit_storage(lua_State* L)
	{
		//lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
		//if (lua_istable(L, -1))
		//{
		//	int len = 0;
		//	lua_pushnil(L);
		//	while (lua_next(L, -2)) {
		//		lua_pop(L, 1);
		//		++len;
		//	}
		//	log_add("len store [%02d]", len);
		//}
		//lua_pop(L, 1);
		lua_newtable(L);
		lua_rawsetp(L, LUA_REGISTRYINDEX, gui_st_name);
	}
#pragma optimize( "", off )
	// снимает с вершины стека значение и сохраняет в хранилище
	// @result индекс в хранилище
	int lua_reg_store(lua_State* L)
	{
		/* ... data */
		//lua_pushvalue(L, -1);/* ... data data*/
		//int ret =  luaL_ref(L, LUA_REGISTRYINDEX);/* ... data */

		//lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);/* ... data tbl */
		//lua_insert(L, -2);/* ... tbl data */
		//lua_rawseti(L, -2, ret);
		//lua_pop(L, 1);
		//log_add("lua_reg_store [%02d]", ret);

		lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);/* ... data tbl */
		lua_insert(L, -2);/* ... tbl data */
		int ret = luaL_ref(L, -2);/* ... tbl */
		lua_pop(L, 1);/* ... */
		return ret;
	}

	// кладет на стек данные по указанному индексу
	// @param индекс данных в хранилище
	void lua_reg_restore(lua_State* L, int idx)
	{
		//lua_rawgeti(L, LUA_REGISTRYINDEX, idx);

		//lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name); /* .. data tbl */
		//lua_rawgeti(L, -1, idx); /* .. data tbl data2 */
		//if(lua_rawequal(L, -1, -3))
		//{
		//	//log_add("lua_reg_restore : stack");
		//	//DSL
		//	//log_add("lua_reg_restore : equal [%02d]", idx);
		//}
		//else
		//{
		//	//log_add("lua_reg_restore : stack");
		//	//DSL
		//	//log_add("lua_reg_restore : non equal [%02d]", idx);
		//}
		//lua_pop(L, 2);

		lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
		lua_rawgeti(L, -1, idx);
		lua_remove(L, -2);
	}

	// удаляет данные из хранилища,
	// проверяет аргумент индекса на валидность
	// @param idx ненулевой индекс данных в хранилище
	void lua_reg_release(lua_State* L, int idx)
	{
		if (idx)
		{
			//log_add("lua_reg_release [%02d]", idx);
			//luaL_unref(L, LUA_REGISTRYINDEX, idx);

			//lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
			//lua_pushnil(L);
			//lua_rawseti(L, LUA_REGISTRYINDEX, idx);
			//lua_pop(L, 1);
			lua_rawgetp(L, LUA_REGISTRYINDEX, gui_st_name);
			luaL_unref(L, -1, idx);
			lua_pop(L, 1);
		}
	}
#pragma optimize( "", on )


	// https://ilovelua.wordpress.com

	int errorHandler(lua_State* L)
	{
		//stack: err
		lua_getglobal(L, "debug"); // stack: err debug
		lua_getfield(L, -1, "traceback"); // stack: err debug debug.traceback

		// debug.traceback() возвращает 1 значение
		if (lua_pcall(L, 0, 1, 0))
		{
			lua_pushstring(L, "Error in debug.traceback() call: ");
			lua_insert(L, -2);
			lua_pushstring(L, "\n");
			lua_concat(L, 3); //"Error in debug.traceback() call: "+err+"\n"
			//OutputMessage(L);
		}
		else
		{
			// stack: err debug stackTrace
			lua_insert(L, -2); // stack: err stackTrace debug
			lua_pop(L, 1); // stack: err stackTrace
			lua_pushstring(L, "Error:"); // stack: err stackTrace "Error:"
			lua_insert(L, -3); // stack: "Error:" err stackTrace  
			lua_pushstring(L, "\n"); // stack: "Error:" err stackTrace "\n"
			lua_insert(L, -2); // stack: "Error:" err "\n" stackTrace
			lua_concat(L, 4); // stack: "Error:"+err+"\n"+stackTrace
		}
		return 1;
	}
	//#pragma optimize( "", off )
		// маленький помощник, чтобы самим не считать количество lua_push...() и lua_pop()
	class LuaStackGuard
	{
		lua_State* luaState_;
		int top_;
	public:
		explicit LuaStackGuard(lua_State* L) : luaState_(L)
		{
			//log_add("lsg init");
			top_ = lua_gettop(L);
		}

		~LuaStackGuard()
		{
			//log_add("lsg destroy");
			lua_settop(luaState_, top_);
		}
	};
	//#pragma optimize( "", on )
}
#define LSG auto lsg = std::make_unique<LuaStackGuard>(L)

// static int call_named_function(lua_State* L, const char* name, lua_Integer arg)
// {
	// LSG;
	// lua_getglobal(L, name);
	// if (lua_isfunction(L, -1)) {
		// lua_pushinteger(L, arg);
		// if (lua_pcall(L, 1, 1, 0)) {
			// OutputMessage(L);
		// }
		// else {
			// return lua_gettop(L) ? lua_toboolean(L, -1) : 0;
		// }
	// }
	// return 0;
// }

static void force_contents_resize()
{
	// get the code pane's extents, and don't try to resize it again!
	code_window->get_rect(m, true);
	if (cwb.right == m.right && cwb.left == m.left) return; // top and left is 0 every times
	int w = extra_window->width();
	int h = m.height();
	int sw = extra_window_splitter->width();
	extra = m;
	cwb = m;
	if (extra_window->align() == Alignment::alLeft) {
		// on the left goes the extra pane, followed by the splitter
		extra.right = extra.left + w;
		extra_window->resize(m.left, m.top, w, h);
		extra_window_splitter->resize(m.left + w, m.top, sw, h);
		cwb.left += w + sw;
	}
	else {
		int margin = m.right - w;
		extra.left = margin;
		extra_window->resize(margin, m.top, w, h);
		extra_window_splitter->resize(margin - sw, m.top, sw, h);
		cwb.right -= w + sw;
	}
	// and then the code pane; note the hack necessary to prevent a nasty recursion here.
	forced_resize = true;
	code_window->resize(cwb);
	forced_resize = false;
}

static LRESULT SciTEWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage)
	{
	case WM_ACTIVATEAPP:
		//PaletteWindow::set_visibility(wParam);
		//set_visibility(wParam);
		//call_named_function(sL, "OnActivate", wParam);
		//floating toolbars may grab the focus, so restore it.
		if (wParam) code_window->set_focus();
		break;

	case WM_CLOSE:
		//call_named_function(sL, "OnClosing", 0); //!- disabled
		break;

	case WM_COMMAND:
		//if (call_named_function(sL, "OnCommand", LOWORD(wParam))) return TRUE;
		break;
	}
	return CallWindowProc(old_scite_proc, hwnd, iMessage, wParam, lParam);
}

static LRESULT ScintillaWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if (iMessage == WM_SIZE) {
		if (extra_window) {
			if (!forced_resize) {
				force_contents_resize();
			}
		}
		if (extra_window_splitter)// fix hiding splitter when change horz size
			extra_window_splitter->invalidate();
	}
	return CallWindowProc(old_scintilla_proc, hwnd, iMessage, wParam, lParam);
}

static LRESULT ContentWndProc(HWND hwnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if (iMessage == WM_SETCURSOR) {
		Point ptCursor;
		GetCursorPos(&ptCursor);
		ScreenToClient(hContent, &ptCursor);
		if (extra.is_inside(ptCursor)) {
			return DefWindowProc(hSciTE, iMessage, wParam, lParam);
		}
	}
	return CallWindowProc(old_content_proc, hwnd, iMessage, wParam, lParam);
}

static void subclass_scite_window()
{
	static bool subclassed = false;
	if (!subclassed) {  // to prevent a recursion
		old_scite_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hSciTE, GWLP_WNDPROC, (LONG_PTR)SciTEWndProc));
		old_content_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hContent, GWLP_WNDPROC, (LONG_PTR)ContentWndProc));
		old_scintilla_proc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hCode, GWLP_WNDPROC, (LONG_PTR)ScintillaWndProc));
		subclassed = true;
	}
}

//////// dialogs functions
static inline COLORREF convert_colour_spec(const char* clrs)
{
	unsigned int r = 0, g = 0, b = 0;
	sscanf_s(clrs, "#%02x%02x%02x", &r, &g, &b);
	return RGB(r, g, b);
}

/** gui.colour_dlg(default_colour)
	@param default_colour  colour either in form '#RRGGBB" or as a 32-bit integer
	@return chosen colour, in same form as default_colour
*/
int do_colour_dlg(lua_State* L)
{
	bool in_rgb = lua_isstring(L, 1);
	COLORREF cval = in_rgb ? convert_colour_spec(lua_tostring(L, 1)) : luaL_optinteger(L, 1, 0);
	if (run_colordlg((HWND)get_parent()->handle(), cval)) {
		if (in_rgb) {
			char buff[12];
			sprintf_s(buff, "#%02X%02X%02X", GetRValue(cval), GetGValue(cval), GetBValue(cval));
			lua_pushstring(L, buff);
		}
		else
			lua_pushinteger(L, cval);
		return 1;
	}
	return 0;
}

/** gui.message(message_string, window_type)
	@param message_string
	@param window_type (0 for plain message, 1 for warning box)
	MSG_ERROR=2,MSG_WARNING=1, MSG_QUERY=3;
*/

int do_message(lua_State* L)
{
	auto msg = StringFromUTF8(luaL_checkstring(L, 1));
	auto kind = StringFromUTF8(luaL_optstring(L, 2, "message"));
	int type = 0;
	//if (kind == L"message") type = 0; else
	if (kind == L"warning") type = 1; else
		if (kind == L"error") type = 2; else
			if (kind == L"query") type = 3;
	lua_pushboolean(L, get_parent()->message(msg.c_str(), type));
	return 1;
}

/* gui.open_dlg([sCaption = "Open File"][, sFilter = "All (*.*)|*.*"])
* @param sCaption [= "Open File"]
* @param sFilter  [= "All (*.*)|*.*"]
* @return sFileName or nil
*/
int do_open_dlg(lua_State* L)
{
	auto caption = StringFromUTF8(luaL_optstring(L, 1, "Open File"));
	auto filter = StringFromUTF8(luaL_optstring(L, 2, "All (*.*)|*.*"));
	bool multi = optboolean(L, 3);
	constexpr int PATHSIZE = 1024;
	TCHAR tmp[PATHSIZE]{};
	if (!run_ofd((HWND)get_parent()->handle(), tmp, caption, filter, multi)) return 0;
	gui_string path = tmp;
	// tmp : path\0file1\0file2\0..\0filen
	if (path.find(L'.') == std::wstring::npos)
	{
		int count = 0;
		TCHAR str[MAX_PATH]{};
		TCHAR* filename = tmp + wcslen(tmp) + 1;
		while (*filename != L'\0' && (filename - tmp < PATHSIZE)) {
			gui_string file = tmp;
			file += L"\\";
			file += filename;
			lua_pushwstring(L, file);
			filename += wcslen(filename) + 1;
			count++;
		}
		return count;
	}
	// tmp : pathfile
	else
	{
		lua_pushwstring(L, tmp);
	}
	return 1;
}

/* gui.save_dlg([sCaption = "Save File"][, sFilter = "All (*.*)|*.*"])
* @param sCaption [= "Save File"]
* @param sFilter  [= "All (*.*)|*.*"]
* @return sFileName or nil
*/
int do_save_dlg(lua_State* L)
{
	auto caption = StringFromUTF8(luaL_optstring(L, 1, "Save File"));
	auto filter = StringFromUTF8(luaL_optstring(L, 2, "All (*.*)|*.*"));
	TCHAR tmp[1024]{};
	if (!run_ofd((HWND)get_parent()->handle(), tmp, caption, filter)) return 0;
	lua_pushwstring(L, tmp);
	return 1;
}

// gui.select_dir_dlg([sDescription = ""][, sInitialdir = ""])
// @param sDescription [= ""]
// @param sInitialdir  [= ""]
// @return sDirName or nil
int do_select_dir_dlg(lua_State* L)
{
	auto descr = StringFromUTF8(luaL_optstring(L, 1, "Browse for folder..."));
	auto initdir = StringFromUTF8(luaL_optstring(L, 2, "C:\\"));
	TCHAR tmp[MAX_PATH]{};
	if (!run_seldirdlg((HWND)get_parent()->handle(), tmp, descr.c_str(), initdir.c_str())) return 0;
	lua_pushwstring(L, tmp);
	return 1;
}


/* gui.prompt_value( sPrompt_string [, default_value = ""])
* @param sPrompt_string
* @param sDefaultValue [=""]
*/
int do_prompt_value(lua_State* L)
{
	//auto varname = StringFromUTF8(luaL_checkstring(L, 1));
	auto def_value = StringFromUTF8(luaL_optstring(L, 2, ""));
	//PromptDlg dlg(get_parent(), varname.c_str(), def_value.c_str());
	//dlg.set_icon_from_window(get_parent());
	//lua_pushwstring(L, dlg.show_modal() ? dlg.m_val : def_value);
	output("function 'gui.prompt_value()' removed!!! please use shell.inputbox()!!");
	lua_pushwstring(L, def_value);
	return 1;
}
/// end dialos functions

/// others functions
//static int append_file(lua_State* L, int idx, int attrib, bool look_for_dir, pchar value)
//{
//	if (((attrib & _A_SUBDIR) != 0) == look_for_dir) {
//		if (look_for_dir && (EQ(value, L".") || EQ(value, L".."))) return idx;
//		lua_pushinteger(L, idx);
//		lua_pushwstring(L, value);
//		lua_settable(L, -3);
//		return idx + 1;
//	}
//	return idx;
//}

static int do_files(lua_State* L)
{
	_wfinddata_t c_file;
	gui_string mask = StringFromUTF8(luaL_checkstring(L, 1));
	bool look_for_dir = optboolean(L, 2);
	intptr_t hFile = _wfindfirst(mask.c_str(), &c_file);
	lua_newtable(L);
	if (hFile == /*INVALID_HANDLE_VALUE*/ -1) { return 1; }
	int idx = 1;
	do
		if (((c_file.attrib & _A_SUBDIR) != 0) == look_for_dir) {
			if (look_for_dir && (EQ(c_file.name, L".") || EQ(c_file.name, L".."))) continue;
			lua_pushinteger(L, idx++);
			lua_pushwstring(L, c_file.name);
			lua_settable(L, -3);
		}
	while (!_wfindnext(hFile, &c_file));
	return 1;
}

static int do_chdir(lua_State* L)
{
	const char* dirname = luaL_checkstring(L, 1);
	int res = _chdir(dirname);
	lua_pushboolean(L, res == 0);
	return 1;
}

int do_run(lua_State* L)
{
	auto wsFile = StringFromUTF8(luaL_checkstring(L, 1));
	auto wsParameters = StringFromUTF8(lua_tostring(L, 2));
	auto wsDirectory = StringFromUTF8(lua_tostring(L, 3));
	lua_Integer res = (lua_Integer)ShellExecute(
		NULL,
		L"open",
		wsFile.c_str(),
		wsParameters.c_str(),
		wsDirectory.c_str(),
		SW_SHOWDEFAULT
	);
	if (res <= HINSTANCE_ERROR) {
		lua_pushboolean(L, 0);
		lua_pushinteger(L, res);
		return 2;
	}
	else {
		lua_pushinteger(L, res);
		return 1;
	}
}

std::string getAscii(unsigned char value);
std::string getHtmlName(unsigned char value);
int getHtmlNumber(unsigned char value);

/*
* @param uChar
* @return symbol, html number, html code
*/
int do_get_ascii(lua_State* L)
{
	unsigned char x = static_cast<unsigned char>(luaL_checkinteger(L, 1));
	std::string s = getAscii(x);
	lua_pushstring(L, s.c_str());

	char htmlNumber[8]{};
	if ((x >= 32 && x <= 126) || (x >= 160 /*&& x <= 255*/))
	{
		sprintf_s(htmlNumber, "&#%d", x);
	}
	else
	{
		int n = getHtmlNumber(x);
		if (n > -1)
		{
			sprintf_s(htmlNumber, "&#%d", n);
		}
		else
		{
			sprintf_s(htmlNumber, "");
		}
	}
	lua_pushstring(L, htmlNumber);

	std::string htmlName = getHtmlName(x);
	lua_pushstring(L, htmlName.c_str());
	return 3;
}
/// end others functions

/// windows functions

////// This acts as the top-level frame window containing these controls; it supports
////// adding extra buttons and actions.

template<typename T>
void lua_pushargs(lua_State* L, T value)
{
	lua_pushinteger(L, (lua_Integer)value);
}

void lua_pushargs(lua_State* L, const char* value)
{
	lua_pushstring(L, value);
}

void lua_pushargs(lua_State* L, bool value)
{
	lua_pushboolean(L, value);
}

void lua_pushargs(lua_State* L, void* value)
{
	lua_pushlightuserdata(L, value);
}

template<typename T, typename... Args>
void lua_pushargs(lua_State* L, T value, Args... args)
{
	lua_pushargs(L, value);
	if (sizeof...(args)) lua_pushargs(L, args...);
}

int dispatch_ref(lua_State* L, int data)
{
	if (data != 0) {
		LSG;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
		const int errorHandlerIndex = -2;
		lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
		if (lua_pcall(L, 0, 1, errorHandlerIndex))
			OutputMessage(L);
		else
			return lua_toboolean(L, -1);
	}
	return 0;
}

template<typename... Args>
int dispatch_ref(lua_State* L, int data, Args... args)
{
	if (data != 0) {
		LSG;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		lua_pushargs(L, args...);// stack: func arg1 arg2 ... argn
		lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
		const int args_count = sizeof...(args);
		const int errorHandlerIndex = -(args_count + 2);
		lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
		if (lua_pcall(L, args_count, 1, errorHandlerIndex))
			OutputMessage(L);
		else
			return lua_toboolean(L, -1);
	}
	return 0;
}

static void throw_error(lua_State* L, const char* msg)
{
	lua_pushstring(L, msg);
	lua_error(L);
}

void function_ref(lua_State* L, int idx, int* pr)
{
	//if (*pr) luaL_unref(L, LUA_REGISTRYINDEX, *pr);
	lua_reg_release(L, *pr);
	if (!lua_isfunction(L, idx)) throw_error(L, "function required");
	lua_pushvalue(L, idx);
	//*pr = luaL_ref(L, LUA_REGISTRYINDEX);
	*pr = lua_reg_store(L);
}

class LuaWindow : public TEventWindow
{
protected:
	lua_State* L;
	int on_close_idx;
	int on_show_idx;
	int on_command_idx;
	int on_scroll_idx;
	int on_timer_idx;
public:
	LuaWindow(pchar caption, lua_State* l, TWin* parent, DWORD stylex = 0, bool is_child = false, DWORD style = -1)
		:TEventWindow(caption, parent, stylex, is_child, style), L(l),
		on_close_idx(0), on_show_idx(0), on_command_idx(0), on_scroll_idx(0), on_timer_idx(0)
	{}

	void set_on_command(int iarg)
	{
		function_ref(L, iarg, &on_command_idx);
	}

	bool command(int arg1, int arg2) override
	{
		return dispatch_ref(L, on_command_idx, arg1, arg2);
	}

	void set_on_scroll(int iarg)
	{
		function_ref(L, iarg, &on_scroll_idx);
	}

	void scroll(int code, int posn) override
	{
		dispatch_ref(L, on_scroll_idx, code, posn);
	}

	void set_on_timer(int iarg, int delay)
	{
		function_ref(L, iarg, &on_timer_idx);
		create_timer(delay);
	}

	void timer() override
	{
		dispatch_ref(L, on_timer_idx);
	}

	void handler(Item* item)
	{
		LSG;
		int data = (size_t)item->data;
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		if (lua_isinteger(L, -1))
		{
			call_SciteCommand();
		}
		else if (lua_isstring(L, -1))
		{
			// IDM_*
			// func name
			// cmd id

			std::string str = lua_tostring(L, -1);
			if (str._Starts_with("IDM_")) {
				lua_remove(L, -1);
				lua_getglobal(L, str.c_str());
				if (lua_isinteger(L, -1))
				{
					call_SciteCommand();
				}
				else
					output_unknown(str.c_str());
			}
			else
			{
				if (lua_isnumber(L, -1)) // can convert to cmd id?
				{
					int id = lua_tonumber(L, -1);
					lua_pushinteger(L, id);
					lua_remove(L, -2);
					call_SciteCommand();
				}
				else
				{
					lua_getglobal(L, str.c_str());
					lua_remove(L, -2);
					if (lua_isfunction(L, -1)) { // func name
						if (lua_pcall(L, 0, 0, 0)) OutputMessage(L);
					}
					else
						output_unknown(str.c_str());
				}
			}
		}
		else if (lua_isfunction(L, -1)) {
			if (lua_pcall(L, 0, 0, 0)) OutputMessage(L);
		}
		else
			output_unknown(lua_tostring(L, -1));
	}

	void set_on_close(int iarg)
	{
		function_ref(L, iarg, &on_close_idx);
	}

	void destroy() override
	{
		kill_timer();
	}

	void on_close() override
	{
		dispatch_ref(L, on_close_idx);
	}

	void set_on_show(int iarg)
	{
		function_ref(L, iarg, &on_show_idx);
	}

	void on_showhide(bool show) override
	{
		dispatch_ref(L, on_show_idx, show);
	}

private:
	void output_unknown(const char* str)
	{
		lua_pushfstring(L, "unknown command '%s'", str);
		OutputMessage(L);
	};

	void call_SciteCommand()
	{
		// val
		lua_getglobal(L, "scite");			// val, scite
		lua_getfield(L, -1, "MenuCommand"); // val, scite, scite.MenuCommand
		lua_insert(L, 1);					// scite.MenuCommand, val, scite
		lua_pop(L, 1);						// scite.MenuCommand, val
		if (lua_pcall(L, 1, 0, 0)) OutputMessage(L); // call scite.MenuCommand( cmdID )
	}
};

class PaletteWindow : public LuaWindow
{
protected:
	bool m_shown;
public:
	PaletteWindow(pchar caption, lua_State* l, TWin* parent = nullptr, DWORD stylex = 0, DWORD style = -1)
		: LuaWindow(caption, l, parent, stylex, false, style), m_shown(false)
	{}
	void show(int how = SW_SHOW) override
	{
		TEventWindow::show(how);
		m_shown = true;
	}

	void hide() override
	{
		TEventWindow::hide();
		m_shown = false;
	}

	bool query_close() override
	{
		hide();
		return false;
	}

	void really_show() { TEventWindow::show(); }

	void really_hide() { TEventWindow::hide(); }

	bool is_shown() { return m_shown; }
};

struct WinWrap {
	TWin* window;
	void* data;
};

static int wrap_window(lua_State* L, TWin* win)
{
	WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_newuserdata(L, sizeof(WinWrap)));
	wrp->window = win;
	wrp->data = nullptr;
	luaL_getmetatable(L, WINDOW_CLASS);
	lua_setmetatable(L, -2);
	return 1;
}

inline TWin* window_arg(lua_State* L, int idx = 1)
{
	WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, idx));
	if (!wrp) {
		throw_error(L, "not a window");
		return nullptr;
	}
	return wrp->window;
}

//static void*& window_data(lua_State* L, int idx = 1)
//{
//	WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, idx));
//	if (!wrp) throw_error(L, "not a window");
//	return wrp->data;
//}

//static void window_data_set(lua_State* L, int idx = 1, TWin* p)
//{
//	WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, idx));
//	if (!wrp) throw_error(L, "not a window");
//	return wrp->data;
//}

class ContainerWindow : public PaletteWindow
{
public:
	ContainerWindow(pchar caption, lua_State* l)
		//: PaletteWindow(caption, l, get_parent(), WS_EX_TOPMOST, WS_OVERLAPPEDWINDOW) // WS_EX_TOOLWINDOW WS_EX_TOPMOST
		: PaletteWindow(caption, l, get_parent(), WS_EX_TOPMOST, WS_CAPTION | WS_SYSMENU) // WS_EX_TOOLWINDOW WS_EX_TOPMOST
	{
		set_icon_from_window(get_parent());
	}
	virtual ~ContainerWindow() {
		//log_add("~ContainerWindow");
	};
};

// show window w
// w:show()
int window_show(lua_State* L)
{
	window_arg(L)->show();
	return 0;
}

// hide window w
// w:hide()
int window_hide(lua_State* L)
{
	window_arg(L)->hide();
	return 0;
}

// enable window w
// w:enable(true)
int window_enable(lua_State* L)
{
	if (TWin* win = window_arg(L))
		win->set_enable(optboolean(L, 2));
	return 0;
}

/** set or get window size
	w:size(width, height)
	@param window
	@param width [=250]
	@param height [=300]
*/
int window_size(lua_State* L)
{
	TWin* win = window_arg(L);
	if (lua_gettop(L) > 1)
	{
		int w = luaL_optnumber(L, 2, 250);
		int h = luaL_optnumber(L, 3, 200);
		window_arg(L)->resize(w, h);
		return 0;
	}
	else
	{
		Rect rt;
		win->get_rect(rt, true);
		lua_pushinteger(L, rt.width());
		lua_pushinteger(L, rt.height());
		return 2;
	}
}

/** centered window2 inside window1
	win1:center_h(win2)
*/
int window_center_h(lua_State* L)
{
	TWin* win = window_arg(L);
	TWin* win2 = window_arg(L, 2);
	Rect rt{};
	win->get_rect(rt, true);
	Rect rt2{};
	win2->get_rect(rt2, true);
	rt.left += rt.width()/2 - rt2.width()/2;
	//rt2.top = (rt.height() - rt2.height()) / 2;
	win2->move(rt.left, rt2.top);
	return 0;
}

int window_center_v(lua_State* L)
{
	TWin* win = window_arg(L);
	TWin* win2 = window_arg(L, 2);
	Rect rt{};
	win->get_rect(rt, true);
	Rect rt2{};
	win2->get_rect(rt2, true);
	//rt.left += (rt.width() - rt2.width()) / 2;
	rt.top += rt.height()/2 - rt2.height()/2;
	win2->move(rt2.left, rt.top);
	return 0;
}

int window_center(lua_State* L)
{
	window_center_v(L);
	window_center_h(L);
	return 0;
}

int window_resize(lua_State* L)
{
	if (TEventWindow* win = dynamic_cast<TEventWindow*>(window_arg(L)))
	{
		bool resize = lua_toboolean(L, 2);
		int w = luaL_checkinteger(L, 3);
		int h = luaL_checkinteger(L, 4);
		win->enable_resize(resize, w, h);
		win->size();
	}
	return 0;
}

/** set or get window position
	w:position()
	@param self
	@param pos_x
	@param pos_y
*/
int window_position(lua_State* L)
{
	if (lua_gettop(L) > 1)
	{
		int x = (int)luaL_optnumber(L, 2, 10);
		int y = (int)luaL_optnumber(L, 3, 10);
		window_arg(L)->move(x, y);
		return 0;
	}
	else
	{
		TWin* win = window_arg(L);
		Rect rt;
		win->get_rect(rt, true);

		lua_pushinteger(L, rt.left);
		lua_pushinteger(L, rt.top);
		return 2;
	}
}

/*
* wnd:bounds()
* @return bVisible, left, top, width, height
*/
int window_get_bounds(lua_State* L)
{
	TWin* win = window_arg(L);
	Rect rt;
	win->get_rect(rt);
	lua_pushboolean(L, win->visible());
	lua_pushinteger(L, rt.left);
	lua_pushinteger(L, rt.top);
	lua_pushinteger(L, rt.width());
	lua_pushinteger(L, rt.height());
	return 5;
}

static std::vector<std::unique_ptr<ContainerWindow>> collect_windows;
/** create new window
	w = gui.window( sCaption )
	@param strCaption
	@return window
*/
int new_window(lua_State* L)
{
	auto caption = StringFromUTF8(luaL_checkstring(L, 1));
	collect_windows.push_back(std::make_unique<ContainerWindow>(caption.c_str(), L));
	ContainerWindow* cw = collect_windows.back().get();
	s_last_parent = cw;
	return wrap_window(L, cw);
}

class PanelWindow : public LuaWindow
{
public:
	explicit PanelWindow(lua_State* L) : LuaWindow(L"", L, get_parent(), 0, true)
	{};
};

/** create new panel
	gui.panel( iWidth )
	@param iWidth [=100]
	@return window
*/
int new_panel(lua_State* L)
{
	PanelWindow* pw = new PanelWindow(L);
	pw->align(Alignment::alLeft, (int)luaL_optinteger(L, 1, 100));
	s_last_parent = pw;
	return wrap_window(L, pw);
}

class LuaControl
{
protected:
	lua_State* L;
	int select_idx;
	int double_idx;
	int onkey_idx;
	int rclick_idx;
	int onclose_idx;
	int onfocus_idx;
	int ontip_idx;

public:
	explicit LuaControl(lua_State* l)
		: L(l), select_idx(0), double_idx(0), onkey_idx(0), rclick_idx(0)
		, onclose_idx(0), onfocus_idx(0), ontip_idx(0)
	{}

	void set_select(int iarg)
	{
		function_ref(L, iarg, &select_idx);
	}

	void set_double_click(int iarg)
	{
		function_ref(L, iarg, &double_idx);
	}

	void set_onkey(int iarg)
	{
		function_ref(L, iarg, &onkey_idx);
	}

	void set_rclick(int iarg)
	{
		function_ref(L, iarg, &rclick_idx);
	}

	void set_on_close(int iarg)
	{
		function_ref(L, iarg, &onclose_idx);
	}

	void set_on_focus(int iarg)
	{
		function_ref(L, iarg, &onfocus_idx);
	}

	void set_on_tip(int iarg)
	{
		function_ref(L, iarg, &ontip_idx);
	}
};

class TTabControlLua : public TTabControl, public LuaControl
{
public:
	TTabControlLua(TEventWindow* parent, lua_State* L, bool multiline = false)
		: TTabControl(parent, multiline), LuaControl(L)
	{}
	// implement
	void handle_select(int id) override
	{
		TWin* page = reinterpret_cast<TWin*>(get_data(id));
		get_parent_win()->set_client(page);
		get_parent_win()->size();
		dispatch_ref(L, select_idx, id);
	}
	int handle_rclick(int id) override
	{
		show_popup();
		return 0;
	}
};

/* gui.tabbar(window)
* @return tabbar window
*/
int new_tabbar(lua_State* L)
{
	TEventWindow* form = (TEventWindow*)window_arg(L);
	TTabControlLua* tab = new TTabControlLua(form, L, lua_toboolean(L, 2));
	tab->align(Alignment::alTop);
	form->add(tab);
	return wrap_window(L, tab);
}

/* tabbar:add_tab(sCaption, wndPanel)
* @param sCaption
* @param panel
*/
int tabbar_add(lua_State* L)
{
	if (TTabControl* tab = (TTabControl*)window_arg(L))
	{
		gui_string caption = StringFromUTF8(luaL_checkstring(L, 2));
		if (TWin* wnd = window_arg(L, 3))
		{
			int icon_idx = luaL_optinteger(L, 4, -1);
			tab->add(caption.data(), wnd, icon_idx);
		}
	}
	return 0;
}

/* idx = tabbar:select_tab()
/* tabbar:select_tab(idx)
* @param sCaption
* @param panel
*/
int tabbar_sel(lua_State* L)
{
	if (TTabControl* tab = (TTabControl*)window_arg(L))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, tab->selected());
			return 1;
		}
		tab->selected(luaL_checkinteger(L, 2));
	}
	return 0;
}

int window_remove(lua_State* L)
{
	TEventWindow* form = (TEventWindow*)window_arg(L);

	if (WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, 2)))
	{
		// remove window
		form->remove(wrp->window);

		// remove splitter
		if (TWin* split = reinterpret_cast<TWin*>(wrp->data))
			form->remove(split);
	}
	return 0;
}
/*
	wnd_parent:add( child_window[, sAligment = "client"][, iSize = 100][, bSplitted = true])
	@param wnd
	@param sAligment [= "client"]
	@param iSize [= 100]
	@param bSplitted [= true]
*/
int window_add(lua_State* L)
{
	TEventWindow* cw = (TEventWindow*)window_arg(L);
	TWin* child = window_arg(L, 2);
	std::string align = luaL_optstring(L, 3, "client");
	int sz = (int)luaL_optnumber(L, 4, 100);
	bool splitter = optboolean(L, 5);
	child->set_parent(cw);
	if (align == "top") {
		child->align(Alignment::alTop, sz);
	}
	else if (align == "bottom") {
		child->align(Alignment::alBottom, sz);
	}
	else if (align == "left") {
		child->align(Alignment::alLeft, sz);
	}
	else if (align == "none") {
		child->align(Alignment::alNone);
	}
	else if (align == "right") {
		child->align(Alignment::alRight, sz);
	}
	else {
		child->align(Alignment::alClient, sz);
	}
	cw->add(child);
	if (splitter && child->align() != Alignment::alClient && child->align() != Alignment::alNone) {
		TSplitter* split = new TSplitter(cw, child);
		cw->add(split);
		if (WinWrap* wrp = reinterpret_cast<WinWrap*>(lua_touserdata(L, 2)))
			wrp->data = split;
	}
	return 0;
}

int new_button(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		auto caption = luaL_checkstring(L, 1);
		int cmd_id = luaL_checkinteger(L, 2);
		return wrap_window(L, new TButton(p, StringFromUTF8(caption).c_str(), cmd_id));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.button'");
		return 0;
	}
}

int new_groupbox(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		auto caption = luaL_checkstring(L, 1);
		return wrap_window(L, new TGroupBox(p, StringFromUTF8(caption).c_str()));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.groupbox'");
		return 0;
	}
}

int new_checkbox(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		auto caption = luaL_optstring(L, 1, nullptr);
		int cmd_id = luaL_optinteger(L, 2, -1);
		bool is3state = optboolean(L, 3);
		return wrap_window(L, new TCheckBox(p, StringFromUTF8(caption).c_str(), cmd_id, is3state));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.checkbox'");
		return 0;
	}
}

int new_radiobutton(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		auto caption = luaL_checkstring(L, 1);
		int cmd_id = luaL_optinteger(L, 2, -1);
		bool is_auto = optboolean(L, 3);
		return wrap_window(L, new TRadioButton(p, StringFromUTF8(caption).c_str(), cmd_id, is_auto));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.radiobutton'");
		return 0;
	}
}

// button, checkbox
int do_check(lua_State* L)
{
	TButtonBase* btn = reinterpret_cast<TButtonBase*>(window_arg(L));
	if (!btn) return 0;
	int args = lua_gettop(L);
	if (args > 1) {
		btn->check(luaL_checkinteger(L, 2));
		return 0;
	}
	lua_pushinteger(L, btn->check());
	return 1;
}

int new_label(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		auto caption = luaL_optstring(L, 1, nullptr);
		DWORD label_style = luaL_optinteger(L, 2, 1);
		return wrap_window(L, new TLabel(p, StringFromUTF8(caption).c_str(), label_style));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.label'");
		return 0;
	}
}

int do_ctrl_set_icon(lua_State* L)
{
	if (THasIconCtrl* lbl = dynamic_cast<THasIconCtrl*>(window_arg(L)))
	{
		auto path = luaL_optstring(L, 2, DEFAULT_ICONLIB);
		int icon_idx = luaL_optinteger(L, 3, 0);
		lbl->set_icon(StringFromUTF8(path).c_str(), icon_idx);
	}
	return 0;
}

int do_ctrl_set_bitmap(lua_State* L)
{
	if (THasIconCtrl* lbl = dynamic_cast<THasIconCtrl*>(window_arg(L)))
	{
		auto path = luaL_checkstring(L, 2);
		lbl->set_bitmap(StringFromUTF8(path).c_str());
	}
	return 0;
}

int new_trackbar(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		DWORD style = luaL_optinteger(L, 1, 0);
		int cmd_id = luaL_optinteger(L, 2, 0);
		return wrap_window(L, new TTrackBar(p, style, cmd_id));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.trackbar'");
		return 0;
	}
}

int trbar_get_pos(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
	{
		lua_pushinteger(L, trb->pos());
		return 1;
	}
	return 0;
}

int trbar_set_pos(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
		trb->pos(luaL_optinteger(L, 2, 0));
	return 0;
}

int trbar_sel_clear(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
		trb->sel_clear();
	return 0;
}

int trbar_set_range(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
	{
		int imin = luaL_optinteger(L, 2, 0);
		int imax = luaL_optinteger(L, 3, 100);
		trb->range(imin, imax);
	}
	return 0;
}

class TMemoLua : public TMemo, public LuaControl
{
public:
	TMemoLua(TEventWindow* parent, lua_State* L, int id, bool do_scroll = false, bool plain = false)
		:TMemo(parent, id, do_scroll, plain), LuaControl(L)
	{}

	int handle_onkey(int id) override
	{
		return dispatch_ref(L, onkey_idx, id);
	}

	int handle_rclick() override
	{
		show_popup();
		return 0;
	}
};

int new_memo(lua_State* L)
{
	if (TEventWindow* p = (TEventWindow*)get_last_parent()) {
		return wrap_window(L, new TMemoLua(p, L, 1));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.memo'");
		return 0;
	}
}

int window_set_text(lua_State* L)
{
	if (TWin* win = window_arg(L))
		win->set_text(StringFromUTF8(luaL_checkstring(L, 2)).c_str());
	return 0;
}

int window_get_text(lua_State* L)
{
	if (TWin* win = window_arg(L))
	{
		gui_string str;
		win->get_text(str);
		lua_pushwstring(L, str);
	}
	return 1;
}

int memo_set_colour(lua_State* L)
{
	if (TMemoLua* wnd = (TMemoLua*)window_arg(L))
	{
		wnd->set_text_colour(convert_colour_spec(luaL_checkstring(L, 2))); // Must be only ASCII
		wnd->set_background_colour(convert_colour_spec(luaL_checkstring(L, 3)));
	}
	return 0;
}

static void shake_scite_descendants()
{
	Rect frt;
	get_parent()->get_rect(frt, false);
	get_parent()->send_msg(WM_SIZE, SIZE_RESTORED, (LPARAM)MAKELONG(frt.width(), frt.height()));
}

class SideSplitter : public TSplitterB
{
public:
	SideSplitter(TWin* form, TWin* control)
		: TSplitterB(form, control, 5)
	{}

	void paint(TDC& dc) override
	{
		Rect rt(this);
		dc.rectangle(rt);
	}

	void on_resize(const Rect& rt) override
	{
		TSplitterB::on_resize(rt);
		shake_scite_descendants();
	}
};

// gui.set_panel() - hide sidebar panel
// gui.set_panel(parent_window, sAlignment) - show sidebar and attach to parent_window with sAlignment
static int do_set_panel(lua_State* L)
{
	if (!content_window) {
		lua_pushstring(L, "Window subclassing was not successful");
		lua_error(L);
	}
	if (!lua_isuserdata(L, 1) && extra_window) {
		extra_window->hide();
		extra_window = nullptr;
		extra_window_splitter->close();
		delete extra_window_splitter;
		extra_window_splitter = nullptr;
		shake_scite_descendants();
	}
	else {
		extra_window = window_arg(L);
		auto align = StringFromUTF8(luaL_optstring(L, 2, "left"));
		if (align == L"left")
			extra_window->align(Alignment::alLeft);
		else
			extra_window->align(Alignment::alRight);

		extra_window->set_parent(content_window);
		extra_window->show();
		extra_window_splitter = new SideSplitter(content_window, extra_window);
		extra_window_splitter->show();
		force_contents_resize();

		// fix resizing
		Rect rt{};
		extra_window->get_rect(rt, true);
		rt.top += 1;
		extra_window->resize(rt);
		rt.top -= 1;
		extra_window->resize(rt);
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/* do_day_of_year */
#include <algorithm>
#include <ctime>

int     days
(
	int     day,
	int     month,
	int     year
)
{
	int     a = (14 - month) / 12;
	int     y = year - a;
	int     mnth = month + 12 * a - 3;

	return      day
		+ (153 * mnth + 2) / 5
		+ 365 * y
		+ y / 4
		- y / 100
		+ y / 400;
}

int do_day_of_year(lua_State* L)
{
	if (lua_gettop(L) == 3)
	{
		int dd = std::clamp((int)luaL_checkinteger(L, 1), 1, 31);
		int mm = std::clamp((int)luaL_checkinteger(L, 2), 1, 12);
		int yy = luaL_checkinteger(L, 3);
		lua_pushinteger(L, (lua_Integer)days(dd, mm, yy) - days(0, 1, yy));
	}
	else {
		time_t seconds = time(NULL);
		tm timeinfo{};
		localtime_s(&timeinfo, &seconds);
		lua_pushinteger(L, (lua_Integer)timeinfo.tm_yday + 1);
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////
int do_test_function(lua_State* L)
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// 
//  this will allow us to hand keyboard focus over to editor
//  gui.pass_focus() переключаем фокус на редактор
static int do_pass_focus(lua_State* L)
{
	lua_getglobal(L, "editor");
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, "Focus");
	lua_pop(L, 1);
	if (code_window) code_window->set_focus();
	return 0;
}

// parent_wnd:client(child_wnd)
int window_client(lua_State* L)
{
	TEventWindow* cw = (TEventWindow*)window_arg(L);
	if (TWin* child = window_arg(L, 2))
	{
		child->set_parent(cw);
		cw->set_client(child);
	}
	else
		throw_error(L, "wnd:client(arg) - arg must provide a child window");
	return 0;
}

class TListViewLua : public TListViewB, public LuaControl
{
public:
	TListViewLua(TEventWindow* parent, lua_State* L, bool multiple_columns = false, bool single_select = true, bool large_icons = false)
		: TListViewB(parent, large_icons, multiple_columns, single_select),
		LuaControl(L)
	{
		if (!multiple_columns) {
			add_column(L"*", 200);
		}
	}

	void clear() override
	{
		int items_count = count();
		for (int i = 0; i < items_count; i++)
			//luaL_unref(L, LUA_REGISTRYINDEX, data);
			lua_reg_release(L, get_item_data(i));
		TListViewB::clear();
		//log_add("TListViewLua clear [%d]", items_count);
	}

	// implement
	void handle_select(intptr_t id) override
	{
		dispatch_ref(L, select_idx, id);
	}

	void handle_double_click(int row, int col, const char* s) override
	{
		dispatch_ref(L, double_idx, row, col, s);
	}

	void handle_onkey(int id) override
	{
		dispatch_ref(L, onkey_idx, id);
	}

	int handle_rclick() override
	{
		show_popup();
		return 0;
	}

	void handle_onfocus(bool yes) override
	{
		dispatch_ref(L, onfocus_idx, yes);
	}

	void remove_item(int idx) override
	{
		lua_reg_release(L, get_item_data(idx));
		TListViewB::remove_item(idx);
	}
};

class TTreeViewLua : public TTreeView, public LuaControl
{
public:
	TTreeViewLua(TEventWindow* parent, lua_State* L, DWORD style)
		:TTreeView(parent, style), LuaControl(L)
	{}
	// implement
	void handle_select(void* itm) override
	{
		dispatch_ref(L, select_idx, itm);
	}

	void handle_dbclick(void* itm) override
	{
		dispatch_ref(L, double_idx, itm);
	}

	int handle_rclick() override
	{
		show_popup();
		return 0;
	}

	int handle_onkey(int id) override
	{
		return dispatch_ref(L, onkey_idx, id);
	}

	size_t handle_ontip(void* item, TCHAR* str) override
	{
		//dispatch_ref(L, ontip_idx, item);
		if (ontip_idx != 0) {
			LSG;
			//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
			lua_reg_restore(L, ontip_idx);
			lua_pushargs(L, item);// stack: func arg1 arg2 ... argn
			lua_pushcfunction(L, errorHandler);// stack: func arg1 arg2 ... argn errorHandler
			const int args_count = 1;
			const int errorHandlerIndex = -(1 + 2);
			lua_insert(L, errorHandlerIndex); //stack: errorHandler func arg1 arg2 ... argn
			if (lua_pcall(L, 1, 1, errorHandlerIndex))
				OutputMessage(L);
			else
			{
				gui_string res = StringFromUTF8(lua_tostring(L, -1));
				wcscpy_s(str, MAX_PATH, res.c_str());
				return res.size();
			}
		}
		return 0;
	}

private:
	void clean_data(int data) override
	{
		//luaL_unref(L, LUA_REGISTRYINDEX, data);
		lua_reg_release(L, data);
	}
};

// gui.tree([has_lines = true] [, editable = false])
// create new tree
int new_tree(lua_State* L)
{
	if (TEventWindow* p = reinterpret_cast<TEventWindow*>(get_last_parent())) {
		int style = luaL_optinteger(L, 1, 2);
		return wrap_window(L, new TTreeViewLua(p, L, style));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.tree'");
		return 0;
	}
}

inline TTreeViewLua* check_treewnd(lua_State* L)
{
	return dynamic_cast<TTreeViewLua*>(window_arg(L));
}

//tree:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
//tabbar:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
//list_wnd:set_iconlib( [path = "toolbar\\cool.dll"] [, small = true] )
int do_set_iconlib(lua_State* L)
{
	THasIconWin* win = dynamic_cast<THasIconWin*>(window_arg(L));
	if (win) {
		gui_string txt = StringFromUTF8(luaL_optstring(L, 2, "toolbar\\cool.dll"));
		bool small_size = optboolean(L, 3, true);
		int icons_loaded = win->load_icons(txt.c_str(), small_size);
		lua_pushinteger(L, icons_loaded);
		return 1;
	}
	else
		luaL_error(L, "do_tree_set_iconlib: there is no iconed at 1st arg");
	return 0;
}

// tree_wnd:tree_set_colour() -- set colors
int do_tree_set_colour(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		tr->set_foreground(convert_colour_spec(luaL_checkstring(L, 2)));
		tr->set_background(convert_colour_spec(luaL_checkstring(L, 3)));
	}
	else
		luaL_error(L, "do_tree_set_colour: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_expand( item )
int tree_expand(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle itm = lua_touserdata(L, 2);
		if (itm)
			tr->expand(itm);
		else
			luaL_error(L, "tree_expand: there is no tree_item at 1st arg");
	}
	else
		luaL_error(L, "tree_expand: there is no tree");
	return 0;
}

// tree_wnd:tree_collapse( item )
int tree_collapse(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle itm = lua_touserdata(L, 2);
		if (itm)
			tr->expand(itm);
		else
			luaL_error(L, "tree_collapse: there is no tree_item at 1st arg");
	}
	else
		luaL_error(L, "tree_collapse: there is no tree");
	return 0;
}

// tree_wnd:tree_remove_item( item )
int tree_remove_item(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle itm = lua_touserdata(L, 2);
		if (itm)
			tr->remove_item(itm);
		else
			luaL_error(L, "tree_remove_item: there is no tree_item at 1st arg");
	}
	else
		luaL_error(L, "tree_remove_item: there is no tree");
	return 0;
}

// tree_wnd:tree_remove_childs( item )
int tree_remove_childs(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle itm = lua_touserdata(L, 2);
		if (itm)
			tr->remove_childs(itm);
		else
			luaL_error(L, "tree_remove_childs: there is no tree_item at 1st arg");
	}
	else
		luaL_error(L, "tree_remove_childs: there is no tree");
	return 0;
}

// tree_wnd:set_insert_mode( item )
// tree_wnd:set_insert_mode( 'type one of "last, first, sort, root"' )
int tree_set_insert_mode(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		int type_id = lua_type(L, 2);
		switch (type_id)
		{
		case LUA_TSTRING:
			tr->insert_mode(lua_tostring(L, 2));
			break;
		case LUA_TLIGHTUSERDATA:
		{
			Handle item = lua_touserdata(L, 2);
			if (item)
			{
				tr->insert_mode(item);
				break;
			}
		}
		[[fallthrough]];
		default:
			tr->insert_mode("last");
		}
	}
	else
		luaL_error(L, "tree_set_insert_mode: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_get_item_parent() -- return handle parent of item
int tree_get_item_parent(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle hItem = lua_touserdata(L, 2);
		if (Handle hParent = tr->get_item_parent(hItem))
		{
			lua_pushlightuserdata(L, hParent);
			return 1;
		}
	}
	else
		luaL_error(L, "get_item_parent: there is no tree at 1st arg");
	return 0;
}
// tree_wnd:tree_get_item() -- return handle of item
int tree_get_item(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		const char* caption = luaL_checkstring(L, 2);
		Handle parent = lua_touserdata(L, 3);
		if (Handle sel_itm = tr->get_item_by_name(StringFromUTF8(caption).c_str(), parent))
		{
			lua_pushlightuserdata(L, sel_itm);
			return 1;
		}
	}
	else
		luaL_error(L, "tree_get_item: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_get_item_selected() -- return handle of selected item
int tree_get_item_selected(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle sel_itm = tr->get_selected();
		if (sel_itm)
			lua_pushlightuserdata(L, sel_itm);
		else
			lua_pushnil(L);
		return 1;
	}
	else
		luaL_error(L, "tree_get_item_selected: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_set_item_text( item_ud, caption )
int tree_set_item_text(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle ud = lua_touserdata(L, 2);
		if (ud)
		{
			gui_string txt = StringFromUTF8(luaL_checkstring(L, 3));
			tr->set_item_text(ud, txt.data());
		}
		else
			luaL_error(L, "tree_set_item_text: there is no tree at 2nd arg");
	}
	else
		luaL_error(L, "tree_set_item_text: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_get_item_text( item_ud )
int tree_get_item_text(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		Handle ud = lua_touserdata(L, 2);
		if (ud) {
			const gui_string str = tr->get_item_text(ud);
			lua_pushwstring(L, str);
			return 1;
		}
		else
			luaL_error(L, "tree_get_item_text: there is no treeitem at 2nd arg");
	}
	else
		luaL_error(L, "tree_get_item_text: there is no tree at 1st arg");
	return 0;
}

// tree_wnd:tree_get_item_data( item_ud )
int tree_get_item_data(lua_State* L)
{
	if (auto tr = check_treewnd(L)) {
		if (Handle ud = lua_touserdata(L, 2)) {
			if (int data = tr->get_item_data(ud))
				//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
				lua_reg_restore(L, data);
			else
				lua_pushnil(L);
			return 1;
		}
		else
			luaL_error(L, "tree_get_item_data: there is no treeitem at 2nd arg");
	}
	else
		luaL_error(L, "tree_get_item_data: there is no tree at 1st arg");
	return 0;
}

int do_tree_set_LabelEditable(lua_State* L)
{
	if (auto tr = check_treewnd(L))
		tr->makeLabelEditable(lua_toboolean(L, 2));
	else
		luaL_error(L, "tree_makeLabelEditable: there is no tree at 1st arg");
	return 0;
}

inline TListViewLua* list_window_arg(lua_State* L)
{
	return dynamic_cast<TListViewLua*>(window_arg(L));
}

int do_wnd_set_theme(lua_State* L)
{
	if (auto lst = list_window_arg(L))
		lst->set_theme(lua_toboolean(L, 2));
	else if (auto tree = check_treewnd(L))
		tree->set_theme(lua_toboolean(L, 2));
	else
		luaL_error(L, "do_wnd_set_theme: there is no tree or list at 1st arg");
	return 0;
}

// list_wnd:select_item( id )
// tree_wnd:select_item( item_ud )
int window_select_item(lua_State* L)
{
	if (auto lv = list_window_arg(L)) {
		lv->select_item((int)luaL_checkinteger(L, 2));
	}
	else if (auto tr = check_treewnd(L)) {
		tr->select(lua_touserdata(L, 2));
	}
	return 0;
}

const vecws table_to_str_array(lua_State* L, int idx)
{
	vecws res;
	if (!lua_istable(L, idx)) {
		throw_error(L, "table argument expected");
	}
	lua_pushnil(L); // first key
	while (lua_next(L, idx) != 0) {
		res.emplace_back(StringFromUTF8(lua_tostring(L, -1)));
		lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
	}
	return res;
}

// list_wnd:delete_item(index)
int window_delete_item(lua_State* L)
{
	if (auto lv = list_window_arg(L))
		lv->remove_item((int)luaL_checkinteger(L, 2));
	return 0;
}

void fill_menu(lua_State* L, Menu& mnu)
{
	const vecws items = table_to_str_array(L, 2);
	for (const auto& it : items) {
		size_t pos = it.find_first_of(L"|");
		if (pos == std::string::npos)
		{
			mnu.add_separator();
		}
		else
		{
			auto text = it.substr(0, pos);
			auto fun = it.substr(pos + 1);
			lua_pushwstring(L, fun);
			//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			int ref_idx = lua_reg_store(L);
			//lua_pop(L, 1);
			mnu << Item(text.data(), (EH)&LuaWindow::handler, (void*)ref_idx);
		}
	}
}

int window_context_menu(lua_State* L)
{
	TWin* w = window_arg(L);
	if (LuaWindow* cw = dynamic_cast<LuaWindow*> (w)) {
		ContextMenu mnu(cw);
		fill_menu(L, mnu);
	}
	else if (TNotifyWin* tc = dynamic_cast<TNotifyWin*>(w)) { //notify
		TEventWindow* parent = tc->get_parent_win();
		HMENU hm = CreatePopupMenu();
		Popup mnu(hm);
		fill_menu(L, mnu);
		tc->set_popup_menu(hm);
		mnu.get_menu_handler()->set_form(parent);
		parent->add_handler(mnu.get_menu_handler());
	}
	return 0;
}

int window_aux_item(lua_State* L, bool at_index)
{
	TWin* w = window_arg(L);
	if (TListViewLua* lv = dynamic_cast<TListViewLua*>(w)) {
		int ref_idx = 0;
		int next_arg = at_index ? 3 : 2;
		int ipos = at_index ? luaL_checkinteger(L, 2) : lv->count();
		if (!lua_isnoneornil(L, next_arg + 1)) {
			lua_pushvalue(L, next_arg + 1);
			//ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			ref_idx = lua_reg_store(L);
		}
		if (lua_isstring(L, next_arg)) {
			lv->add_item_at(ipos, StringFromUTF8(luaL_checkstring(L, next_arg)).c_str(), luaL_optinteger(L, next_arg + 2, 0), ref_idx); // single column init with string
		}
		else {
			vecws items = table_to_str_array(L, next_arg);
			const int _min = min(lv->columns(), items.size());
			int idx = lv->add_item_at(ipos, items.at(0).data(), 0, ref_idx); // init first column
			for (int i = 1; (i < _min) && items.at(i).size(); ++i) // init others
				lv->add_subitem(idx, items.at(i).data(), i);
		}
	}
	else if (TTreeViewLua* tv = dynamic_cast<TTreeViewLua*>(w)) {
		Handle parent = lua_touserdata(L, 3);
		int icon_idx = luaL_optinteger(L, 4, -1);
		int selicon_idx = luaL_optinteger(L, 5, icon_idx);
		int ref_idx = 0;
		if (!lua_isnoneornil(L, 6))
		{
			lua_pushvalue(L, 6);
			//ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			ref_idx = lua_reg_store(L);
		}
		Handle h = tv->add_item(StringFromUTF8(luaL_checkstring(L, 2)).c_str(), parent, icon_idx, selicon_idx, ref_idx);
		if (h) {
			lua_pushlightuserdata(L, h);
			return 1;
		}
	}
	return 0;
}

// list_wnd:add_item(text, data)
// list_wnd:add_item({text1,text2}, data)
// tree:add_item( text [, parent_item = root][, id_icon = -1][, id_selicon = -1][, data = null] )
int window_add_item(lua_State* L)
{
	return window_aux_item(L, false);
}

// list_wnd:insert_item(index, string)
int window_insert_item(lua_State* L)
{
	window_aux_item(L, true);
	return 0;
}

// list_wnd:add_column( sTitle, iSize)
// @param sTitle
// @param iWidth
int window_add_column(lua_State* L)
{
	if (auto lv = list_window_arg(L))
		lv->add_column(StringFromUTF8(luaL_checkstring(L, 2)).c_str(), (int)luaL_checkinteger(L, 3));
	return 0;
}

// list_wnd:get_item_text( index )
int window_get_item_text(lua_State* L)
{
	if (auto lv = list_window_arg(L))
	{
		std::wstring buff(MAX_PATH, 0);
		lv->get_item_text((int)luaL_checkinteger(L, 2), buff.data(), MAX_PATH);
		lua_pushwstring(L, buff);
		return 1;
	}
	return 0;
}

// list_wnd:get_item_data( index )
int window_get_item_data(lua_State* L)
{
	if (auto lv = list_window_arg(L))
	{
		int data = lv->get_item_data((int)luaL_checkinteger(L, 2));
		//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
		lua_reg_restore(L, data);
		return 1;
	}
	return 0;
}

// list_wnd:set_list_colour( sForeColor, sBackColor )
int window_set_colour(lua_State* L)
{
	if (auto ls = list_window_arg(L)) {
		ls->set_foreground(convert_colour_spec(luaL_checkstring(L, 2)));
		ls->set_background(convert_colour_spec(luaL_checkstring(L, 3)));
	}
	return 0;
}

// list_wnd:get_selected_item(index)
// @return index
int window_selected_item(lua_State* L)
{
	if (auto lv = list_window_arg(L))
		lua_pushinteger(L, lv->selected_id());
	else
		lua_pushinteger(L, -1);
	return 1;
}

// list_wnd:get_selected_items(index)
// @return {idx1, idx2, idx3,...}
int window_selected_items(lua_State* L)
{
	TListViewLua* lv = list_window_arg(L);
	int i = -1;
	int idx = 0;
	lua_newtable(L);
	while (lv) {
		i = lv->next_selected_id(i);
		if (i < 0) break;
		lua_pushinteger(L, i);
		lua_rawseti(L, -2, ++idx);
	}
	return 1;
}

/*list_wnd:selected_count()
* @return number of selected items
*/
int window_selected_count(lua_State* L)
{
	if (auto lv = list_window_arg(L))
		lua_pushinteger(L, lv->selected_count());
	else
		lua_pushinteger(L, 0);
	return 1;
}

/* gui.list([multiple_columns = false][, single_select = true])
* @param bMultiColumn [=false]
* @param bSingleSelect [=true]
* @return list window or error
*/
int new_list_window(lua_State* L)
{
	if (TEventWindow* p = reinterpret_cast<TEventWindow*>(get_last_parent())) {
		bool multiple_columns = optboolean(L, 1);
		bool single_select = optboolean(L, 2, true);
		TListViewLua* lv = new TListViewLua(p, L, multiple_columns, single_select);
		return wrap_window(L, lv);
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.list'");
		return 0;
	}
}

/* list_wnd:autosize( index [, by_contents = false])
*  @param iIndex
*  @param bFlag [=false]
*/
int window_autosize(lua_State* L)
{
	list_window_arg(L)->autosize_column((int)luaL_checkinteger(L, 2), optboolean(L, 3));
	return 0;
}

//////////////////////////
class TListBoxLua :public TListBox, public LuaControl
{
public:
	TListBoxLua(TWin* parent, lua_State* L, int id, bool is_sorted)
		: TListBox(parent, id, is_sorted), LuaControl(L)
	{};
	void clear() override
	{
		const int items_count = count();
		for (int i = 0; i < items_count; i++)
			//luaL_unref(L, LUA_REGISTRYINDEX, data);
			lua_reg_release(L, get_data(i));
		TListBox::clear();
	}
};

// create new listbox
int new_listbox(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		int id = luaL_checkinteger(L, 1);
		bool is_sorted = optboolean(L, 2);
		TListBoxLua* lb = new TListBoxLua(p, L, id, is_sorted);
		return wrap_window(L, lb);
	}
	else {
		luaL_error(L, "There is no parent panel to create 'new_listbox'");
		return 0;
	}
}

inline TListBoxLua* check_listbox(lua_State* L)
{
	return dynamic_cast<TListBoxLua*>(window_arg(L));
}

int do_listbox_insert(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		int id = luaL_checkinteger(L, 2) - 1;
		const char* val = luaL_checkstring(L, 3);
		lb->insert(id, StringFromUTF8(val).c_str());
		if (!lua_isnil(L, 4))
		{
			lua_pushvalue(L, 4);
			//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			int ref_idx = lua_reg_store(L);
			lb->set_data(id, ref_idx);
		}
	}
	else
		luaL_error(L, "argument 1 is not a ListBox");
	return 0;
}

int do_listbox_append(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		int id = lb->count();
		const char* val = luaL_checkstring(L, 2);
		lb->insert(id, StringFromUTF8(val).c_str());
		if (!lua_isnil(L, 3))
		{
			lua_pushvalue(L, 3);
			//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			int ref_idx = lua_reg_store(L);
			lb->set_data(id, ref_idx);
		}
	}
	else
		luaL_error(L, "argument 1 is not a ListBox");
	return 0;
}

int do_listbox_remove(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		int id = luaL_checkinteger(L, 2) - 1;
		//luaL_unref(L, LUA_REGISTRYINDEX, data);
		lua_reg_release(L, lb->get_data(id));
		lb->remove(id);
	}
	else
		luaL_error(L, "argument 1 is not a ListBox");
	return 0;
}

/*list_wnd:count()
* listbox:count()
* @return count of elements
*/
int do_list_elements_count(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		lua_pushinteger(L, lb->count());
		return 1;
	}
	else if (auto lv = list_window_arg(L))
	{
		int sz = lv->count();
		lua_pushinteger(L, sz);
		return 1;
	}
	else
		luaL_error(L, "argument 1 is not a ListBox or ListView");
	return 0;
}

int do_listbox_get_text(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		int id = luaL_checkinteger(L, 2) - 1;
		TCHAR tmp[MAX_PATH]{};
		lb->get_text(id, tmp);
		lua_pushwstring(L, tmp);
		return 1;
	}
	else
		luaL_error(L, "argument 1 is not a ListBox");
	return 0;
}

int do_listbox_get_data(lua_State* L)
{
	if (auto lb = check_listbox(L))
	{
		int id = luaL_checkinteger(L, 2) - 1;
		if (int data = lb->get_data(id))
		{
			//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
			lua_reg_restore(L, data);
			return 1;
		}
	}
	else
		luaL_error(L, "argument 1 is not a ListBox");
	return 0;
}

////////////////////////// generic methods for window //////////////////////////

// list_wnd:clear()
// tree_wnd:clear()
// listbox_wnd:clear()
int window_clear(lua_State* L)
{
	TWin* w = window_arg(L);
	if (TListViewLua* lv = dynamic_cast<TListViewLua*>(w)) {
		lv->clear();
	}
	else if (TTreeViewLua* tr = dynamic_cast<TTreeViewLua*>(w)) {
		tr->clear();
	}
	else if (TListBoxLua* lb = dynamic_cast<TListBoxLua*>(w)) {
		lb->clear();
	}
	return 0;
}

int do_wnd_selection(lua_State* L)
{
	if (TTrackBar* trb = dynamic_cast<TTrackBar*>(window_arg(L)))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, trb->sel_start());
			lua_pushinteger(L, trb->sel_end());
			return 2;
		}
		else
		{
			int imin = luaL_optinteger(L, 2, 0);
			int imax = luaL_optinteger(L, 3, 100);
			trb->selection(imin, imax);
		}
	}
	else if (auto lb = check_listbox(L))
	{
		if (lua_gettop(L) == 1)
		{
			lua_pushinteger(L, (lua_Integer)lb->selected() + 1);
			return 1;
		}
		else
		{
			// set selection
			int id = luaL_checkinteger(L, 2) - 1;
			lb->selected(id);
		}
	}
	return 0;
}

// wnd:on_tip(function or <name global function>)
int window_on_tip(lua_State* L)
{
	if (LuaControl* lc = dynamic_cast<LuaControl*>(window_arg(L)))
		lc->set_on_tip(2);
	else
		luaL_error(L, "argument 1 is not a LuaControl");
	return 0;
}

// wnd:on_select(function or <name global function>)
int window_on_select(lua_State* L)
{
	if (LuaControl* lc = dynamic_cast<LuaControl*>(window_arg(L)))
		lc->set_select(2);
	else
		luaL_error(L, "argument 1 is not a LuaControl");
	return 0;
}

// wnd:on_double_click(function or <name global function>)
int window_on_double_click(lua_State* L)
{
	if (LuaControl* lc = dynamic_cast<LuaControl*>(window_arg(L)))
		lc->set_double_click(2);
	else
		luaL_error(L, "argument 1 is not a LuaControl");
	return 0;
}

// wnd:on_close(function or <name global function>)
int window_on_close(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->set_on_close(2);
	return 0;
}

// wnd:on_command(function or <name global function>)
int window_on_command(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->set_on_command(2);
	return 0;
}

// wnd:on_scroll(function or <name global function>)
int window_on_scroll(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->set_on_scroll(2);
	return 0;
}

// wnd:on_timer(function, delay)
int window_on_timer(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->set_on_timer(2, luaL_optinteger(L, 3, 1) * 1000);
	return 0;
}

// wnd:stop_timer()
int window_stop_timer(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->kill_timer();
	return 0;
}

// wnd:on_show(function or <name global function>)
int window_on_show(lua_State* L)
{
	if (LuaWindow* cw = dynamic_cast<LuaWindow*>(window_arg(L)))
		cw->set_on_show(2);
	return 0;
}

// wnd:on_focus(function or <name global function>)
int window_on_focus(lua_State* L)
{
	if (LuaControl* lc = dynamic_cast<LuaControl*>(window_arg(L)))
		lc->set_on_focus(2);
	else
		luaL_error(L, "argument 1 is not a LuaControl");
	return 0;
}

// wnd:on_key(function or <name global function>)
int window_on_key(lua_State* L)
{
	TWin* w = window_arg(L);
	if (TListViewLua* lv = dynamic_cast<TListViewLua*>(w)) {
		lv->set_onkey(2);
	}
	else if (TMemoLua* memo = dynamic_cast<TMemoLua*>(w)) {
		memo->set_onkey(2);
	}
	else if (TTreeViewLua* tr = dynamic_cast<TTreeViewLua*>(w)) {
		tr->set_onkey(2);
	}
	return 0;
}

////////////// ComboBox //////////////

int new_combobox(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		int cmd_id = luaL_optinteger(L, 1, -1);
		DWORD style = luaL_optinteger(L, 2, CBS_DROPDOWN | CBS_AUTOHSCROLL);
		return wrap_window(L, new TComboBox(p, cmd_id, style));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.combobox'");
		return 0;
	}
}

inline TComboBox* check_combo(lua_State* L)
{
	return dynamic_cast<TComboBox*>(window_arg(L));
}

int do_cbox_clear(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		int cnt = cb->count();
		for (int idx = 0; idx < cnt; ++idx)
			//luaL_unref(L, LUA_REGISTRYINDEX, data);
			lua_reg_release(L, cb->get_data(idx));
		cb->reset();
	}
	return 0;
}

int do_cbox_append_string(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		const char* text = luaL_checkstring(L, 2);
		int idx = cb->add_string(StringFromUTF8(text).c_str());
		if (!lua_isnil(L, 3))
		{
			lua_pushvalue(L, 3);
			//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			int ref_idx = lua_reg_store(L);
			cb->set_data(idx, ref_idx);
		}
	}
	return 0;
}

int do_cbox_insert_string(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		int id = luaL_checkinteger(L, 2);
		const char* text = luaL_checkstring(L, 3);
		cb->ins_string(id, StringFromUTF8(text).c_str());
		if (!lua_isnil(L, 4))
		{
			lua_pushvalue(L, 4);
			//int ref_idx = luaL_ref(L, LUA_REGISTRYINDEX);
			int ref_idx = lua_reg_store(L);
			cb->set_data(id, ref_idx);
		}
	}
	return 0;
}

int do_cbox_get_data(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		int id = cb->get_cursel();
		if (int data = cb->get_data(id))
		{
			//lua_rawgeti(L, LUA_REGISTRYINDEX, data);
			lua_reg_restore(L, data);
			return 1;
		}
	}
	return 0;
}

int do_cbox_set_items_h(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		int sz = luaL_checkinteger(L, 2);
		cb->set_height(sz);
	}
	return 0;
}

int do_cb_setcursel(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		int sz = luaL_checkinteger(L, 2);
		cb->set_cursel(sz - 1);
	}
	return 0;
}

int do_cb_getcursel(lua_State* L)
{
	if (auto cb = check_combo(L))
	{
		lua_Integer idx = cb->get_cursel();
		lua_pushinteger(L, idx + 1);
		return 1;
	}
	return 0;
}

//////////////////////  EditBox   //////////////////////////////////

int new_editbox(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		const char* caption = luaL_optstring(L, 1, nullptr);
		DWORD style = luaL_optinteger(L, 2, 0);
		DWORD cmd_id = luaL_optinteger(L, 3, -1);
		return wrap_window(L, new TEdit(p, StringFromUTF8(caption).c_str(), cmd_id, style));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.editbox'");
		return 0;
	}
}

////////////////// Tool Tip ////////////////////////////////////////

int do_tooltip(lua_State* L)
{
	if (TWin* wnd = window_arg(L))
	{
		int id = luaL_checkinteger(L, 2);
		const char* text = luaL_checkstring(L, 3);
		bool balloon = optboolean(L, 4);
		wnd->set_tooltip(id, StringFromUTF8(text).c_str(), balloon);
	}
	return 0;
}

int do_get_ctrl_id(lua_State* L)
{
	TWin* w = window_arg(L);
	if (dynamic_cast<LuaControl*>(w))
		lua_pushinteger(L, w->get_ctrl_id());
	else
		lua_pushinteger(L, w->get_id());
	return 1;
}

/////////////////// status bar //////////////////////////////

//win_parent:statusbar(100, ..., 200,-1)
int do_statusbar(lua_State* L)
{
	if (TEventWindow* wnd = dynamic_cast<TEventWindow*>(window_arg(L)))
	{
		const int nbParts = lua_gettop(L) - 1;
		auto parts = std::make_unique<int[]>(nbParts);
		for (int idx = 0; idx < nbParts; idx++)
			parts[idx] = static_cast<int>(luaL_checkinteger(L, idx + 2));
		wnd->set_statusbar(nbParts, parts.get());
	}
	return 0;
}

// win_parent:status_setpart(part_id,"text")
int do_status_setpart(lua_State* L)
{
	if (TEventWindow* wnd = dynamic_cast<TEventWindow*>(window_arg(L)))
	{
		int part_id = luaL_checkinteger(L, 2);
		const char* text = luaL_checkstring(L, 3);
		wnd->set_statusbar_text(part_id, StringFromUTF8(text).c_str());
	}
	return 0;
}

//////////////////  ProgressControl  ///////////////////////////////
int new_progress(lua_State* L)
{
	if (TWin* p = get_last_parent()) {
		bool vertical = optboolean(L, 2);
		bool hasborder = optboolean(L, 3);
		bool smooth = optboolean(L, 4);
		bool smoothrevers = optboolean(L, 5);
		return wrap_window(L, new TProgressControl(p, -1, vertical, hasborder, smooth, smoothrevers));
	}
	else {
		luaL_error(L, "There is no parent panel to create 'gui.progress'");
		return 0;
	}
}

int do_set_progress_pos(lua_State* L)
{
	if (TProgressControl* wnd = dynamic_cast<TProgressControl*>(window_arg(L)))
		wnd->set_pos(luaL_checkinteger(L, 2));
	return 0;
}

int do_set_progress_range(lua_State* L)
{
	if (TProgressControl* wnd = dynamic_cast<TProgressControl*>(window_arg(L)))
		wnd->set_range(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
	return 0;
}

int do_progress_go(lua_State* L)
{
	if (TProgressControl* wnd = dynamic_cast<TProgressControl*>(window_arg(L)))
		wnd->go();
	return 0;
}

int do_progress_step(lua_State* L)
{
	if (TProgressControl* wnd = dynamic_cast<TProgressControl*>(window_arg(L)))
		wnd->set_step(luaL_checkinteger(L, 2));
	return 0;
}

////////////////////////////////////////////////////////////////////

int do_log(lua_State* L)
{
	const char* text = luaL_checkstring(L, 1);
	log_add(text);
	return 0;
}

int do_create_link(lua_State* L)
{
	gui_string path = StringFromUTF8(luaL_checkstring(L, 1));
	gui_string link = StringFromUTF8(luaL_checkstring(L, 2));
	gui_string wd = StringFromUTF8(lua_tostring(L, 3));
	if (wd.empty())
	{
		size_t pos = path.find_last_of(L'\\');
		wd = path.substr(0, pos);
	}
	gui_string descr = StringFromUTF8(lua_tostring(L, 4));
	int res = CreateShellLink(path.c_str(), link.c_str(),wd.c_str(), descr.c_str());
	lua_pushinteger(L, res);
	return 1;
}

int do_get_shell_folder(lua_State* L)
{
	int folder_id = luaL_checkinteger(L, 1);
	gui_string res = GetKnownFolder(folder_id);
	lua_pushwstring(L, res);
	return 1;
}

int do_get_scite_window(lua_State* L)
{
	return wrap_window(L, get_parent());
}

int do_get_desktop_window(lua_State* L)
{
	return wrap_window(L, get_desktop_window());
}
////////////////////////////////////////////////////////////////////

#include "luabinder.h"

int new_inifile(lua_State* L);
void lua_openclass_iniFile(lua_State* L);

int new_updown(lua_State* L);
void lua_openclass_UDCtrl(lua_State* L);

////////////////////////////////////////////////////////////////////

static const luaL_Reg gui[] = {
	//log
	{"log",				do_log},
	{"create_link",		do_create_link},
	{"get_folder",		do_get_shell_folder},

	// dialogs
	{"colour_dlg",		do_colour_dlg},
	{"message",			do_message},
	{"open_dlg",		do_open_dlg},
	{"save_dlg",		do_save_dlg},
	{"select_dir_dlg",	do_select_dir_dlg},
	{"prompt_value",	do_prompt_value},

	// others
	{"day_of_year",	do_day_of_year},
	{"ini_file",	new_inifile},
	{"version",		do_version},
	{"files",		do_files},
	{"chdir",		do_chdir},
	{"run",			do_run},
	{"get_ascii",	do_get_ascii},
	{"pass_focus",	do_pass_focus},
	{"set_panel",	do_set_panel},
	//{"test",		do_test_function},

	//windows 
	{"scite_window",do_get_scite_window},
	{"desktop",		do_get_desktop_window},
	{"window",		new_window},
	{"panel",		new_panel},
	{"tabbar",		new_tabbar},
	{"list",		new_list_window},
	{"memo",		new_memo},
	{"tree",		new_tree},
	{"button",		new_button},
	{"checkbox",	new_checkbox},
	{"radiobutton",	new_radiobutton},
	{"label",		new_label},
	{"trackbar",	new_trackbar},
	{"listbox",		new_listbox},
	{"combobox",	new_combobox},
	{"editbox",		new_editbox},
	{"progress",	new_progress},
	{"updown",		new_updown},
	{"groupbox",	new_groupbox},

	{NULL, NULL},
};

static const luaL_Reg window_methods[] = {
	// window
	{"show",			window_show},
	{"hide",			window_hide},
	{"size",			window_size},
	{"enable",			window_enable},
	{"position",		window_position},
	{"center_h",		window_center_h},
	{"center_v",		window_center_v},
	{"center",			window_center},
	{"resize",			window_resize},
	{"bounds",			window_get_bounds},
	{"client",			window_client},
	{"add",				window_add},
	{"remove_child",	window_remove},
	{"context_menu",	window_context_menu},
	{"tooltip",			do_tooltip},
	{"statusbar",		do_statusbar},
	{"status_setpart",	do_status_setpart},
	{"get_ctrl_id",		do_get_ctrl_id},

	// progress bar
	{"set_progress_pos",	do_set_progress_pos},
	{"set_progress_range",	do_set_progress_range},
	{"progress_go",			do_progress_go},
	{"set_step",			do_progress_step},

	{"on_close",	window_on_close},
	{"on_show",		window_on_show},

	//{"on_focus",window_on_focus},
	{"on_key",		window_on_key},
	{"on_command",	window_on_command},
	{"on_scroll",	window_on_scroll},
	{"on_timer",	window_on_timer},
	{"stop_timer",	window_stop_timer},

	// tabbar	
	{"add_tab",		tabbar_add},
	{"select_tab",	tabbar_sel},

	// list, tree
	{"on_select",			window_on_select},
	{"on_double_click",		window_on_double_click},
	{"on_tip",				window_on_tip},
	{"clear",				window_clear},
	{"set_selected_item",	window_select_item},
	{"count",				do_list_elements_count},
	{"delete_item",			window_delete_item},
	{"insert_item",			window_insert_item},
	{"add_item",			window_add_item},
	{"add_column",			window_add_column},
	{"get_item_text",		window_get_item_text},
	{"get_item_data",		window_get_item_data},
	{"set_list_colour",		window_set_colour},
	{"selected_count",		window_selected_count},
	{"get_selected_item",	window_selected_item},
	{"get_selected_items",	window_selected_items},
	{"autosize",			window_autosize},

	// tree
	{"set_tree_colour",		do_tree_set_colour},
	{"set_tree_editable",	do_tree_set_LabelEditable},
	{"set_theme",			do_wnd_set_theme},

	// tree, tab
	{"set_iconlib",			do_set_iconlib},

	// treeItem
	{"tree_get_item_selected",	tree_get_item_selected},
	{"tree_set_insert_mode",	tree_set_insert_mode},
	{"tree_remove_item",		tree_remove_item},
	{"tree_get_item",			tree_get_item},
	{"tree_get_item_parent",	tree_get_item_parent},
	{"tree_remove_childs",		tree_remove_childs},
	{"tree_get_item_text",		tree_get_item_text},
	{"tree_set_item_text",		tree_set_item_text},
	{"tree_get_item_data",		tree_get_item_data},
	{"tree_collapse",			tree_collapse},
	{"tree_expand",				tree_expand},

	// trackbar
	{"get_pos",		trbar_get_pos},
	{"set_pos",		trbar_set_pos},
	{"select",		do_wnd_selection},
	{"sel_clear",	trbar_sel_clear},
	{"range",		trbar_set_range},

	//memo
	{"set_memo_colour", memo_set_colour},

	// memo, label
	{"set_text",	window_set_text},
	{"get_text",	window_get_text},

	// label, button
	{"set_icon",	do_ctrl_set_icon},
	{"set_bitmap",	do_ctrl_set_bitmap},

	// button, checkbox, radio
	{"check",		do_check},

	// do_listbox_insert
	{"insert",			do_listbox_insert},
	{"append",			do_listbox_append},
	{"remove",			do_listbox_remove},
	{"get_line_text",	do_listbox_get_text},
	{"get_line_data",	do_listbox_get_data},

	// combobox
	{"cb_append",		do_cbox_append_string},
	{"cb_insert",		do_cbox_insert_string},
	{"cb_clear",		do_cbox_clear},
	{"cb_items_h",		do_cbox_set_items_h},
	{"cb_setcursel",	do_cb_setcursel},
	{"cb_getcursel",	do_cb_getcursel},
	{"cb_getdata",		do_cbox_get_data},

	{NULL, NULL},
};

void on_attach()
{
	// at this point, the SciTE window is available. Can't always assume
	// that it is the foreground window, so we hunt through all windows
	// associated with this thread (the main GUI thread) to find a window
	// matching the appropriate class name

	//EnumThreadWindows(GetCurrentThreadId(),CheckSciteWindow,(LPARAM)&hSciTE);
	hSciTE = FindWindow(L"SciTEWindow", NULL);
	s_parent = new TWin(hSciTE);

	// Its first child shold be the content pane (editor+output), 
	// but we check this anyway....	

	//EnumChildWindows(hSciTE,CheckContainerWindow,(LPARAM)&hContent);
	hContent = FindWindowEx(hSciTE, NULL, L"SciTEWindowContent", NULL);

	// the first child of the content pane is the editor pane.
	bool subclassed = false;
	if (hContent != NULL) {
		log_add("gui inited");
		content_window = new TWin(hContent);
		hCode = GetWindow(hContent, GW_CHILD);
		if (hCode != NULL) {
			code_window = new TWin(hCode);
			subclass_scite_window();
			subclassed = true;
		}
	}
	if (!subclassed) {
		get_parent()->message(L"Cannot subclass SciTE Window", 2);
	}
}

void destroy_windows()
{
	if (extra_window) {
		extra_window->hide();
		extra_window->set_parent(NULL);
		extra_window->close();
		//log_add("destroy extra_window");
		delete extra_window;
		extra_window = nullptr;
	}
	if (extra_window_splitter) {
		extra_window_splitter->hide();
		extra_window_splitter->set_parent(NULL);
		extra_window_splitter->close();
		//log_add("destroy extra_window_splitter");
		delete extra_window_splitter;
		extra_window_splitter = nullptr;
	}
	extra.bottom = extra.top = extra.left = extra.right = 0;
	//shake_scite_descendants();

	//log_add("clear collect");
	collect_windows.clear();
}

extern "C" __declspec(dllexport)
int luaopen_gui(lua_State * L)
{
	destroy_windows();

	reinit_storage(L);
	lua_openclass_iniFile(L); // IniFile
	lua_openclass_UDCtrl(L);  // UpDownControl

	luaL_newmetatable(L, WINDOW_CLASS);  // create metatable for window objects
	lua_pushvalue(L, -1);  // push metatable
	lua_setfield(L, -2, "__index");  // metatable.__index = metatable

#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, window_methods);
	luaL_openlib(L, "gui", gui, 0);
#else
	luaL_setfuncs(L, window_methods, 0);
	luaL_newlib(L, gui);
#endif

	lua_pushvalue(L, -1);  /* copy of module */
	lua_setglobal(L, "gui");
	return 1;
}

void on_destroy()
{
	log_add("gui:on_destroy");
	destroy_windows();
	//log_add("destroy s_parent");
	delete s_parent; //scite
	//log_add("destroy content_window");
	delete content_window;
	//log_add("destroy code_window");
	delete code_window; // editor
}