//Jason Thai

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <unistd.h>

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <GL/glx.h>
extern "C"
{
#include "fonts.h"
}

#include <X11/extensions/Xdbe.h>

#include <string>
#include <iostream>

#include <unistd.h>

using namespace std;
#define GRAVITY -9.8

//macro to swap two integers
#define SWAP(x,y) (x)^=(y);(y)^=(x);(x)^=(y)

//--------------------
//XWindows vaariables
Display *dpy;
Window win;
GC gc;
XdbeBackBuffer backBuffer;
XdbeSwapInfo swapInfo;
//--------------------
//a few more global variables
enum {
    MODE_NONE,
    MODE_LINES,
    LINETYPE_XWIN,
    LINETYPE_BRESENHAM
};

//Setup timers
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
struct timespec timeStart, timeCurrect;
struct timespec timePause;
double physicsCountdown = 0.0;
double timeSpan = 0.0;
double timeDiff(struct timespec *start, struct timespec *end)
{
    return (double)(end->tv_sec - start->tv_sec) +
        (double)(end->tv_nsec - start->tv_nsec) * oobillion;
}
void timeCopy(struct timespec *dest, struct timespec *source)
{
    memcpy(dest, source, sizeof(struct timespec));
}
//


struct Vec
{
    float x, y, z;
};

struct Shape
{
    float width, height;
    float radius;
    Vec center;
    float color[3];

    // enemies make use of where they are on map
    int location;
};

struct Bullet
{
    Vec pos;
    Vec vel;
    float color[3];
    struct timespec time;
    struct Bullet *prev;
    struct Bullet *next;
    Bullet()
    {
        prev = NULL;
        next = NULL;
    }
};

struct Grenade
{
    Vec pos;
    Vec vel;
    float color[3];
    struct timespec time;
    struct Grenade *prev;
    struct Grenade *next;
    Grenade()
    {
        prev = NULL;
        next = NULL;
    }
};

struct Tower
{
    Vec pos;
    float color[3];
    int ammo;
    struct Tower *prev;
    struct Tower *next;
    Tower()
    {
        prev = NULL;
        next = NULL;
    }
};

struct Resource
{
    int resourceA;
    int resourceB;
    int resourceC;
    int bullets;
    int grenades;
    int towers;
    Resource()
    {
        resourceA = 5;
        resourceB = 5;
        resourceC = 5;
        bullets = 5;
        grenades = 5;
        towers = 5;
    }
};

struct GResource
{
    int center[2];
    float color[3];
    int abc;
    int location;
};

struct Grid
{
    int size;
    int column[100];
    int row[100];

    int start[2];
    int destination[2];

    // NOTE: will change to incorporate walls
    bool open_x[100];   // check if the space is open or a wall
    bool open_y[100];
    Grid()
    {
        for(int i = 0; i < 100; i++)
        {
            open_x[i] = true;
            open_y[i] = true;
        }
    }
};

struct Options
{
    int menu;
    bool help;
    Options()
    {
        menu = 0;
        help = 1;
    }
};

struct Resolution
{
    int menu;
    int selection;
    Resolution()
    {
        menu = 0;
        selection = 0;
    }
};

struct Maps
{
    int menu;
    int area[10][4];    // how many screens in the map, the 4 boundaries
    int location;       // where the player is, what to render
    Grid grid[10];      // establish the grid for the screens
    int start;
    int destination;
    int turns;
    GResource resource[10];
};

struct Enemy
{
    Shape A;    // 1st enemy
    Shape monster[4];
    bool spawn[4];
    int spawnRate[4];
    Enemy()
    {
        for(int i = 0; i < 4; i++)
        {
            spawn[i] = false;       // which enemy has spawned, set all to "not spawned yet"
            spawnRate[i] = 3;
        }
    }
};

#define MAX_POINTS 400
struct Global {
    int xres, yres;
    int done;

    struct timespec timer;

    Shape circle;
    Vec aim;

    bool flashlight;
    float angle;
    int dist;
    Vec point1;
    Vec point2;

    int direction;

    int inventory;

    Bullet *bhead;
    //int nbullets;

    Grenade *ghead;

    Tower *thead;
    int totalTowers;

    Resource resource;

    bool combine;

    bool title;
    int menu;

    Grid grid;
    bool drawGrid;

    Options options;
    bool showUI;
    bool showOptions;
    Resolution resolution;
    bool showRes;

    Maps maps;
    bool showMaps;

    bool showComplete;
    int cmenu;

    Enemy enemy;
    int spawnRate;

    Global() {
        //constructor
        xres = 640;
        yres = 480;

        clock_gettime(CLOCK_REALTIME, &timer);

        circle.radius = 20.0;
        circle.center.x = xres/2 - circle.radius;
        circle.center.y = yres/2 - circle.radius;

        flashlight = 0;
        angle = 3.14/4;
        dist = 200;

        direction = 0;

        inventory = 1;

        bhead = NULL;
        //nbullets = 0;

        ghead = NULL;

        thead = NULL;
        totalTowers = 0;

        combine = 0;

        title = 1;
        menu = 0;

        drawGrid = false;

        showUI = 0;
        showOptions = 0;
        showRes = 0;

        showMaps = 0;

        showComplete = 0;
        cmenu = 0;

        enemy.monster[0].radius = circle.radius;
        spawnRate = 3;
    }
} g;

//-------------------
//function prototypes
void initXwindows();
void cleanupXwindows();
void checkResize(XEvent *e);
void checkMouse(XEvent *e);
void checkKeys(XEvent *e);
void physics();
void showMenu(int x, int y);
void render();
void clearScreen();

void tower();
void spawnEnemy(int x);
void enemy();

void initGrid();

void clearMap();
void initMap1();
void initMap2();

int main()
{
    initGrid();
    srand((unsigned)time(NULL));
    initXwindows();
    while (!g.done) {
        //Check the event queue
        while (XPending(dpy)) {
            //Handle events one-by-one
            XEvent e;
            XNextEvent(dpy, &e);
            checkResize(&e);
            checkMouse(&e);
            checkKeys(&e);
        }
        //if(!g.enemy.spawn[0])
        //    spawnEnemy(0);
        clearScreen();
        //tower();
        physics();
        render();
        XdbeSwapBuffers(dpy, &swapInfo, 1);
        usleep(8000);
    }
    cleanupXwindows();
    return 0;
}

