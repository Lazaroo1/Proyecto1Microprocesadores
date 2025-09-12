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
// esta clase es la que maneja todo el juego, el loop principal, el menu, etc
// usa ncurses para dibujar en terminal
// y usa las clases Ship, Asteroid y Projectile para los objetos del juego

Game::Game()
: player(40, 12), player2(40, 14), quitFlag(false), paused(false), mode(1), winScore(60) {
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

    // colores
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);   // jugador 1 (cian)
        init_pair(2, COLOR_GREEN, -1);  // jugador 2 (verde)
        init_pair(3, COLOR_YELLOW, -1); // asteroides / hud accent
        init_pair(4, COLOR_WHITE, -1);  // bullets fallback
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
        "Puntajes (placeholder)",
        "Salir"
    };
    int choice = -1;
    while (choice == -1) {
        getmaxyx(stdscr, maxy, maxx);
        clear();
        std::string title = "ASTEROIDS (PROTOTIPO FASE 2)";
        mvprintw(2, (maxx - (int)title.size())/2, "%s", title.c_str());

        // Mode selector small
        std::string modeStr = "Modo: 1 (3 ast.)  2 (5 ast.)  3 (2 players) - Usa teclas 1/2/3 para cambiar";
        mvprintw(4, (maxx - (int)modeStr.size())/2, "%s", modeStr.c_str());
        mvprintw(5, (maxx - 30)/2, "Meta: %d pts", (mode==1)?60:((mode==2)?100:100));

        for (int i=0;i<(int)options.size();++i) {
            int y = 8 + i*2;
            if (i == highlight) attron(A_REVERSE);
            mvprintw(y, (maxx - (int)options[i].size())/2, "%s", options[i].c_str());
            if (i == highlight) attroff(A_REVERSE);
        }
        mvprintw(maxy-2, 2, "Usa ↑ ↓ para navegar, Enter para seleccionar.");

        int ch = getch();
        if (ch == KEY_UP) highlight = (highlight - 1 + (int)options.size()) % (int)options.size();
        else if (ch == KEY_DOWN) highlight = (highlight + 1) % (int)options.size();
        else if (ch == '1') { mode = 1; winScore = 60; }
        else if (ch == '2') { mode = 2; winScore = 100; }
        else if (ch == '3') { mode = 3; winScore = 100; } // Modo 3: comparativo - usamos meta 100
        else if (ch == '\n' || ch == KEY_ENTER) choice = highlight;
        else if (ch == 'q' || ch == 'Q') { quitFlag = true; return; }

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        refresh();
    }

    if (choice == 0) startGame();
    else if (choice == 1) showInstructions();
    else if (choice == 2) showScores();
    else if (choice == 3) { quitFlag = true; }
}

