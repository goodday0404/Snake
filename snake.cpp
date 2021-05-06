/*
Commands to compile and run:

    g++ -o snake snake.cpp -std=c++11 -L/usr/X11R6/lib -lX11 -lstdc++
    ./snake

Note: the -L option and -lstdc++ may not be needed on some machines.
*/

#include <iostream>
#include <list>
#include <cstdlib>
#include <sys/time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <chrono> // for timer function below

/*
 * Header files for X functions
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using namespace std;
using namespace std::chrono;

/******************* Global variables for game desicription **************************

     Purpose: variable to be used in describing how to play game int the splash screen

     		  before start the game.

**************************************************************************************/
const string stars = "************************************************************************************************";
const string title = "                                                SNAKE                                           ";
// const string Name = "********************************";
const string toStart = "Press s to start game.";
const string howToPlay1 = "1. Use arrow keys to move.";
const string howToPlay2 = "2. Snake grows and will score up when eating fruit (green square).";
const string howToPlay3 = "3. Game items:";
const string howToPlay4 = "      - green square: food. Increases snake length and score";
const string howToPlay5 = "      - blue square: Invicible potion. Snake can cross walls and itself.";
const string howToPlay6 = "      - brown square: Snake gains 2x score when eating fruits.";
const string howToPlay7 = "      - the blue & brown squares' effect lasts for 30 seconds.";
const string howToPlay8 = "3. Game over when snake crashes into walls or eats itself";
//const string howToPlay8 = "      - snake crashes into walls or eats itself";
//const string howToPlay6 = "      - snake eats itself";
const string howToPlay9 = "4. Snake grows from head. Be careful when eating near walls!";
const string keyboard1 = "5. Keyboard commands:";
//const string keyboard2 = "      - Start: s (from initial screen)";
const string keyboard2 = "      - Pause: p      - Restart: r      - Quick: q";
//const string keyboard4 = "      - Yes: y, No: n (in the restart screen)";
/********************** Global game state variables ********************** ************

     Purpose: 

**************************************************************************************/
enum State {Initial = 0, Play, Pause, Restart, Yes, No}; // indicate of current game state.
enum ItemState {Regular = 0, Invin, Dscore}; // indicate that if the effect of item still continue.
											 // the effect lasts 30 seconds.
enum Count {TenSec = 0, Zero}; // show that effect of item ends in 10 second or gone.

const int Border = 1;
const int BufferSize = 3;
const int width = 800;
const int height = 600;
int FPS = 30;
int SpeedRate = 4; // moving speed of the snake
int Score = 0;  // tracking score for eating fruits.
State GameState = Initial; // current game state.
ItemState itemState = Regular; // effect of item at current time.
Count counting = Zero; // timer is zero at the start of game.
bool item = false; // wheter item is created on the screen.
bool countStart = true; // whether timer started to count 30 seconds.

high_resolution_clock::time_point before; // time when snake eat an item.
high_resolution_clock::time_point current; // current time when new event happens 
										   // since snake ate the item.

/********************** Information to draw on the window *****************************

     Purpose: struct that contains information Xserver. 

**************************************************************************************/
struct XInfo {
	Display	 *display;
	int		 screen;
	Window	 window;
	GC		 gc[3];
	Pixmap	pixmap;		// temperally buffer for double buffer
	int		width;		// size of window
	int		height;
	char key;           // key pressed in the current event
};

enum {White=0, Black, Red, Green, Blue, Yellow, Cyan, Magenta, Orange, Brown}; // Available colours.
unsigned long colours[10]; // constainer to store colour information.

/******************* Fowrd declarations for Global procedures *************************

     Purpose: used to control Xserver side. 

**************************************************************************************/
/*
 * Function to put out a message on error exits.
 */
void error( string str ) {
  cerr << str << endl;
  exit(0);
}

// convert int to string.
string toString(int i) {
	stringstream ss;
	string converted;
	ss << i;
	ss >> converted;
	return converted;
} // toString

// forward declations to draw rectangles. See definitions below.
void fillRectangle(XInfo &xinfo, int x, int y, int width, int height, int colour);
void paintRectangle(XInfo &xinfo, int x, int y, int width, int height, int thick, int line, int bg);
unsigned long now(); 
bool timer();
/********************** Displayable **************************************************

     Purpose: An abstract class representing displayable things.

**************************************************************************************/
class Displayable {
	protected:
		int x, y;
	public:
		Displayable(int x, int y): x(x), y(y) {}

		void setX(int x) {
			this->x = x;
		} // setX