void cleanupXwindows(void)
{
    if(!XdbeDeallocateBackBufferName(dpy, backBuffer))
    {
        fprintf(stderr, "Error : unable to deallocate back buffer.\n");
    }
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void initXwindows(void)
{
    XSetWindowAttributes attributes;
    int major, minor;
    XdbeBackBufferAttributes *backAttr;

    dpy = XOpenDisplay(NULL);
    //List of events we want to handle
    attributes.event_mask = ExposureMask | StructureNotifyMask |
        PointerMotionMask | ButtonPressMask |
        ButtonReleaseMask | KeyPressMask;
    //Various window attributes
    attributes.backing_store = Always;
    attributes.save_under = True;
    attributes.override_redirect = False;
    attributes.background_pixel = 0x00000000;
    //Get default root window
    Window root = DefaultRootWindow(dpy);
    //Create a window
    win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWBackingStore | CWOverrideRedirect | CWEventMask |
            CWSaveUnder | CWBackPixel, &attributes);
    //Create gc
    gc = XCreateGC(dpy, win, 0, NULL);
    //Get DBE version
    if (!XdbeQueryExtension(dpy, &major, &minor)) {
        fprintf(stderr, "Error : unable to fetch Xdbe Version.\n");
        XFreeGC(dpy, gc);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
        exit(1);
    }
    printf("Xdbe version %d.%d\n",major,minor);
    //Get back buffer and attributes (used for swapping)
    backBuffer = XdbeAllocateBackBufferName(dpy, win, XdbeUndefined);
    backAttr = XdbeGetBackBufferAttributes(dpy, backBuffer);
    swapInfo.swap_window = backAttr->window;
    swapInfo.swap_action = XdbeUndefined;
    XFree(backAttr);
    //Map and raise window
    XMapWindow(dpy, win);
    XRaiseWindow(dpy, win);
}


void checkResize(XEvent *e)
{
    //ConfigureNotify is sent when window size changes.
    if (e->type != ConfigureNotify)
        return;
    XConfigureEvent xce = e->xconfigure;
    g.xres = xce.width;
    g.yres = xce.height;
    initGrid();
}

void clearScreen(void)
{
    //XClearWindow(dpy, backBuffer);
    XSetForeground(dpy, gc, 0x00000000);
    XFillRectangle(dpy, backBuffer, gc, 0, 0, g.xres, g.yres);
}

void checkMouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;
    //
    int mx = e->xbutton.x;
    int my = e->xbutton.y;
    //

    g.aim.x = mx;
    g.aim.y = my;

    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        //Log("ButtonPress %i %i\n", e->xbutton.x, e->xbutton.y);
        if (e->xbutton.button==1) {
            //Left button pressed
        }
        if (e->xbutton.button==3) {
            //Right button pressed
        }
    }
    if (savex != mx || savey != my) {
        //mouse moved
        savex = mx;
        savey = my;

        g.aim.x = savex;
        g.aim.y = savey;
    }
}

void shootBullet(float xLocation, float yLocation, float xDirection, float yDirection)
{
    Bullet *b = new Bullet;

    struct timespec bt;
    clock_gettime(CLOCK_REALTIME, &bt);
    timeDiff(&g.timer, &bt);
    timeCopy(&g.timer, &bt);
    timeCopy(&b->time, &bt);

    b->pos.x = xLocation;
    b->pos.y = yLocation;

    b->vel.x = xDirection;
    b->vel.y = yDirection;

    b->next = g.bhead;
    if(g.bhead != NULL)
        g.bhead->prev = b;
    g.bhead = b;
}

float distance(float px, float ex, float py, float ey)
{
    return sqrt(pow(px - ex, 2) + pow(py - ey, 2));
}

void enemyMovement(Shape player, Shape enemy)   // only works one enemy at a time, CHANGE LATER
{
    // check enemy location
    // check surrounding open spaces
    // decide which direction to go

    if(g.enemy.spawn[0] == true)
    {
        int dist = distance(player.center.x, enemy.center.x, player.center.y, enemy.center.y); // get distance between player and enemy

        if(distance(player.center.x, enemy.center.x, player.center.y, enemy.center.y - enemy.radius*2) < dist) // check whether going up places enemy closer to player
        {
            if((enemy.center.x != player.center.x) || (enemy.center.y - enemy.radius*2 != player.center.y))
                g.enemy.monster[0].center.y -= g.enemy.monster[0].radius*2;
            return;
        }

        if(distance(player.center.x, enemy.center.x - enemy.radius*2, player.center.y, enemy.center.y) < dist) // left
        {
            if((enemy.center.x - enemy.radius*2 != player.center.x) || (enemy.center.y != player.center.y))
                g.enemy.monster[0].center.x -= g.enemy.monster[0].radius*2;
            return;
        }

        if(distance(player.center.x, enemy.center.x, player.center.y, enemy.center.y + enemy.radius*2) < dist) // down
        {
            if((enemy.center.x != player.center.x) || (enemy.center.y + enemy.radius*2 != player.center.y))
                g.enemy.monster[0].center.y += g.enemy.monster[0].radius*2;
            return;
        }

        if(distance(player.center.x, enemy.center.x + enemy.radius*2, player.center.y, enemy.center.y) < dist) // right
        {
            if((enemy.center.x + enemy.radius*2 != player.center.x) || (enemy.center.y != player.center.y))
                g.enemy.monster[0].center.x += g.enemy.monster[0].radius*2;
            return;
        }
    }
}

void nextTurn()
{
    tower();
    enemyMovement(g.circle, g.enemy.monster[0]); 
    g.maps.turns--;
    if(g.maps.turns == 0)
    {
        g.showComplete ^= 1;
        g.cmenu = 0;
    }
    /*g.spawnRate--;
    if(g.spawnRate == 0)
    {
        for(int i = 0; i < 4; i++)
        {
            if(g.enemy.spawn[i] == false)
            {
                spawnEnemy(i);
                g.enemy.spawn[i] = true;
            }
        }
        g.spawnRate = 3;
    }*/
    for(int i = 0; i < 4; i++)
    {
        if(g.enemy.spawn[i] == false)
        {
            g.enemy.spawnRate[i]--;
            if(g.enemy.spawnRate[i] == 0)
            {
                spawnEnemy(i);
                g.enemy.spawn[i] = true;
            }
        }
    }
}

