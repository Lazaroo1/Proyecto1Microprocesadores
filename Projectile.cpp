#include "Projectile.h"
#include <cmath>
//aqui van los metodos de la clase Projectile
// ver Projectile.h para mas detalles
Projectile::Projectile(double x, double y, double vx, double vy, int lifeTicks, int owner_) {
    pos.x = x; pos.y = y;
    vel.x = vx; vel.y = vy;
    life = lifeTicks;
    owner = owner_;
}

void Projectile::update(double dt, int maxx, int maxy) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;
    // wrap
    if (pos.x < 0) pos.x += maxx;
    if (pos.x >= maxx) pos.x -= maxx;
    if (pos.y < 1) pos.y += (maxy-1);
    if (pos.y >= maxy) pos.y -= (maxy-1);
    life--;
}

bool Projectile::alive() const { return life > 0; }
