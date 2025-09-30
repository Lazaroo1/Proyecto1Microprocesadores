#include "Game.h"
#include <ncurses.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <fstream>
#include <cstring>

Game::Game()
: player(40, 12), player2(40, 14), quitFlag(false), paused(false), 
  gameRunning(false), returnToMenu(false), mode(1), winScore(60) {
    srand(time(nullptr));
    initscr();
    getmaxyx(stdscr, maxy, maxx);
    shutdownNcurses(); 
}

Game::~Game() {}

void Game::initNcurses() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE); 
    timeout(0);
    getmaxyx(stdscr, maxy, maxx);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_GREEN, -1);
        init_pair(3, COLOR_YELLOW, -1);
        init_pair(4, COLOR_WHITE, -1);
    }
}

void Game::shutdownNcurses() {
    endwin();
}

void Game::run() {
    initNcurses();
    while (!quitFlag) {
        mainMenu();
    }
    shutdownNcurses();
}

void Game::mainMenu() {
    clear();
    int highlight = 0;
    std::vector<std::string> options = {
        "Iniciar partida",
        "Instrucciones",
        "Puntajes",
        "Salir"
    };
    int choice = -1;

    while (choice == -1) {
        getmaxyx(stdscr, maxy, maxx);
        clear();
        std::string title = "ASTEROIDS (FASE 3 - HILOS)";
        mvprintw(2, (maxx - (int)title.size())/2, "%s", title.c_str());

        std::string modeStr = "Modo: 1 (3 ast.)  2 (5 ast.)  3 (2 players) - Usa teclas 1/2/3";
        mvprintw(4, (maxx - (int)modeStr.size())/2, "%s", modeStr.c_str());
        mvprintw(5, (maxx - 30)/2, "Meta: %d pts", (mode==1)?60:((mode==2)?100:100));

        for (int i=0; i<(int)options.size(); ++i) {
            int y = 8 + i*2;
            if (i == highlight) attron(A_REVERSE);
            mvprintw(y, (maxx - (int)options[i].size())/2, "%s", options[i].c_str());
            if (i == highlight) attroff(A_REVERSE);
        }
        mvprintw(maxy-2, 2, "Usa teclas de flecha para navegar, Enter para seleccionar.");

        int ch = ERR;
        {
            std::lock_guard<std::mutex> lock(mtxNcurses);  // protege getch de ncurses 
            ch = getch();
        }
        if (ch == KEY_UP) highlight = (highlight - 1 + (int)options.size()) % (int)options.size();
        else if (ch == KEY_DOWN) highlight = (highlight + 1) % (int)options.size();
        else if (ch == '1') { mode = 1; winScore = 60; }
        else if (ch == '2') { mode = 2; winScore = 100; }
        else if (ch == '3') { mode = 3; winScore = 100; }
        else if (ch == '\n' || ch == KEY_ENTER) choice = highlight;
        else if (ch == 'q' || ch == 'Q') { quitFlag = true; return; }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        refresh();
    }

    if (choice == 0) startGame();
    else if (choice == 1) showInstructions();
    else if (choice == 2) showScores();
    else if (choice == 3) { quitFlag = true; }
}

void Game::startGame() {
    // preparar estado inicial del juego 
    resetGame();
    gameRunning = true;
    paused = false;
    returnToMenu = false;

    // crear los 10 hilos 
    pthread_t threads[10];
    
    // hilos principales
    pthread_create(&threads[0], NULL, inputThread, this);
    pthread_create(&threads[1], NULL, updateThread, this);
    pthread_create(&threads[2], NULL, collisionThread, this);
    pthread_create(&threads[3], NULL, drawThread, this);
    pthread_create(&threads[4], NULL, gameLogicThread, this);
    
    // hilos auxiliares
    pthread_create(&threads[5], NULL, asteroidUpdateThread, this);
    pthread_create(&threads[6], NULL, projectileUpdateThread, this);
    pthread_create(&threads[7], NULL, ship1UpdateThread, this);
    pthread_create(&threads[8], NULL, ship2UpdateThread, this);
    pthread_create(&threads[9], NULL, hudUpdateThread, this);

    // esperar a que terminen todos
    for (int i = 0; i < 10; i++) {
        pthread_join(threads[i], NULL);
    }


    showEndGameScreen();

    returnToMenu = true;
}

