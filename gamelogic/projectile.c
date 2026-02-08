#include "../gameDataStructures.h"
#include <math.h>

void shoot_projectile(int id, const struct InputBuffer *buffers, struct ProjectilePool *projectiles, struct ProjectilePool *newProjectiles) {
    struct Projectile projectile = {
        .x=buffers[id].x,
        .y=buffers[id].y,
        .dx=(float)cos(M_PI / 180 * buffers[id].angle),
        .dy=(float)sin(M_PI / 180 * buffers[id].angle),
        .id=(uint16_t)id,
        .travelled = PROJECTILE_DISTANCE,
        .castSpell=buffers[id].castSpell
    };
    projectiles->array[projectiles->length] = projectile;
    projectiles->length++;
    newProjectiles->array[newProjectiles->length] = projectile;
    newProjectiles->length++;
}
void explode_projectile(int i, struct ProjectilePool *projectiles, struct intPool *explodingProjectiles) {
    explodingProjectiles->array[explodingProjectiles->length] = projectiles->array[i].id;
    explodingProjectiles->length++;
    projectiles->array[i] = projectiles->array[projectiles->length - 1];
    projectiles->length--;
}
void detect_collission_projectiles(struct ProjectilePool *projectiles, struct Player players, struct intPool *explodingProjectiles) {
    for (int i = 0; i < projectiles->length; i++) {
        explode_projectile(i, projectiles, explodingProjectiles);
        i--;
    }
}
void move_projectiles(struct ProjectilePool *projectiles, struct intPool *explodingProjectiles) {
    for (int i = 0; i < projectiles->length; i++) {
        if (projectiles->array[i].travelled <= 0) {
            explode_projectile(i,projectiles, explodingProjectiles);
            i--;
        }else {
            projectiles->array[i].x += projectiles->array[i].dx * PROJECTILE_SPEED;
            projectiles->array[i].y += projectiles->array[i].dy * PROJECTILE_SPEED;
            projectiles->array[i].travelled -= TICK_RATE_MS;
        }
    }
}