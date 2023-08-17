/*
    梦幻之星中国 Ping功能
    版权 (C) 2022, 2023 Sancaros

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

#define ICMP_ECHO_REQUEST 8 //定义回显请求类型
#define DEF_ICMP_DATA_SIZE 20 //定义发送数据长度
#define DEF_ICMP_PACK_SIZE 32 //定义数据包长度
#define MAX_ICMP_PACKET_SIZE 1024 //定义最大数据包长度
#define DEF_ICMP_TIMEOUT 3000  //定义超时为3秒
#define ICMP_TIMEOUT 11 //ICMP超时报文
#define ICMP_ECHO_REPLY 0 //定义回显应答类型

/*
 *IP报头结构
 */
typedef struct
{
    uint8_t h_len_ver; //IP版本号
    uint8_t tos; // 服务类型
    unsigned short total_len; //IP包总长度
    unsigned short ident; // 标识
    unsigned short frag_and_flags; //标志位
    uint8_t ttl; //生存时间
    uint8_t proto; //协议
    unsigned short cksum; //IP首部校验和
    unsigned long sourceIP; //源IP地址
    unsigned long destIP; //目的IP地址
} IP_HEADER;

/*
 *定义ICMP数据类型
 */
typedef struct _ICMP_HEADER
{
    uint8_t type; //类型-----8
    uint8_t code; //代码-----8
    unsigned short cksum; //校验和------16
    unsigned short id; //标识符-------16
    unsigned short seq; //序列号------16
    unsigned int choose; //选项-------32
} ICMP_HEADER;

typedef struct
{
    int usSeqNo; //记录序列号
    DWORD dwRoundTripTime; //记录当前时间
    uint8_t ttl; //生存时间
    struct in_addr dwIPaddr; //源IP地址
} DECODE_RESULT;

/*
 *产生网际校验和
 */
unsigned short GenerateChecksum(unsigned short* pBuf, int iSize)
{
    unsigned long cksum = 0; //开始时将网际校验和初始化为0
    while (iSize > 1)
    {
        cksum += *pBuf++; //将待校验的数据每16位逐位相加保存在cksum中
        iSize -= sizeof(unsigned short); //每16位加完则将带校验数据量减去16
    }
    //如果待校验的数据为奇数，则循环完之后需将最后一个字节的内容与之前结果相加
    if (iSize)
    {
        cksum += *(unsigned char*)pBuf;
    }
    //之前的结果产生了进位，需要把进位也加入最后的结果中
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (unsigned short)(~cksum);
}