		void setY(int y) {
			this->y = y;
		} // setY

		int getX() {
			return x;
		} // getX

		int getY() {
			return y;
		} // getY

		// return absolute vale of given val.
		int abs(int val) {
        	return (val > 0) ? val : val * -1;
        } // abs

		virtual void paint(XInfo &xinfo) = 0;
};       


/********************** Snake *********************************************************

     Purpose: Inherit Displayble and represent snake on the game board.

**************************************************************************************/
int snakeInitX = 115, snakeInitY = 445; // initial coordinate of the snake when starting game.

class Snake : public Displayable {
		static list<Snake*> body; // container store pieces of snake body.
		static int blockSize; // size of the snake body block
		static int lineSize; // thickness of the line srounding snake body block.
		static int minX, minY, maxX, maxY; // min and max index number of x and y on the board.

	public:
		Snake(int x, int y): Displayable(x,y) {} // constructor

		// deallocate the snake body.
		static void cleanUp() {
			list<Snake*>::iterator it;
			for (it = body.begin(); it != body.end(); it++) {
				if (*it) delete *it;
			} // for

			while (!body.empty()) body.pop_front();
		} // cleanUp

		// provide access to the snake body. Should be assigned to lvalue that has
		// type const list<Snake*> &.
		static list<Snake*> getBody() {
			return body;
		} // getBody

		// paint snake on the screen.
		virtual void paint(XInfo &xinfo) {
			list<Snake*>::iterator it;
			for (it = body.begin(); it != body.end(); it++) {
				paintRectangle(xinfo,(*it)->getX(),(*it)->getY(),Snake::blockSize,Snake::blockSize,Snake::lineSize,Black,Red);
			} // for
		} // paint

		// initialize snake when starting game.
		void initSnake() {
			int cx = x;
			Snake::body.push_back(new Snake(snakeInitX,snakeInitY)); // the end of snake tail.
			// snake is drawn by overlapped 22 X 22 squares. The distance from left-top coordinate
			// of a square under another squar to left-top coordinate of "the another squar" is 
			// same as SpeedRate. Thus, the size of snake at the start varies by moving speed of
			// the snake. 
			for (int i = 0; i < 10; i++) {
				cx += SpeedRate;
				setX(cx);
				Snake::body.push_back(new Snake(cx,y)); // the squar added at the last is snake head.
			} // for 
		} // initSnake

		void move(XInfo &xinfo) {
			list<Snake*>::iterator begin = body.begin();
			Snake *s = *begin; // the end of snake tail

			// update the next moving direction of snake according to arrow key pressed.
			if (xinfo.key == 'L') setX(x - SpeedRate); // left
			else if (xinfo.key == 'U') setY(y - SpeedRate); // up
			else if (xinfo.key == 'R') setX(x + SpeedRate); // right
			else if (xinfo.key == 'D') setY(y + SpeedRate); // down

			// colision check only when snake is not invincible by item.
			if (itemState != Invin) collision(); 

			body.push_back(new Snake(x,y)); // add to a new head
			body.pop_front(); // remove tail
			delete s; // delete tail from heap
		} // move

		// dectect collision when snake crash walls or eat itself. It recognizes movement as 
		// collision only when the range of head's coornate overraps with other part of snake body.
		// It's not collision snake body is overrapped itself or its part of body is outside screen
		// when the effect of invincible just disppeared as long as its head doesn't crash wall or
		// its body.
		void collision() {
			if (x < minX || x > maxX || y < minY || y > maxY || isSnake(x,y)) {
				Snake::cleanUp(); // deallocate snake body.
				exit(0);
			} // if
		} // isCollision

		// extend snake body when eating fruit. Extended as much as SpeedRate.
		// Extended from its head.
        void didEatFruit(XInfo xinfo) {   
			for (int i = 0; i < 5; i++) {
				// update new head's coordinate.
				if (xinfo.key == 'L') setX(x - SpeedRate);
				else if (xinfo.key == 'U') setY(y - SpeedRate);
				else if (xinfo.key == 'R') setX(x + SpeedRate);
				else if (xinfo.key == 'D') setY(y + SpeedRate);
				Snake::body.push_back(new Snake(x,y)); 
			} // for 
        } // didEatFruit