void Game::showInstructions() {
    clear();
    std::vector<std::string> lines = {
        "INSTRUCCIONES - ASTEROIDS (PROTOTIPO)",
        "",
        "Objetivo: destruir asteroides. Cada asteroide pequeño = +10 pts.",
        "Modo 1: 3 asteroides grandes (meta 60 pts).",
        "Modo 2: 5 asteroides grandes (meta 100 pts).",
        "Modo 3: 2 jugadores (compiten por puntaje).",
        "",
        "Controles jugador 1:",
        "  A -> girar izquierda",
        "  D -> girar derecha",
        "  W -> thrust (acelerar)",
        "  Space -> disparo",
        "",
        "Controles jugador 2 (modo 3):",
        "  ← / → -> girar",
        "  ↑ -> thrust (acelerar)",
        "  Enter -> disparo",
        "",
        "Otros:",
        "  P -> pausar/reanudar",
        "  Q -> volver al menu / salir partida",
        "",
        "HUD: fila superior muestra Score | Lives | Mode",
        "",
        "Presiona cualquier tecla para volver al menu..."
    };
    int start = 2;
    for (size_t i=0;i<lines.size();++i) {
        mvprintw(start + (int)i, 2, "%s", lines[i].c_str());
    }
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

void Game::showScores() {
    clear();
    mvprintw(2,2,"PUNTAJES (PLACEHOLDER)");
    mvprintw(4,2,"Aqui mostraremos los puntajes guardados en futuras entregas (archivo scores.txt).");
    mvprintw(6,2,"Ejemplo:");
    mvprintw(7,4,"Lazaro_D  -  150");
    mvprintw(8,4,"Dally_R   -  120");
    mvprintw(maxy-2, 2, "Presiona cualquier tecla para volver.");
    nodelay(stdscr, FALSE);
    getch();
    nodelay(stdscr, TRUE);
}

void Game::startGame() {
    // reset state
    getmaxyx(stdscr, maxy, maxx);
    player.reset(maxx/3.0, maxy/2.0);
    player.lives = 3;
    player.score = 0;

    player2.reset(2*maxx/3.0, maxy/2.0);
    player2.lives = 3;
    player2.score = 0;

    bullets.clear();
    asteroids.clear();
    paused = false;

    spawnInitialAsteroids();

    // game loop basic timing
    using clock = std::chrono::steady_clock;
    auto last = clock::now();
    const double tick = 1.0; // unit for velocity scaling
    int fpsDelay = 33; // ~30 fps

    bool inGame = true;
    while (inGame) {
        auto now = clock::now();
        std::chrono::duration<double> elapsed = now - last;
        last = now;
        double dt = elapsed.count() * tick;

        int ch = getch();
        if (ch != ERR) handleInput(ch);
        if (!paused) updateAll(dt);
        tryCollisions();
        drawAll();

        // check win/lose conditions
        if (mode != 3) {
            if (player.score >= winScore) {
                // show win screen singleplayer
                clear();
                mvprintw(maxy/2, (maxx-20)/2, "¡FELICIDADES! Ganaste.");
                mvprintw(maxy/2+2, (maxx-30)/2, "Score: %d   Presiona any key...", player.score);
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                // save score
                saveScoresAfterGame(false);
                inGame = false;
                break;
            }
            if (player.lives <= 0) {
                clear();
                mvprintw(maxy/2, (maxx-20)/2, "GAME OVER");
                mvprintw(maxy/2+2, (maxx-30)/2, "Score: %d   Presiona any key...", player.score);
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);
                saveScoresAfterGame(false);
                inGame = false;
                break;
            }
        } else {
            // modo 3: termina si alguno alcanza winScore o ambos sin vidas
            if (player.score >= winScore || player2.score >= winScore ||
                (player.lives <= 0 && player2.lives <= 0)) {
                clear();
                // mostrar resultados
                mvprintw(maxy/2 - 2, (maxx-40)/2, "PARTIDA TERMINADA - Resultados:");
                mvprintw(maxy/2, (maxx-40)/2, "Jugador 1: Score=%d  Lives=%d", player.score, player.lives);
                mvprintw(maxy/2+1, (maxx-40)/2, "Jugador 2: Score=%d  Lives=%d", player2.score, player2.lives);

                std::string winner;
                if (player.score > player2.score) winner = "Jugador 1 gana!";
                else if (player2.score > player.score) winner = "Jugador 2 gana!";
                else winner = "Empate!";

                mvprintw(maxy/2+3, (maxx-40)/2, "%s   Presiona any key...", winner.c_str());
                nodelay(stdscr, FALSE);
                getch();
                nodelay(stdscr, TRUE);

                // guardar ambos puntajes
                saveScoresAfterGame(true);

                inGame = false;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(fpsDelay));
    }
}

void Game::spawnInitialAsteroids() {
    int count = (mode == 1) ? 3 : 5;
    // if mode 3, keep same as mode 2 initial density
    if (mode == 3) count = 5;
    for (int i=0;i<count;++i) {
        double x = rand() % (maxx-8) + 4;
        double y = (rand() % (maxy-8)) + 2;
        // avoid spawning too close to players
        if (fabs(x - player.pos.x) < 8 && fabs(y - player.pos.y) < 4) { x += 10; y += 3; }
        if (fabs(x - player2.pos.x) < 8 && fabs(y - player2.pos.y) < 4) { x -= 10; y -= 3; }
        double vx = ( (rand()%200) / 100.0 - 1.0 ) * 0.6;
        double vy = ( (rand()%200) / 100.0 - 1.0 ) * 0.6;
        if (fabs(vx) < 0.05) vx = 0.2;
        if (fabs(vy) < 0.05) vy = -0.2;
        asteroids.emplace_back(x,y,vx,vy,2);
    }
}

void Game::updateAll(double dt) {
    // update ships
    player.update(dt, maxx, maxy);
    if (mode == 3) player2.update(dt, maxx, maxy);

    // update bullets
    for (auto &b : bullets) b.update(dt * 6.0, maxx, maxy); // bullets faster
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                 [](const Projectile& p){ return !p.alive(); }), bullets.end());

    // update asteroids
    for (auto &a : asteroids) a.update(dt * 1.2, maxx, maxy);
}