/*
 *对ping应答信息进行解析
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
    //指针指向ICMP报文的首地址
    ICMP_HEADER* pIcmpHrd = (ICMP_HEADER*)(pBuf + iIphedLen);
    unsigned short usID = 0, usSeqNo = 0;
    //获得的数据包的type字段为ICMP_ECHO_REPLY，即收到一个回显应答ICMP报文
    if (pIcmpHrd->type == ICMP_ECHO_REPLY)
    {
        usID = pIcmpHrd->id;
        //接收到的是网络字节顺序的seq字段信息 ， 需转化为主机字节顺序
        usSeqNo = ntohs(pIcmpHrd->seq);
    }
    if (usID != GetCurrentProcessId() || usSeqNo != stDecodeResult->usSeqNo)
    {
        printf("usID error!\n");
        return 0;
    }
    //记录对方主机的IP地址以及计算往返的时延RTT
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
    srvip = inet_addr(IP); //将IP地址转化为长整形
    if (srvip == INADDR_NONE)
    {
        //转化不成功时按域名解析
        HOSTENT* pHostent = gethostbyname(IP);
        if (pHostent)
        {
            srvip = (*(IN_ADDR*)pHostent->h_addr).s_addr; //将HOSTENT转化为长整形
            CONFIG_LOG("Ping 完成...");
            return 0;
        }
        else
        {
            CONFIG_LOG("地址解析失败！");
            return -1;
        }
    }

    return 0;
    ////填充目的Socket地址
    //SOCKADDR_IN destSockAddr; //定义目的地址
    //ZeroMemory(&destSockAddr, sizeof(SOCKADDR_IN)); //将目的地址清空
    //destSockAddr.sin_family = AF_INET;
    //destSockAddr.sin_addr.s_addr = srvip;
    //destSockAddr.sin_port = htons(0);
    ////初始化WinSock
    //WORD wVersionRequested = MAKEWORD(2, 2);
    //WSADATA wsaData;
    //if (WSAStartup(wVersionRequested, &wsaData) != 0)
    //{
    //    printf("初始化WinSock失败！\n");
    //    return;
    //}
    ////使用ICMP协议创建Raw Socket
    //SOCKET sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, WSA_FLAG_OVERLAPPED);
    //if (sockRaw == INVALID_SOCKET)
    //{
    //    printf("创建Socket失败 !\n");
    //    return;
    //}
    ////设置端口属性
    //int iTimeout = DEF_ICMP_TIMEOUT;
    //if (setsockopt(sockRaw, SOL_SOCKET, SO_RCVTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
    //{
    //    printf("设置参数失败！\n");
    //    return;
    //}
    //if (setsockopt(sockRaw, SOL_SOCKET, SO_SNDTIMEO, (char*)&iTimeout, sizeof(iTimeout)) == SOCKET_ERROR)
    //{
    //    printf("设置参数失败！\n");
    //    return;
    //}
    ////定义发送的数据段
    //char IcmpSendBuf[DEF_ICMP_PACK_SIZE];
    ////填充ICMP数据包个各字段
    //ICMP_HEADER* pIcmpHeader = (ICMP_HEADER*)IcmpSendBuf;
    //pIcmpHeader->type = ICMP_ECHO_REQUEST;
    //pIcmpHeader->code = 0;
    //pIcmpHeader->id = (unsigned short)GetCurrentProcessId();
    //memset(IcmpSendBuf + sizeof(ICMP_HEADER), 'E', DEF_ICMP_DATA_SIZE);
    ////循环发送四个请求回显icmp数据包
    //int usSeqNo = 0;
    //DECODE_RESULT stDecodeResult;
    //while (usSeqNo <= 3)
    //{
    //    pIcmpHeader->seq = htons(usSeqNo);
    //    pIcmpHeader->cksum = 0;
    //    pIcmpHeader->cksum = GenerateChecksum((unsigned short*)IcmpSendBuf, DEF_ICMP_PACK_SIZE); //生成校验位
    //    //记录序列号和当前时间
    //    stDecodeResult.usSeqNo = usSeqNo;
    //    stDecodeResult.dwRoundTripTime = GetTickCount();
    //    //发送ICMP的EchoRequest数据包
    //    if (sendto(sockRaw, IcmpSendBuf, DEF_ICMP_PACK_SIZE, 0, (SOCKADDR*)&destSockAddr, sizeof(destSockAddr)) == SOCKET_ERROR)
    //    {
    //        //如果目的主机不可达则直接退出
    //        if (WSAGetLastError() == WSAEHOSTUNREACH)
    //        {
    //            printf("目的主机不可达！\n");
    //            exit(0);
    //        }
    //    }
    //    SOCKADDR_IN from;
    //    int iFromLen = sizeof(from);
    //    int iReadLen;
    //    //定义接收的数据包
    //    char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
    //    while (1)
    //    {
    //        iReadLen = recvfrom(sockRaw, IcmpRecvBuf, MAX_ICMP_PACKET_SIZE, 0, (SOCKADDR*)&from, &iFromLen);
    //        if (iReadLen != SOCKET_ERROR)
    //        {
    //            if (DecodeIcmpResponse_Ping(IcmpRecvBuf, sizeof(IcmpRecvBuf), &stDecodeResult))
    //            {
    //                printf("来自 %s 的回复: 字节 = %d 时间 = %dms TTL = %d\n", inet_ntoa(stDecodeResult.dwIPaddr),
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
    //            printf("发生未知错误！\n");
    //            break;
    //        }
    //    }
    //    usSeqNo++;
    //}
    ////输出屏幕信息
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