        // dectect whether snake eats its body itself.
        bool isSnake(int cx, int cy) {
        	bool crash = false;  
        	int i = 0; // count up to (blockSize / SpeedRate + 1). Max count can be is 22, which is the 
        			   // length of one side of the square that builds the snake and also the snake head.

		    // This for loop finds and skips all squares in the snake body that overlaps the head. 
        	// Otherwise, the snake will be in collision with its body the moment the game starts and 
        	// it will be game over.
        	list<Snake*>::const_reverse_iterator it;
        	for (it = body.rbegin(); it != body.rend(); ++it) {
        		if (i > blockSize / SpeedRate + 1) { // first few blocks from head.
        			if (abs((*it)->getX() - x) < SpeedRate && abs((*it)->getY() - y) < SpeedRate) {
	    					crash = true; break;
	    			} // if
        		} // if	
        		++i;
        	} // for
        	return crash;
        } // isSnake
}; // Snake

// initialize static fields.
list<Snake*> Snake::body;
int Snake::blockSize = 22;
int Snake::lineSize = 2;
int Snake::minX = 5, Snake::minY = 5;
int Snake::maxX = 770;
int Snake::maxY = 523;
/********************** Fruit *********************************************************

     Purpose: Inherit Displayble and represent a fruit item on the game board.

**************************************************************************************/
class Fruit : public Displayable {
	protected:
	int blockSize, lineSize, fx, fy; // fx, fy: coordinate of fruit on the screen
	Snake *snake; // pointer ot snake.
	public:
		virtual void paint(XInfo &xinfo) {
			paintRectangle(xinfo,x,y,blockSize,blockSize,lineSize,Black,Green);
        }

        Fruit(int x = 50, int y = 50): Displayable(x,y), blockSize(22), lineSize(2), snake(NULL), 
        							   fx(0), fy(0) {}

        void setSnake(Snake *snake) {
        	this->snake = snake;
        } // setSnake

        void setFruitPosition(int cx, int cy) {
	    	fx = cx;
	    	fy = cy;
	    } // setFruitPosition

	    // regenerate fruit on the screen. Snake grows from head, fruit won't be generated on
	    // each edge corners(most left-top, right-top, left-bottom, right bottom corners) to
	    // avoid colision with wall when snake body grows.
	    // Coordinate of the fruit will be randomly generated.
        void getNewFruit() {
        	int nx, ny;
        	bool crash = false, corner = false, fruit = false;
        	// repeat iteration until it make sure new fruit that will be drawn from nx and ny
        	// won't overrap with walls and snake body. 
        	do {
        		crash = false;
        		nx = randomNumber(now(),770); // generate random number ranged [0, 769]
            	ny = randomNumber(now(),523); // generate random number ranged [0, 522]
            	crash = crashSnake(nx,ny); // check colision with snake body
            	corner = isCorners(nx,ny); // check if fruit will be located on any corner.
            	fruit = crashFruit(nx,ny); // check colision with fruit (checked for items).
        	} while (crash || corner || fruit);

            setX(nx);
            setY(ny);
        } // getFruit

        // check whether the new location of newly generated fruit overrap with snake.
        bool crashSnake(int nx, int ny) {
        	bool crash = false;
        	const list<Snake*> &piece = snake->getBody();
        	list<Snake*>::const_iterator it;
        	for (it = piece.begin(); it != piece.end(); it++) {
        		if (abs((*it)->getX() - nx) < blockSize && abs((*it)->getY() - ny) < blockSize) {
        			crash = true; break;
        		} // if
        	} // for
        	return crash;
        } // crashSnake

       // check colision with fruit (checked for items).
       virtual bool crashFruit(int nx, int ny) {
        	return (itemState == Regular) ? 
        				// there is no potion in on the screen
        				(abs(1000 - nx) < blockSize && abs(1000 - ny) < blockSize) :
        				// check if overraping with potion
        				(abs(fx - nx) < blockSize && abs(fy - ny) < blockSize); 
        } // crashFruit

        // check if fruit will be located on any corner.
        bool isCorners(int cx, int cy) {
        	bool corner = false;
        	if (cx >= 5 && cx <= 28 && cy >= 5 && cy <= 28) corner = true;
        	else if (cx + 22 >= 770 && cy >= 5 && cy <= 28) corner = true;
        	else if (cx >= 5 && cx <= 28 && cy + 22 >= 523) corner = true;
        	else if (cx + 22 >= 770 && cy + 22 >= 523) corner = true;
        	return corner;
        } // isCorners

        // generate random number from 5 to mod - 1.
        int randomNumber(unsigned long ran, int mod) {
        	int random = ran % mod;
        	if (random < 6) random = 5; // 0 - 4 is the range of wall.
        	return random;
        } // randomNumber

