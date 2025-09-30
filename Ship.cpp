#include "Ship.h"

Ship::Ship(double x, double y) {
    pos.x = x; 
    pos.y = y;
    vel.x = vel.y = 0.0;
    angle = -M_PI/2; // apunta arriba
    lives.store(3);  // Use store() para inicializar atomic 
    score.store(0);
}

void Ship::rotateLeft(double delta) { 
    angle -= delta; 
}

void Ship::rotateRight(double delta) { 
    angle += delta; 
}

void Ship::thrust(double power) {
    vel.x += cos(angle) * power;
    vel.y += sin(angle) * power;
    
    // limitar velocidad maxima a 3.5 unidades para evitar que se salga de control 
    double speed = sqrt(vel.x*vel.x + vel.y*vel.y);
    double maxs = 4.2;
    if (speed > maxs) {
        vel.x = vel.x / speed * maxs;
        vel.y = vel.y / speed * maxs;
    }
}

void Ship::update(double dt, int maxx, int maxy) {
    // aplicar friccion suave para simular espacio vacío 
    vel.x *= 0.99;
    vel.y *= 0.99;
    
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    pos.x = fmod(pos.x, (double)maxx);
    if (pos.x < 0) pos.x += (double)maxx;
    
    double y_min = 1.0;
    double y_range = (double)(maxy - 1);
    pos.y = y_min + fmod(pos.y - y_min, y_range);
    if (pos.y < y_min) pos.y += y_range;
}

char Ship::glyph() const {
    double a = angle;
    double pi2 = M_PI/2.0;
    
    // normalizar angulo a rango [-pi, pi]
    while (a > M_PI) a -= 2*M_PI;
    while (a < -M_PI) a += 2*M_PI;
    
    if (a >= -pi2/2 && a < pi2/2) return '>';
    if (a >= pi2/2 && a < 3*pi2/2) return 'v';
    if (a >= 3*pi2/2 || a < -3*pi2/2) return '<';
    return '^';
}

std::string Ship::glyphStr() const {
    char g = glyph();
    return std::string(1, g);
}

void Ship::reset(double x, double y) {
    pos.x = x; 
    pos.y = y;
    vel.x = vel.y = 0;
    angle = -M_PI/2;
}