void checkKeys(XEvent *e)
{
    if (e->type != KeyPress)
        return;
    //A key was pressed
    int key = XLookupKeysym(&e->xkey, 0);

    if(g.title && !g.showOptions && !g.showRes && !g.showMaps) // title
    {
        switch(key)
        {
            case XK_Escape:
                g.done = 1;
                break;
            case XK_w:
                if(g.menu-1 < 0)
                {
                    g.menu = 3;
                }
                g.menu = (g.menu-1)%3;
                break;
            case XK_s:
                g.menu = (g.menu+1)%3;
                break;
            case XK_space:
                if(g.menu == 0)
                {
                    g.title ^= 1;
                    g.showMaps ^= 1;
                    g.maps.menu = 0;
                }
                else if(g.menu == 1)
                {
                    g.title ^= 1;
                    g.showOptions ^= 1;
                    g.options.menu = 0;
                }
                else if(g.menu == 2)
                {
                    g.done = 1;
                }
                break;
        }
    }
    else if(!g.title && g.showOptions && !g.showRes && !g.showMaps) // options
    {
        switch(key)
        {
            case XK_Escape:
                g.title ^= 1;
                g.showOptions ^= 1;
                g.menu = 0;
                break;
            case XK_w:
                if(g.options.menu-1 < 0)
                {
                    g.options.menu = 3;
                }
                g.options.menu = (g.options.menu-1)%3;
                break;
            case XK_s:
                g.options.menu = (g.options.menu+1)%3;
                break;
            case XK_space:
                if(g.options.menu == 0)
                {
                    g.options.help ^= 1;
                }
                else if(g.options.menu == 1)
                {
                    g.showOptions ^= 1;
                    g.showRes ^= 1;
                    g.resolution.menu = 0;
                }
                else if(g.options.menu == 2)
                {
                    g.title ^= 1;
                    g.showOptions ^= 1;
                    g.menu = 0;
                }
                break;
        }
    }
    else if(!g.title && !g.showOptions && g.showRes && !g.showMaps) // resolution
    {
        switch(key)
        {
            case XK_Escape:
                g.showRes ^= 1;
                g.showOptions ^= 1;
                g.options.menu = 0;
                break;
            case XK_w:
                if(g.resolution.menu-1 < 0)
                {
                    g.resolution.menu = 4;
                }
                g.resolution.menu = (g.resolution.menu-1)%4;
                break;
            case XK_s:
                g.resolution.menu = (g.resolution.menu+1)%4;
                break;
            case XK_space:

                if(g.resolution.menu == 0)
                {
                    g.xres = 640;
                    g.yres = 480;
                    g.resolution.selection = 0;
                    g.circle.radius = 20.0;
                    cleanupXwindows();
                    initXwindows();
                }
                else if(g.resolution.menu == 1)
                {
                    g.xres = 840;
                    g.yres = 640;
                    g.resolution.selection = 1;
                    g.circle.radius = 40.0;
                    cleanupXwindows();
                    initXwindows();

                }
                else if(g.resolution.menu == 2)
                {
                    g.xres = 1080;
                    g.yres = 840;
                    g.resolution.selection = 2;
                    g.circle.radius = 40.0;
                    cleanupXwindows();
                    initXwindows();
                }
                else if(g.resolution.menu == 3)
                {
                    g.showRes ^= 1;
                    g.showOptions ^= 1;
                    g.options.menu = 0;
                }
                break;
        }
    }
    else if(!g.title && !g.showOptions && !g.showRes && g.showMaps) // maps
    {
        switch(key)
        {
            case XK_Escape:
                g.showMaps ^= 1;
                g.title ^= 1;
                g.menu = 0;
                break;
            case XK_w:
                if(g.maps.menu-1 < 0)
                {
                    g.maps.menu = 3;
                }
                g.maps.menu = (g.maps.menu-1)%3;
                break;
            case XK_s:
                g.maps.menu = (g.maps.menu+1)%3;
                break;
            case XK_space:
                if(g.maps.menu == 0)
                {
                    initMap1();
                    g.showMaps ^= 1;
                }
                else if(g.maps.menu == 1)
                {
                    initMap2();
                    g.showMaps ^= 1;
                }
                else if(g.maps.menu == 2)
                {
                    g.showMaps ^= 1;
                    g.title ^= 1;
                    g.menu = 0;
                }
                break;
        }
    }
    else if(!g.title && !g.showOptions && !g.showRes && !g.showMaps && g.showComplete) // completed map
    {
        switch(key)
        {
            case XK_Escape:
                g.showComplete ^= 1;
                g.title ^= 1;
                g.menu = 0;
                break;
            case XK_w:
                if(g.cmenu-1 < 0)
                {
                    g.cmenu = 3;
                }
                g.cmenu = (g.cmenu-1)%3;
                break;
            case XK_s:
                g.cmenu = (g.cmenu+1)%3;
                break;
            case XK_space:
                if(g.cmenu == 0)
                {
                    if(g.maps.menu == 0)
                        initMap1();
                    else if(g.maps.menu == 1)
                        initMap2();
                    g.showComplete ^= 1;
                }
                else if(g.cmenu == 1)
                {
                    g.showComplete ^= 1;
                    g.showMaps ^= 1;
                    g.maps.menu = 0;
                }
                else if(g.cmenu == 2)
                {
                    g.showComplete ^= 1;
                    g.title ^= 1;
                    g.menu = 0;
                }
                break;
        }
    }


    else if(!g.title && !g.showOptions && !g.showRes && !g.showMaps) // game
    {
        switch (key) {
            case XK_Escape:
                //quitting the program
                //g.done=1;
                g.title ^= 1;
                g.menu = 0;
                break;
            case XK_w:
                //g.circle.center.y -= g.circle.radius*2;
                //g.direction = 0;
                if(g.direction == 0 && (g.circle.center.y - g.circle.radius*2) < 0 && g.maps.area[g.maps.location][0] > -1)
                {
                    g.circle.center.y = g.yres-g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][0];
                }
                else if(g.direction == 0 && (g.circle.center.y - g.circle.radius*2) > 0)
                    g.circle.center.y -= g.circle.radius*2;

                if(g.direction == 1 && (g.circle.center.x - g.circle.radius*2) < 0 && g.maps.area[g.maps.location][1] > -1)
                {
                    g.circle.center.x = g.xres-g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][1];
                }
                else if(g.direction == 1 && (g.circle.center.x - g.circle.radius*2) > 0)
                    g.circle.center.x -= g.circle.radius*2;

                if(g.direction == 2 && (g.circle.center.y + g.circle.radius*2) > g.yres && g.maps.area[g.maps.location][2] > -1)
                {
                    g.circle.center.y = g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][2];
                }
                else if(g.direction == 2 && (g.circle.center.y + g.circle.radius*2) < g.yres)
                    g.circle.center.y += g.circle.radius*2;

                if(g.direction == 3 && (g.circle.center.x + g.circle.radius*2) > g.xres && g.maps.area[g.maps.location][3] > -1)
                {
                    g.circle.center.x = g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][3];
                }
                else if(g.direction == 3 && (g.circle.center.x + g.circle.radius*2) < g.xres)
                    g.circle.center.x += g.circle.radius*2;

                if(g.circle.center.x/g.grid.size-.5 == g.maps.grid[g.maps.destination].destination[0] &&
                        g.circle.center.y/g.grid.size-.5 == g.maps.grid[g.maps.destination].destination[1] &&
                        g.maps.location == g.maps.destination)
                {
                    g.showComplete ^= 1;
                    g.cmenu = 0;
                }

                for(int i = 0; i < 10; i++)
                {
                    if(g.circle.center.x/g.grid.size-.5 == g.maps.resource[i].center[0] &&
                            g.circle.center.y/g.grid.size-.5 == g.maps.resource[i].center[1] &&
                            g.maps.location == g.maps.resource[i].location)
                    {
                        if(g.maps.resource[i].abc == 0)
                            g.resource.resourceA++;
                        if(g.maps.resource[i].abc == 1)
                            g.resource.resourceB++;
                        if(g.maps.resource[i].abc == 2)
                            g.resource.resourceC++;
                        g.maps.resource[i].location = -1;
                    }
                }

                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                  g.maps.turns--;
                  if(g.maps.turns == 0)
                  {
                  g.showComplete ^= 1;
                  g.cmenu = 0;
                  }*/
                nextTurn();
                // NOTE: location of circle regarding grid is offset by 0.5
                //cout << g.circle.center.x/g.grid.size << endl << g.circle.center.y/g.grid.size << endl << endl;
                break;
            case XK_a:
                //g.circle.center.x -= g.circle.radius*2;
                //g.direction = 1;
                g.direction = (g.direction+1)%4;
                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                  g.maps.turns--;
                  if(g.maps.turns == 0)
                  {
                  g.showComplete ^= 1;
                  g.cmenu = 0;
                  }*/
                nextTurn();
                break;
            case XK_s:
                //g.circle.center.y += g.circle.radius*2;
                //g.direction = 2;
                //g.direction = (g.direction+2)%4;
                //tower();
                //break;
                if(g.direction == 0 && (g.circle.center.y + g.circle.radius*2) > g.yres && g.maps.area[g.maps.location][2] > -1)
                {
                    g.circle.center.y = g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][2];
                }
                else if(g.direction == 0 && (g.circle.center.y + g.circle.radius*2) < g.yres)
                    g.circle.center.y += g.circle.radius*2;

                if(g.direction == 1 && (g.circle.center.x + g.circle.radius*2) > g.xres && g.maps.area[g.maps.location][3] > -1)
                {
                    g.circle.center.x = g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][3];
                }
                else if(g.direction == 1 && (g.circle.center.x + g.circle.radius*2) < g.xres)
                    g.circle.center.x += g.circle.radius*2;

                if(g.direction == 2 && (g.circle.center.y - g.circle.radius*2) < 0 && g.maps.area[g.maps.location][0] > -1)
                {
                    g.circle.center.y = g.yres-g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][0];
                }
                else if(g.direction == 2 && (g.circle.center.y - g.circle.radius*2) > 0)
                    g.circle.center.y -= g.circle.radius*2;

                if(g.direction == 3 && (g.circle.center.x - g.circle.radius*2) < 0 && g.maps.area[g.maps.location][1] > -1)
                {
                    g.circle.center.x = g.xres-g.circle.radius;
                    g.maps.location = g.maps.area[g.maps.location][1];
                }
                else if(g.direction == 3 && (g.circle.center.x - g.circle.radius*2) > 0)
                    g.circle.center.x -= g.circle.radius*2;

                if(g.circle.center.x/g.grid.size-.5 == g.maps.grid[g.maps.destination].destination[0] &&
                        g.circle.center.y/g.grid.size-.5 == g.maps.grid[g.maps.destination].destination[1] &&
                        g.maps.location == g.maps.destination)
                {
                    g.showComplete ^= 1;
                    g.cmenu = 0;
                }

                for(int i = 0; i < 10; i++)
                {
                    if(g.circle.center.x/g.grid.size-.5 == g.maps.resource[i].center[0] &&
                            g.circle.center.y/g.grid.size-.5 == g.maps.resource[i].center[1] &&
                            g.maps.location == g.maps.resource[i].location)
                    {
                        if(g.maps.resource[i].abc == 0)
                            g.resource.resourceA++;
                        if(g.maps.resource[i].abc == 1)
                            g.resource.resourceB++;
                        if(g.maps.resource[i].abc == 2)
                            g.resource.resourceC++;
                        g.maps.resource[i].location = -1;
                    }
                }

                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                // NOTE: location of circle regarding grid is offset by 0.5
                //cout << g.circle.center.x/g.grid.size << endl << g.circle.center.y/g.grid.size << endl << endl;
                g.maps.turns--;
                if(g.maps.turns == 0)
                {
                g.showComplete ^= 1;
                g.cmenu = 0;
                }*/
                nextTurn();
                break;

            case XK_d:
                //g.circle.center.x += g.circle.radius*2;
                //g.direction = 3;
                g.direction = (g.direction+3)%4;
                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                  g.maps.turns--;
                  if(g.maps.turns == 0)
                  {
                  g.showComplete ^= 1;
                  g.cmenu = 0;
                  }*/
                nextTurn();
                break;

            case XK_q:
                g.direction = (g.direction+2)%4;
                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                  g.maps.turns--;
                  if(g.maps.turns == 0)
                  {
                  g.showComplete ^= 1;
                  g.cmenu = 0;
                  }*/
                nextTurn();
                break;

            case XK_f:
                g.flashlight ^= 1;
                break;

            case XK_1:
                if(g.combine == 0)
                    g.inventory = 1;
                else if(g.combine == 1 && g.resource.resourceA > 0 && g.resource.resourceB > 0)
                {
                    g.resource.resourceA--;
                    g.resource.resourceB--;
                    g.resource.bullets += 3;
                }
                break;

            case XK_2:
                if(g.combine == 0)
                    g.inventory = 2;
                else if(g.combine == 1 && g.resource.resourceA > 0 && g.resource.resourceC > 0)
                {
                    g.resource.resourceA--;
                    g.resource.resourceC--;
                    g.resource.grenades++;;
                }
                break;

            case XK_3:
                if(g.combine == 0)
                    g.inventory = 3;
                else if(g.combine == 1 && g.resource.resourceB > 0 && g.resource.resourceC > 0)
                {
                    g.resource.resourceB--;
                    g.resource.resourceC--;
                    g.resource.towers++;;
                }
                break;

            case XK_space:
                if(g.inventory == 1)
                {
                    if(g.resource.bullets > 0)
                    {
                        if(g.direction == 0)
                            shootBullet(g.circle.center.x, g.circle.center.y, 
                                    0, -g.circle.radius);
                        if(g.direction == 1)
                            shootBullet(g.circle.center.x, g.circle.center.y,
                                    -g.circle.radius, 0);
                        if(g.direction == 2)
                            shootBullet(g.circle.center.x, g.circle.center.y, 
                                    0, g.circle.radius);
                        if(g.direction == 3)
                            shootBullet(g.circle.center.x, g.circle.center.y, 
                                    g.circle.radius, 0);
                        g.resource.bullets--;
                    }
                }
                else if(g.inventory == 2)
                {
                    if(g.resource.grenades > 0)
                    {
                        Grenade *gr = new Grenade;

                        struct timespec grt;
                        clock_gettime(CLOCK_REALTIME, &grt);
                        timeDiff(&g.timer, &grt);
                        timeCopy(&g.timer, &grt);
                        timeCopy(&gr->time, &grt);

                        gr->pos.x = g.circle.center.x;
                        gr->pos.y = g.circle.center.y;
                        if(g.direction == 0)
                        {
                            gr->vel.x = 0;
                            gr->vel.y = -g.circle.radius/4;
                        }
                        if(g.direction == 1)
                        {
                            gr->vel.x = -g.circle.radius/4;
                            gr->vel.y = 0;
                        }
                        if(g.direction == 2)
                        {
                            gr->vel.x = 0;
                            gr->vel.y = g.circle.radius/4;
                        }
                        if(g.direction == 3)
                        {
                            gr->vel.x = g.circle.radius/4;
                            gr->vel.y = 0;
                        }

                        gr->next = g.ghead;
                        if(g.ghead != NULL)
                            g.ghead->prev = gr;
                        g.ghead = gr;
                        g.resource.grenades--;
                    }
                }
                else if(g.inventory == 3)
                {
                    if(g.resource.towers > 0)
                    {
                        Tower *t = new Tower;

                        if(g.direction == 0)
                        {
                            t->pos.x = g.circle.center.x;
                            t->pos.y = g.circle.center.y - g.circle.radius*2;
                        }
                        if(g.direction == 1)
                        {
                            t->pos.x = g.circle.center.x - g.circle.radius*2;
                            t->pos.y = g.circle.center.y;
                        }
                        if(g.direction == 2)
                        {
                            t->pos.x = g.circle.center.x;
                            t->pos.y = g.circle.center.y + g.circle.radius*2;
                        }
                        if(g.direction == 3)
                        {
                            t->pos.x = g.circle.center.x + g.circle.radius*2;
                            t->pos.y = g.circle.center.y;
                        }
                        t->ammo = 5;

                        t->next = g.thead;
                        if(g.thead != NULL)
                            g.thead->prev = t;
                        g.thead = t;
                        g.totalTowers++;
                        g.resource.towers--;
                    }
                }
                /*tower();
                  enemyMovement(g.circle, g.enemy.A); 
                  g.maps.turns--;
                  if(g.maps.turns == 0)
                  {
                  g.showComplete ^= 1;
                  g.cmenu = 0;
                  }*/
                nextTurn();
                break;

            case XK_c:
                g.combine ^= 1;
                break;

            case XK_u:
                g.showUI ^= 1;
                break;

                // DEBUG ///////////////////////////////////////////////
            case XK_r:
                g.resource.resourceA = 5;
                g.resource.resourceB = 5;
                g.resource.resourceC = 5;
                g.resource.bullets = 5;
                g.resource.grenades = 5;
                g.resource.towers = 5;
                break;

            case XK_g:
                g.drawGrid ^= 1;
                break;
                // DEBUG ///////////////////////////////////////////////
        }
    }
    clearScreen();
}

