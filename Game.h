#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include "Ship.h"
#include "Asteroid.h"
#include "Projectile.h"

class Game {
public:
    Game();
    ~Game();

    void run(); // este es el loop principal del juego y maneja todo el flujo del juego

private:
    // screen size de la terminal
    int maxx, maxy;

    // objects del juego (2 ships, asteroids, bullets)
    Ship player;      // jugador 1
    Ship player2;     // jugador 2 (para modo 3)
    std::vector<Asteroid> asteroids;
    std::vector<Projectile> bullets;

    // state flags y variables para el juego
    bool quitFlag;
    bool paused;
    int mode; // 1,2,3
    int winScore;

    // metodos privados para manejar el juego y la logica
    void initNcurses();
    void shutdownNcurses();
    void mainMenu();
    void showInstructions();
    void showScores(); // placeholder por ahora
    // void showPauseScreen(); // pausa y resume pero no implementado aun
    void startGame();
    void spawnInitialAsteroids();
    void updateAll(double dt);
    void drawAll();
    void handleInput(int ch);
    void tryCollisions();
    double dist(double x1,double y1,double x2,double y2);
    void saveScoresAfterGame(bool twoPlayers);
};
#endif
