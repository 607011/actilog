#pragma once

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

#include <windows.h>
#include <tchar.h>
#include <stdarg.h>

class Logger {
public:
	Logger();
	~Logger();
	void setFilename(const TCHAR* pszFilename);
	bool open(bool bOverwrite, const TCHAR* pszFilename = NULL);
	void log(const TCHAR* pszFormat, ...);
	void flush();
	void close();
	void logWithTimestamp(const TCHAR* pszFormat, ...);
	void logWithTimestampNoLF(const TCHAR* pszFormat, ...);
	const TCHAR* filename() const { return pszOutputFile; }

private:
	static const TCHAR* ConsoleOutputFile;
	const TCHAR* pszOutputFile;
	HANDLE hOutputFile;
	void logv(const TCHAR* pszFormat, va_list args);
	void logTimestamp();
	void logWithTimestampNoLFv(const TCHAR* pszFormat, va_list argp);
};

