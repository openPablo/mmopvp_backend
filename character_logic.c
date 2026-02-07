//
// Created by pablo on 2/7/26.
//
#include "gameDataStructures.h"
#include "character_logic.h"

//void buffer_status_to_players(struct Player *players, struct InputBuffer *buffers, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
//    for (int i = 0; i < MAX_PLAYERS; i++) {
//        if (players[i].flags & ACTIVE) {
//            players[i].x += buffers[i].dir_x * SPEED;
//            players[i].y += buffers[i].dir_y * SPEED;
//            players[i].angle = buffers[i].angle;
//            buffers[i].dir_x  = 0.0f;
//            buffers[i].dir_y  = 0.0f;
//            if (buffers[i].castSpell > 0 && !(players[i].flags & IS_CASTING)) {
//                buffers[i].castSpell &= -buffers[i].castSpell;
//                players[i].flags |= buffers[i].castSpell;
//                players[i].animating_ms = HERO_DATA[players[i].hero].cast_time_ms;
//                shoot_projectile(players[i].id,buffers,projectiles, newProjectiles); //todo: Check if spell casted needs to shoot a projectile or not
//                buffers[i].castSpell = 0;
//            }
//            if (players[i].flags & IS_CASTING) {
//                players[i].animating_ms -= TICK_RATE_MS;
//                if (players[i].animating_ms <= 0) {
//                    players[i].flags &= ~IS_CASTING;
//                }
//            }
//
//        }
//    }
//}