void Game::resetGame() {
    getmaxyx(stdscr, maxy, maxx);
    player.reset(maxx/3.0, maxy/2.0);
    player.lives.store(3);
    player.score.store(0);
    player2.reset(2*maxx/3.0, maxy/2.0);
    player2.lives.store(3);
    player2.score.store(0);
    bullets.clear();
    asteroids.clear();
    paused = false;
    returnToMenu = false;
    spawnInitialAsteroids();
}

void Game::spawnInitialAsteroids() {
    std::lock_guard<std::mutex> lock(mtxAsteroids);
    int count = (mode == 1) ? 3 : 5;
    if (mode == 3) count = 5;
    for (int i=0; i<count; ++i) {
        double x = rand() % (maxx-8) + 4;
        double y = (rand() % (maxy-8)) + 2;
        if (fabs(x - player.pos.x) < 8 && fabs(y - player.pos.y) < 4) { x += 10; y += 3; }
        if (fabs(x - player2.pos.x) < 8 && fabs(y - player2.pos.y) < 4) { x -= 10; y -= 3; }
        double vx = ((rand()%200)/100.0 - 1.0) * 0.8;
        double vy = ((rand()%200)/100.0 - 1.0) * 0.8;
        if (fabs(vx) < 0.1) vx = 0.3;
        if (fabs(vy) < 0.1) vy = -0.3;
        asteroids.emplace_back(x, y, vx, vy, 2);
    }
}

//============================================================================
// HILOS PRINCIPALES (5)
//============================================================================

void* Game::inputThread(void* arg) {
    Game* g = (Game*)arg;
    
// El bucle depende de gameRunning para salir 
    while (g->gameRunning) { 
        // Si gameRunning es false, saldrá inmediatamente
        if (!g->gameRunning) break; 
        
        int ch = ERR;
        {
            // La protección con mutex es correcta
            std::lock_guard<std::mutex> lock(g->mtxNcurses);  
            ch = getch();
        }
        if (ch != ERR) {
            g->handleInput(ch);
        }
        usleep(20000); // 50 Hz input polling para reducir CPU 
    }
    
    return NULL;
}

void* Game::updateThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            g->cvUpdate.notify_all(); 
        }
        usleep(33000); // ~30 Hz update rate para reducir CPU
    }
    
    return NULL;
}

void* Game::collisionThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            std::lock_guard<std::mutex> lockShips(g->mtxShips);
            std::lock_guard<std::mutex> lockAst(g->mtxAsteroids);
            std::lock_guard<std::mutex> lockBul(g->mtxBullets);
            
            g->tryCollisions();
        }
        usleep(50000); // 20 Hz collision detection para reducir CPU 
    }
    
    return NULL;
}

void* Game::drawThread(void* arg) {
    Game* g = (Game*)arg;
    
    // Solo depende de gameRunning para salir del bucle
    while (g->gameRunning) { 
        if (!g->gameRunning) break; 

        {
            std::lock_guard<std::mutex> lock(g->mtxNcurses);
            g->drawAll();
        }
        // Se mantiene el usleep, pero el bucle depende de la bandera de gameRunning
        usleep(33000); 
    }
    
    return NULL;
}

