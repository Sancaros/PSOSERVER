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

#include <f_logs.h>
#include <DbgHelp.h>

#include "pso_crash_handle.h"

LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info) {
    DWORD displacement;
    IMAGEHLP_LINE64 line = { 0 };
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    ERR_LOG("由于发生异常程序崩溃了");

    // 获取异常信息
    PEXCEPTION_RECORD exception_record = exception_info->ExceptionRecord;
    DWORD exception_code = exception_record->ExceptionCode;
    PVOID exception_address = exception_record->ExceptionAddress;

    ERR_LOG("异常码: 0x%X", exception_code);
    ERR_LOG("异常内存地址: 0x%p", exception_address);

    // 获取异常地址
    DWORD64 exceptionAddress = (DWORD64)exception_info->ExceptionRecord->ExceptionAddress;

    // 初始化符号引擎
    SymInitialize(GetCurrentProcess(), NULL, TRUE);

    // 解析异常地址对应的函数名称和代码行信息
    if (SymGetLineFromAddr64(GetCurrentProcess(), exceptionAddress, &displacement, &line))
        ERR_LOG("异常发生在函数：%s 的第 %u 行", line.FileName, line.LineNumber);
    else
        ERR_LOG("无法定位到异常发生位置的函数和行号");

    SymCleanup(GetCurrentProcess());

    return EXCEPTION_EXECUTE_HANDLER;
}