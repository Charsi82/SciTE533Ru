// twl_dialogs.h
#pragma once
#include "utf.h"
#include <windows.h>

bool run_colordlg(HWND win, COLORREF& cl);
bool run_ofd(HWND win, TCHAR* result, const gui_string& caption, const gui_string& filter, bool multi = false);
bool run_seldirdlg(HWND win, TCHAR* result, pchar descr, pchar initial_dir);
HRESULT CreateShellLink(LPCWSTR pszShortcutFile, LPCWSTR pszLink, LPCWSTR pszWorkingDir, LPCWSTR pszDesc);
gui_string GetKnownFolder(int folder_id);