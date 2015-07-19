#ifndef _util_h_
#define _util_h_

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <crtdbg.h>
#include <string>
#include <sstream>

// logging 
#define LL_DEBG		0
#define LL_INFO		1
#define LL_ERRR		2
#define LL_NONE		3

#define con_err		write_to_console( LL_ERRR, __FUNCTIONW__, 
#define con_info	write_to_console( LL_INFO, __FUNCTIONW__, 
#define con_dbg		write_to_console( LL_DEBG, __FUNCTIONW__, 
#define con_msg		write_to_console( LL_NONE, __FUNCTIONW__, 
#define con_end		);

#ifndef _slogger_included
#define log_err		con_err
#define log_dbg		con_dbg
#define log_info	con_info
#define	log_msg		con_msg
#define log_end		con_end
#endif //_slogger_included

using namespace std;

void write_to_console(_In_ DWORD log_level, _In_ wchar_t* function, _In_ wchar_t* format, ...);




bool is_file_existsW(_In_ const wchar_t* file_path);
bool is_file_existsA(_In_ const char* file_path);


wchar_t* MbsToWcs(_In_ const char* mbs);
char* WcsToMbs(_In_ const wchar_t* wcs);
char* WcsToMbsUTF8(IN const wchar_t* wcs);

static const std::wstring _null_stringw(L"");
static const std::string  _null_stringa("");

std::wstring MbsToWcsEx(_In_ const char *mbs);
std::string WcsToMbsEx(_In_ const wchar_t *wcs);
std::string WcsToMbsUTF8Ex(_In_ const wchar_t *wcs);


#endif