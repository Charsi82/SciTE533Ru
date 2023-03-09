//build@ gcc -shared -o shell.dll -I shell.cpp scite.la -lstdc++

#include <shlwapi.h>
#include "utf.h" // Needs for WideString and UTF8 conversions
#include "md5.h"
#include "sha-256.h"
#include "lua.hpp"

// включает экспорт функий shell.to_utf8 и shell.from_utf8
//#define STRFUNC_EXROPT

class CPath
{
public:
	CPath(const wchar_t* lpszFileName)
	{
		if (lpszFileName)
		{
			// сохраняем оригинал
			m_sPathOriginal.append(lpszFileName);

			if (::PathIsURL(lpszFileName))
			{
				m_sPath.append(lpszFileName);
			}
			else // делаем преобразования
			{
				// 1. Раскрываем переменные окружения
				//CMemBuffer< wchar_t, 1024 > sExpanded;
				std::wstring sExpanded(1024, 0);
				::ExpandEnvironmentStrings(lpszFileName, sExpanded.data(), 1024);
				// 2. Убираем в пути .. и . (приводим к каноническому виду)
				std::wstring  sCanonical(1024, 0);
				::PathCanonicalize(sCanonical.data(), sExpanded.data());
				// 3. Убираем лишние пробелы
				::PathRemoveBlanks(sCanonical.data());
				// 4. Проверяем существует ли преобразованный путь
				if (::PathFileExists(sCanonical.data()) == TRUE)
				{
					::PathMakePretty(sCanonical.data());
					::PathRemoveBackslash(sCanonical.data());
					m_sPath.append(sCanonical.data());
					if (::PathIsDirectory(sCanonical.data()) == FALSE)
					{
						m_sFileName.append(::PathFindFileName(sCanonical.data()));
						::PathRemoveFileSpec(sCanonical.data());
					}
					m_sPathDir.append(sCanonical);
				}
				else
				{
					// 5. Отделяем аргументы
					wchar_t* pArg = ::PathGetArgs(sCanonical.data());
					m_sFileParams.append(pArg);
					::PathRemoveArgs(sCanonical.data());
					// 6. Делаем путь по красивше
					::PathUnquoteSpaces(sCanonical.data());
					::PathRemoveBackslash(sCanonical.data());
					::PathMakePretty(sCanonical.data());
					// 7. Проверяем преобразованный путь это дирректория
					if (::PathIsDirectory(sCanonical.data()) != FALSE)
					{
						m_sPath.append(sCanonical);
						m_sPathDir.append(sCanonical);
					}
					else
					{
						// 8. Добавляем расширение к файлу .exe, если нету
						::PathAddExtension(sCanonical.data(), NULL);
						// 9. Проверяем есть ли такой файл
						if (::PathFileExists(sCanonical.data()))
						{
							m_sPath.append(sCanonical.data());
							m_sFileName.append(::PathFindFileName(sCanonical.data()));
							::PathRemoveFileSpec(sCanonical.data());
							m_sPathDir.append(sCanonical);
						}
						else
						{
							// 10. Производим поиск
							::PathFindOnPath(sCanonical.data(), NULL);
							::PathMakePretty(sCanonical.data());
							m_sPath.append(sCanonical);
							if (::PathFileExists(sCanonical.data()) == TRUE)
							{
								m_sFileName.append(::PathFindFileName(sCanonical.data()));
								::PathRemoveFileSpec(sCanonical.data());
								m_sPathDir.append(sCanonical);
							}
						}
					}
				}
			}
		}
	}

	const wchar_t* GetPath()
	{
		return m_sPath.data();
	}

	const wchar_t* GetDirectory()
	{
		return m_sPathDir.data();
	}

