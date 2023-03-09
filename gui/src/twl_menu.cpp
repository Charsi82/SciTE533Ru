/* TWL_MENU.CPP
 * Steve Donovan, 2003
 * This is GPL'd software, and the usual disclaimers apply.
 * See LICENCE
 */
#include "twl_menu.h"
#include <string>

namespace {
	UINT last_id = 100;
}

Menu::Menu(TEventWindow* form)
	: m_form(form)
{
	m_menu_handler = new MessageHandler(form);
	create();
}

void Menu::create()
{
	if (m_form && m_form->get_menu()) {
		m_handle = m_form->get_menu();
		m_form = NULL;
	}
	else {
		m_handle = CreateMenu();
		m_accel_list = new AccelList;
	}
}

//#define EQ(s1,s2) (wcscmp(s1,s2)==0)

void Menu::add_menu(Item& item)
{
	// if (item.id == -1) item.id = last_id++;
	item.id = last_id++; //*new
	AppendMenu(m_handle, MF_STRING, item.id, item.caption);
	m_menu_handler->add(item);
	wchar_t buff[120]{};
	wcscpy_s(buff, item.caption);
	wchar_t* next_token = NULL;
	wcstok_s(buff, L"\t", &next_token);
	//wchar_t* astr = wcstok_s(NULL, L"", &next_token);
	//if (astr) {
	//	ACCEL accl;
	//	wchar_t* ckey, * vkey;
	//	ckey = wcstok_s(astr, L"-", &next_token);
	//	vkey = wcstok_s(NULL, L" ", &next_token);
	//	if (vkey == NULL) { vkey = ckey; ckey = NULL; }
	//	else _wcsupr(ckey);
	//	_wcsupr(vkey);
	//	int key;
	//	accl.fVirt = FVIRTKEY;
	//	if (*vkey == 'F' && *(vkey + 1) != '\0') {
	//		int fkey_id = _wtoi(vkey + 1);
	//		key = VK_F1 + fkey_id - 1;
	//	}
	//	else
	//		if (EQ(vkey, L"UP"))     key = VK_UP; else
	//			if (EQ(vkey, L"DOWN"))   key = VK_DOWN; else
	//				if (EQ(vkey, L"RIGHT"))  key = VK_RIGHT; else
	//					if (EQ(vkey, L"LEFT"))   key = VK_LEFT; else
	//						if (EQ(vkey, L"DELETE")) key = VK_DELETE;
	//						else {
	//							//accl.fVirt = 0;
	//							key = (int)vkey[0];
	//						}
	//	accl.key = (WORD)key;
	//	if (ckey != NULL) {
	//		if (EQ(ckey, L"CTRL")) accl.fVirt |= FCONTROL; else
	//			if (EQ(ckey, L"ALT")) accl.fVirt |= FALT; else
	//				if (EQ(ckey, L"SHIFT")) accl.fVirt |= FSHIFT;
	//	}
	//	accl.cmd = (WORD)item.id;
	//	//if (accl.fVirt & FVIRTKEY)  // for now...
	//	m_accel_list->push_back(accl);
	//}
}

void Menu::add_menu(Menu& menu)
{
	wchar_t* name = ((Popup&)menu).name();
	AppendMenu(m_handle, MF_STRING | MF_POPUP, (UINT_PTR)menu.m_handle, name);
	m_menu_handler->add_handler(menu.m_menu_handler);
	m_accel_list->splice(m_accel_list->end(), *menu.m_accel_list);
}

void Menu::add_separator()
{
	AppendMenu(m_handle, MF_SEPARATOR, 0, NULL);
}

MessageHandler* Menu::get_menu_handler()
{
	return m_menu_handler;
}

void Menu::insert_menu(UINT id_before, Item& item)
{
	if (item.id == -1) item.id = last_id++;
	InsertMenu(m_handle, id_before, MF_STRING, item.id, item.caption);
	m_menu_handler->add(item);
}

