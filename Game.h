#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Ship.h"
#include "Asteroid.h"
#include "Projectile.h"

// esta clase maneja el juego con hilos POSIX (fase 3)
// arquitectura: 5 hilos principales + 5 auxiliares = 10 total
class Game {
public:
    Game();
    ~Game();

    void run(); // loop principal del juego y manejo de menú

    // banderas globales (atomic para thread-safety sin mutex)
    std::atomic<bool> quitFlag;
    std::atomic<bool> paused;
    std::atomic<bool> gameRunning;
    std::atomic<bool> returnToMenu;
    
    int maxx, maxy; // tamaño pantalla
    int mode; // 1,2,3
    int winScore;

    // objetos compartidos del juego 
    Ship player;
    Ship player2;
    std::vector<Asteroid> asteroids;
    std::vector<Projectile> bullets;

    // mutex globales para proteger acceso a objetos compartidos
    std::mutex mtxShips;
    std::mutex mtxAsteroids;
    std::mutex mtxBullets;
    std::mutex mtxDraw;
    std::mutex mtxGameState;
    std::mutex mtxNcurses;  // NEW: For protecting ncurses calls

    // sincronización con condition variables
    std::condition_variable cvUpdate;
    std::condition_variable cvDraw;

private:
    // métodos privados para pantallas y setup
    void initNcurses();
    void shutdownNcurses();
    void mainMenu();
    void showInstructions();
    void showScores(); 
    void startGame();
    void spawnInitialAsteroids();
    void saveScoresAfterGame(bool twoPlayers);
    void resetGame();
    void checkWinLoseConditions();
    void showEndGameScreen();

    // === HILOS PRINCIPALES (5) ===
    // hilo 1: captura input del usuario
    static void* inputThread(void* arg);
    
    // hilo 2: actualiza física del juego
    static void* updateThread(void* arg);
    
    // hilo 3: detecta colisiones
    static void* collisionThread(void* arg);
    
    // hilo 4: renderiza todo en pantalla
    static void* drawThread(void* arg);
    
    // hilo 5: controla lógica de ganar/perder
    static void* gameLogicThread(void* arg);

    // === HILOS AUXILIARES (5) ===
    // hilo 6: actualiza asteroides específicamente
    static void* asteroidUpdateThread(void* arg);
    
    // hilo 7: actualiza proyectiles específicamente
    static void* projectileUpdateThread(void* arg);
    
    // hilo 8: actualiza nave 1
    static void* ship1UpdateThread(void* arg);
    
    // hilo 9: actualiza nave 2 (solo en modo 3)
    static void* ship2UpdateThread(void* arg);
    
    // hilo 10: actualiza HUD y stats
    static void* hudUpdateThread(void* arg);

    // helpers internos
    void handleInput(int ch);
    double dist(double x1, double y1, double x2, double y2);
    void tryCollisions();
    void drawAll();
};

#endif