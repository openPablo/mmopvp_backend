#include "area_of_effect.h"

#include <math.h>


void cast_aoe_circle(int duration_ms,int distance, int radius, int id, const struct InputBuffer *buffer, struct AoECirclePool *newCircles) {
    struct AoECircle circle = {
        .x=buffer->x + (float)distance * cosf(buffer->angle * (M_PI / 180.0f)),
        .y=buffer->y + (float)distance * sin(buffer->angle * (M_PI / 180.0f)),
        .radius=radius,
        .time_ms = duration_ms,
        .id=(uint16_t)id,
        .castSpell=buffer->castSpell
    };
    newCircles->array[newCircles->length] = circle;
    newCircles->length++;
}
void cast_aoe_cone(int duration_ms, int length, int id, const struct InputBuffer *buffer, struct AoEConePool *newCones) {
    struct AoECone cone = {
        .x=buffer->x,
        .y=buffer->y,
        .angle=buffer->angle,
        .time_ms = duration_ms,
        .id=(uint16_t)id,
        .length = length,
        .castSpell=buffer->castSpell
    };
    newCones->array[newCones->length] = cone;
    newCones->length++;
}
void timelapse_cones(struct AoEConePool *cones) {
    for (int i = 0; i < cones->length; i++) {
        cones->array[i].time_ms -= TICK_RATE_MS;
        if (cones->array[i].time_ms <= 0) {
            cones->array[i] = cones->array[cones->length - 1];
            cones->length--;
            i--;
        }
    }
}
void timelapse_circles(struct AoECirclePool *circles) {
    for (int i = 0; i < circles->length; i++) {
        circles->array[i].time_ms -= TICK_RATE_MS;
        if (circles->array[i].time_ms <= 0) {
            circles->array[i] = circles->array[circles->length - 1];
            circles->length--;
            i--;
        }
    }
}