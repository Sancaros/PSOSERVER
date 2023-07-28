/*
    梦幻之星中国 服务器 异常处理机制
    版权 (C) 2023 Sancaros

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License version 3
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PSO_CRASH_HANDLE_H
#define PSO_CRASH_HANDLE_H

#include <stdio.h>
#include <Windows.h>
#include <excpt.h>
#include <DbgHelp.h>
#pragma comment(lib , "dbghelp.lib")

LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info);

#endif // !PSO_CRASH_HANDLE_H

