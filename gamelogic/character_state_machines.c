#include "../gameDataStructures.h"
#include "projectile.h"
#include "selfCast.h"
#include "area_of_effect.h"

int applyMovement(struct Player *player, struct InputBuffer *buffer) {
    player->angle = buffer->angle;
    if (buffer->dir_x != 0 || buffer->dir_y != 0) {
        player->x += buffer->dir_x * SPEED;
        player->y += buffer->dir_y * SPEED;
        buffer->dir_x  = 0.0f;
        buffer->dir_y  = 0.0f;
        return 1;
    }
    return 0;
}
void setSpell(int animating_ms, struct Player *player, struct InputBuffer *buffer) {
    player->animating_ms = 400;
    player->state = buffer->castSpell;
    buffer->castSpell = 0;
}
void compute_airmage_state(struct Player *player, struct InputBuffer *buffer, struct SpellsContext *newSpells) {
    switch (player->state) {
        case IDLE:
        case WALKING:
            switch (buffer->castSpell) {
                case CASTING_1:
                    shoot_projectile(400, player->id,buffer, &newSpells->projectiles);
                    setSpell(400, player, buffer);
                    return;
                case CASTING_2:
                    cast_aoe_cone(400, 100, player->id, buffer, &newSpells->cones);
                    setSpell(400, player, buffer);
                    return;
                case CASTING_3:
                    blink(300, player);
                    setSpell(400, player, buffer);
                    return;
                case CASTING_ULTI:
                    cast_aoe_circle(4000, 400, 100, player->id, buffer, &newSpells->circles);
                    setSpell(400, player, buffer);
                    return;
            }
            if (applyMovement(player,buffer)) {
                player->state = WALKING;
            } else {
                player->state = IDLE;
            }
            break;
        case CASTING_1:
        case CASTING_2:
        case CASTING_3:
        case CASTING_ULTI:
            player->animating_ms -= TICK_RATE_MS;
            if (player->animating_ms <= 0) {
                player->state = IDLE;
            }
            break;
        case DEAD:
        case INVULNERABLE:
        case STUNNED:
            break;
    }
}