        // check if fruit is eated by snake.
        bool IsEaten(int cx, int cy, char kind) {
        	bool eaten = false;
        	if (abs(cx - x) < blockSize && abs(cy - y) < blockSize) {
        		eaten = true;
        		++Score;
        		if (itemState == Dscore) ++Score; // double score when snake eats double scoreing item.
        		if (kind == 'R') getNewFruit(); // regenerate only regular fruit, no items.
        	} // if
        	return eaten;
        } // IsEaten
};

/***************************** Invincible *********************************************

     Purpose: Inherit Fruit and represent a invincible item on the game board.
     		  Snake don't die while the effect of this item lasing. Thus it can go through
     		  walls and its own body.

**************************************************************************************/
class Invincible: public Fruit {
	int fx, fy; // coordinate of Fruit on the screen.
public:
	Invincible(int x = 0, int y = 0): Fruit(x,y) {}

	virtual void paint(XInfo &xinfo) {
		if (item && x && y) paintRectangle(xinfo,x,y,blockSize,blockSize,lineSize,Black,Blue);
    } // paint

    // prevent that the area where this item is generated doesn't overrap with fruit.
    virtual bool crashFruit(int nx, int ny) {
    	// nx and ny are randomly gerated coordinate and pass by getNewFruit().
    	return (abs(fx - nx) < blockSize && abs(fy - ny) < blockSize);
    } // crashFruit
};

/***************************** DoubleScore *******************************************

     Purpose: Inherit Fruit and represent a invincible item on the game board.
     		  Snake don't die while the effect of this item lasing. Thus it can go through
     		  walls and its own body.

**************************************************************************************/
class DoubleScore: public Fruit {
	int fx, fy; // coordinate of Fruit on the screen.
public:
	DoubleScore(int x = 0, int y = 0): Fruit(x,y) {}

	virtual void paint(XInfo &xinfo) {
		if (item && x && y) paintRectangle(xinfo,x,y,blockSize,blockSize,lineSize,Black,Brown);
    } // paint

    // prevent that the area where this item is generated doesn't overrap with fruit.
    virtual bool crashFruit(int nx, int ny) {
    	// nx and ny are randomly gerated coordinate and pass by getNewFruit().
    	return (abs(fx - nx) < blockSize && abs(fy - ny) < blockSize);
    } // crashFruit
};

/****************** Global procedures to controll Xserver side ************************

     Purpose: used to control Xserver side. 

**************************************************************************************/
list<Displayable *> dList;  // list of Displayables
Snake snake(snakeInitX,snakeInitY); 
Fruit fruit;
Invincible invincible; // invincible potion
DoubleScore dScore; // double score potion

/*
 * Initialize X and create a window
 */
void initX(int argc, char *argv[], XInfo &xInfo) {
	XSizeHints hints;
	unsigned long white, black;

   /*
	* Display opening uses the DISPLAY	environment variable.
	* It can go wrong if DISPLAY isn't set, or you don't have permission.
	*/	
	xInfo.display = XOpenDisplay( "" );
	if ( !xInfo.display )	{
		error("Can't open display.");
	}
	
   /*
	* Find out some things about the display you're using.
	*/
	xInfo.screen = DefaultScreen( xInfo.display );

	white = XWhitePixel( xInfo.display, xInfo.screen );
	black = XBlackPixel( xInfo.display, xInfo.screen );

	hints.x = 100;
	hints.y = 100;
	hints.width = 800;
	hints.height = 600;
	hints.flags = PPosition | PSize;

	xInfo.window = XCreateSimpleWindow( 
		xInfo.display,				// display where window appears
		DefaultRootWindow( xInfo.display ), // window's parent in window tree
		hints.x, hints.y,			// upper left corner location
		hints.width, hints.height,	// size of the window
		Border,						// width of window's border
		black,						// window border colour
		white );					// window background colour
		
	XSetStandardProperties(
		xInfo.display,		// display containing the window
		xInfo.window,		// window whose properties are set
		"Snake",		// window's title
		"Snake",			// icon's title
		None,				// pixmap for the icon
		argv, argc,			// applications command line args
		&hints );			// size hints for the window

	/* 
	 * Create Graphics Contexts
	 */
	int i = 0;
	xInfo.gc[i] = XCreateGC(xInfo.display, xInfo.window, 0, 0);
	XSetForeground(xInfo.display, xInfo.gc[i], BlackPixel(xInfo.display, xInfo.screen));
	XSetBackground(xInfo.display, xInfo.gc[i], WhitePixel(xInfo.display, xInfo.screen));
	XSetFillStyle(xInfo.display, xInfo.gc[i], FillSolid);
	XSetLineAttributes(xInfo.display, xInfo.gc[i],
	                     1, LineSolid, CapButt, JoinRound);

	// setting double buffer
	int depth = DefaultDepth(xInfo.display, DefaultScreen(xInfo.display));
	xInfo.pixmap = XCreatePixmap(xInfo.display, xInfo.window, hints.width, hints.height, depth);
	xInfo.width = hints.width;
	xInfo.height = hints.height;
	xInfo.key = 'R'; // initialize snake head's initial moving direction.

	// Set up colours.
	XColor xcolour;
	Colormap cmap;
	char color_vals[10][10]={"white", "black", "red", "green", "blue", "yellow", "cyan", "magenta", "orange", "brown"};
							
	cmap=DefaultColormap(xInfo.display,DefaultScreen(xInfo.display));
	for(int i=0; i < 10; ++i) {
	  XParseColor(xInfo.display,cmap,color_vals[i],&xcolour);
	  XAllocColor(xInfo.display,cmap,&xcolour);
	  colours[i]=xcolour.pixel;
	}

	XSelectInput(xInfo.display, xInfo.window, 
		ButtonPressMask | KeyPressMask | 
		PointerMotionMask | 
		EnterWindowMask | LeaveWindowMask |
		StructureNotifyMask);  // for resize events

	/*
	 * Don't paint the background -- reduce flickering
	 */
	XSetWindowBackgroundPixmap(xInfo.display, xInfo.window, None);

	/*
	 * Put the window on the screen.
	 */
	XMapRaised( xInfo.display, xInfo.window );
	XFlush(xInfo.display);
}