void Game::drawAll() {
    clear();
    // HUD top row (adapta para modo 3)
    std::ostringstream hud;
    if (mode != 3) {
        hud << "Score: " << player.score << "   Lives: ";
        for (int i=0;i<player.lives;++i) hud << "<3 ";
        hud << "   Mode: " << mode;
        if (has_colors()) attron(COLOR_PAIR(3));
        mvprintw(0, 1, "%s", hud.str().c_str());
        if (has_colors()) attroff(COLOR_PAIR(3));
    } else {
        // show both players
        std::ostringstream p1, p2s;
        p1 << "P1 Score: " << player.score << " L: ";
        for (int i=0;i<player.lives;++i) p1 << "<3 ";
        p2s << "  |  P2 Score: " << player2.score << " L: ";
        for (int i=0;i<player2.lives;++i) p2s << "<3 ";
        p2s << "   Mode: 3";
        std::string full = p1.str() + p2s.str();
        if (has_colors()) attron(COLOR_PAIR(3));
        mvprintw(0, 1, "%s", full.c_str());
        if (has_colors()) attroff(COLOR_PAIR(3));
    }

    // draw asteroids
    for (auto &a : asteroids) {
        int ax = (int)round(a.pos.x) % maxx;
        int ay = (int)round(a.pos.y);
        if (ay < 1) ay = 1;
        if (has_colors()) attron(COLOR_PAIR(3));
        mvaddch(ay, ax, a.glyph());
        if (has_colors()) attroff(COLOR_PAIR(3));
    }

    // draw bullets (with owner color)
    for (auto &b : bullets) {
        int bx = (int)round(b.pos.x) % maxx;
        int by = (int)round(b.pos.y);
        if (by < 1) by = 1;
        if (has_colors()) {
            if (b.owner == 1) attron(COLOR_PAIR(1));
            else if (b.owner == 2) attron(COLOR_PAIR(2));
        }
        mvaddch(by, bx, '.');
        if (has_colors()) {
            if (b.owner == 1) attroff(COLOR_PAIR(1));
            else if (b.owner == 2) attroff(COLOR_PAIR(2));
        }
    }

    // draw ship 1
    int sx = (int)round(player.pos.x) % maxx;
    int sy = (int)round(player.pos.y);
    if (sy < 1) sy = 1;
    if (has_colors()) attron(COLOR_PAIR(1) | A_BOLD);
    mvaddch(sy, sx, player.glyph());
    if (has_colors()) attroff(COLOR_PAIR(1) | A_BOLD);

    // draw ship 2 only in mode 3
    if (mode == 3) {
        int s2x = (int)round(player2.pos.x) % maxx;
        int s2y = (int)round(player2.pos.y);
        if (s2y < 1) s2y = 1;
        if (has_colors()) attron(COLOR_PAIR(2) | A_BOLD);
        mvaddch(s2y, s2x, player2.glyph());
        if (has_colors()) attroff(COLOR_PAIR(2) | A_BOLD);
    }

    // bottom helper
    if (mode != 3) {
        mvprintw(maxy-1, 2, "A D = girar | W = thrust | Space = disparo | P = pausar | Q = menu");
    } else {
        mvprintw(maxy-1, 2, "P1: A D W Space | P2: ← → ↑ Enter | P = pausar | Q = menu");
    }

    refresh();
}

