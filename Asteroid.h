#ifndef ASTEROID_H
#define ASTEROID_H

#include "Ship.h"

struct Asteroid {
    Vec2 pos;
    Vec2 vel;
    int size; // 2 = grande, 1 = pequeño
    
    Asteroid(double x, double y, double vx, double vy, int size_);
    void update(double dt, int maxx, int maxy);
    char glyph() const;
    double radius() const;
};

#endif