/******************************* Drawing functions ***********************************

     Purpose: Functions used in drawing images. 

**************************************************************************************/

// draw a renctangle on the given coordinate with using the given widthh, height and colour.
void fillRectangle(XInfo &xinfo, int x, int y, int width, int height, int colour) {
  XSetForeground(xinfo.display, xinfo.gc[0], colours[colour]);
  XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], x, y, width, height);
  XSetForeground(xinfo.display, xinfo.gc[0], colours[White]);
} // fillRectangle

// draw a rectangle that has borders with the given thickness.
// line: colour of border, bg: background colour.
void paintRectangle(XInfo &xinfo, int x, int y, int width, int height, int thick, int line, int bg) {
	fillRectangle(xinfo,x,y,thick,height,line); // left side
	fillRectangle(xinfo,x,y,width,thick,line); // top side
	fillRectangle(xinfo,x+width - thick,y,thick,height,line); // right side
	fillRectangle(xinfo,x,y+height,width,thick,line); // bottom side
	fillRectangle(xinfo,x+thick,y+thick,width - 2*thick,height - thick,bg); // window background
} // paintRectangle

// draw a string with the given colour and size.
void drawString(XInfo &xinfo, int x, int y, string msg, int colour, int size) {
  XSetForeground(xinfo.display,xinfo.gc[0],colours[colour]);
  //set default font
  Font f = XLoadFont(xinfo.display, "6x13");
  // Font f = XLoadFont(d, "-*-helvetica-bold-r-normal--*-240-*-*-*-*-*");
  ostringstream name;
  name << "-*-helvetica-bold-r-*-*-*-240-" << width/size << "-" << height/size << "-*-*-*-*";

  XFontStruct * fStruct = XLoadQueryFont(xinfo.display, name.str().c_str());

  if (fStruct) { //font was found, replace default
    f = fStruct->fid;
  }

  XTextItem ti;
  ti.chars = const_cast<char*>(msg.c_str());
  ti.nchars = msg.length();
  ti.delta = 0;
//  ti.font = f->fid;
  ti.font = f;
  XDrawText(xinfo.display, xinfo.pixmap,xinfo.gc[0], x, y, &ti, 1);
  XSetForeground(xinfo.display, xinfo.gc[0], colours[Black]);
  XFreeFont(xinfo.display,fStruct); // free XFontStruct
} // drawString