	const wchar_t* GetFileParams()
	{
		return m_sFileParams.data();
	}

private:
	std::wstring m_sPathOriginal;
	std::wstring m_sPath;
	std::wstring m_sPathDir;
	std::wstring m_sFileName;
	std::wstring m_sFileParams;

public:
	static DWORD GetFileAttributes(const wchar_t* lpszFileName)
	{
		WIN32_FILE_ATTRIBUTE_DATA fad{};
		if (::GetFileAttributesEx(lpszFileName, GetFileExInfoStandard, &fad) == FALSE)
		{
			return ((DWORD)-1); //INVALID_FILE_ATTRIBUTES;
		}
		return fad.dwFileAttributes;
	}
	static BOOL SetFileAttributes(const wchar_t* lpszFileName, DWORD dwFileAttributes)
	{
		return ::SetFileAttributes(lpszFileName, dwFileAttributes);
	}
	static BOOL IsDirectory(const wchar_t* lpszFileName)
	{
		return ::PathIsDirectory(lpszFileName) != FALSE;
	}
	static BOOL IsFileExists(const wchar_t* lpszFileName)
	{
		return IsPathExist(lpszFileName) == TRUE &&
			IsDirectory(lpszFileName) == FALSE;
	}
	static BOOL IsPathExist(const wchar_t* lpszFileName)
	{
		return ::PathFileExists(lpszFileName) != FALSE;
	}
};

// получить последнее сообщение об ошибке
static std::wstring GetLastErrorString(DWORD* lastErrorCode, size_t* iLenMsg)
{
	LPWSTR lpMsgBuf = NULL;
	*lastErrorCode = ::GetLastError();
	::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		*lastErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf,
		0,
		NULL);

	*iLenMsg = wcslen(lpMsgBuf);

	// trim right
	while (*iLenMsg > 0)
	{
		(*iLenMsg)--;
		if (lpMsgBuf[*iLenMsg] == L'\n' ||
			lpMsgBuf[*iLenMsg] == L'\r' ||
			lpMsgBuf[*iLenMsg] == L'.' ||
			lpMsgBuf[*iLenMsg] == L' ')
		{
			lpMsgBuf[*iLenMsg] = 0;
		}
		else
		{
			break;
		}
	}
	(*iLenMsg)++;
	std::wstring res(lpMsgBuf);
	LocalFree(lpMsgBuf);
	return res;
}

static void lua_pushlasterr(lua_State* L, const wchar_t* lpszFunction)
{
	DWORD dw;
	size_t iLenMsg = 0;
	const std::wstring lpMsgBuf = GetLastErrorString(&dw, &iLenMsg);

	if (lpszFunction == NULL)
	{
		lua_pushstring(L, UTF8FromString(lpMsgBuf).c_str());
	}
	else
	{
		size_t uBytes = (iLenMsg + wcslen(lpszFunction) + 40);
		auto lpDisplayBuf = std::make_unique<wchar_t[]>(1024);
		swprintf_s(lpDisplayBuf.get(), uBytes, L"%s failed with error %lu: %s", lpszFunction, dw, lpMsgBuf.c_str());
		lua_pushstring(L, UTF8FromString(lpDisplayBuf.get()).c_str());
	}
}

static int msgbox(lua_State* L)
{
	auto text = StringFromUTF8(luaL_checkstring(L, 1));
	auto title = StringFromUTF8(luaL_optstring(L, 2, "SciTE"));
	UINT options = (UINT)luaL_optnumber(L, 3, 0) | MB_TASKMODAL;
	int retCode = ::MessageBox(NULL, text.data(), title.data(), options);
	lua_pushinteger(L, retCode);
	return 1;
}

static int getfileattr(lua_State* L)
{
	auto FN = StringFromUTF8(luaL_checkstring(L, -1));
	lua_pushinteger(L, CPath::GetFileAttributes(FN.data()));
	return 1;
}

static int setfileattr(lua_State* L)
{
	auto FN = StringFromUTF8(luaL_checkstring(L, -2));
	auto attr = luaL_checkinteger(L, -1); //luaL_checkint
	lua_pushboolean(L, CPath::SetFileAttributes(FN.data(), (DWORD)attr));
	return 1;
}

static int fileexists(lua_State* L)
{
	auto FN = StringFromUTF8(luaL_checkstring(L, 1));
	lua_pushboolean(L, CPath::IsPathExist(FN.data()));
	return 1;
}

