#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <memory>

//typedef std::wstring gui_string;
std::wstring StringFromUTF8(const char* s);
std::string UTF8FromString(const std::wstring& s);
