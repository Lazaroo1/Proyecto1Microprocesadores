#include "Projectile.h"
#include <cmath>

Projectile::Projectile(double x, double y, double vx, double vy, int lifeTicks, int owner_) {
    pos.x = x; 
    pos.y = y;
    vel.x = vx; 
    vel.y = vy;
    life = lifeTicks;
    owner = owner_;
}

void Projectile::update(double dt, int maxx, int maxy) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    
    pos.x = fmod(pos.x, (double)maxx);
    if (pos.x < 0) pos.x += (double)maxx;
    
    double y_min = 1.0;
    double y_range = (double)(maxy - 1);
    pos.y = y_min + fmod(pos.y - y_min, y_range);
    if (pos.y < y_min) pos.y += y_range;
    
    life--;
}

bool Projectile::alive() const { 
    return life > 0; 
}