#ifndef ASTEROID_H
#define ASTEROID_H

#include "Ship.h"
//esta clase representa un asteroide en el juego
// tiene posicion, velocidad y tamaño (grande o pequeño) 
struct Asteroid {
    Vec2 pos;
    Vec2 vel;
    int size; // 2 = grnade, 1 = pequeño
    Asteroid(double x, double y, double vx, double vy, int size_);
    void update(double dt, int maxx, int maxy);
    char glyph() const;
    double radius() const;
};
#endif
