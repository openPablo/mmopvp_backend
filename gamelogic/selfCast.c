#include "../gameDataStructures.h"
#include "selfCast.h"
#include <math.h>
// todo: add boundschecking when done implementing collission
void blink(float distance ,struct Player *player) {
    player->x += cosf(player->angle * (M_PI / 180.0f)) * distance;
    player->y += sinf(player->angle * (M_PI / 180.0f)) * distance;
}
