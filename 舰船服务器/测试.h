#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRADE_ITEMS 10 // 双方最多交易物品数

// 物品结构体
typedef struct {
    uint16_t id; // 物品 ID
    const char* name; // 物品名称
    uint8_t level; // 物品等级（1 到 200）
    uint16_t price; // 物品价格
} Item;

// 玩家结构体
typedef struct {
    char name[32]; // 玩家名称
    uint32_t meseta; // 玩家梅塞塔数量
    Item items[10]; // 玩家背包中最多容纳 10 个物品
    uint8_t item_count; // 当前玩家已经拥有的物品数量
    uint8_t trade_items[MAX_TRADE_ITEMS]; // 玩家要交易的物品列表
    uint8_t trade_item_count; // 当前要交易的物品数量
    uint8_t ready; // 玩家是否准备就绪
} Player;

// 根据物品名称查找物品 ID
uint16_t find_item_id(const char* item_name) {
    // 假设这里是一个物品 ID 对应表
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
        return 0; // 如果物品名称无效，则返回 0
    }
}

// 添加物品到玩家背包中
bool add_item(Player* player, const char* item_name) {
    // 检查当前是否已经拥有最大数量的物品
    if (player->item_count >= 10) {
        return false;
    }

    // 根据物品名称查找物品 ID
    uint16_t item_id = find_item_id(item_name);
    if (item_id == 0) {
        return false;
    }

    // 假设这里是一个根据物品 ID 查找物品信息的表
    Item item = { item_id, item_name, rand() % 200 + 1, rand() % 1000 + 1 };

    // 将物品添加到背包中
    player->items[player->item_count++] = item;

    return true;
}

// 从玩家背包中移除物品
bool remove_item(Player* player, int index) {
    // 检查索引是否越界
    if (index < 0 || index >= player->item_count) {
        return false;
    }

    // 将玩家背包中的物品向前移动一位
    for (int i = index; i < player->item_count - 1; i++) {
        player->items[i] = player->items[i + 1];
    }

    // 减少物品数量
    player->item_count--;

    return true;
}

// 检查玩家是否拥有指定的物品
bool has_item(const Player* player, uint16_t item_id) {
    for (int i = 0; i < player->item_count; i++) {
        if (player->items[i].id == item_id) {
            return true;
        }
    }
    return false;
}

// 玩家之间交易物品
void trade_items(Player* player1, Player* player2) {
    // 检查双方是否都准备就绪
    if (!player1->ready || !player2->ready) {
        return;
    }

    // 检查双方要交易的物品数量是否合法
    if (player1->trade_item_count > MAX_TRADE_ITEMS || player2->trade_item_count > MAX_TRADE_ITEMS) {
        return;
    }

    // 检查双方要交易的物品是否合法（即双方是否都拥有这些物品）
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

    // 计算双方的交易物品的总价值
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

    // 检查双方的梅塞塔是否足够支付交易物品的总价值
    if (player1->meseta < price2 || player2->meseta < price1) {
        return;
    }

    // 将交易物品从玩家背包中移除，并将它们添加到对方的背包中
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

    // 更新双方的梅塞塔数量
    player1->meseta += price2 - price1;
    player2->meseta += price1 - price2;
    // 清空双方的交易物品列表和准备状态
    player1->trade_item_count = 0;
    player2->trade_item_count = 0;
    player1->ready = false;
    player2->ready = false;

    printf("%s 和 %s 进行了一次交易\n", player1->name, player2->name);
}

int main() {
    // 模拟两个玩家之间的交易
    Player player1 = { "Alice", 10000 };
    add_item(&player1, "Photon Claw");
    add_item(&player1, "Flower Bouquet");
    add_item(&player1, "Dragon Slayer");

    Player player2 = { "Bob", 5000 };
    add_item(&player2, "Flower Bouquet");
    add_item(&player2, "Dragon Slayer");

    // 玩家1要交易 Photon Claw 和 Flower Bouquet，玩家2要交易 Dragon Slayer
    player1.trade_items[0] = 100;
    player1.trade_items[1] = 101;
    player1.trade_item_count = 2;

    player2.trade_items[0] = 102;
    player2.trade_item_count = 1;

    // 玩家1和玩家2都准备就绪
    player1.ready = true;
    player2.ready = true;

    // 进行交易
    trade_items(&player1, &player2);

    // 输出交易后的玩家信息
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