#ifndef PICKING_HPP
#define PICKING_HPP

#include "render.hpp"

void set_picking_name(size_t name);
void set_picking_object(GlobalState* state, CelestialBody* object);
void clear_picking_object(GlobalState* state);
CelestialBody* pick(GlobalState* state);

#endif