// draw strings for a splash screen when staring game.
void splashScreen(XInfo &xinfo) {
	int stringSize = 12; // string size.
	drawString(xinfo,20,20,stars,Blue,stringSize);
	drawString(xinfo,20,40,"",Blue,stringSize);
	drawString(xinfo,20,60,title,Blue,stringSize);
	drawString(xinfo,20,80,"",Blue,stringSize);
	drawString(xinfo,20,100,stars,Blue,stringSize);
	drawString(xinfo,20,120,"",Blue,stringSize);
	drawString(xinfo,20,140,toStart,Blue,stringSize);
	drawString(xinfo,20,160,"",Blue,stringSize);
	drawString(xinfo,20,180,howToPlay1,Blue,stringSize);
	drawString(xinfo,20,200,"",Blue,stringSize);
	drawString(xinfo,20,220,howToPlay2,Blue,stringSize);
	drawString(xinfo,20,240,"",Blue,stringSize);
	drawString(xinfo,20,260,howToPlay3,Blue,stringSize);
	drawString(xinfo,20,280,"",Blue,stringSize);
	drawString(xinfo,20,300,howToPlay4,Blue,stringSize);
	drawString(xinfo,20,320,"",Blue,stringSize);
	drawString(xinfo,20,340,howToPlay5,Blue,stringSize);
	drawString(xinfo,20,360,"",Blue,stringSize);
	drawString(xinfo,20,380,howToPlay6,Blue,stringSize);
	drawString(xinfo,20,400,"",Blue,stringSize);
	drawString(xinfo,20,420,howToPlay7,Blue,stringSize);
	drawString(xinfo,20,440,"",Blue,stringSize);
	drawString(xinfo,20,460,howToPlay8,Blue,stringSize);
	drawString(xinfo,20,480,"",Blue,stringSize);
	drawString(xinfo,20,500,howToPlay9,Blue,stringSize);
	drawString(xinfo,20,520,"",Blue,stringSize);
	drawString(xinfo,20,540,keyboard1,Blue,stringSize);
	drawString(xinfo,20,560,"",Blue,stringSize);
	drawString(xinfo,20,580,keyboard2,Blue,stringSize);
} // splashScreen

// draw strings for a splash screen when restaring game.
void confirmRestart(XInfo &xinfo) {
	const string asking = "Do you want to restart game?";
	const string answer = "Click: Y / N";
	drawString(xinfo,160,220,asking,Blue,8);
	drawString(xinfo,150,240," ",Blue,8);
	drawString(xinfo,150,260," ",Blue,8);
	drawString(xinfo,290,280,answer,Blue,8);
} // confirmRestart

/*
 * Function to repaint a display list
 */
// big black rectangle to draw background
	// lett-top = (5,5)
	// right-top = (795,5)
	// left-bottom = (5,555)
	// right-bottom = (795,555)

void repaint( XInfo &xinfo) {
	list<Displayable *>::const_iterator begin = dList.begin();
	list<Displayable *>::const_iterator end = dList.end();

	XClearWindow( xinfo.display, xinfo.window );

	// get height and width of window (might have changed since last repaint)

	XWindowAttributes windowInfo;
	XGetWindowAttributes(xinfo.display, xinfo.window, &windowInfo);
	unsigned int height = windowInfo.height;
	unsigned int width = windowInfo.width;

	// oriX & oriY: origin coordinate inside borders of background window.
	// numSpace: number of " ".
	int oriX = 5, oriY = 5, numSpace = 6, len = toString(Score).length();
	string space = "";
	for (int i = 0; i < numSpace - len; i++) space += " ";

	string gameInfo = "Score: " + toString(Score) + space + "FPS: " + toString(FPS) + space +
					  "Speed: " + toString(SpeedRate);

	// draw background with white colour
	XFillRectangle(xinfo.display, xinfo.pixmap, xinfo.gc[0], 0, 0, xinfo.width, xinfo.height);
	if (GameState == Initial) { // splash screen when starting game
		splashScreen(xinfo);
	} else if (GameState == Play) {

		int colour = Yellow; // background colour
		// background colour changes to cyan when snake eats invincible potion.
		if (itemState == Invin) colour = Cyan; 
		// background colour changes to magenta when snake eats double score potion.
		else if (itemState == Dscore) colour = Magenta; 
		// background colour changes to orange when the effect of potion remains 10 seconds.
		if (itemState != Regular && counting == TenSec) colour = Orange;

		// draw background with black borders and yellow inside.
		paintRectangle(xinfo,0,0,800,550,5,Black,colour);
		// draw strings of score, FPS, Speed on the screeen.
		drawString(xinfo,8,590,gameInfo,Blue,5); 

		// draw display list
		while( begin != end ) {
			Displayable *d = *begin;
			d->paint(xinfo);
			begin++;
		} // while
	} else if (GameState == Restart) { // restart screen when 'r' key is pressed.
		confirmRestart(xinfo);
	} // if
	
	// copy buffer to window
	XCopyArea(xinfo.display, xinfo.pixmap, xinfo.window, xinfo.gc[0], 
		0, 0, 800,600,  // region of pixmap to copy    555
		0, 0); // position to put top left corner of pixmap in window
	XFlush( xinfo.display );
} // repaint

