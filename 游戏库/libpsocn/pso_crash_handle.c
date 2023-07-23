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

LONG WINAPI crash_handler(EXCEPTION_POINTERS* exception_info) {
    ERR_LOG("���ڷ����쳣���������");

	// ��ȡ�쳣��Ϣ
	PEXCEPTION_RECORD exception_record = exception_info->ExceptionRecord;
	DWORD exception_code = exception_record->ExceptionCode;
	PVOID exception_address = exception_record->ExceptionAddress;

	ERR_LOG("�쳣��: 0x%X", exception_code);
    ERR_LOG("�쳣�ڴ��ַ: 0x%p", exception_address);

	// ������ִ���쳣�������߼��������ӡ������Ϣ���ṩ�û��Ѻõ���ʾ��

	return EXCEPTION_EXECUTE_HANDLER;
}