void* Game::gameLogicThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) { 
        if (!g->gameRunning) break; 

        if (!g->paused) {
            bool shouldEnd = false;
            {
                std::lock_guard<std::mutex> lock(g->mtxShips);
                if (g->mode == 3) {
                    if (g->player.lives.load() <= 0 && g->player2.lives.load() <= 0) {
                        shouldEnd = true;
                    }
                    else if (g->player.score.load() >= g->winScore || g->player2.score.load() >= g->winScore) {
                        shouldEnd = true;
                    }
                } else {
                    if (g->player.lives.load() <= 0) {
                        shouldEnd = true;
                    }
                    else if (g->player.score.load() >= g->winScore) {
                        shouldEnd = true;
                    }
                }
            }
            
            if (shouldEnd) {

                g->gameRunning = false; // Bajar bandera
                g->returnToMenu = true; // Bajar bandera secundaria (para menu)

                break;
            }
        }
        usleep(50000); 
    }
    
    return NULL;
}

//============================================================================
// HILOS AUXILIARES (5)
//============================================================================

void* Game::asteroidUpdateThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            std::lock_guard<std::mutex> lock(g->mtxAsteroids);
            for (auto &a : g->asteroids) {
                a.update(0.033, g->maxx, g->maxy);
            }
        }
        usleep(33000);
    }
    
    return NULL;
}

void* Game::projectileUpdateThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            std::lock_guard<std::mutex> lock(g->mtxBullets);
            for (auto &b : g->bullets) {
                b.update(0.2, g->maxx, g->maxy);
            }
            g->bullets.erase(
                std::remove_if(g->bullets.begin(), g->bullets.end(),
                    [](const Projectile& p){ return !p.alive(); }),
                g->bullets.end()
            );
        }
        usleep(25000);
    }
    
    return NULL;
}

void* Game::ship1UpdateThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            std::lock_guard<std::mutex> lock(g->mtxShips);
            g->player.update(0.033, g->maxx, g->maxy);
        }
        usleep(33000);
    }
    
    return NULL;
}

void* Game::ship2UpdateThread(void* arg) {
    Game* g = (Game*)arg;
    
    if (g->mode != 3) return NULL;
    
    while (g->gameRunning && !g->returnToMenu) {
        if (!g->paused) {
            std::lock_guard<std::mutex> lock(g->mtxShips);
            g->player2.update(0.033, g->maxx, g->maxy);
        }
        usleep(33000);
    }
    
    return NULL;
}

void* Game::hudUpdateThread(void* arg) {
    Game* g = (Game*)arg;
    
    while (g->gameRunning && !g->returnToMenu) {
        usleep(200000);
    }
    
    return NULL;
}

//============================================================================
// LÓGICA DEL JUEGO
//============================================================================

void Game::handleInput(int ch) {
    std::lock_guard<std::mutex> lockShips(mtxShips);
    std::lock_guard<std::mutex> lockBul(mtxBullets);
    
    // Player 1 controls
    if (ch == 'a' || ch == 'A') {
        player.rotateLeft(0.3);
    }
    else if (ch == 'd' || ch == 'D') {
        player.rotateRight(0.3);
    }
    else if (ch == 'w' || ch == 'W') {
        player.thrust(0.3);
    }
    else if (ch == ' ') {
        double speed = 10.0;
        double vx = cos(player.angle) * speed + player.vel.x;
        double vy = sin(player.angle) * speed + player.vel.y;
        bullets.emplace_back(player.pos.x + cos(player.angle), player.pos.y + sin(player.angle), vx, vy, 15, 1);
    }

    // Player 2 (solo en modo 3)
    if (mode == 3) {
        if (ch == KEY_LEFT) {
            player2.rotateLeft(0.3);
        }
        else if (ch == KEY_RIGHT) {
            player2.rotateRight(0.3);
        }
        else if (ch == KEY_UP) {
            player2.thrust(0.3);
        }
        else if (ch == '\n' || ch == KEY_ENTER) {
            double speed = 10.0;
            double vx = cos(player2.angle) * speed + player2.vel.x;
            double vy = sin(player2.angle) * speed + player2.vel.y;
            bullets.emplace_back(player2.pos.x + cos(player2.angle), player2.pos.y + sin(player2.angle), vx, vy, 15, 2);
        }
    }

    // Controles comunes
    if (ch == 'p' || ch == 'P') {
        // FIX: La variable 'paused' no es atómica en Game.h, debería protegerse.
        // Asumiendo que es atómica o que mtxShips la cubre de forma efectiva aquí.
        paused = !paused;
    }
    else if (ch == 'q' || ch == 'Q') {
        returnToMenu = true;
        gameRunning = false;
        
    }
}

