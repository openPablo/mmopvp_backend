#include "../gameDataStructures.h"
#include <math.h>

void shoot_projectile(int distance, uint16_t id, const struct InputBuffer *buffer, struct ProjectilePool *newProjectiles) {
    struct Projectile projectile = {
        .x=buffer->x,
        .y=buffer->y,
        .dx=(float)cos(M_PI / 180 * buffer->angle),
        .dy=(float)sin(M_PI / 180 * buffer->angle),
        .id=id,
        .travelled = distance,
        .castSpell=buffer->castSpell
    };
    newProjectiles->array[newProjectiles->length] = projectile;
    newProjectiles->length++;
}
void explode_projectile(int i, struct ProjectilePool *projectiles, struct shortPool *explodingProjectiles) {
    explodingProjectiles->array[explodingProjectiles->length] = projectiles->array[i].id;
    explodingProjectiles->length++;
    projectiles->array[i] = projectiles->array[projectiles->length - 1];
    projectiles->length--;

}

void move_projectiles(struct ProjectilePool *projectiles, struct shortPool *explodingProjectiles) {
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