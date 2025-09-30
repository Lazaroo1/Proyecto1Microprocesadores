#ifndef SHIP_H
#define SHIP_H

#include <cmath>
#include <string>
#include <atomic>

struct Vec2 {
    double x = 0.0, y = 0.0;
};

class Ship {
public:
    Vec2 pos;
    Vec2 vel;
    double angle; // radianes (0 = derecha, pi/2 = abajo, -pi/2 = arriba)
    std::atomic<int> lives;  // Cambiar a atomic
    std::atomic<int> score;  // Cambiar a atomic

    Ship(double x = 0.0, double y = 0.0);
    
    void rotateLeft(double delta);
    void rotateRight(double delta);
    void thrust(double power);
    void update(double dt, int maxx, int maxy);
    char glyph() const;
    std::string glyphStr() const;
    void reset(double x, double y);
};

#endif