void Game::tryCollisions() {
    std::vector<Asteroid> newAst;
    std::vector<size_t> astToErase;
    std::vector<size_t> bulToErase;

    // bullets vs asteroids
    for (size_t i = 0; i < asteroids.size(); ++i) {
        for (size_t j = 0; j < bullets.size(); ++j) {
            if (std::find(bulToErase.begin(), bulToErase.end(), j) != bulToErase.end()) {
                continue;
            }
            
            double d = dist(asteroids[i].pos.x, asteroids[i].pos.y, 
                          bullets[j].pos.x, bullets[j].pos.y);
            
            if (d <= (asteroids[i].radius() + 0.5)) {
                bulToErase.push_back(j);
                
                if (asteroids[i].size >= 2) {
                    for (int k = 0; k < 2; ++k) {
                        double nx = asteroids[i].pos.x + (k == 0 ? 1.5 : -1.5);
                        double ny = asteroids[i].pos.y + (k == 0 ? 0.5 : -0.5);
                        double nvx = asteroids[i].vel.x + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                        double nvy = asteroids[i].vel.y + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                        newAst.emplace_back(nx, ny, nvx, nvy, 1);
                    }
                }
                
                if (asteroids[i].size == 1) {
                    if (bullets[j].owner == 1) player.score.fetch_add(10);  // Use atomic
                    else if (bullets[j].owner == 2) player2.score.fetch_add(10);
                }
                
                astToErase.push_back(i);
                break;
            }
        }
    }

    // eliminar balas y asteroides marcados
    std::sort(bulToErase.begin(), bulToErase.end());
    bulToErase.erase(std::unique(bulToErase.begin(), bulToErase.end()), bulToErase.end());
    for (int k = (int)bulToErase.size()-1; k >= 0; --k) {
        if (bulToErase[k] < bullets.size()) {
            bullets.erase(bullets.begin() + bulToErase[k]);
        }
    }

    // eliminar asteroides y agregar nuevos
    std::sort(astToErase.begin(), astToErase.end());
    astToErase.erase(std::unique(astToErase.begin(), astToErase.end()), astToErase.end());
    for (int k = (int)astToErase.size()-1; k >= 0; --k) {
        if (astToErase[k] < asteroids.size()) {
            asteroids.erase(asteroids.begin() + astToErase[k]);
        }
    }
    
    for (auto &na : newAst) asteroids.push_back(na);

    // ship vs asteroids (player 1) 
    for (size_t i = 0; i < asteroids.size(); ++i) {
        double d = dist(player.pos.x, player.pos.y, 
                       asteroids[i].pos.x, asteroids[i].pos.y);
        
        if (d <= (asteroids[i].radius() + 1.0)) {
            player.lives.fetch_sub(1);  
            player.reset(maxx/3.0, maxy/2.0);
            
            if (asteroids[i].size >= 2) {
                for (int k = 0; k < 2; ++k) {
                    double nx = asteroids[i].pos.x + (k == 0 ? 1.5 : -1.5);
                    double ny = asteroids[i].pos.y + (k == 0 ? 0.5 : -0.5);
                    double nvx = asteroids[i].vel.x + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                    double nvy = asteroids[i].vel.y + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                    asteroids.emplace_back(nx, ny, nvx, nvy, 1);
                }
            }
            asteroids.erase(asteroids.begin()+i);
            break;
        }
    }

    // ship vs asteroids (player 2)
    if (mode == 3) {
        for (size_t i = 0; i < asteroids.size(); ++i) {
            double d = dist(player2.pos.x, player2.pos.y, 
                           asteroids[i].pos.x, asteroids[i].pos.y);
            
            if (d <= (asteroids[i].radius() + 1.0)) {
                player2.lives.fetch_sub(1); 
                player2.reset(2*maxx/3.0, maxy/2.0);
                
                if (asteroids[i].size >= 2) {
                    for (int k = 0; k < 2; ++k) {
                        double nx = asteroids[i].pos.x + (k == 0 ? 1.5 : -1.5);
                        double ny = asteroids[i].pos.y + (k == 0 ? 0.5 : -0.5);
                        double nvx = asteroids[i].vel.x + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                        double nvy = asteroids[i].vel.y + ((rand() % 200) / 100.0 - 1.0) * 0.8;
                        asteroids.emplace_back(nx, ny, nvx, nvy, 1);
                    }
                }
                asteroids.erase(asteroids.begin()+i);
                break;
            }
        }
    }

    // reponer asteroides si no quedan
    if (asteroids.empty()) {
        spawnInitialAsteroids();
    }
}

