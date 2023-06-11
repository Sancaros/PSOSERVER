#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRADE_ITEMS 10 // ˫����ཻ����Ʒ��

// ��Ʒ�ṹ��
typedef struct {
    uint16_t id; // ��Ʒ ID
    const char* name; // ��Ʒ����
    uint8_t level; // ��Ʒ�ȼ���1 �� 200��
    uint16_t price; // ��Ʒ�۸�
} Item;

// ��ҽṹ��
typedef struct {
    char name[32]; // �������
    uint32_t meseta; // ���÷��������
    Item items[10]; // ��ұ������������ 10 ����Ʒ
    uint8_t item_count; // ��ǰ����Ѿ�ӵ�е���Ʒ����
    uint8_t trade_items[MAX_TRADE_ITEMS]; // ���Ҫ���׵���Ʒ�б�
    uint8_t trade_item_count; // ��ǰҪ���׵���Ʒ����
    uint8_t ready; // ����Ƿ�׼������
} Player;

// ������Ʒ���Ʋ�����Ʒ ID
uint16_t find_item_id(const char* item_name) {
    // ����������һ����Ʒ ID ��Ӧ��
    if (strcmp(item_name, "Photon Claw") == 0) {
        return 100;
    }
    else if (strcmp(item_name, "Flower Bouquet") == 0) {
        return 101;
    }
    else if (strcmp(item_name, "Dragon Slayer") == 0) {
        return 102;
    }
    else {
        return 0; // �����Ʒ������Ч���򷵻� 0
    }
}

// �����Ʒ����ұ�����
bool add_item(Player* player, const char* item_name) {
    // ��鵱ǰ�Ƿ��Ѿ�ӵ�������������Ʒ
    if (player->item_count >= 10) {
        return false;
    }

    // ������Ʒ���Ʋ�����Ʒ ID
    uint16_t item_id = find_item_id(item_name);
    if (item_id == 0) {
        return false;
    }

    // ����������һ��������Ʒ ID ������Ʒ��Ϣ�ı�
    Item item = { item_id, item_name, rand() % 200 + 1, rand() % 1000 + 1 };

    // ����Ʒ��ӵ�������
    player->items[player->item_count++] = item;

    return true;
}

// ����ұ������Ƴ���Ʒ
bool remove_item(Player* player, int index) {
    // ��������Ƿ�Խ��
    if (index < 0 || index >= player->item_count) {
        return false;
    }

    // ����ұ����е���Ʒ��ǰ�ƶ�һλ
    for (int i = index; i < player->item_count - 1; i++) {
        player->items[i] = player->items[i + 1];
    }

    // ������Ʒ����
    player->item_count--;

    return true;
}

// �������Ƿ�ӵ��ָ������Ʒ
bool has_item(const Player* player, uint16_t item_id) {
    for (int i = 0; i < player->item_count; i++) {
        if (player->items[i].id == item_id) {
            return true;
        }
    }
    return false;
}

// ���֮�佻����Ʒ
void trade_items(Player* player1, Player* player2) {
    // ���˫���Ƿ�׼������
    if (!player1->ready || !player2->ready) {
        return;
    }

    // ���˫��Ҫ���׵���Ʒ�����Ƿ�Ϸ�
    if (player1->trade_item_count > MAX_TRADE_ITEMS || player2->trade_item_count > MAX_TRADE_ITEMS) {
        return;
    }

    // ���˫��Ҫ���׵���Ʒ�Ƿ�Ϸ�����˫���Ƿ�ӵ����Щ��Ʒ��
    for (int i = 0; i < player1->trade_item_count; i++) {
        if (!has_item(player2, player1->trade_items[i])) {
            return;
        }
    }
    for (int i = 0; i < player2->trade_item_count; i++) {
        if (!has_item(player1, player2->trade_items[i])) {
            return;
        }
    }

    // ����˫���Ľ�����Ʒ���ܼ�ֵ
    uint32_t price1 = 0;
    for (int i = 0; i < player1->trade_item_count; i++) {
        for (int j = 0; j < player1->item_count; j++) {
            if (player1->items[j].id == player1->trade_items[i]) {
                price1 += player1->items[j].price;
                break;
            }
        }
    }
    uint32_t price2 = 0;
    for (int i = 0; i < player2->trade_item_count; i++) {
        for (int j = 0; j < player2->item_count; j++) {
            if (player2->items[j].id == player2->trade_items[i]) {
                price2 += player2->items[j].price;
                break;
            }
        }
    }

    // ���˫����÷�����Ƿ��㹻֧��������Ʒ���ܼ�ֵ
    if (player1->meseta < price2 || player2->meseta < price1) {
        return;
    }

    // ��������Ʒ����ұ������Ƴ�������������ӵ��Է��ı�����
    for (int i = 0; i < player1->trade_item_count; i++) {
        for (int j = 0; j < player1->item_count; j++) {
            if (player1->items[j].id == player1->trade_items[i]) {
                remove_item(player1, j);
                add_item(player2, player1->items[j].name);
                break;
            }
        }
    }
    for (int i = 0; i < player2->trade_item_count; i++) {
        for (int j = 0; j < player2->item_count; j++) {
            if (player2->items[j].id == player2->trade_items[i]) {
                remove_item(player2, j);
                add_item(player1, player2->items[j].name);
                break;
            }
        }
    }

    // ����˫����÷��������
    player1->meseta += price2 - price1;
    player2->meseta += price1 - price2;
    // ���˫���Ľ�����Ʒ�б��׼��״̬
    player1->trade_item_count = 0;
    player2->trade_item_count = 0;
    player1->ready = false;
    player2->ready = false;

    printf("%s �� %s ������һ�ν���\n", player1->name, player2->name);
}

int main() {
    // ģ���������֮��Ľ���
    Player player1 = { "Alice", 10000 };
    add_item(&player1, "Photon Claw");
    add_item(&player1, "Flower Bouquet");
    add_item(&player1, "Dragon Slayer");

    Player player2 = { "Bob", 5000 };
    add_item(&player2, "Flower Bouquet");
    add_item(&player2, "Dragon Slayer");

    // ���1Ҫ���� Photon Claw �� Flower Bouquet�����2Ҫ���� Dragon Slayer
    player1.trade_items[0] = 100;
    player1.trade_items[1] = 101;
    player1.trade_item_count = 2;

    player2.trade_items[0] = 102;
    player2.trade_item_count = 1;

    // ���1�����2��׼������
    player1.ready = true;
    player2.ready = true;

    // ���н���
    trade_items(&player1, &player2);

    // ������׺�������Ϣ
    printf("%s:\n", player1.name);
    printf("   Meseta: %u\n", player1.meseta);
    printf("   Items: ");
    for (int i = 0; i < player1.item_count; i++) {
        printf("%s, ", player1.items[i].name);
    }
    printf("\n");

    printf("%s:\n", player2.name);
    printf("   Meseta: %u\n", player2.meseta);
    printf("   Items: ");
    for (int i = 0; i < player2.item_count; i++) {
        printf("%s, ", player2.items[i].name);
    }
    printf("\n");

    return 0;
}