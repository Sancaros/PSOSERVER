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

LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info) {
    ERR_LOG("由于发生异常程序崩溃了");

	// 获取异常信息
	PEXCEPTION_RECORD exception_record = exception_info->ExceptionRecord;
	DWORD exception_code = exception_record->ExceptionCode;
	PVOID exception_address = exception_record->ExceptionAddress;

	ERR_LOG("异常码: 0x%X", exception_code);
    ERR_LOG("异常内存地址: 0x%p", exception_address);

	// 在这里执行异常处理后的逻辑，例如打印错误信息或提供用户友好的提示。

	return EXCEPTION_EXECUTE_HANDLER;
}
