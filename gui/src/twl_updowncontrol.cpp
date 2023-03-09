//////////////////////////////
// UpDownControl
//////////////////////////////

#include "twl.h"
#include <CommCtrl.h>
#include "luabinder.h"

class TUpDownControl
{
	HWND m_ud;
public:
	TUpDownControl(TWin* parent, TWin* buddy, DWORD style);
	//~TUpDownControl();
	static const char* classname() { return "TUpDownControl"; }
	void set_range(int nUpper, int nLower);
	void set_current(int _pos);
};

TUpDownControl::TUpDownControl(TWin* parent, TWin* buddy, DWORD style)
{
	Rect rt;
	buddy->get_rect(rt, true);
	m_ud = CreateUpDownControl(WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_AUTOBUDDY | style,
		rt.right, rt.top, rt.height(), rt.height(),
		(HWND)parent->handle(), -1, hInst, (HWND)buddy->handle(), 10, 0, 5);
}

//TUpDownControl::~TUpDownControl()
//{
//	log_add("~updown");
//}

void TUpDownControl::set_range(int nUpper, int nLower)
{
	SendMessage(m_ud, UDM_SETRANGE32, nUpper, nLower);
}

void TUpDownControl::set_current(int _pos)
{
	SendMessage(m_ud, UDM_SETPOS, 0, _pos);
}

///////////////////////////////////
TWin* window_arg(lua_State* L, int idx = 1);
#define check_updown check_arg<TUpDownControl>

int updown_set_range(lua_State* L)
{
	if (TUpDownControl* updown = check_updown(L))
		updown->set_range(luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
	return 0;
}

int updown_set_current(lua_State* L)
{
	if (TUpDownControl* updown = check_updown(L))
		updown->set_current(luaL_checkinteger(L, 2));
	return 0;
}

const luaL_Reg LuaBinder<TUpDownControl>::metamethods[] =
{
	{ "__gc",			do_destroy<TUpDownControl> },
	{ NULL, NULL}
};

const luaL_Reg LuaBinder<TUpDownControl>::methods[] =
{
	{ "set_current",	updown_set_current},
	{ "set_range",		updown_set_range},
	{ NULL, NULL}
};

void lua_openclass_UDCtrl(lua_State* L)
{
	LuaBinder<TUpDownControl>().createClass(L);
}

int new_updown(lua_State* L)
{
	TWin* p = window_arg(L);
	if (!p)
	{
		luaL_error(L, "1st arg: There is no parent panel to create 'gui.do_updown'");
		return 0;
	}
	TWin* buddy_wnd = window_arg(L, 2);
	if (!buddy_wnd)
	{
		luaL_error(L, "2nd arg: There is no buddy window for updown control 'gui.do_updown'");
		return 0;
	}
	DWORD style = luaL_optinteger(L, 3, 0);
	//lua_push_newobject(L, new TUpDownControl(p, buddy_wnd, style));
	lua_push_newobject<TUpDownControl>(L, p, buddy_wnd, style);
	return 1;
}
