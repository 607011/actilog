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

HANDLE hOutputFile = NULL;


void logv(const TCHAR* pszFormat, va_list args)
{
	static const DWORD dwBufSize = 2048;
	TCHAR pszDest[dwBufSize];
	size_t szLength;
	StringCchVPrintf(pszDest, dwBufSize, pszFormat, args);
	StringCchLength(pszDest, dwBufSize, &szLength);
	DWORD dwBytesWritten;
	WriteFile(hOutputFile, pszDest, szLength, &dwBytesWritten, NULL);
}


void log(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logv(pszFormat, argp);
	va_end(argp);
}


void logTimestamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	log("%4d-%02d-%02d %02d:%02d:%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
}


void logFlush()
{
	log("\r\n");
	FlushFileBuffers(hOutputFile);
}


void logWithTimestamp(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logTimestamp();
	logv(pszFormat, argp);
	va_end(argp);
	logFlush();
}


void logWithTimestampNoLFv(const TCHAR* pszFormat, va_list argp)
{
	logTimestamp();
	logv(pszFormat, argp);
}


void logWithTimestampNoLF(const TCHAR* pszFormat, ...)
{
	va_list argp;
	va_start(argp, pszFormat);
	logWithTimestampNoLFv(pszFormat, argp);
	va_end(argp);
}