//static int internalConv(lua_State* L, bool toUTF8)
//{
//	bool success = false;
//	if (lua_isstring(L, 1))
//	{
//		size_t len;
//		const char* src = lua_tolstring(L, 1, &len);
//		int cp = (int)luaL_optinteger(L, 2, CP_ACP);
//		const auto wstr = StringFromUTF8(src, toUTF8 ? cp : CP_UTF8);
//		if (wstr.size())
//		{
//			auto str = UTF8FromString(wstr, toUTF8 ? CP_UTF8 : cp);
//			if (str.size())
//			{
//				lua_pushstring(L, str.c_str());
//				success = true;
//			}
//		}
//	}
//	if (!success)
//		lua_pushnil(L);
//	return 1;
//}

std::string ConvertFromUTF8(const std::string& s, int codePage) {
	if (codePage == CP_UTF8) {
		return s;
	}
	else {
		std::wstring sWide = StringFromUTF8(s.c_str());
		int len = static_cast<int>(sWide.length());
		int cchMulti = ::WideCharToMultiByte(codePage, 0, sWide.c_str(), len, NULL, 0, NULL, NULL);
		//char* pszMulti = new char[cchMulti + 1];
		std::string ret(cchMulti + 1, 0);
		::WideCharToMultiByte(codePage, 0, sWide.c_str(), len, ret.data(), cchMulti + 1, NULL, NULL);
		//pszMulti[cchMulti] = 0;
		//delete[]pszMulti;
		return ret;
	}
}

std::string ConvertToUTF8(const std::string& s, int codePage) {
	if (codePage == CP_UTF8) {
		return s;
	}
	else {
		const char* original = s.c_str();
		int cchWide = ::MultiByteToWideChar(codePage, 0, original, -1, NULL, 0);
		//wchar_t* pszWide = new wchar_t[cchWide + 1];
		std::wstring sWide(cchWide + 1, 0);
		::MultiByteToWideChar(codePage, 0, original, -1, sWide.data(), cchWide + 1);
		//std::string ret = UTF8FromString(sWide);
		//delete[]pszWide;
		return UTF8FromString(sWide);
	}
}

#ifdef STRFUNC_EXROPT
static int from_utf8(lua_State* L)
{
	if (!lua_isstring(L, 1)) return 0;
	const std::string tmp = lua_tostring(L, 1);
	const int cp = (int)luaL_optinteger(L, 2, CP_ACP);
	lua_pushstring(L, ConvertFromUTF8(tmp, cp).c_str());
	return 1;
}

static int to_utf8(lua_State* L)
{
	if (!lua_isstring(L, 1)) return 0;
	const std::string tmp = lua_tostring(L, 1);
	const int cp = (int)luaL_optinteger(L, 2, CP_ACP);
	lua_pushstring(L, ConvertToUTF8(tmp, cp).c_str());
	return 1;
}
#endif

