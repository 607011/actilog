///    Copyright (C) 2013 Oliver Lau <ola@ct.de>
///
///    This program is free software: you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation, either version 3 of the License, or
///    (at your option) any later version.
///
///    This program is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License
///    along with this program.  If not, see <http://www.gnu.org/licenses/>.
///

#include "log.h"
#include <strsafe.h>
#include <Shlwapi.h>

const TCHAR* Logger::ConsoleOutputFile = TEXT("CONOUT$");


Logger::Logger()
	: hOutputFile(NULL)
	, pszOutputFile(ConsoleOutputFile)
{
	// ...
}


Logger::~Logger()
{
	close();
}


void Logger::close()
{
	if (hOutputFile)
		CloseHandle(hOutputFile);
	hOutputFile = NULL;
}


void Logger::setFilename(const TCHAR* pszFilename)
{
	if (pszFilename)
		pszOutputFile = pszFilename;
}


bool Logger::open(bool bOverwrite, const TCHAR* pszFilename)
{
	close();
	if (pszFilename)
		setFilename(pszFilename);
	if (StrCmp(pszOutputFile, ConsoleOutputFile) == 0)
		bOverwrite = true;
	hOutputFile = CreateFile(pszOutputFile, bOverwrite? GENERIC_WRITE : FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return hOutputFile != INVALID_HANDLE_VALUE;
}


void Logger::logv(const TCHAR* pszFormat, va_list args)
{
	static const DWORD dwBufSize = 2048;
	TCHAR pszDest[dwBufSize];
	size_t szLength;
	StringCchVPrintf(pszDest, dwBufSize, pszFormat, args);
	StringCchLength(pszDest, dwBufSize, &szLength);
	DWORD dwBytesWritten;
	WriteFile(hOutputFile, pszDest, szLength, &dwBytesWritten, NULL);
}


void Logger::log(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logv(pszFormat, argp);
	va_end(argp);
}


void Logger::logTimestamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	log(TEXT("%4d-%02d-%02d %02d:%02d:%02d "), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
}


void Logger::flush()
{
	log(TEXT("\r\n"));
	FlushFileBuffers(hOutputFile);
}


void Logger::logWithTimestamp(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logTimestamp();
	logv(pszFormat, argp);
	va_end(argp);
	flush();
}


void Logger::logWithTimestampNoLFv(const TCHAR* pszFormat, va_list argp)
{
	logTimestamp();
	logv(pszFormat, argp);
}


void Logger::logWithTimestampNoLF(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logWithTimestampNoLFv(pszFormat, argp);
	va_end(argp);
}