void handleKeyPress(XInfo &xinfo, XEvent &event) {
	KeySym key;
	char text[BufferSize];
	int left = 65361, up = 65362, right = 65363, down = 65364, start = 115;
	
	/*
	 * Exit when 'q' is typed.
	 * This is a simplified approach that does NOT use localization.
	 */
	int i = XLookupString( 
		(XKeyEvent *)&event, 	// the keyboard event
		text, 					// buffer when text will be written
		BufferSize, 			// size of the text buffer
		&key, 					// workstation-independent key symbol
		NULL );					// pointer to a composeStatus structure (unused)
	if ( i == 1) {
		printf("Got key press -- %c\n", text[0]);
		if (text[0] == 'q') {
			Snake::cleanUp();
			//error("Terminating normally.");
			exit(0);
		} else if (text[0] == 'p') { // pause and resume playing game
			if (GameState == Play) GameState = Pause; // pause game 
			else GameState = Play; // resume game
		} else if (text[0] == 's' && GameState == Initial) { // start game from initial screen.
			GameState = Play;
		} else if (text[0] == 'r' && GameState == Play) { // show restart plash screen.
			GameState = Restart;
		} else if (text[0] == 'y' && GameState == Restart) { // restart game from restart plash screen.
			GameState = Yes;
		} else if (text[0] == 'n' && GameState == Restart) { // return to game play.
			GameState = No;
		}
	} // if

	// see which key was pressed
	if (key == left) {
		// prevent snake from turning 180 degree.
		if (xinfo.key == 'U' || xinfo.key == 'D') xinfo.key = 'L';
	} else if (key == up) {
		// prevent snake from turning 180 degree.
		if (xinfo.key == 'L' || xinfo.key == 'R') xinfo.key = 'U';
	} else if (key == right) {
		// prevent snake from turning 180 degree.
		if (xinfo.key == 'U' || xinfo.key == 'D') xinfo.key = 'R';
	} else if (key == down) {
		// prevent snake from turning 180 degree.
		if (xinfo.key == 'L' || xinfo.key == 'R') xinfo.key = 'D';
	} // if
} // handleKeyPress

void handleAnimation(XInfo &xinfo, int inside) {
	if (GameState == Play) {
		snake.move(xinfo);
		// checnk if snake ate fruit.
		if (fruit.IsEaten(snake.getX(),snake.getY(),'R')) { // 'R' indicate fruit/
			snake.didEatFruit(xinfo); // score up and regerate fruit.
			// update new locatino of fruit to invincible and double score objects
			// to prevent them from generate in the same area with the fruit.
			invincible.setFruitPosition(fruit.getX(),fruit.getY()); 
			dScore.setFruitPosition(fruit.getX(),fruit.getY());
		} // if

		if (item) { // when invincible or double score potions is generated.
			if (dScore.IsEaten(snake.getX(),snake.getY(),'D')) { // snake ate double score potion
				itemState = Dscore;
				item = false;
				// delete image of this potion from the screen
				dScore.setX(0);
				dScore.setY(0);
			} else if (invincible.IsEaten(snake.getX(),snake.getY(),'I')) { // ate invincible potion
				itemState = Invin;
				item = false;
				// delete image of this potion from the screen
				invincible.setX(0);
				invincible.setY(0);
			} // if
		} else if (!item && itemState == Regular) { // no potion is not generated on the screen.
			// need to generate any potion by probability set below.
			unsigned long ran = now() % 1000; // generate random number and divide by any mode.
											  // the random number will be between 0 - 999, which
											  // means the probability becom low as mode increase. 
//cerr << "ran: " << ran << endl;
			if (ran == 13) { // generate invincible potion.
				item = true;
				invincible.getNewFruit();
				// let fruit know where this potino is placed to avoid colision
				fruit.setFruitPosition(invincible.getX(),invincible.getY()); 
				countStart = true;
			} else if (ran == 78) { // generate double score potion.
				item = true;
				dScore.getNewFruit();
				// let fruit know where this potino is placed to avoid colision
				fruit.setFruitPosition(dScore.getX(),dScore.getY());
				countStart = true;
			} // if
		} // if

		// Potino effect lasts for 30 seconds. Conut 30 seconds when any potion is eaten.
		if (itemState == Invin || itemState == Dscore) timer();
	} // if
} // handleAnimation

