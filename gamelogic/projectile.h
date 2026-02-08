#ifndef PVPMMO_BACKEND_PROJECTILE_H
#define PVPMMO_BACKEND_PROJECTILE_H

void shoot_projectile(int distance, int id, const struct InputBuffer *buffers, struct ProjectilePool *newProjectiles);
void explode_projectile(int i, struct ProjectilePool *projectiles, struct intPool *explodingProjectiles);
void detect_collission_projectiles(struct ProjectilePool *projectiles, struct Player players, struct intPool *explodingProjectiles);
void move_projectiles(struct ProjectilePool *projectiles, struct intPool *explodingProjectiles);
#endif //PVPMMO_BACKEND_PROJECTILE_H