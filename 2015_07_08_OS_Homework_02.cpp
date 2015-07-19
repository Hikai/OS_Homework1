// 2015_07_08_OS_Homework_02.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "util.h"
#include <atlstr.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <strSafe.h>
#include <Windows.h>

wchar_t* MbsToWcs(_In_ const char* mbs)
{
	_ASSERTE(NULL != mbs);
	if (NULL == mbs) return NULL;

	int outLen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbs, -1, NULL, 0);
	if (0 == outLen) return NULL;

	wchar_t* outWchar = (wchar_t*)malloc(outLen * (sizeof(wchar_t)));  // outLen contains NULL char
	if (NULL == outWchar) return NULL;
	RtlZeroMemory(outWchar, outLen);

	if (0 == MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbs, -1, outWchar, outLen))
	{
		log_err L"MultiByteToWideChar() failed, errcode=0x%08x", GetLastError() log_end

			free(outWchar);
		return NULL;
	}

	return outWchar;
}

char* WcsToMbsUTF8(_In_ const wchar_t* wcs)
{
	_ASSERTE(NULL != wcs);
	if (NULL == wcs) return NULL;

	int outLen = WideCharToMultiByte(CP_UTF8, 0, wcs, -1, NULL, 0, NULL, NULL);
	if (0 == outLen) return NULL;

	char* outChar = (char*)malloc(outLen * sizeof(char));
	if (NULL == outChar) return NULL;
	RtlZeroMemory(outChar, outLen);

	if (0 == WideCharToMultiByte(CP_UTF8, 0, wcs, -1, outChar, outLen, NULL, NULL))
	{
		log_err L"WideCharToMultiByte() failed, errcode=0x%08x", GetLastError() log_end

			free(outChar);
		return NULL;
	}

	return outChar;
}

void print(_In_ const char* fmt, _In_ ...)
{
	char log_buffer[2048];
	va_list args;

	va_start(args, fmt);
	HRESULT hRes = StringCbVPrintfA(log_buffer, sizeof(log_buffer), fmt, args);
	if (S_OK != hRes)
	{
		fprintf(
			stderr,
			"%s, StringCbVPrintfA() failed. res = 0x%08x",
			__FUNCTION__,
			hRes
			);
		return;
	}

	OutputDebugStringA(log_buffer);
	fprintf(stdout, "%s \n", log_buffer);
}

void write_to_console(_In_ DWORD log_level, _In_ wchar_t* function, _In_ wchar_t* format, ...)
{
	static HANDLE	_stdout_handle = INVALID_HANDLE_VALUE;
	static WORD		_old_color = 0x0000;

	//> initialization for console text color manipulation.
	if (INVALID_HANDLE_VALUE == _stdout_handle)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(_stdout_handle, &csbi);
		_old_color = csbi.wAttributes;
	}

	std::wstringstream strm_prefix;
	switch (log_level)

	{
	case LL_DEBG:
	{
		strm_prefix << L"[DEBG] " << function << L", ";
		break;
	}
	case LL_INFO:
	{
		strm_prefix << L"[INFO] " << function << L", ";
		SetConsoleTextAttribute(_stdout_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	}
	case LL_ERRR:
	{
		strm_prefix << L"[ERR ] " << function << L", ";
		SetConsoleTextAttribute(_stdout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;
	}
	case LL_NONE:
	default:
	{
		//strm_prefix << function << L", ";
		break;
	}
	}

	DWORD len;
	va_list args;
	WCHAR msg[4096];

	va_start(args, format);
	if (TRUE != SUCCEEDED(StringCchVPrintf(msg, sizeof(msg), format, args))){ return; }
	va_end(args);

	WriteConsole(_stdout_handle, strm_prefix.str().c_str(), (DWORD)strm_prefix.str().size(), &len, NULL);
	WriteConsole(_stdout_handle, msg, (DWORD)wcslen(msg), &len, NULL);
	WriteConsole(_stdout_handle, L"\n", (DWORD)wcslen(L"\n"), &len, NULL);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), _old_color);

#ifdef _USE_AHNTRACE_
	switch (log_level)
	{
	case LL_DEBG: DEBUGLOG(msg); break;
	case LL_INFO: INFOLOG(msg); break;
	case LL_ERRR: ERRORLOG(msg); break;
	default:
		break;
	}
#endif//_USE_AHNTRACE_
}