// запустить через CreateProcess в скрытом режиме
static bool RunProcessHide(CPath& path, DWORD* out_exitcode, std::wstring& strOut)
{
	static constexpr int MAX_CMD = 1024;

	STARTUPINFOW si{};
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	// устанавливаем именованные каналы на потоки ввода/вывода
	BOOL bUsePipes = FALSE;
	HANDLE FWritePipe = NULL;
	HANDLE FReadPipe = NULL;
	SECURITY_ATTRIBUTES pa = { sizeof(pa), NULL, TRUE };
	bUsePipes = ::CreatePipe(&FReadPipe, &FWritePipe, &pa, 0);
	if (bUsePipes != FALSE)
	{
		si.hStdOutput = FWritePipe;
		si.hStdInput = FReadPipe;
		si.hStdError = FWritePipe;
		si.dwFlags = STARTF_USESTDHANDLES | si.dwFlags;
	}

	// запускаем процесс
	std::wstring bufCmdLine(MAX_CMD, 0); // строковой буфер длиной MAX_CMD
	//wcscat_s(bufCmdLine.data(), MAX_CMD, L"\"");
	bufCmdLine.append(L"\"");
	//wcscat_s(bufCmdLine.data(), MAX_CMD, path.GetPath());
	bufCmdLine.append(path.GetPath());
	//wcscat_s(bufCmdLine.data(), MAX_CMD, L"\"");
	bufCmdLine.append(L"\"");
	if (path.GetFileParams())
	{
		//wcscat_s(bufCmdLine.data(), MAX_CMD, L" ");
		bufCmdLine.append(L" ");
		//wcscat_s(bufCmdLine.data(), MAX_CMD, path.GetFileParams());
		bufCmdLine.append(path.GetFileParams());
	}

	PROCESS_INFORMATION pi = { };
	BOOL RetCode = ::CreateProcess(NULL, // не используем имя файла, все в строке запуска
		bufCmdLine.data(), // строка запуска
		NULL, // Process handle not inheritable
		NULL, // Thread handle not inheritable
		TRUE, // Set handle inheritance to FALSE
		0, // No creation flags
		NULL, // Use parent's environment block
		NULL, //path.GetDirectory(), // устанавливаем дирректорию запуска
		&si, // STARTUPINFO
		&pi); // PROCESS_INFORMATION

	// если провалили запуск сообщаем об ошибке
	if (RetCode == FALSE)
	{
		::CloseHandle(FReadPipe);
		::CloseHandle(FWritePipe);
		return FALSE;
	}

	// закрываем описатель потока, в нем нет необходимости
	::CloseHandle(pi.hThread);

	// ожидаем завершение работы процесса
	try
	{
		DWORD BytesToRead = 0;
		DWORD BytesRead = 0;
		DWORD TotalBytesAvail = 0;
		DWORD PipeReaded = 0;
		DWORD exit_code = 0;
		std::string bufStr(MAX_CMD, 0); // строковой буфер длиной MAX_CMD
		while (::PeekNamedPipe(FReadPipe, NULL, 0, &BytesRead, &TotalBytesAvail, NULL))
		{
			if (TotalBytesAvail == 0)
			{
				if (::GetExitCodeProcess(pi.hProcess, &exit_code) == FALSE ||
					exit_code != STILL_ACTIVE)
				{
					break;
				}
				else
				{
					continue;
				}
			}
			else
			{
				while (TotalBytesAvail > BytesRead)
				{
					if (TotalBytesAvail - BytesRead > MAX_CMD - 1)
					{
						BytesToRead = MAX_CMD - 1;
					}
					else
					{
						BytesToRead = TotalBytesAvail - BytesRead;
					}
					if (::ReadFile(FReadPipe,
						bufStr.data(),
						BytesToRead,
						&PipeReaded,
						NULL) == FALSE)
					{
						break;
					}
					if (PipeReaded <= 0) continue;
					BytesRead += PipeReaded;
					bufStr[PipeReaded] = '\0';
					//MB2W wc(bufStr.data(), 866);
					strOut += StringFromUTF8(ConvertFromUTF8(bufStr, CP_OEMCP).c_str()); // Текущая кодовая страница OEM системы
				}
			}
		}
	}
	catch (...)
	{
	}

	// Код завершения процесса
	::GetExitCodeProcess(pi.hProcess, out_exitcode);
	::CloseHandle(pi.hProcess);
	::CloseHandle(FReadPipe);
	::CloseHandle(FWritePipe);
	return TRUE;
}