// get microseconds
unsigned long now() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000 + tv.tv_usec;
} // now

// timer count 30 seconds and return true once counting is done.
bool timer() {
	bool timeUp = false;
	using frame_period = std::chrono::duration<long long, std::ratio<1, 30>>;
	if (countStart) { // start counting.
		before = std::chrono::high_resolution_clock::now(); // time that counting is started.
		current = before;
		countStart = false;
	} // if

	auto difference = current-before; // the amount of time passed.
	
	if (difference < frame_period(30*30)) { // less than 30 second
	  current = std::chrono::high_resolution_clock::now();
	  if (difference > frame_period(20*30)) { // between 20 and 30 second
	  	counting = TenSec; // change background colour to notice that 10 second left before
	  					   // the effect of the item disappear.
	  } // if
	} else { // time is up. Set flags.
		counting = Zero;
		itemState = Regular;
		timeUp = true;
	} // if                  
	return timeUp; 
} // timer

// reset all game state to restart game.
void handleRestart(XInfo &xinfo) {
	xinfo.key = 'R';
	Snake::cleanUp();
	snake.setX(snakeInitX);
	snake.setY(snakeInitY);
	snake.initSnake();
	fruit.setX(50);
	fruit.setY(50);
	invincible.setX(0);
	invincible.setY(0);
	dScore.setX(0);
	dScore.setY(0);
	invincible.setFruitPosition(50,50); 
	dScore.setFruitPosition(50,50);
	Score = 0;
	GameState = Initial;
	itemState = Regular;
	countStart = false;
	item = false;

} // handleRestart

void eventLoop(XInfo &xinfo) {
	// Add stuff to paint to the display list
	snake.initSnake();
	fruit.setSnake(&snake);
	dList.push_front(&snake);
    dList.push_front(&fruit);
    dList.push_front(&invincible);
    dList.push_front(&dScore);
	
	XEvent event;
	unsigned long lastRepaint = 0;
	int inside = 0;

	while( true ) {
		/*
		 * This is NOT a performant event loop!  
		 * It needs help!
		 */
		if (XPending(xinfo.display) > 0) {
			XNextEvent( xinfo.display, &event );
			//cout << "event.type=" << event.type << "\n";
			switch( event.type ) {
				case KeyPress:
					handleKeyPress(xinfo, event);
					break;
				case EnterNotify:
					inside = 1;
					break;
				case LeaveNotify:
					inside = 0;
					break;
			} // switch

			if (GameState == Yes) {
				handleRestart(xinfo);
			} else if (GameState == No) {
				GameState = Play;
			} // if

			if (GameState == Play || GameState == Initial || GameState == Restart) { // update only when game state is not pause.
				usleep(1000000/FPS);
				handleAnimation(xinfo, inside);
				if (XPending(xinfo.display) == 0) repaint(xinfo); // wait until window is ready.
			} // if
		} // if
	} // which
} // eventLoop


/*********** InputValidation *********************************************************

     Purpose: validate command-line input

**************************************************************************************/
class InputValidation {
	void validate(int argc, char *argv[]) {
		if (argc == 3) {
			FPS = toInt(argv[1]);
			SpeedRate = toInt(argv[2]);
		} // if

		else if (argc != 1) {
			throw(string("USAGE: ./snake [frame_rate] [snake_speed]\n"
				"Rage: frame_rate: 1 - 100\nsnake_speed: 1 - 10"));
		} // if
	} // validate

	// convert command-line input to int. If the input is not number, throw error.
	int toInt(char *argv) {
		string cmd = argv;
		stringstream ss(cmd);
		int num = 0;
		if (!(ss >> num)) throw(error);
		return num;
	} // toInt
public:
	InputValidation(int argc, char *argv[]) {
		validate(argc,argv);
	}
}; // InputValidation



/************************************** Main ******************************************

     Purpose: used to control Xserver side. 

**************************************************************************************/
/*
 * Start executing here.
 *	 First initialize window.
 *	 Next loop responding to events.
 *	 Exit forcing window manager to clean up - cheesy, but easy.
 */
int main ( int argc, char *argv[] ) {
	XInfo xInfo;
	try {
		InputValidation input(argc,argv); // validate command-line input
		initX(argc, argv, xInfo);
		eventLoop(xInfo);
		XCloseDisplay(xInfo.display);
		Snake::cleanUp();
	} catch(string &error) {
		cerr << error << endl;
		Snake::cleanUp();
	} // catch
} // main



