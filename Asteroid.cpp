#include "Asteroid.h"
#include <cmath>
//este archivo contiene la implementacion de los metodos de la clase Asteroid
// ver Asteroid.h para mas detalles
Asteroid::Asteroid(double x, double y, double vx, double vy, int size_) {
    pos.x = x; pos.y = y; vel.x = vx; vel.y = vy; size = size_;
}

void Asteroid::update(double dt, int maxx, int maxy) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    if (pos.x < 0) pos.x += maxx;
    if (pos.x >= maxx) pos.x -= maxx;
    if (pos.y < 1) pos.y += (maxy-1);
    if (pos.y >= maxy) pos.y -= (maxy-1);
}

char Asteroid::glyph() const { return (size >= 2) ? 'O' : 'o'; }

double Asteroid::radius() const { return (size >= 2) ? 1.8 : 0.9; }
