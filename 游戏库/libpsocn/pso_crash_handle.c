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

// 声明用于存储函数地址的数组
void* callstack[MAX_BACKTRACE_DEPTH];

LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info) {
    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    CONTEXT context;
    RtlCaptureContext(&context);

    STACKFRAME64 stackFrame;
    ZeroMemory(&stackFrame, sizeof(STACKFRAME64));
    DWORD machineType = IMAGE_FILE_MACHINE_UNKNOWN;

#ifdef _M_IX86
    machineType = IMAGE_FILE_MACHINE_I386;
    stackFrame.AddrPC.Offset = context.Eip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Ebp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Esp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_X64
    machineType = IMAGE_FILE_MACHINE_AMD64;
    stackFrame.AddrPC.Offset = context.Rip;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.Rsp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.Rsp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#elif _M_IA64
    machineType = IMAGE_FILE_MACHINE_IA64;
    stackFrame.AddrPC.Offset = context.StIIP;
    stackFrame.AddrPC.Mode = AddrModeFlat;
    stackFrame.AddrFrame.Offset = context.IntSp;
    stackFrame.AddrFrame.Mode = AddrModeFlat;
    stackFrame.AddrBStore.Offset = context.RsBSP;
    stackFrame.AddrBStore.Mode = AddrModeFlat;
    stackFrame.AddrStack.Offset = context.IntSp;
    stackFrame.AddrStack.Mode = AddrModeFlat;
#endif

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

    ERR_LOG("调用堆栈:");

    int frameIndex = 0;
    for (; frameIndex < MAX_BACKTRACE_DEPTH; frameIndex++) {
        if (!StackWalk64(machineType, process, thread, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            break;
        }

        DWORD64 address = stackFrame.AddrPC.Offset;
        callstack[frameIndex] = (void*)address;

        char symbolInfoBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
        PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolInfoBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;

        if (SymFromAddr(process, address, NULL, symbol)) {
            ERR_LOG("函数[%d]: %s", frameIndex, symbol->Name);
        }
    }

    SymCleanup(GetCurrentProcess());

    // 对于未填充的调用堆栈位置，将其设为NULL
    for (; frameIndex < MAX_BACKTRACE_DEPTH; frameIndex++) {
        callstack[frameIndex] = NULL;
    }

    // 在此处可以将 callstack 数组记录到日志文件或其他适当的位置

    return EXCEPTION_EXECUTE_HANDLER;
}
