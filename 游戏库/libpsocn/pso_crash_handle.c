/*
    �λ�֮���й� ������ �쳣�������
    ��Ȩ (C) 2023 Sancaros

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

    ERR_LOG("���ڷ����쳣���������");

    // ��ȡ�쳣��Ϣ
    PEXCEPTION_RECORD exception_record = exception_info->ExceptionRecord;
    DWORD exception_code = exception_record->ExceptionCode;
    PVOID exception_address = exception_record->ExceptionAddress;

    ERR_LOG("�쳣��: 0x%X", exception_code);
    ERR_LOG("�쳣�ڴ��ַ: 0x%p", exception_address);

    // ��ȡ�쳣��ַ
    DWORD64 exceptionAddress = (DWORD64)exception_info->ExceptionRecord->ExceptionAddress;

    // ��ʼ����������
    SymInitialize(GetCurrentProcess(), NULL, TRUE);

    // �����쳣��ַ��Ӧ�ĺ������ƺʹ�������Ϣ
    if (SymGetLineFromAddr64(GetCurrentProcess(), exceptionAddress, &displacement, &line))
        ERR_LOG("�쳣�����ں�����%s �ĵ� %u ��", line.FileName, line.LineNumber);
    else
        ERR_LOG("�޷���λ���쳣����λ�õĺ������к�");

    SymCleanup(GetCurrentProcess());

    return EXCEPTION_EXECUTE_HANDLER;
}