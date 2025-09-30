#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "Ship.h"

struct Projectile {
    Vec2 pos;
    Vec2 vel;
    int life; 
    int owner; // 1 = player1, 2 = player2

    Projectile(double x, double y, double vx, double vy, int lifeTicks=60, int owner_ = 1);
    void update(double dt, int maxx, int maxy);
    bool alive() const;
};

#endif