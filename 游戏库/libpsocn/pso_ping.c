/*
    �λ�֮���й� Ping����
    ��Ȩ (C) 2022, 2023 Sancaros

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

#include <stdio.h>
#include <Winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#pragma comment(lib , "Ws2_32.lib")

#include <f_logs.h>

#include "pso_ping.h"

#define ICMP_ECHO_REQUEST 8 //���������������
#define DEF_ICMP_DATA_SIZE 20 //���巢�����ݳ���
#define DEF_ICMP_PACK_SIZE 32 //�������ݰ�����
#define MAX_ICMP_PACKET_SIZE 1024 //����������ݰ�����
#define DEF_ICMP_TIMEOUT 3000  //���峬ʱΪ3��
#define ICMP_TIMEOUT 11 //ICMP��ʱ����
#define ICMP_ECHO_REPLY 0 //�������Ӧ������

/*
 *IP��ͷ�ṹ
 */
typedef struct
{
    uint8_t h_len_ver; //IP�汾��
    uint8_t tos; // ��������
    unsigned short total_len; //IP���ܳ���
    unsigned short ident; // ��ʶ
    unsigned short frag_and_flags; //��־λ
    uint8_t ttl; //����ʱ��
    uint8_t proto; //Э��
    unsigned short cksum; //IP�ײ�У���
    unsigned long sourceIP; //ԴIP��ַ
    unsigned long destIP; //Ŀ��IP��ַ
} IP_HEADER;

/*
 *����ICMP��������
 */
typedef struct _ICMP_HEADER
{
    uint8_t type; //����-----8
    uint8_t code; //����-----8
    unsigned short cksum; //У���------16
    unsigned short id; //��ʶ��-------16
    unsigned short seq; //���к�------16
    unsigned int choose; //ѡ��-------32
} ICMP_HEADER;

typedef struct
{
    int usSeqNo; //��¼���к�
    DWORD dwRoundTripTime; //��¼��ǰʱ��
    uint8_t ttl; //����ʱ��
    struct in_addr dwIPaddr; //ԴIP��ַ
} DECODE_RESULT;

/*
 *��������У���
 */
unsigned short GenerateChecksum(unsigned short* pBuf, int iSize)
{
    unsigned long cksum = 0; //��ʼʱ������У��ͳ�ʼ��Ϊ0
    while (iSize > 1)
    {
        cksum += *pBuf++; //����У�������ÿ16λ��λ��ӱ�����cksum��
        iSize -= sizeof(unsigned short); //ÿ16λ�����򽫴�У����������ȥ16
    }
    //�����У�������Ϊ��������ѭ����֮���轫���һ���ֽڵ�������֮ǰ������
    if (iSize)
    {
        cksum += *(unsigned char*)pBuf;
    }
    //֮ǰ�Ľ�������˽�λ����Ҫ�ѽ�λҲ�������Ľ����
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (unsigned short)(~cksum);
}

/*
 *��pingӦ����Ϣ���н���
 */
int DecodeIcmpResponse_Ping(char* pBuf, int iPacketSize, DECODE_RESULT* stDecodeResult)
{
    IP_HEADER* pIpHrd = (IP_HEADER*)pBuf;
    int iIphedLen = 20;
    if (iPacketSize < (int)(iIphedLen + sizeof(ICMP_HEADER)))
    {
        printf("size error! \n");
        return 0;
    }
    //ָ��ָ��ICMP���ĵ��׵�ַ
    ICMP_HEADER* pIcmpHrd = (ICMP_HEADER*)(pBuf + iIphedLen);
    unsigned short usID = 0, usSeqNo = 0;
    //��õ����ݰ���type�ֶ�ΪICMP_ECHO_REPLY�����յ�һ������Ӧ��ICMP����
    if (pIcmpHrd->type == ICMP_ECHO_REPLY)
    {
        usID = pIcmpHrd->id;
        //���յ����������ֽ�˳���seq�ֶ���Ϣ �� ��ת��Ϊ�����ֽ�˳��
        usSeqNo = ntohs(pIcmpHrd->seq);
    }
    if (usID != GetCurrentProcessId() || usSeqNo != stDecodeResult->usSeqNo)
    {
        printf("usID error!\n");
        return 0;
    }
    //��¼�Է�������IP��ַ�Լ�����������ʱ��RTT
    if (pIcmpHrd->type == ICMP_ECHO_REPLY)
    {
        stDecodeResult->dwIPaddr.s_addr = pIpHrd->sourceIP;
        stDecodeResult->ttl = pIpHrd->ttl;
        stDecodeResult->dwRoundTripTime = GetTickCount() - stDecodeResult->dwRoundTripTime;
        return 1;
    }
    return 0;
}

