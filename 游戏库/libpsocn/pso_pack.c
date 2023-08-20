/*
    �λ�֮���й� ѹ������
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

#include "pso_pack.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <f_logs.h>

int compress_data(const char* uncompressed_data, uLong uncompressed_length, char* compressed_data, uLong* compressed_length, int compression_level) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit(&stream, compression_level) != Z_OK) {
        return Z_ERRNO; // ��ʼ��ѹ����ʧ��
    }

    stream.next_in = (Bytef*)uncompressed_data;
    stream.avail_in = (uInt)uncompressed_length;
    stream.next_out = (Bytef*)compressed_data;
    stream.avail_out = (uInt)*compressed_length;

    int result = deflate(&stream, Z_FINISH);

    *compressed_length = stream.total_out;

    deflateEnd(&stream);

    if (result == Z_STREAM_END) {
        return Z_OK; // ѹ���ɹ�
    }
    else {
        return Z_DATA_ERROR; // ѹ��ʧ��
    }
}

int decompress_data(const char* compressed_data, uLong compressed_length, char* decompressed_data, uLong* decompressed_length) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (inflateInit(&stream) != Z_OK) {
        return Z_ERRNO; // ��ʼ����ѹ��ʧ��
    }

    stream.next_in = (Bytef*)compressed_data;
    stream.avail_in = (uInt)compressed_length;
    stream.next_out = (Bytef*)decompressed_data;
    stream.avail_out = (uInt)*decompressed_length;

    int result = inflate(&stream, Z_FINISH);

    *decompressed_length = stream.total_out;

    inflateEnd(&stream);

    if (result == Z_STREAM_END) {
        return Z_OK; // ��ѹ�ɹ�
    }
    else {
        return Z_DATA_ERROR; // ��ѹʧ��
    }
}

void pack(Packet_t* packet, uint8_t packet_type, const void* packet_data, size_t packet_length, int compression_level) {
    packet->type = packet_type;

    // ѹ��ǰ��δѹ������
    const char* uncompressed_data = (const char*)packet_data;
    uLong uncompressed_length = (uLong)packet_length;

    uLong compressed_length = MAX_PACKET_SIZE - sizeof(uint16_t) - sizeof(uint8_t);
    char* compressed_data = (char*)malloc(compressed_length);

    int result = compress_data(uncompressed_data, uncompressed_length, compressed_data, &compressed_length, compression_level);
    if (result == Z_OK) {
        memcpy(packet->data, compressed_data, compressed_length);
        packet->length = (uint16_t)(compressed_length + sizeof(uint16_t) + sizeof(uint8_t));
    }
    else {
        printf("ѹ��ʧ��\n");
        // ѹ��ʧ��
        packet->length = sizeof(uint16_t) + sizeof(uint8_t);
    }

    free(compressed_data);
}

void unpack(Packet_t* packet, void* output_data, size_t output_length) {
    if (packet->length <= sizeof(uint16_t) + sizeof(uint8_t)) {
        // ���ݳ��Ȳ���
        return;
    }

    uLong decompressed_length = (uLong)output_length;
    char* decompressed_data = (char*)output_data;

    int result = decompress_data((const char*)packet->data, packet->length - sizeof(uint16_t) - sizeof(uint8_t), decompressed_data, &decompressed_length);
    if (result == Z_OK) {
        // ��ѹ�ɹ�
    }
    else {
        printf("��ѹʧ��\n");
        // ��ѹʧ��
    }
}

int is_compressed_data_valid(const char* compressed_data, uLong compressed_length) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (inflateInit(&stream) != Z_OK) {
        return 0; // ��ʼ����ѹ��ʧ��
    }

    stream.next_in = (Bytef*)compressed_data;
    stream.avail_in = (uInt)compressed_length;

    int result = inflate(&stream, Z_NO_FLUSH);

    inflateEnd(&stream);

    if (result == Z_STREAM_END || result == Z_BUF_ERROR) {
        return 1; // ѹ��������Ч
    }
    else {
        return 0; // ѹ��������Ч
    }
}

void delay(int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

//int main() {
//    // ����ʾ��
//    Packet_t packet;
//    uint8_t packet_type = 1;
//    const char* data = "Hello, world!";
//    size_t data_length = strlen(data) + 1; // �����ַ�����β�� null �ַ�
//
//    pack(&packet, packet_type, data, data_length, Z_DEFAULT_COMPRESSION);
//    printf("Packet Length: %d\n", packet.length);
//
//    char output[1024];
//    unpack(&packet, output, sizeof(output));
//    printf("Uncompressed Data: %s\n", output);
//
//    return 0;
//}