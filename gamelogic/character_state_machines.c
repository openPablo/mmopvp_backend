#include "../gameDataStructures.h"
#include "projectile.h"

int applyMovement(struct Player *player, struct InputBuffer *buffer) {
    player->angle = buffer->angle;
    player->x += buffer->dir_x * SPEED;
    player->y += buffer->dir_y * SPEED;
    buffer->dir_x  = 0.0f;
    buffer->dir_y  = 0.0f;
    return buffer->dir_x != 0 || buffer->dir_y != 0;
}
int castSpell(struct Player *player, struct InputBuffer *buffer, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    if (buffer->castSpell > 0) {
        player->animating_ms = HERO_DATA[player->hero].cast_time_ms;
        shoot_projectile(player->id,buffer,projectiles, newProjectiles);
        return 1;
    }
    return 0;
}

void compute_airmage_state(struct Player *player, struct InputBuffer *buffer, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    switch (player->state) {
        case IDLE:
            if (applyMovement(player,buffer)) {
                player->state = WALKING;
            }
            if (castSpell(player, buffer, projectiles,newProjectiles)) {
                player->state = buffer->castSpell;
                buffer->castSpell = 0;
            }
            break;
        case WALKING:
            if (!applyMovement(player,buffer)) {
                player->state = IDLE;
            }
            if (castSpell(player, buffer, projectiles,newProjectiles)) {
                player->state = buffer->castSpell;
                buffer->castSpell = 0;
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