// запустить через ShellExecuteEx в скрытом режиме
// (см. шаманство с консолью)
static bool ExecuteHide(CPath& path, DWORD* out_exitcode, std::wstring& strOut)
{
	HANDLE hSaveStdin = NULL;
	HANDLE hSaveStdout = NULL;
	HANDLE hChildStdoutRdDup = NULL;
	HANDLE hChildStdoutWr = NULL;
	try
	{
		// подключаем консоль
		STARTUPINFO si{};
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi = { };
		wchar_t command_line[] = L"cmd";
		::CreateProcess(NULL, // не используем имя файла, все в строке запуска
			command_line, // Command line
			NULL, // Process handle not inheritable
			NULL, // Thread handle not inheritable
			TRUE, // Set handle inheritance to FALSE
			0, // No creation flags
			NULL, // Use parent's environment block
			NULL, // Use parent's starting directory
			&si, // STARTUPINFO
			&pi); // PROCESS_INFORMATION
		// задержка чтобы консоль успела создаться
		::WaitForSingleObject(pi.hProcess, 100);
		BOOL hResult = FALSE;
		HMODULE hLib = LoadLibrary(L"Kernel32.dll");
		if (hLib != NULL)
		{
			typedef BOOL(STDAPICALLTYPE* ATTACHCONSOLE)(DWORD dwProcessId);
			ATTACHCONSOLE _AttachConsole = NULL;
			_AttachConsole = (ATTACHCONSOLE)GetProcAddress(hLib, "AttachConsole");
			if (_AttachConsole) hResult = _AttachConsole(pi.dwProcessId);
			FreeLibrary(hLib);
		}
		if (hResult == FALSE) AllocConsole();

		TerminateProcess(pi.hProcess, 0);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		HANDLE hChildStdinRd;
		HANDLE hChildStdinWr;
		HANDLE hChildStdinWrDup;
		HANDLE hChildStdoutRd;

		// Set the bInheritHandle flag so pipe handles are inherited.
		SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
		BOOL fSuccess;

		// The steps for redirecting child process's STDOUT:
		//     1. Save current STDOUT, to be restored later.
		//     2. Create anonymous pipe to be STDOUT for child process.
		//     3. Set STDOUT of the parent process to be write handle to
		//        the pipe, so it is inherited by the child process.
		//     4. Create a noninheritable duplicate of the read handle and
		//        close the inheritable read handle.

		// Save the handle to the current STDOUT.
		hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE);

		// Create a pipe for the child process's STDOUT.
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) throw(1);

		// Set a write handle to the pipe to be STDOUT.
		if (!SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr)) throw(1);

		// Create noninheritable read handle and close the inheritable read
		// handle.
		fSuccess = DuplicateHandle(GetCurrentProcess(),
			hChildStdoutRd,
			GetCurrentProcess(),
			&hChildStdoutRdDup,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if (fSuccess == FALSE) throw(1);
		CloseHandle(hChildStdoutRd);

		// The steps for redirecting child process's STDIN:
		//     1.  Save current STDIN, to be restored later.
		//     2.  Create anonymous pipe to be STDIN for child process.
		//     3.  Set STDIN of the parent to be the read handle to the
		//         pipe, so it is inherited by the child process.
		//     4.  Create a noninheritable duplicate of the write handle,
		//         and close the inheritable write handle.

		// Save the handle to the current STDIN.
		hSaveStdin = GetStdHandle(STD_INPUT_HANDLE);

		// Create a pipe for the child process's STDIN.
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) throw(1);

		// Set a read handle to the pipe to be STDIN.
		if (!SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd)) throw(1);

		// Duplicate the write handle to the pipe so it is not inherited.
		fSuccess = DuplicateHandle(GetCurrentProcess(),
			hChildStdinWr,
			GetCurrentProcess(),
			&hChildStdinWrDup,
			0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if (fSuccess == FALSE) throw(1);

		CloseHandle(hChildStdinWr);
	}
	catch (...)
	{
		return FALSE;
	}

	// Now create the child process.
	SHELLEXECUTEINFOW shinf{};
	shinf.cbSize = sizeof(SHELLEXECUTEINFOW);
	shinf.lpFile = path.GetPath();
	shinf.lpParameters = path.GetFileParams();
	//shinf.lpDirectory = path.GetDirectory();
	shinf.fMask = SEE_MASK_FLAG_NO_UI |
		SEE_MASK_NO_CONSOLE |
		SEE_MASK_FLAG_DDEWAIT |
		SEE_MASK_NOCLOSEPROCESS;
	shinf.nShow = SW_HIDE;
	BOOL bSuccess = ::ShellExecuteEx(&shinf);
	if (bSuccess && shinf.hInstApp <= (HINSTANCE)32) bSuccess = FALSE;
	HANDLE hProcess = shinf.hProcess;

	try
	{
		if (bSuccess == FALSE || hProcess == NULL) throw(1);

		if (hChildStdoutWr != NULL)
		{
			CloseHandle(hChildStdoutWr);
			hChildStdoutWr = NULL;
		}

		// After process creation, restore the saved STDIN and STDOUT.
		if (hSaveStdin != NULL)
		{
			if (!SetStdHandle(STD_INPUT_HANDLE, hSaveStdin)) throw(1);
			CloseHandle(hSaveStdin);
			hSaveStdin = NULL;
		}

		if (hSaveStdout != NULL)
		{
			if (!SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout)) throw(1);
			CloseHandle(hSaveStdout);
			hSaveStdout = NULL;
		}

		if (hChildStdoutRdDup != NULL)
		{
			// Read output from the child process, and write to parent's STDOUT.
			const int BUFSIZE = 1024;
			DWORD dwRead;
			//CMemBuffer< wchar_t, BUFSIZE > bufStr; // строковой буфер
			std::wstring bufStr(BUFSIZE, 0);
			//CMemBuffer< wchar_t, BUFSIZE > bufCmdLine; // строковой буфер
			std::wstring bufCmdLine(BUFSIZE, 0);
			for (;;)
			{
				if (ReadFile(hChildStdoutRdDup,
					bufCmdLine.data(),
					BUFSIZE,
					&dwRead,
					NULL) == FALSE ||
					dwRead == 0)
				{
					DWORD exit_code = 0;
					if (::GetExitCodeProcess(hProcess, &exit_code) == FALSE ||
						exit_code != STILL_ACTIVE)
					{
						break;
					}
					else
					{
						continue;
					}
				}
				bufCmdLine[dwRead] = '\0';
				//::OemToAnsi( bufCmdLine.data(), bufStr.data() );
				strOut.append(bufCmdLine/*bufStr.data()*/);
			}
			CloseHandle(hChildStdoutRdDup);
			hChildStdoutRdDup = NULL;
		}
		FreeConsole();
	}
	catch (...)
	{
		if (hChildStdoutWr != NULL) CloseHandle(hChildStdoutWr);
		if (hSaveStdin != NULL) CloseHandle(hSaveStdin);
		if (hSaveStdout != NULL) CloseHandle(hSaveStdout);
		if (hChildStdoutRdDup != NULL) CloseHandle(hChildStdoutRdDup);
		if (bSuccess == FALSE || hProcess == NULL) return FALSE;
	}

	::GetExitCodeProcess(hProcess, out_exitcode);
	CloseHandle(hProcess);
	return TRUE;
}