void deleteBullet(Bullet *node)
{
    //remove a node from linked list
    if(node->prev == NULL)
    {
        if(node->next == NULL)
        {
            g.bhead = NULL;
        }
        else
        {
            node->next->prev = NULL;
            g.bhead = node->next;
        }
    }
    else
    {
        if(node->next == NULL)
        {
            node->prev->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }
    delete node;
    node = NULL;
}

void deleteGrenade(Grenade *node)
{
    //remove a node from linked list
    if(node->prev == NULL)
    {
        if(node->next == NULL)
        {
            g.ghead = NULL;
        }
        else
        {
            node->next->prev = NULL;
            g.ghead = node->next;
        }
    }
    else
    {
        if(node->next == NULL)
        {
            node->prev->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }
    delete node;
    node = NULL;
}

void deleteTower(Tower *node)
{
    //remove a node from linked list
    if(node->prev == NULL)
    {
        if(node->next == NULL)
        {
            g.thead = NULL;
        }
        else
        {
            node->next->prev = NULL;
            g.thead = node->next;
        }
    }
    else
    {
        if(node->next == NULL)
        {
            node->prev->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    }
    delete node;
    node = NULL;
    g.totalTowers--;
}

Vec vecNormalize(Vec v)
{
    float length = v.x*v.x + v.y*v.y;
    if(length == 0.0)
        return v;
    length = 1.0 / sqrt(length);
    v.x *= length;
    //cout << v.x << endl;
    v.y *= length;

    return v;
}

void tower(void)
{
    Tower *t = g.thead;
    while(t)
    {
        Vec v;
        v.x = g.circle.center.x - t->pos.x;
        v.y = g.circle.center.y - t->pos.y;
        v = vecNormalize(v);

        shootBullet(t->pos.x, t->pos.y, v.x*g.circle.radius, v.y*g.circle.radius);

        t->ammo--;

        if(t->ammo <= 0)
            deleteTower(t);
        t = t->next;
    }
}

void spawnEnemy(int x)
{
    int rb = rand() % 4;    // which boundary to spawn enemy
    int rx = rand() % (g.xres / g.grid.size);   // where on x coordinate to spawn, based on grid location
    int ry = rand() % (g.yres / g.grid.size);   // same thing, but with y

    if(x == 0)
    {
        if(rb == 0)     // spawn top
        {
            g.enemy.monster[0].center.x = rx * g.grid.size + g.circle.radius;
            g.enemy.monster[0].center.y = g.circle.radius;
        }
        else if(rb == 1) // spawn left
        {
            g.enemy.monster[0].center.x = g.circle.radius;
            g.enemy.monster[0].center.y = ry * g.grid.size + g.circle.radius;
        }
        else if(rb == 2) // spawn bottom
        {
            g.enemy.monster[0].center.x = rx * g.grid.size + g.circle.radius;
            g.enemy.monster[0].center.y = g.yres - g.circle.radius;
        }
        else if(rb == 3) // spawn right
        {
            g.enemy.monster[0].center.x = g.xres - g.circle.radius;
            g.enemy.monster[0].center.y = ry * g.grid.size + g.circle.radius;
        }
        g.enemy.monster[0].location = g.maps.location;
        g.enemy.spawn[0] = true;
    }
}

void deleteEnemy(int x)
{
    if(x == 0)
    {
        g.enemy.monster[0].center.x = 0.0;
        g.enemy.monster[0].center.y = 0.0;
        g.enemy.monster[0].location = 0;
        //delete g.enemy.monster[0];
        //g.enemy.monster[0] == NULL;
        g.enemy.spawn[0] = false;
        g.enemy.spawnRate[0] = 3;
    }
}

void physics(void)
{
    Bullet *b = g.bhead;
    struct timespec bt;
    clock_gettime(CLOCK_REALTIME, &bt);

    while(b)
    {
        b->pos.x += b->vel.x;
        b->pos.y += b->vel.y;
        if(b->pos.x < 0.0 || b->pos.x > (float)g.xres ||
                b->pos.y < 0.0 || b->pos.y > (float)g.yres)
        {
            deleteBullet(b);
            b = b->next;
            continue;
        }

        // bullet hits enemy
        if(b->pos.x > g.enemy.monster[0].center.x - g.circle.radius && b->pos.x < g.enemy.monster[0].center.x + g.circle.radius &&
                b->pos.y > g.enemy.monster[0].center.y - g.circle.radius && b->pos.y < g.enemy.monster[0].center.y + g.circle.radius)
        {
            deleteBullet(b);
            deleteEnemy(0);
            b = b->next;
            continue;
        }

        double ts = timeDiff(&b->time, &bt);
        if(ts > 0.3)
        {
            deleteBullet(b);
        }

        b = b->next;
    }

    Grenade *gr = g.ghead;
    struct timespec grt;
    clock_gettime(CLOCK_REALTIME, &grt);

    while(gr)
    {
        gr->pos.x += gr->vel.x;
        gr->pos.y += gr->vel.y;
        if(gr->pos.x < 0.0 || gr->pos.x > (float)g.xres ||
                gr->pos.y < 0.0 || gr->pos.y > (float)g.yres)
        {
            deleteGrenade(gr);
        }

        double ts = timeDiff(&gr->time, &grt);
        if(ts > 0.2)
        {
            shootBullet(gr->pos.x, gr->pos.y, 0, -g.circle.radius);
            shootBullet(gr->pos.x, gr->pos.y, g.circle.radius, -g.circle.radius);
            shootBullet(gr->pos.x, gr->pos.y, g.circle.radius, 0);
            shootBullet(gr->pos.x, gr->pos.y, g.circle.radius, g.circle.radius);
            shootBullet(gr->pos.x, gr->pos.y, 0, g.circle.radius);
            shootBullet(gr->pos.x, gr->pos.y, -g.circle.radius, g.circle.radius);
            shootBullet(gr->pos.x, gr->pos.y, -g.circle.radius, 0);
            shootBullet(gr->pos.x, gr->pos.y, -g.circle.radius, -g.circle.radius);

            deleteGrenade(gr);
        }
        gr = gr->next;
    }
}

void setColor3i(int r, int g, int b)
{
    unsigned long cref = 0L;
    cref += r;
    cref <<= 8;
    cref += g;
    cref <<= 8;
    cref += b;
    XSetForeground(dpy, gc, cref);
}

void showMenu(int x, int y)
{
    char ts[64];

    (g.showUI==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
    sprintf(ts, "(U) UI: %s", (g.showUI==1)?"ON":"OFF");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    if(g.showUI)
    {
        y += 32;
        setColor3i(255, 100, 100);
        sprintf(ts,"Health: ");
        for(int i = 0; i < 5; i++)
        {
            strcat(ts, "I ");
        }
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

        if(g.options.help)
        {
            y += 32;
            (g.flashlight==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(F) Flashlight: %s", (g.flashlight==1)?"ON":"OFF");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }
        if(g.combine == 0)
        {
            y += 32;
            (g.inventory==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(1) Gun: %s", (g.inventory==1)?"ON":"OFF");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

            y += 16;
            (g.inventory==2) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(2) Grenade: %s", (g.inventory==2)?"ON":"OFF");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

            y += 16;
            (g.inventory==3) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(3) Tower: %s", (g.inventory==3)?"ON":"OFF");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }
        else if(g.combine == 1)
        {
            y += 32;
            (g.resource.resourceA > 0 && g.resource.resourceB > 0) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "1 ResourceA + 1 ResourceB = 3 bullets");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
            y += 16;
            (g.resource.resourceA > 0 && g.resource.resourceC > 0) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "1 ResourceA + 1 ResourceC = 1 grenade");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
            y += 16;
            (g.resource.resourceB > 0 && g.resource.resourceC > 0) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "1 ResourceB + 1 ResourceC = 1 tower");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }

        if(g.options.help)
        {
            y += 32;
            setColor3i(255,255,255);
            sprintf(ts, "(Space) Shoot");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }

        y += 32;
        setColor3i(255,255,255);
        sprintf(ts, "Resource A: %i", g.resource.resourceA);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        y += 16;
        setColor3i(255,255,255);
        sprintf(ts, "Resource B: %i", g.resource.resourceB);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        y += 16;
        setColor3i(255,255,255);
        sprintf(ts, "Resource C: %i", g.resource.resourceC);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        y += 32;
        setColor3i(255,255,255);
        sprintf(ts, "Bullets: %i", g.resource.bullets);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        y += 16;
        setColor3i(255,255,255);
        sprintf(ts, "Grenades: %i", g.resource.grenades);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        y += 16;
        setColor3i(255,255,255);
        sprintf(ts, "Towers: %i", g.resource.towers);
        XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

        if(g.maps.turns > 0)
        {
            y += 32;
            setColor3i(255,255,255);
            sprintf(ts, "Turns left: %i", g.maps.turns);
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }

        if(g.options.help)
        {
            y += 32;
            (g.combine==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(C) Combine: %s", (g.combine==1)?"ON":"OFF");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

            y += 32;
            setColor3i(0,255,255);
            sprintf(ts, "(DEBUG) (R) Reset resources");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

            y += 16;
            (g.drawGrid==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
            sprintf(ts, "(DEBUG) (G) Show Grid");
            XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
        }
    }
}

void showTitle(int x, int y)
{
    char ts[64];

    setColor3i(255, 255, 255);
    sprintf(ts,"Insert Title Here");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    sprintf(ts, "%s Start Game", (g.menu==0)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    sprintf(ts, "%s Options", (g.menu==1)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    sprintf(ts, "%s Exit Game", (g.menu==2)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
}

void showOption(int x, int y)
{
    char ts[64];

    setColor3i(255, 255, 255);
    sprintf(ts,"Options");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    (g.options.help==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
    sprintf(ts, "%s Set Help: %s", (g.options.menu==0)?"-->":"   ", (g.options.help==1)?"ON":"OFF");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    setColor3i(255, 255, 255);
    y += 16;
    sprintf(ts, "%s Set Resolution", (g.options.menu==1)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    sprintf(ts, "%s Return", (g.options.menu==2)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
}

void showRes(int x, int y)
{
    char ts[64];

    setColor3i(255,255,255);
    sprintf(ts, "Resolutions");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    (g.resolution.selection==0) ? setColor3i(0,255,0) : setColor3i(255,0,0);
    sprintf(ts, "%s 640 x 480: %s", (g.resolution.menu==0)?"-->":"   ", (g.resolution.selection==0)?"ON":"OFF");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    (g.resolution.selection==1) ? setColor3i(0,255,0) : setColor3i(255,0,0);
    sprintf(ts, "%s 840 x 640: %s", (g.resolution.menu==1)?"-->":"   ", (g.resolution.selection==1)?"ON":"OFF");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    (g.resolution.selection==2) ? setColor3i(0,255,0) : setColor3i(255,0,0);
    sprintf(ts, "%s 1080 x 840: %s", (g.resolution.menu==2)?"-->":"   ", (g.resolution.selection==2)?"ON":"OFF");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    setColor3i(255,255,255);
    y += 32;
    sprintf(ts, "%s Return", (g.resolution.menu==3)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
}

void showMaps(int x, int y)
{
    char ts[64];

    setColor3i(255,255,255);
    sprintf(ts, "Maps");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    sprintf(ts, "%s Map1", (g.maps.menu==0)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    sprintf(ts, "%s Map2", (g.maps.menu==1)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    sprintf(ts, "%s Return", (g.maps.menu==2)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
}

void showComplete(int x, int y)
{
    char ts[64];

    setColor3i(255,255,255);
    sprintf(ts, "Map completed");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 32;
    sprintf(ts, "%s Retry", (g.cmenu==0)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    sprintf(ts, "%s Select Map", (g.cmenu==1)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));

    y += 16;
    sprintf(ts, "%s Quit", (g.cmenu==2)?"-->":"   ");
    XDrawString(dpy, backBuffer, gc, x, y, ts, strlen(ts));
}

void render(void)
{
    if(g.title)
    {
        showTitle(4, 16);
    }
    else if(!g.title && g.showOptions)
    {
        showOption(4, 16);
    }
    else if(g.showRes)
    {
        showRes(4, 16);
    }
    else if(g.showMaps)
    {
        showMaps(4, 16);
    }
    else if(g.showComplete)
    {
        showComplete(4, 16);
    }
    else if(!g.title && !g.showOptions)     // render game
    {
        if(g.drawGrid)
        {
            setColor3i(255,255,255);
            for(size_t i = 0; i < sizeof(g.grid.column)/4; i++)
            {
                XDrawLine(dpy, backBuffer, gc, g.grid.column[i], 0, g.grid.column[i], g.yres);
            }
            for(size_t i = 0; i < sizeof(g.grid.row)/4; i++)
            {
                XDrawLine(dpy, backBuffer, gc, 0, g.grid.row[i], g.xres, g.grid.row[i]);
            }
        }
        showMenu(4, 16);

        if(g.maps.location == g.maps.start)     // render starting zone
        {
            setColor3i(100,255,255);
            XFillRectangle(dpy, backBuffer, gc, g.maps.grid[g.maps.start].start[0]*g.grid.size, 
                    g.maps.grid[g.maps.start].start[1]*g.grid.size,
                    2*g.circle.radius, 2*g.circle.radius);
        }
        if(g.maps.location == g.maps.destination)   // render end zone
        {
            setColor3i(255,255,100);
            XFillRectangle(dpy, backBuffer, gc, g.maps.grid[g.maps.destination].destination[0]*g.grid.size, 
                    g.maps.grid[g.maps.destination].destination[1]*g.grid.size,
                    2*g.circle.radius, 2*g.circle.radius);
        }

        for(int i = 0; i < 10; i++)     // render resources
        {
            if(g.maps.location == g.maps.resource[i].location)
            {
                setColor3i(g.maps.resource[i].color[0], g.maps.resource[i].color[1], g.maps.resource[i].color[2]);
                XFillRectangle(dpy, backBuffer, gc, g.maps.resource[i].center[0]*g.grid.size,
                        g.maps.resource[i].center[1]*g.grid.size,
                        2*g.circle.radius, 2*g.circle.radius);
            }

        }

        //render player
        setColor3i(255,255,255);
        XDrawArc(dpy, backBuffer, gc, g.circle.center.x-g.circle.radius, g.circle.center.y-g.circle.radius, 
                g.circle.radius*2, g.circle.radius*2, 0, 360*64);
        XFillArc(dpy, backBuffer, gc, g.circle.center.x-g.circle.radius, g.circle.center.y-g.circle.radius, 
                g.circle.radius*2, g.circle.radius*2, 0, 360*64);

        // render enemy
        // *change later to accomodate different enemies*
        if(g.maps.location == g.enemy.monster[0].location)
        {
            setColor3i(255,255,255);
            XDrawArc(dpy, backBuffer, gc, g.enemy.monster[0].center.x-g.circle.radius, g.enemy.monster[0].center.y-g.circle.radius,
                    g.circle.radius*2, g.circle.radius*2, 0, 360*64);
            setColor3i(255,0,0);
            XFillArc(dpy, backBuffer, gc, g.enemy.monster[0].center.x-g.circle.radius, g.enemy.monster[0].center.y-g.circle.radius,
                    g.circle.radius*2, g.circle.radius*2, 0, 360*64);
        }

        //setColor3i(255,0,0);
        //XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
        //        g.aim.x, g.aim.y);

        if(g.direction == 0)
        {
            for(float i = g.angle; i < g.angle*2; i += 0.005)
            {
                if(g.flashlight)
                {
                    setColor3i(255,255,0);
                    g.point1.x = g.circle.center.x - (cos(i)*g.dist);
                    g.point1.y = g.circle.center.y - (sin(i)*g.dist);
                    g.point2.x = g.circle.center.x + (cos(i)*g.dist);
                    g.point2.y = g.circle.center.y - (sin(i)*g.dist);
                }
                else
                {
                    setColor3i(150,150,150);
                    g.point1.x = g.circle.center.x - (cos(i)*g.dist/2);
                    g.point1.y = g.circle.center.y - (sin(i)*g.dist/2);
                    g.point2.x = g.circle.center.x + (cos(i)*g.dist/2);
                    g.point2.y = g.circle.center.y - (sin(i)*g.dist/2);
                }
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point1.x, g.point1.y);
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point2.x, g.point2.y);
            }
        }
        if(g.direction == 1)
        {
            for(float i = 0.0; i < g.angle; i += 0.005)
            {
                if(g.flashlight)
                {
                    setColor3i(255,255,0);
                    g.point1.x = g.circle.center.x - (cos(i)*g.dist);
                    g.point1.y = g.circle.center.y + (sin(i)*g.dist);
                    g.point2.x = g.circle.center.x - (cos(i)*g.dist);
                    g.point2.y = g.circle.center.y - (sin(i)*g.dist);
                }
                else
                {
                    setColor3i(150,150,150);
                    g.point1.x = g.circle.center.x - (cos(i)*g.dist/2);
                    g.point1.y = g.circle.center.y + (sin(i)*g.dist/2);
                    g.point2.x = g.circle.center.x - (cos(i)*g.dist/2);
                    g.point2.y = g.circle.center.y - (sin(i)*g.dist/2);
                }
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point1.x, g.point1.y);
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point2.x, g.point2.y);
            }
        }
        if(g.direction == 2)
        {
            for(float i = g.angle; i < g.angle*2; i += 0.005)
            {
                if(g.flashlight)
                {
                    setColor3i(255,255,0);
                    g.point1.x = g.circle.center.x + (cos(i)*g.dist);
                    g.point1.y = g.circle.center.y + (sin(i)*g.dist);
                    g.point2.x = g.circle.center.x - (cos(i)*g.dist);
                    g.point2.y = g.circle.center.y + (sin(i)*g.dist);
                }
                else
                {
                    setColor3i(150,150,150);
                    g.point1.x = g.circle.center.x + (cos(i)*g.dist/2);
                    g.point1.y = g.circle.center.y + (sin(i)*g.dist/2);
                    g.point2.x = g.circle.center.x - (cos(i)*g.dist/2);
                    g.point2.y = g.circle.center.y + (sin(i)*g.dist/2);
                }
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point1.x, g.point1.y);
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point2.x, g.point2.y);
            }
        }
        if(g.direction == 3)
        {
            for(float i = 0; i < g.angle; i += 0.005)
            {
                if(g.flashlight)
                {
                    setColor3i(255,255,0);
                    g.point1.x = g.circle.center.x + (cos(i)*g.dist);
                    g.point1.y = g.circle.center.y - (sin(i)*g.dist);
                    g.point2.x = g.circle.center.x + (cos(i)*g.dist);
                    g.point2.y = g.circle.center.y + (sin(i)*g.dist);
                }
                else
                {
                    setColor3i(150,150,150);
                    g.point1.x = g.circle.center.x + (cos(i)*g.dist/2);
                    g.point1.y = g.circle.center.y - (sin(i)*g.dist/2);
                    g.point2.x = g.circle.center.x + (cos(i)*g.dist/2);
                    g.point2.y = g.circle.center.y + (sin(i)*g.dist/2);
                }
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point1.x, g.point1.y);
                XDrawLine(dpy, backBuffer, gc, g.circle.center.x, g.circle.center.y,
                        g.point2.x, g.point2.y);
            }
        }

        Bullet *b = g.bhead;
        while(b)
        {
            setColor3i(255,255,255);
            XDrawRectangle(dpy, backBuffer, gc, b->pos.x-g.circle.radius/4, b->pos.y-g.circle.radius/4, 
                    g.circle.radius/2, g.circle.radius/2);
            XFillRectangle(dpy, backBuffer, gc, b->pos.x-g.circle.radius/4, b->pos.y-g.circle.radius/4, 
                    g.circle.radius/2, g.circle.radius/2);
            b = b->next;
        }

        Grenade *gr = g.ghead;
        while(gr)
        {
            setColor3i(0,255,255);
            XDrawRectangle(dpy, backBuffer, gc, gr->pos.x-g.circle.radius/4, gr->pos.y-g.circle.radius/4, 
                    g.circle.radius/2, g.circle.radius/2);
            XFillRectangle(dpy, backBuffer, gc, gr->pos.x-g.circle.radius/4, gr->pos.y-g.circle.radius/4, 
                    g.circle.radius/2, g.circle.radius/2);
            gr = gr->next;
        }

        Tower *t = g.thead;
        while(t)
        {
            setColor3i(0, 0, 255);

            XDrawArc(dpy, backBuffer, gc, t->pos.x-g.circle.radius, t->pos.y-g.circle.radius, 
                    g.circle.radius*2, g.circle.radius*2, 0, 360*64);
            XFillArc(dpy, backBuffer, gc, t->pos.x-g.circle.radius, t->pos.y-g.circle.radius, 
                    g.circle.radius*2, g.circle.radius*2, 0, 360*64);
            t = t->next;
        }
    }
}

void initGrid()
{
    //g.circle.radius = (g.xres + g.yres)/100;
    g.grid.size = g.circle.radius * 2;
    //g.grid.size = g.xres / 10;
    int position = 0;
    for(int i = 0; i < g.xres; i += g.grid.size)
    {
        g.grid.column[position] = i;
        position++;
    }
    position = 0;
    for(int i = 0; i < g.yres; i += g.grid.size)
    {
        g.grid.row[position] = i;
        position++;
    }
    //g.circle.center.x = g.xres/g.grid.size;
    //g.circle.center.y = g.yres/g.grid.size;
}

void clearMap()
{
    for(int i = 0; i < 10; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            g.maps.area[i][j] = -1;
        }
        g.maps.grid[i].start[0] = -1;
        g.maps.grid[i].start[1] = -1;
        g.maps.grid[i].destination[0] = -1;
        g.maps.grid[i].destination[1] = -1;

        g.maps.resource[i].center[0] = -1;
        g.maps.resource[i].center[1] = -1;
        g.maps.resource[i].color[0] = 0;
        g.maps.resource[i].color[1] = 0;
        g.maps.resource[i].color[2] = 0;
        g.maps.resource[i].location = -1;
    }
    g.maps.turns = -1;
}

void initMap1()
{
    clearMap();
    g.maps.area[0][3] = 1;
    g.maps.area[1][1] = 0;
    g.maps.area[1][0] = 2;
    g.maps.area[2][2] = 1;
    g.maps.area[2][3] = 3;
    g.maps.area[3][1] = 2;

    // NOTE: how i can set circle's location to grid [5,5]
    g.maps.grid[0].start[0] = 5;
    g.maps.grid[0].start[1] = 5;
    g.circle.center.x = g.maps.grid[0].start[0] * g.grid.size + g.circle.radius;
    g.circle.center.y = g.maps.grid[0].start[1] * g.grid.size + g.circle.radius;
    g.direction = 0;
    g.maps.location = 0;
    g.maps.start = 0;
    g.maps.destination = 3;

    g.maps.grid[3].destination[0] = 10;
    g.maps.grid[3].destination[1] = 10;

    g.maps.turns = -1;

    for(int i = 0; i < 10; i++)
    {
        // change rm to something based on number of areas
        int rm = rand() % 4;
        int rx = rand() % (g.xres / g.grid.size);
        int ry = rand() % (g.yres / g.grid.size);
        int rabc = rand() % 3;
        while((g.maps.start == rm &&
                    g.maps.grid[g.maps.start].start[0] == rx &&
                    g.maps.grid[g.maps.start].start[1] == ry) ||
                (g.maps.destination == rm &&
                 g.maps.grid[g.maps.destination].destination[0] == rx &&
                 g.maps.grid[g.maps.destination].destination[1] == ry))
        {
            rm = rand() % 4;
            rx = rand() % (g.xres / g.grid.size);
            ry = rand() % (g.yres / g.grid.size);
            rabc = rand() % 3;
        }

        g.maps.resource[i].location = rm;
        g.maps.resource[i].center[0] = rx;
        g.maps.resource[i].center[1] = ry;
        g.maps.resource[i].abc = rabc;
        if(rabc == 0)
            g.maps.resource[i].color[0] = 255;
        if(rabc == 1)
            g.maps.resource[i].color[1] = 255;
        if(rabc == 2)
            g.maps.resource[i].color[2] = 255;
    }

    // reset enemies
    g.enemy.spawn[0] = false;

}

void initMap2()
{
    clearMap();
    g.maps.grid[0].start[0] = 5;
    g.maps.grid[0].start[1] = 5;
    g.circle.center.x = g.maps.grid[0].start[0] * g.grid.size + g.circle.radius;
    g.circle.center.y = g.maps.grid[0].start[1] * g.grid.size + g.circle.radius;
    g.direction = 0;
    g.maps.location = 0;
    g.maps.start = 0;

    g.maps.turns = 20;
}