void Game::checkWinLoseConditions() {

    std::lock_guard<std::mutex> lock(mtxShips);
    if (mode == 3) {
        if (player.lives.load() <= 0 && player2.lives.load() <= 0) {
            gameRunning = false;
            returnToMenu = true;
        }
        else if (player.score.load() >= winScore || player2.score.load() >= winScore) {
            gameRunning = false;
            returnToMenu = true;
        }
    } else {
        if (player.lives.load() <= 0) {
            gameRunning = false;
            returnToMenu = true;
        }
        else if (player.score.load() >= winScore) {
            gameRunning = false;
            returnToMenu = true;
        }
    }
}

void Game::showEndGameScreen() {
    std::lock_guard<std::mutex> lock(mtxNcurses);  
    clear();
    refresh();
    
    // Modo bloqueante para que getch() espere de verdad
    nodelay(stdscr, FALSE);
    timeout(-1);
    curs_set(1);
    echo();
    flushinp();
    
    clear();
    int centerX = maxx / 2;
    
    if (mode != 3) {
        if (player.score.load() >= winScore) {
            mvprintw(maxy/2 - 1, centerX - 10, "*** FELICIDADES! ***");
            mvprintw(maxy/2 + 1, centerX - 8, "Puntaje: %d", player.score.load());
        } else {
            mvprintw(maxy/2 - 1, centerX - 8, "*** GAME OVER ***");
            mvprintw(maxy/2 + 1, centerX - 8, "Puntaje: %d", player.score.load());
        }
    } else {
        mvprintw(maxy/2 - 2, centerX - 12, "*** PARTIDA TERMINADA ***");
        mvprintw(maxy/2, centerX - 15, "Jugador 1: %d puntos", player.score.load());
        mvprintw(maxy/2 + 1, centerX - 15, "Jugador 2: %d puntos", player2.score.load());
        
        std::string winner;
        if (player.score.load() > player2.score.load()) winner = "Jugador 1 GANA!";
        else if (player2.score.load() > player.score.load()) winner = "Jugador 2 GANA!";
        else winner = "EMPATE!";
        
        mvprintw(maxy/2 + 3, centerX - (int)winner.size()/2, "%s", winner.c_str());
    }
    
    mvprintw(maxy/2 + 5, centerX - 20, "Presiona cualquier tecla para continuar...");
    refresh();
    
    // Esperar input real
    getch();
    
    // Guardar puntajes
    saveScoresAfterGame(mode == 3);
    
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    timeout(0);
}