static int exec(lua_State* L)
{
	// считываем запускаемую команду
	CPath file(StringFromUTF8(luaL_checkstring(L, 1)).data());
	auto verb = StringFromUTF8(lua_tostring(L, 2));
	int noshow = lua_toboolean(L, 3);
	int dowait = lua_toboolean(L, 4);

	BOOL useConsoleOut = dowait && noshow && (verb.empty());

	DWORD exit_code = (DWORD)-1;
	BOOL bSuccess = FALSE;
	std::wstring strOut(1024, 0);

	if (useConsoleOut != FALSE)
	{
		bSuccess = RunProcessHide(file, &exit_code, strOut) ||
			ExecuteHide(file, &exit_code, strOut);
	}
	else
	{
		HANDLE hProcess = NULL;
		// запускаем процесс
		if (verb.size() && // если есть команда запуска
			wcscmp(verb.data(), L"explore") == 0 && // если команда запуска explore
			CPath::IsFileExists(file.GetPath())) // проверяем файл ли это
		{
			SHELLEXECUTEINFOW shinf = { }; shinf.cbSize = sizeof(SHELLEXECUTEINFOW);
			shinf.lpFile = L"explorer.exe";
			std::wstring sFileParams;
			sFileParams.append(L"/e, /select,");
			sFileParams.append(file.GetPath());
			shinf.lpParameters = sFileParams.data();
			shinf.fMask = SEE_MASK_FLAG_NO_UI |
				SEE_MASK_NO_CONSOLE |
				SEE_MASK_FLAG_DDEWAIT |
				SEE_MASK_NOCLOSEPROCESS;
			shinf.nShow = noshow ? SW_HIDE : SW_SHOWNORMAL;
			bSuccess = ::ShellExecuteEx(&shinf);
			if (bSuccess && shinf.hInstApp <= (HINSTANCE)32) bSuccess = FALSE;
			hProcess = shinf.hProcess;
		}
		else if (verb.size() && // если есть команда запуска
			wcscmp(verb.data(), L"select") == 0 && // если команда запуска select
			CPath::IsPathExist(file.GetPath())) // проверяем правильный путь
		{
			SHELLEXECUTEINFOW shinf{};
			shinf.cbSize = sizeof(SHELLEXECUTEINFOW);
			shinf.lpFile = L"explorer.exe";
			std::wstring sFileParams;
			sFileParams.append(L"/select,");
			sFileParams.append(file.GetPath());
			shinf.lpParameters = sFileParams.data();
			shinf.fMask = SEE_MASK_FLAG_NO_UI |
				SEE_MASK_NO_CONSOLE |
				SEE_MASK_FLAG_DDEWAIT |
				SEE_MASK_NOCLOSEPROCESS;
			shinf.nShow = noshow ? SW_HIDE : SW_SHOWNORMAL;
			bSuccess = ::ShellExecuteEx(&shinf);
			if (bSuccess && shinf.hInstApp <= (HINSTANCE)32) bSuccess = FALSE;
			hProcess = shinf.hProcess;
		}
		else
		{
			SHELLEXECUTEINFOW shinf{};
			shinf.cbSize = sizeof(SHELLEXECUTEINFOW);
			shinf.lpFile = file.GetPath();
			shinf.lpParameters = file.GetFileParams();
			shinf.lpVerb = verb.data();
			//shinf.lpDirectory = file.GetDirectory();
			shinf.fMask = SEE_MASK_FLAG_NO_UI |
				SEE_MASK_NO_CONSOLE |
				SEE_MASK_FLAG_DDEWAIT;
			if (verb.size())
			{
				shinf.fMask |= SEE_MASK_NOCLOSEPROCESS;
			}
			else
			{
				shinf.fMask |= SEE_MASK_INVOKEIDLIST;
			}
			shinf.nShow = noshow ? SW_HIDE : SW_SHOWNORMAL;
			bSuccess = ::ShellExecuteEx(&shinf);
			if (bSuccess && shinf.hInstApp <= (HINSTANCE)32) bSuccess = FALSE;
			hProcess = shinf.hProcess;
		}

		if (dowait != FALSE && hProcess != NULL)
		{
			// ждем пока процесс не завершится
			::WaitForSingleObject(hProcess, INFINITE);
		}

		if (hProcess != NULL)
		{
			if (dowait != FALSE) ::GetExitCodeProcess(hProcess, &exit_code);
			CloseHandle(hProcess);
		}

		if (bSuccess != FALSE)
		{
			::SetLastError(0);
			DWORD dw;
			size_t len;
			strOut.append(GetLastErrorString(&dw, &len));
		}
	}

	if (bSuccess == FALSE)
	{
		lua_pushboolean(L, FALSE);
		lua_pushlasterr(L, NULL);
	}
	else
	{
		(exit_code != (DWORD)-1) ? lua_pushnumber(L, exit_code) : lua_pushboolean(L, TRUE);
		lua_pushstring(L, UTF8FromString(strOut).data());
	}

	return 2;
}

