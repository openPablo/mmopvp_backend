#include "../gameDataStructures.h"

#ifndef PVPMMO_BACKEND_AREA_OF_EFFECT_H
#define PVPMMO_BACKEND_AREA_OF_EFFECT_H

#endif
void cast_aoe_circle(int duration_ms,int distance, int radius, int id, const struct InputBuffer *buffer, struct AoECirclePool *newCircles);
void cast_aoe_cone(int duration_ms, int length, int id, const struct InputBuffer *buffer, struct AoEConePool *newCones);
void timelapse_cones(struct AoEConePool *cones);
void timelapse_circles(struct AoECirclePool *circles);
