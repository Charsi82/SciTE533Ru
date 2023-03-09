#pragma once
typedef int (TEventWindow::* NotifyEventHandler)(intptr_t id);
typedef NotifyEventHandler NEH;
#include <vector>

class TTabControl : public TNotifyWin, public THasIconWin {
	int m_index;
	int m_last_selected_idx;
	std::vector<TWin*> panels;
public:
	TTabControl(TEventWindow* form, bool multiline = false);
	~TTabControl();
	void add(wchar_t* caption, void* data, int image_idx = -1);
	void remove(int idx = -1);
	void* get_data(int idx = -1);
	void selected(int idx);
	int selected();
	int load_icons(pchar path, bool small_size = true) override;
	int getRowCount() const;

	virtual void handle_select(int id) = 0;
	virtual int handle_rclick(int id) = 0;
	// override
	int handle_notify(void* p) override;
};