void Game::drawAll() {
    clear();
    
    // el HUD
    std::ostringstream hud;
    if (mode != 3) {
        hud << "Score: " << player.score.load() << "   Lives: ";
        for (int i=0; i<player.lives.load(); ++i) hud << "<3 ";
        hud << "   Mode: " << mode;
        if (has_colors()) attron(COLOR_PAIR(3));
        mvprintw(0, 1, "%s", hud.str().c_str());
        if (has_colors()) attroff(COLOR_PAIR(3));
    } else {
        std::ostringstream p1, p2s;
        p1 << "P1 Score: " << player.score.load() << " L: ";
        for (int i=0; i<player.lives.load(); ++i) p1 << "<3 ";
        p2s << "  |  P2 Score: " << player2.score.load() << " L: ";
        for (int i=0; i<player2.lives.load(); ++i) p2s << "<3 ";
        p2s << "   Mode: 3";
        std::string full = p1.str() + p2s.str();
        if (has_colors()) attron(COLOR_PAIR(3));
        mvprintw(0, 1, "%s", full.c_str());
        if (has_colors()) attroff(COLOR_PAIR(3));
    }

    // asteroides
    {
        std::lock_guard<std::mutex> lock(mtxAsteroids);
        for (auto &a : asteroids) {
            int ax = (int)round(a.pos.x);
            int ay = (int)round(a.pos.y);
            if (ax >= 0 && ax < maxx && ay >= 1 && ay < maxy-1) {
                if (has_colors()) attron(COLOR_PAIR(3));
                mvaddch(ay, ax, a.glyph());
                if (has_colors()) attroff(COLOR_PAIR(3));
            }
        }
    }

    // balas
    {
        std::lock_guard<std::mutex> lock(mtxBullets);
        for (auto &b : bullets) {
            int bx = (int)round(b.pos.x);
            int by = (int)round(b.pos.y);
            if (bx >= 0 && bx < maxx && by >= 1 && by < maxy-1) {
                if (has_colors()) {
                    if (b.owner == 1) attron(COLOR_PAIR(1));
                    else if (b.owner == 2) attron(COLOR_PAIR(2));
                }
                mvaddch(by, bx, '*');
                if (has_colors()) {
                    if (b.owner == 1) attroff(COLOR_PAIR(1));
                    else if (b.owner == 2) attroff(COLOR_PAIR(2));
                }
            }
        }
    }

    // naves
    {
        std::lock_guard<std::mutex> lock(mtxShips);
        
        int sx = (int)round(player.pos.x);
        int sy = (int)round(player.pos.y);
        if (sx >= 0 && sx < maxx && sy >= 1 && sy < maxy-1) {
            if (has_colors()) attron(COLOR_PAIR(1) | A_BOLD);
            mvaddch(sy, sx, player.glyph());
            if (has_colors()) attroff(COLOR_PAIR(1) | A_BOLD);
        }

        if (mode == 3) {
            int s2x = (int)round(player2.pos.x);
            int s2y = (int)round(player2.pos.y);
            if (s2x >= 0 && s2x < maxx && s2y >= 1 && s2y < maxy-1) {
                if (has_colors()) attron(COLOR_PAIR(2) | A_BOLD);
                mvaddch(s2y, s2x, player2.glyph());
                if (has_colors()) attroff(COLOR_PAIR(2) | A_BOLD);
            }
        }
    }

    // controles
    if (mode != 3) {
        mvprintw(maxy-1, 2, "A/D=girar W=impulso SPACE=disparo P=pausa Q=menu");
    } else {
        mvprintw(maxy-1, 2, "P1:A/D/W/SPACE P2: flechas/ENTER P=pausa Q=menu");
    }

    refresh();
}

double Game::dist(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx*dx + dy*dy);
}

void Game::showInstructions() {
    std::lock_guard<std::mutex> lock(mtxNcurses);  // Protect ncurses calls
    clear();
    std::vector<std::string> lines = {
        "INSTRUCCIONES - ASTEROIDS",
        "",
        "Objetivo: destruir asteroides. Cada asteroide pequeño = +10 pts.",
        "Modo 1: 3 asteroides grandes (meta 60 pts).",
        "Modo 2: 5 asteroides grandes (meta 100 pts).",
        "Modo 3: 2 jugadores (compiten por puntaje).",
        "",
        "Controles jugador 1: A/D = girar, W = thrust, SPACE = disparo",
        "Controles jugador 2: flecha izq/flecha der = girar, flecha arriba = thrust, ENTER = disparo",
        "",
        "Otros: P = pausar/reanudar, Q = salir",
        "",
        "Presiona cualquier tecla para volver al menu..."
    };
    int start = 2;
    for (size_t i=0; i<lines.size(); ++i) {
        mvprintw(start + (int)i, 2, "%s", lines[i].c_str());
    }
    nodelay(stdscr, FALSE);
    flushinp();
    getch();
    nodelay(stdscr, TRUE);
}