void Game::handleInput(int ch) {
    // Player 1 controls (always active)
    if (ch == 'a' || ch == 'A') player.rotateLeft(0.25);
    else if (ch == 'd' || ch == 'D') player.rotateRight(0.25);
    else if (ch == 'w' || ch == 'W') player.thrust(0.6);
    else if (ch == ' ') {
        // create projectile from ship tip (owner 1)
        double speed = 5.0;
        double vx = cos(player.angle) * speed + player.vel.x;
        double vy = sin(player.angle) * speed + player.vel.y;
        bullets.emplace_back(player.pos.x, player.pos.y, vx, vy, 40, 1);
    }

    // Player 2 (only if mode 3)
    if (mode == 3) {
        if (ch == KEY_LEFT) player2.rotateLeft(0.25);
        else if (ch == KEY_RIGHT) player2.rotateRight(0.25);
        else if (ch == KEY_UP) player2.thrust(0.6);
        else if (ch == '\n' || ch == KEY_ENTER) {
            double speed = 5.0;
            double vx = cos(player2.angle) * speed + player2.vel.x;
            double vy = sin(player2.angle) * speed + player2.vel.y;
            bullets.emplace_back(player2.pos.x, player2.pos.y, vx, vy, 40, 2);
        }
    }

    // common
    if (ch == 'p' || ch == 'P') paused = !paused;
    else if (ch == 'q' || ch == 'Q') {
        // quick return to menu: set lives to 0 to trigger game over path
        if (mode == 3) {
            player.lives = 0;
            player2.lives = 0;
        } else {
            player.lives = 0;
        }
    }
}

double Game::dist(double x1,double y1,double x2,double y2) {
    double dx = x1 - x2;
    double dy = y1 - y2;
    return sqrt(dx*dx + dy*dy);
}

void Game::tryCollisions() {
    // bullets vs asteroids
    std::vector<Asteroid> newAst;
    std::vector<size_t> astToErase;
    std::vector<size_t> bulToErase;

    for (size_t i=0;i<asteroids.size();++i) {
        for (size_t j=0;j<bullets.size();++j) {
            double d = dist(asteroids[i].pos.x, asteroids[i].pos.y, bullets[j].pos.x, bullets[j].pos.y);
            if (d <= (asteroids[i].radius() + 0.5)) {
                // hit
                bulToErase.push_back(j);
                if (asteroids[i].size >= 2) {
                    // split into two small asteroids
                    for (int k=0;k<2;++k) {
                        double nx = asteroids[i].pos.x + (k==0?0.7:-0.7);
                        double ny = asteroids[i].pos.y + (k==0?0.3:-0.3);
                        double nvx = asteroids[i].vel.x + ((rand()%200)/100.0-1.0) * 0.6;
                        double nvy = asteroids[i].vel.y + ((rand()%200)/100.0-1.0) * 0.6;
                        newAst.emplace_back(nx, ny, nvx, nvy, 1);
                    }
                }
                // award points if small destroyed (to bullet owner)
                if (asteroids[i].size == 1) {
                    if (bullets[j].owner == 1) player.score += 10;
                    else if (bullets[j].owner == 2) player2.score += 10;
                }
                astToErase.push_back(i);
                break;
            }
        }
    }

    // erase bullets (unique)
    std::sort(bulToErase.begin(), bulToErase.end());
    bulToErase.erase(std::unique(bulToErase.begin(), bulToErase.end()), bulToErase.end());
    for (int k=(int)bulToErase.size()-1;k>=0;--k) {
        if (bulToErase[k] < bullets.size()) bullets.erase(bullets.begin() + bulToErase[k]);
    }

    // erase asteroids (unique)
    std::sort(astToErase.begin(), astToErase.end());
    astToErase.erase(std::unique(astToErase.begin(), astToErase.end()), astToErase.end());
    for (int k=(int)astToErase.size()-1;k>=0;--k) {
        if (astToErase[k] < asteroids.size()) asteroids.erase(asteroids.begin() + astToErase[k]);
    }
    // append new small asteroids
    for (auto &na : newAst) asteroids.push_back(na);

    // ship vs asteroids (player 1)
    for (size_t i=0;i<asteroids.size();++i) {
        double d = dist(player.pos.x, player.pos.y, asteroids[i].pos.x, asteroids[i].pos.y);
        if (d <= (asteroids[i].radius() + 0.8)) {
            // collision with player1
            player.lives -= 1;
            player.reset(maxx/2.0, maxy/2.0);
            if (asteroids[i].size >= 2) {
                for (int k=0;k<2;++k) {
                    double nx = asteroids[i].pos.x + (k==0?0.7:-0.7);
                    double ny = asteroids[i].pos.y + (k==0?0.3:-0.3);
                    double nvx = asteroids[i].vel.x + ((rand()%200)/100.0-1.0) * 0.6;
                    double nvy = asteroids[i].vel.y + ((rand()%200)/100.0-1.0) * 0.6;
                    asteroids.emplace_back(nx, ny, nvx, nvy, 1);
                }
            }
            asteroids.erase(asteroids.begin()+i);
            break;
        }
    }

    // ship vs asteroids (player 2 if mode 3)
    if (mode == 3) {
        for (size_t i=0;i<asteroids.size();++i) {
            double d = dist(player2.pos.x, player2.pos.y, asteroids[i].pos.x, asteroids[i].pos.y);
            if (d <= (asteroids[i].radius() + 0.8)) {
                // collision player2
                player2.lives -= 1;
                player2.reset(maxx/2.0 + 4, maxy/2.0);
                if (asteroids[i].size >= 2) {
                    for (int k=0;k<2;++k) {
                        double nx = asteroids[i].pos.x + (k==0?0.7:-0.7);
                        double ny = asteroids[i].pos.y + (k==0?0.3:-0.3);
                        double nvx = asteroids[i].vel.x + ((rand()%200)/100.0-1.0) * 0.6;
                        double nvy = asteroids[i].vel.y + ((rand()%200)/100.0-1.0) * 0.6;
                        asteroids.emplace_back(nx, ny, nvx, nvy, 1);
                    }
                }
                asteroids.erase(asteroids.begin()+i);
                break;
            }
        }
    }

    // if no asteroids remain, spawn new wave (simple)
    if (asteroids.empty()) spawnInitialAsteroids();
}