static int getclipboardtext(lua_State* L)
{
	std::wstring tmp;
	if (::IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		if (::OpenClipboard(NULL))
		{
			if (HANDLE hData = ::GetClipboardData(CF_UNICODETEXT))
			{
				if (LPCWSTR str = reinterpret_cast<LPCWSTR>(::GlobalLock(hData)))
				{
					tmp.append(str);
				}
				::GlobalUnlock(hData);
			}
			::CloseClipboard();
		}
	}
	lua_pushstring(L, UTF8FromString(tmp).c_str());
	return 1;
}

static void pushFFTime(lua_State* L, FILETIME* ft)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(ft, &st);
	lua_newtable(L);
	lua_pushstring(L, "year");
	lua_pushnumber(L, st.wYear);
	lua_rawset(L, -3);
	lua_pushstring(L, "month");
	lua_pushnumber(L, st.wMonth);
	lua_rawset(L, -3);
	lua_pushstring(L, "dayofweek");
	lua_pushnumber(L, st.wDayOfWeek);
	lua_rawset(L, -3);
	lua_pushstring(L, "day");
	lua_pushnumber(L, st.wDay);
	lua_rawset(L, -3);
	lua_pushstring(L, "hour");
	lua_pushnumber(L, st.wHour);
	lua_rawset(L, -3);
	lua_pushstring(L, "min");
	lua_pushnumber(L, st.wMinute);
	lua_rawset(L, -3);
	lua_pushstring(L, "sec");
	lua_pushnumber(L, st.wSecond);
	lua_rawset(L, -3);
	lua_pushstring(L, "msec");
	lua_pushnumber(L, st.wMilliseconds);
	lua_rawset(L, -3);
}