void Game::showScores() {
    std::lock_guard<std::mutex> lock(mtxNcurses); 
    clear();
    mvprintw(2, 2, "PUNTAJES GUARDADOS");
    mvprintw(4, 2, "===================");
    
    std::ifstream ifs("scores.txt");
    if (!ifs.is_open()) {
        mvprintw(6, 2, "No hay puntajes guardados todavia");
        mvprintw(8, 2, "Juega una partida para guardar tu puntaje");
    } else {
        std::string line;
        int row = 6;
        while (std::getline(ifs, line) && row < maxy - 4) {
            mvprintw(row, 4, "%s", line.c_str());
            row++;
        }
        ifs.close();
        
        if (row == 6) {
            mvprintw(6, 2, "El archivo esta vacio. Juega para guardar puntajes");
        }
    }
    
    mvprintw(maxy-2, 2, "Presiona cualquier tecla para volver.");
    nodelay(stdscr, FALSE);
    flushinp();
    getch();
    nodelay(stdscr, TRUE);
}

void Game::saveScoresAfterGame(bool twoPlayers) {
    std::lock_guard<std::mutex> lock(mtxNcurses);  
    nodelay(stdscr, FALSE);
    timeout(-1);
    echo();
    curs_set(1);

    char namebuf[64];
    std::ofstream ofs("scores.txt", std::ios::app);
    
  // Verificar si se pudo abrir el archivo

    if (!ofs.is_open()) {
        mvprintw(maxy-3, 2, "ERROR: No se pudo abrir scores.txt para guardar.");
        mvprintw(maxy-2, 2, "Presiona una tecla para continuar...");
        refresh();
        flushinp();
        getch();
    } else {
        if (!twoPlayers) {
            mvprintw(maxy-4, 2, "Ingresa tu nombre (max 20 caracteres): ");
            move(maxy-4, 44);
            memset(namebuf, 0, sizeof(namebuf));
            getnstr(namebuf, 20);
            std::string name = std::string(namebuf);
            if (name.empty()) name = "Anonimo";
            
            ofs << name << " - " << player.score.load() << std::endl;
            
            mvprintw(maxy-3, 2, "Puntaje guardado: %s - %d", name.c_str(), player.score.load());
            mvprintw(maxy-2, 2, "Presiona una tecla para volver al menu...");
            refresh();
            flushinp();
            getch();
        } else {
            mvprintw(maxy-6, 2, "Nombre Jugador 1 (max 20 caracteres): ");
            move(maxy-6, 43);
            memset(namebuf, 0, sizeof(namebuf));
            getnstr(namebuf, 20);
            std::string n1 = std::string(namebuf);
            if (n1.empty()) n1 = "P1"; //aon

            mvprintw(maxy-5, 2, "Nombre Jugador 2 (max 20 caracteres): ");
            move(maxy-5, 43);
            memset(namebuf, 0, sizeof(namebuf));
            getnstr(namebuf, 20);
            std::string n2 = std::string(namebuf);
            if (n2.empty()) n2 = "P2";

            ofs << n1 << " - " << player.score.load() << std::endl;
            ofs << n2 << " - " << player2.score.load() << std::endl;

            mvprintw(maxy-3, 2, "Puntajes guardados ");
            mvprintw(maxy-2, 2, "Presiona una tecla para volver al menu...");
            refresh();
            flushinp();
            getch();
        }
        ofs.close();
    }

    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    timeout(0);
}