bool read_file_using_memory_map()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ test.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\test.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}
	free(buf); buf = NULL;
	/*
	if (true == is_file_existsW(file_name))
	{
		::DeleteFileW(file_name);
	}*/




	HANDLE file_handle = CreateFileW(
		(LPCWSTR)file_name,
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		print("err, CreateFile(%ws) failed, gle = %u", file_name, GetLastError());
		return false;
	}

	// check file size
	//
	LARGE_INTEGER fileSize;
	if (TRUE != GetFileSizeEx(file_handle, &fileSize))
	{
		print("err, GetFileSizeEx(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// [ WARN ]
	//
	// 4Gb 이상의 파일의 경우 MapViewOfFile()에서 오류가 나거나
	// 파일 포인터 이동이 문제가 됨
	// FilIoHelperClass 모듈을 이용해야 함
	//
	_ASSERTE(fileSize.HighPart == 0);
	if (fileSize.HighPart > 0)
	{
		print("file size = %I64d (over 4GB) can not handle. use FileIoHelperClass",
			fileSize.QuadPart);
		CloseHandle(file_handle);
		return false;
	}

	DWORD file_size = (DWORD)fileSize.QuadPart;
	HANDLE file_map = CreateFileMapping(
		file_handle,
		NULL,
		PAGE_READONLY,
		0,	
		0,
		NULL
		);
	if (NULL == file_map)
	{
		print("err, CreateFileMapping(%ws) failed, gle = %u", file_name, GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	PCHAR file_view = (PCHAR)MapViewOfFile(
		file_map,
		FILE_MAP_READ,
		0,
		0,
		0
		);
	if (file_view == NULL)
	{
		print("err, MapViewOfFile(%ws) failed, gle = %u", file_name, GetLastError());

		CloseHandle(file_map);
		CloseHandle(file_handle);
		return false;
	}

	// do some io
	char a = file_view[0];  // 0x d9
	char b = file_view[1];  // 0xb3



	// close all
	UnmapViewOfFile(file_view);
	CloseHandle(file_map);
	CloseHandle(file_handle);
	return true;
}

bool is_file_existsW(_In_ const wchar_t* file_path)
{
	_ASSERTE(NULL != file_path);
	_ASSERTE(TRUE != IsBadStringPtrW(file_path, MAX_PATH));
	if ((NULL == file_path) || (TRUE == IsBadStringPtrW(file_path, MAX_PATH))) return false;

	WIN32_FILE_ATTRIBUTE_DATA info = { 0 };

	if (GetFileAttributesExW(file_path, GetFileExInfoStandard, &info) == 0)
		return false;
	else
		return true;
}

bool is_file_existsA(_In_ const char* file_path)
{
	WCHAR* wcs = MbsToWcs(file_path);
	if (NULL == wcs) { return false; }

	bool ret = is_file_existsW(wcs); free(wcs);
	return ret;
}

bool create_bob()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}
	free(buf); buf = NULL;

	if (true == is_file_existsW(file_name))
	{
		::DeleteFileW(file_name);
	}

	// 파일 생성
	HANDLE file_handle = CreateFileW(
		file_name,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		print("err, CreateFile(path=%ws), gle=0x%08x", file_name, GetLastError());
		return false;
	}

	DWORD bytes_written = 0;
	wchar_t * string_buf = L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라만세";
	char * UTF_string_buf = WcsToMbsUTF8(string_buf);

	if (!WriteFile(file_handle, UTF_string_buf, strlen(UTF_string_buf), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	string_buf = L"All work and no play makes jack a dull boy.";
	UTF_string_buf = WcsToMbsUTF8(string_buf);

	if (!WriteFile(file_handle, UTF_string_buf, strlen(UTF_string_buf), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	wchar_t * string_bufa = L"동해물과 백두산이 마르고 닳도록 하느님이 보우하사 우리나라만세";
	char * UTF_string_bufa = WcsToMbsUTF8(string_bufa);

	if (!WriteFile(file_handle, UTF_string_bufa, strlen(UTF_string_bufa), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	string_bufa = L"All work and no play makes jack a dull boy.";
	UTF_string_bufa = WcsToMbsUTF8(string_bufa);

	if (!WriteFile(file_handle, UTF_string_bufa, strlen(UTF_string_bufa), &bytes_written, NULL))
	{
		print("err, WriteFile() failed. gle = 0x%08x", GetLastError());
		CloseHandle(file_handle);
		return false;
	}

	// 파일 닫기
	CloseHandle(file_handle);
	return true;

}

bool copy_bob2()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}

	wchar_t file_name2[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name2,
		sizeof(file_name2),
		L"%s\\bob2.txt",
		buf))) {
		print("err, can not create file name");
		free(buf);
		return false;
	}
	
	CopyFile(file_name, file_name2, is_file_existsW(file_name2));

	free(buf); buf = NULL;

	if (true == is_file_existsW(file_name))
	{
		::DeleteFileW(file_name);
	}
	
	return true;
}

bool read_bob2()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob2.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob2.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}
	
	HANDLE read_handle = CreateFileW(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (read_handle == INVALID_HANDLE_VALUE) {
		print("err, CreateFile(path=%ws)u,gle=0x%08x", read_handle, GetLastError());
	}

	char readChar[2048] = { 0, };
	wchar_t middleChar[2048] = { 0, };
	DWORD dwRead;

	ReadFile(read_handle, readChar, 2048, &dwRead, NULL);
	MultiByteToWideChar(CP_UTF8, 0, readChar, strlen(readChar), middleChar, strlen(readChar));
	WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), middleChar, dwRead, NULL, NULL);
	printf("\n");

	free(buf); buf = NULL;

	if (true == is_file_existsW(file_name))
	{
		::DeleteFileW(file_name);
	}

	return true;
}

bool remove_bob2()
{
	// current directory 를 구한다.
	wchar_t *buf = NULL;
	uint32_t buflen = 0;
	buflen = GetCurrentDirectoryW(buflen, buf);
	if (0 == buflen)
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		return false;
	}

	buf = (PWSTR)malloc(sizeof(WCHAR) * buflen);
	if (0 == GetCurrentDirectoryW(buflen, buf))
	{
		print("err, GetCurrentDirectoryW() failed. gle = 0x%08x", GetLastError());
		free(buf);
		return false;
	}

	// current dir \\ bob2.txt 파일명 생성
	wchar_t file_name[260];
	if (!SUCCEEDED(StringCbPrintfW(
		file_name,
		sizeof(file_name),
		L"%ws\\bob2.txt",
		buf)))
	{
		print("err, can not create file name");
		free(buf);
		return false;
	}

	DeleteFile(file_name);
}

int _tmain(int argc, _TCHAR* argv[])
{
	create_bob();
	copy_bob2();
	read_bob2();
	return 0;
}