#ifndef SHIP_H
#define SHIP_H
// esta clase representa una nave en el juego
// tiene posicion, velocidad, angulo, vidas y puntaje
// usa radianes para el angulo (0 = derecha, pi/2 = abajo, -pi/2 = arriba, pi o -pi = izquierda)
// el angulo se usa para determinar la direccion de la nave y hacia donde apunta 
// tambien se usa para calcular la aceleracion cuando se aplica thrust
// el puntaje aumenta al destruir asteroides
// las vidas disminuyen al chocar con asteroides
//  y el juego termina si el puntaje alcanza un valor objetivo o si las vidas llegan a 0
#include <cmath>
#include <string>

struct Vec2 {
    double x = 0.0, y = 0.0;
};
 // clase Ship
class Ship {
public:
    Vec2 pos;
    Vec2 vel;
    double angle; // radianes (0 = derecha, pi/2 = abajo, -pi/2 = arriba, pi o -pi = izquierda)
    int lives;
    int score;
// constructor, con posicion inicial (x,y)
    Ship(double x = 0.0, double y = 0.0);
// metodos para controlar la nave
    void rotateLeft(double delta);
    void rotateRight(double delta);
    void thrust(double power);
    void update(double dt, int maxx, int maxy);
    char glyph() const;
    std::string glyphStr() const; // version string del glyph, el glyph es un char pero a veces es mas facil con string
    void reset(double x, double y);
};
// fin de la clase Ship
#endif
