#include "Ship.h"
//aqui van los metodos de la clase Ship
// ver Ship.h para mas detalles
Ship::Ship(double x, double y) {
    pos.x = x; pos.y = y;
    vel.x = vel.y = 0.0;
    angle = -M_PI/2; // pointing "up"
    lives = 3;
    score = 0;
}

void Ship::rotateLeft(double delta) { angle -= delta; }
void Ship::rotateRight(double delta) { angle += delta; }
void Ship::thrust(double power) {
    // aqui se aplica la aceleracion en la direccion actual de la nave (angle) 
    vel.x += cos(angle) * power;
    vel.y += sin(angle) * power;
    // con esto se limita la velocidad maxima de la nave a un valor maxs (2.2) 
    double speed = sqrt(vel.x*vel.x + vel.y*vel.y);
    double maxs = 2.2;
    if (speed > maxs) {
        vel.x = vel.x / speed * maxs;
        vel.y = vel.y / speed * maxs;
    }
}
 // actualiza la posicion de la nave segun su velocidad y el tiempo dt transcurrido 
void Ship::update(double dt, int maxx, int maxy) {
    pos.x += vel.x * dt;
    pos.y += vel.y * dt;

    // esto hace el wrap-around en los bordes de la pantalla para que la nave reaparezca del otro lado
    // la fila 0 se reserva para el HUD, por eso pos.y < 1

    if (pos.x < 0) pos.x += maxx;
    if (pos.x >= maxx) pos.x -= maxx;
    if (pos.y < 1) pos.y += (maxy-1); // keep row 0 for HUD
    if (pos.y >= maxy) pos.y -= (maxy-1);
}

char Ship::glyph() const {
    // direccion aproximada basada en el angulo de la nave
    double a = angle;
    double pi2 = M_PI/2.0;
    if (a >= -pi2/2 && a < pi2/2) return '>';
    if (a >= pi2/2 && a < 3*pi2/2) return 'v';
    if (a < -3*pi2/2 || a >= 3*pi2/2) return '<';
    return '^';
}
 
std::string Ship::glyphStr() const {
    char g = glyph();
    return std::string(1, g);
}

// resetea la nave a una posicion dada, con velocidad 0 y apuntando "arriba" 
void Ship::reset(double x, double y) {
    pos.x = x; pos.y = y;
    vel.x = vel.y = 0;
    angle = -M_PI/2;
}