void Game::saveScoresAfterGame(bool twoPlayers) {
    // prompt for names and append to "scores.txt"
    nodelay(stdscr, FALSE);
    echo();
    curs_set(1);

    char namebuf[64];
    std::ofstream ofs("scores.txt", std::ios::app);
    if (!ofs.is_open()) {
        mvprintw(maxy-3, 2, "ERROR: No se pudo abrir scores.txt para guardar.");
        mvprintw(maxy-2, 2, "Presione una tecla para continuar...");
        getch();
    } else {
        if (!twoPlayers) {
            mvprintw(maxy-4, 2, "Ingresa tu nombre para guardar el puntaje: ");
            move(maxy-4, 42);
            memset(namebuf,0,sizeof(namebuf));
            getnstr(namebuf, 50);
            std::string name = std::string(namebuf);
            if (name.empty()) name = "Anonimo";
            ofs << name << " - " << player.score << std::endl;
            mvprintw(maxy-3, 2, "Puntaje guardado en scores.txt como: %s - %d", name.c_str(), player.score);
            mvprintw(maxy-2, 2, "Presiona una tecla para volver al menu...");
            getch();
        } else {
            // mode 3: ask for both names
            mvprintw(maxy-6, 2, "Ingresa nombre Jugador 1: ");
            move(maxy-6, 26);
            memset(namebuf,0,sizeof(namebuf));
            getnstr(namebuf, 50);
            std::string n1 = std::string(namebuf);
            if (n1.empty()) n1 = "P1_Anon";

            mvprintw(maxy-5, 2, "Ingresa nombre Jugador 2: ");
            move(maxy-5, 26);
            memset(namebuf,0,sizeof(namebuf));
            getnstr(namebuf, 50);
            std::string n2 = std::string(namebuf);
            if (n2.empty()) n2 = "P2_Anon";

            ofs << n1 << " - " << player.score << std::endl;
            ofs << n2 << " - " << player2.score << std::endl;

            mvprintw(maxy-3, 2, "Puntajes guardados: %s - %d   %s - %d", n1.c_str(), player.score, n2.c_str(), player2.score);
            mvprintw(maxy-2, 2, "Presiona una tecla para volver al menu...");
            getch();
        }
        ofs.close();
    }

    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
}
