//
// Created by pablo on 2/8/26.
//

#include "area_of_effect.h"
void cast_aoe_cone(int length, int id, const struct InputBuffer *buffer, struct AoEConePool *newCones) {
    struct AoECone cone = {
        .x=buffer->x,
        .y=buffer->y,
        .id=(uint16_t)id,
        .length = length,
        .castSpell=buffer->castSpell
    };
    newCones->array[newCones->length] = cone;
    newCones->length++;
}
