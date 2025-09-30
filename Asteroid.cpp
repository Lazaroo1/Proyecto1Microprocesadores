#include "Asteroid.h"
#include <cmath>

Asteroid::Asteroid(double x, double y, double vx, double vy, int size_) {
    pos.x = x; 
    pos.y = y; 
    vel.x = vx; 
    vel.y = vy; 
    size = size_;
}

void Asteroid::update(double dt, int maxx, int maxy) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    
    pos.x = fmod(pos.x, (double)maxx);
    if (pos.x < 0) pos.x += (double)maxx;
    
    double y_min = 1.0;
    double y_range = (double)(maxy - 1);
    pos.y = y_min + fmod(pos.y - y_min, y_range);
    if (pos.y < y_min) pos.y += y_range;
}

char Asteroid::glyph() const { 
    return (size >= 2) ? 'O' : 'o'; 
}

double Asteroid::radius() const { 
    return (size >= 2) ? 1.8 : 0.9; 
}