static int findfiles(lua_State* L)
{
	auto filename = StringFromUTF8(luaL_checkstring(L, 1));

	WIN32_FIND_DATA findFileData;
	HANDLE hFind = ::FindFirstFile(filename.data(), &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		// create table for result
		lua_createtable(L, 1, 0);

		lua_Integer num = 1;
		BOOL isFound = TRUE;
		while (isFound != FALSE)
		{
			// store file info
			lua_pushinteger(L, num);
			lua_createtable(L, 0, 7);

			lua_pushstring(L, UTF8FromString(findFileData.cFileName).data());
			lua_setfield(L, -2, "name");

			lua_pushboolean(L, findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
			lua_setfield(L, -2, "isdirectory");

			lua_pushnumber(L, findFileData.dwFileAttributes);
			lua_setfield(L, -2, "attributes");

			lua_pushnumber(L, findFileData.nFileSizeHigh * ((lua_Number)MAXDWORD + 1) +
				findFileData.nFileSizeLow);
			lua_setfield(L, -2, "size");

			pushFFTime(L, &(findFileData.ftCreationTime));
			lua_setfield(L, -2, "creationtime");

			pushFFTime(L, &(findFileData.ftLastAccessTime));
			lua_setfield(L, -2, "lastaccesstime");

			pushFFTime(L, &(findFileData.ftLastWriteTime));
			lua_setfield(L, -2, "lastwritetime");

			lua_settable(L, -3);
			num++;

			// next
			isFound = ::FindNextFile(hFind, &findFileData);
		}

		::FindClose(hFind);

		return 1;
	}

	// files not found
	return 0;
}

int showinputbox(lua_State*);

int do_md5(lua_State* L)
{
	MD5 md5;
	if (lua_toboolean(L, 2))
	{
		auto FN = StringFromUTF8(luaL_checkstring(L, 1));
		if (CPath::IsPathExist(FN.c_str()))
		{
			lua_pushstring(L, md5.digestFile(lua_tostring(L, 1)));
			return 1;
		}
	}
	lua_pushstring(L, md5.digestString(lua_tostring(L, 1)));
	return 1;
}

int do_sha256(lua_State* L)
{
	std::string content = lua_tostring(L, 1);
	uint8_t sha2hash[32]{};
	calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());

	wchar_t sha2hashStr[65]{};
	for (size_t i = 0; i < 32; i++)
		wsprintf(sha2hashStr + i * 2, L"%02x", sha2hash[i]);

	std::string res = UTF8FromString(sha2hashStr);
	lua_pushstring(L, res.c_str());
	return 1;
}

static const struct luaL_Reg shell[] =
{
	{ "exec", exec },
	{ "msgbox", msgbox },
	{ "getfileattr", getfileattr },
	{ "setfileattr", setfileattr },
	{ "fileexists", fileexists },
	{ "getclipboardtext", getclipboardtext },
	{ "findfiles", findfiles },
	{ "inputbox", showinputbox },
#ifdef STRFUNC_EXROPT
	{ "to_utf8", to_utf8 },
	{ "from_utf8", from_utf8 },
#endif
	{ "calc_md5", do_md5},
	{ "calc_sha256", do_sha256},
	{ NULL, NULL }
};

extern "C" __declspec(dllexport)
int luaopen_shell(lua_State* L)
{
#if LUA_VERSION_NUM < 502
	luaL_register(L, "shell", shell); //Lua5.1
#else
	luaL_newlib(L, shell); //Lua5.2+
#endif
	lua_pushvalue(L, -1);  /* copy of module */
	lua_setglobal(L, "shell");
	return 1;
}