int Ping_Pong(char* IP, uint32_t srvip)
{
    srvip = inet_addr(IP); //��IP��ַת��Ϊ������
    if (srvip == INADDR_NONE)
    {
        //ת�����ɹ�ʱ����������
        HOSTENT* pHostent = gethostbyname(IP);
        if (pHostent)
        {
            srvip = (*(IN_ADDR*)pHostent->h_addr).s_addr; //��HOSTENTת��Ϊ������
            CONFIG_LOG("Ping ���...");
            return 0;
        }
        else
        {
            CONFIG_LOG("��ַ����ʧ�ܣ�");
            return -1;
        }
    }

    return 0;
    ////���Ŀ��Socket��ַ
    //SOCKADDR_IN destSockAddr; //����Ŀ�ĵ�ַ
    //ZeroMemory(&destSockAddr, sizeof(SOCKADDR_IN)); //��Ŀ�ĵ�ַ���
    //destSockAddr.sin_family = AF_INET;
    //destSockAddr.sin_addr.s_addr = srvip;
    //destSockAddr.sin_port = htons(0);
    ////��ʼ��WinSock
    //WORD wVersionRequested = MAKEWORD(2, 2);
    //WSADATA wsaData;
    //if (WSAStartup(wVersionRequested, &wsaData) != 0)
    //{
    //    printf("��ʼ��WinSockʧ�ܣ�\n");
    //    return;
    //}
    ////ʹ��ICMPЭ�鴴��Raw Socket
    //SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
    //if (sockRaw == INVALID_SOCKET)
    //{
    //    printf("����Socketʧ�� !\n");
    //    return;
    //}
    ////���ö˿�����
    //int iTimeout = DEF_ICMP_TIMEOUT;
    //if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
    //{
    //    printf("���ò���ʧ�ܣ�\n");
    //    return;
    //}
    //if (setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
    //{
    //    printf("���ò���ʧ�ܣ�\n");
    //    return;
    //}
    ////���巢�͵����ݶ�
    //char IcmpSendBuf[DEF_ICMP_PACK_SIZE];
    ////���ICMP���ݰ������ֶ�
    //ICMP_HEADER* pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
    //pIcmpHeader->type = ICMP_ECHO_REQUEST;
    //pIcmpHeader->code = 0;
    //pIcmpHeader->id = (unsigned short)GetCurrentProcessId();
    //memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE);
    ////ѭ�������ĸ��������icmp���ݰ�
    //int usSeqNo = 0;
    //DECODE_RESULT stDecodeResult;
    //while (usSeqNo <= 3)
    //{
    //    pIcmpHeader->seq = htons(usSeqNo);
    //    pIcmpHeader->cksum = 0;
    //    pIcmpHeader->cksum = GenerateChecksum((unsigned short*)IcmpSendBuf, DEF_ICMP_PACK_SIZE); //����У��λ
    //    //��¼���кź͵�ǰʱ��
    //    stDecodeResult.usSeqNo = usSeqNo;
    //    stDecodeResult.dwRoundTripTime = GetTickCount();
    //    //����ICMP��EchoRequest���ݰ�
    //    if (sendto(sockRaw, IcmpSendBuf, DEF_ICMP_PACK_SIZE, 0, (SOCKADDR*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
    //    {
    //        //���Ŀ���������ɴ���ֱ���˳�
    //        if (WSAGetLastError() == WSAEHOSTUNREACH)
    //        {
    //            printf("Ŀ���������ɴ\n");
    //            exit(0);
    //        }
    //    }
    //    SOCKADDR_IN from;
    //    int iFromLen = sizeof(from);
    //    int iReadLen;
    //    //������յ����ݰ�
    //    char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
    //    while (1)
    //    {
    //        iReadLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (SOCKADDR*)&from, &iFromLen);
    //        if (iReadLen != SOCKET_ERROR)
    //        {
    //            if (DecodeIcmpResponse_Ping(IcmpRecvBuf, sizeof(IcmpRecvBuf), &stDecodeResult))
    //            {
    //                printf("���� %s �Ļظ�: �ֽ� = %d ʱ�� = %dms TTL = %d\n", inet_ntoa(stDecodeResult.dwIPaddr),
    //                    iReadLen - 20, stDecodeResult.dwRoundTripTime, stDecodeResult.ttl);
    //            }
    //            break;
    //        }
    //        else if (WSAGetLastError() == WSAETIMEDOUT)
    //        {
    //            printf("time out !  *****\n");
    //            break;
    //        }
    //        else
    //        {
    //            printf("����δ֪����\n");
    //            break;
    //        }
    //    }
    //    usSeqNo++;
    //}
    ////�����Ļ��Ϣ
    //printf("Ping complete...\n");
    //closesocket(sockRaw);
    //WSACleanup();
}

int get_ips_by_domain(const char* const domain) {
    struct hostent* _hostent = NULL;
    _hostent = gethostbyname(domain);
    int i = 0;
    while (_hostent->h_addr_list[i] != NULL) {
        char* ipaddr = inet_ntoa(*((struct in_addr*)_hostent->h_addr_list[i]));
        printf("ip addr%d: %s\n", i + 1, ipaddr);
        i++;
    }
    return 0;
}