void Menu::delete_menu(UINT id)
{
	DeleteMenu(m_handle, id, MF_BYCOMMAND);
	m_menu_handler->remove(id);
}

void Menu::release()
{
	if (!m_form) return;
	m_form->set_menu(m_handle);
	m_form->add_handler(m_menu_handler);
	if (m_accel_list->size()) {
		ACCEL* accels = new ACCEL[m_accel_list->size() * sizeof(ACCEL)];
		//AccelList::iterator ali;
		int i = 0;
		//for(ali = m_accel_list->begin(); ali != m_accel_list->end(); ++ali)
			//accels[i++] = *ali;
		for (const auto& al : *m_accel_list)
		{
			accels[i++] = al;
		}
		HACCEL hAccel = CreateAcceleratorTable(accels, i);
		m_form->add_accelerator(hAccel);
		delete[] accels;
	}
}

Popup::Popup(wchar_t* name) :Menu(NULL), m_name(name)
{}

Popup::Popup(HMENU h)
	: Menu(NULL), m_name((wchar_t*)L"")
{
	m_handle = h;
}

void Popup::create()
{
	m_handle = CreatePopupMenu();
}

void Popup::release()
{
	// does nothing!!
}

PopupMenu::PopupMenu(TEventWindow* form, int index)
	: Menu(form), m_index(index)
{
	m_handle = GetSubMenu(m_handle, m_index);
}

void PopupMenu::create() {}

void PopupMenu::release() {}

ContextMenu::ContextMenu(TEventWindow* form)
	: Menu(NULL), m_main_form(form)
{}

void ContextMenu::create()
{
	m_handle = CreatePopupMenu();
}

void ContextMenu::release()
{
	// save the popup handle
	Popup pop(m_handle);
	// create a top-level menu and add the popup to it...
	Menu::create();
	add_menu(pop);
	// restore the popup handle and add it to the form
	m_handle = pop.get_handle();
	m_main_form->set_popup_menu(m_handle);
	m_menu_handler->set_form(m_main_form);
	m_main_form->add_handler(m_menu_handler);
}

Item::Item(wchar_t* _caption, EventHandler _handler, void* _data, UINT _id, bool inactive)
	: caption(_caption), handler(_handler), data(_data), id(_id), inactive_data(inactive)
{ }

MessageHandler::MessageHandler(TEventWindow* form)
	: AbstractMessageHandler(form)
{
	m_list = new ItemList;
}

MessageHandler::~MessageHandler()
{
	m_list->clear();
	delete m_list;
}

void MessageHandler::add(Item& item)
{
	if (item.id == -1) item.id = last_id++;
	m_list->push_back(item);
}

void MessageHandler::remove(UINT id)
{
	m_list->remove_if([&](const Item& item) { return item.id == id; });
}

bool MessageHandler::dispatch(UINT id, int notify, Handle ctrl_handle)
{
	auto it = std::find_if(m_list->begin(), m_list->end(), [&](const Item& item) {return item.id == id; });
	if (it != m_list->end())
	{
		Item& item = *it;
		if (item.handler) {
			if (item.data) {
				DataEventHandler handler = (DataEventHandler)item.handler;
				(m_form->*handler)(&item);
			}
			else
				(m_form->*item.handler)();
			return true;  // handled with an action
		}
		return true;// found, but no action
	}
	return false;
}

void MessageHandler::read()
{
	//for (Item& itm : *m_list)
	//{
	//	void* data = itm.data;
	//	if (data && !itm.inactive_data)
	//		PData(data)->read();
	//}
}

void MessageHandler::write()
{
	//try {
	//	for (Item& itm : *m_list)
	//		if (itm.data) PData(itm.data)->write();
	//}
	//catch (...) {
	//	m_form->message(L"Bad Number!", MSG_ERROR);
	//}
}

void MessageHandler::add_handler(AbstractMessageHandler* _hndlr)
{
	if (MessageHandler* hndlr = (MessageHandler*)_hndlr)
		m_list->splice(m_list->end(), *hndlr->m_list);
}
