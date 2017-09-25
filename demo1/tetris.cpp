#include <Arduino.h>

#include "tetris.h"
#include <stdlib.h>
#include <string.h>
#include "font.h"

#ifdef __linux__
#define DEBUG(x...)  printf(x)
#else
#define DEBUG(x...)
#endif

#define ICACHE_FLASH_ATTR

static int px = -1; /* Piece current X position */
static int py = 0; /* Piece current Y position */
static int tick = 0;
static int tickmax = 30;
static unsigned score=0; /* Our score */
//static unsigned level=0; /* Current level */
static unsigned lines=0; /* Number of lines made (all levels) */
//static unsigned currentlevel_lines=0; /* This level number of lines made */
static unsigned lines_total=0;

static uint8_t lval[4], cval[4];

/* This will hold the current game area */
static unsigned char area[BLOCKS_X][BLOCKS_Y];

/* Current piece falling */
static struct piece *currentpiece;
static uint8_t currentrotate;

/* Next piece */
static struct piece *nextpiece;

/* Whether the current piece is valid, or if we need to allocate a new one */
static bool currentpiecevalid=false;

#define LINEDONE_COUNTER_MAX 30
#define LINEDONE_SELECTOR (1<<2)

static unsigned linedone_counter;

const int OPERATION_CLEAR=0;
const int OPERATION_DRAW=1;

typedef uint32_t color_t;

typedef void (*loopfunc)(gfxinfo_t*);

static void ICACHE_FLASH_ATTR draw_block(gfxinfo_t *gfx, int x, int y, color_t color);

static void ICACHE_FLASH_ATTR draw_block_small(gfxinfo_t *gfx, int x, int y, color_t color);

//typedef Font font_t;

static const font_t *font = NULL;

static int8_t completed_lines[5]; // Never more than 4. Extra placeholder for end.
static int8_t lines_made;


static color_t colors[] = {
    0x00000000,
    0x0000FF00, // Green
    0x00FF00FF, // Purple
    0x00FFFF00, // Yellow
    0x00FFA500, // Orange
    0x0000FFFF, // Cyan
    0x000000FF, // Blue
    0x00FF0000, // RED
};


enum {
    PALETTE,
    START,
    PLAY,
    LINEDONE
} game_state = PALETTE;

extern int isDown();
extern int isLeft();
extern int isRight();
extern int isRotate();

static void ICACHE_FLASH_ATTR draw_hline(gfxinfo_t*gfx, int x, int y, int w, color_t color);


static int ICACHE_FLASH_ATTR checkPress(uint8_t current, uint8_t *latest, uint8_t delay, uint8_t repeat)
{
    if (current==0) {
        (*latest) = 0;
        return 0;
    }
    // Pressed.
    if ((*latest)==0) {
        *latest = *latest + 1;
        return 1; // Press event.
    }
    delay++;

    // Now, check for delay
    if (*latest<delay) {
        (*latest)++;
        return 0;
    }
    // Delayed now, check repeat.
    if (*latest==delay) {
        (*latest)++;
        return 1;
    }
    // Increase by repeat
    (*latest)++;

    if ((*latest)>(delay+repeat)) {
        (*latest)=delay;
    }
    return 0;
}

enum event_t ICACHE_FLASH_ATTR hasEvent()
{
    enum event_t ret = event_none;

    cval[0] = isDown();
    cval[1] = isRotate();
    cval[2] = isLeft();
    cval[3] = isRight();

    do {
        if (checkPress(cval[0], &lval[0], /* Delay */ 1, /* Repeat */ 2)) {
            ret = event_down;
            break;
        }
        if (checkPress(cval[1], &lval[1], /* Delay */ 10, /* Repeat */ 10)) {
            ret = event_rotate;
            break;
        }
        if (checkPress(cval[2], &lval[2], /* Delay */ 5, /* Repeat */ 5)) {
            ret = event_left;
            break;
        }
        if (checkPress(cval[3], &lval[3], /* Delay */ 5, /* Repeat */ 5)) {
            ret=  event_right;
            break;
        }
    } while (0);

#if 0
    lval[0] = cval[0];
    lval[1] = cval[1];
    lval[2] = cval[2];
    lval[3] = cval[3];
#endif

    return ret;
}

#include "pieces.h"

/* Random function (sorta) */
#ifndef __linux__
static unsigned xrand() {
    // TODO
    return rand();
}
#else
#define xrand rand
#endif

static struct piece * ICACHE_FLASH_ATTR getRandomPiece()
{
    int i = xrand() % (sizeof(allpieces)/sizeof(struct piece));
    DEBUG("Returning piece %i\n",i);
    return &allpieces[i];
}

static void ICACHE_FLASH_ATTR area_init()
{
    memset(area,0,sizeof(area));
}

static bool  ICACHE_FLASH_ATTR can_place(int x, int y, struct piece *p, int rotate)
{
    int i,j;
    for (i=0;i<p->size;i++)
        for (j=0;j<p->size;j++) {
            if ((*p->layout[rotate])[j][i]) {
                if ( (x+i) >= BLOCKS_X || (x+i) <0 ) {
                    DEBUG("X overflow %d, %d\n",x,i);
                    return false;
                }
                if ( (y+j) >= BLOCKS_Y || (x+y) <0 ) {
                    DEBUG("Y overflow %d, %d\n",y,j);
                    return false;
                }

                if (area[x+i][y+j]) {
                    DEBUG("Collision at %d %d\n", x+i, y+j);
                    return false;
                } else {
                    //DEBUG("Placement OK at %d %d\n", x+i, y+j);
                }
            }
        }
    return true;
}

static void ICACHE_FLASH_ATTR do_place(int x, int y, int piece_size, piecedef *p)
{
    int i,j;
    for (i=0;i<piece_size;i++) {
        for (j=0;j<piece_size;j++) {
            if ((*p)[j][i]) {
                DEBUG("Marking %d %d -> %d\n", x+i,y+j, (*p)[j][i]);
                area[x+i][y+j] = (*p)[j][i];
            }
        }
    }
}

typedef void (*draw_block_func)(gfxinfo_t*, int, int, color_t);

static void ICACHE_FLASH_ATTR draw_piece_impl(gfxinfo_t *gfx,
                                              int x,
                                              int y,
                                              int piece_size,
                                              piecedef *p,
                                              int operation,
                                              int size,
                                              draw_block_func fn)
{
    int i,j,ax,ay;
    ay = board_y0 + y*size;

    for (i=0;i<piece_size;i++) {
        ax= board_x0 + x*size;

        for (j=0;j<piece_size;j++) {
            if ((*p)[i][j]) {
                if (operation==OPERATION_CLEAR) {
                    fn(gfx , ax, ay, 0);
                } else {
                    fn( gfx, ax, ay, (*p)[i][j]);
                }
            }
            ax+=size;
        }
        ay+=size;
    }
}

static void ICACHE_FLASH_ATTR draw_piece(gfxinfo_t*gfx, int x, int y, int piecesize, piecedef *p, int operation)
{
    draw_piece_impl(gfx, x, y, piecesize, p, operation, BLOCKSIZE, &draw_block );
}


static void ICACHE_FLASH_ATTR update_score(gfxinfo_t *gfx)
{
#if 1
    char tscore[16];

    textrendersettings_t render;

    if (NULL==font)
        font = font_find("thumb");

    if (NULL==font)
        return;

    render.font = font;
    render.w = 32;
    render.h = 7;
    render.align = ALIGN_LEFT;
    render.wrap = 0;
    render.direction = T_ROTATE;
    sprintf(tscore,"%d",score);
    drawText(gfx,&render, 2, 1, tscore, 0xFFFFFF, 0x00);
#endif

}

static void ICACHE_FLASH_ATTR board_draw(gfxinfo_t *gfx)
{
    int x,y;
#if 0
    gfx->drawFastHLine(0, board_y0-1, gfx->height(), 0xffffff);
#else
    gfx->drawFastVLine(board_y0-1, 0, gfx->height(), 0xffffff);
#endif
    for (x=0;x<BLOCKS_X;x++) {
        for (y=0;y<BLOCKS_Y;y++) {
            if (area[x][y]) {
                draw_block(gfx,
                           board_x0+x*BLOCKSIZE,
                           board_y0+y*BLOCKSIZE,
                           area[x][y]);
            }
        }
    }
}

void ICACHE_FLASH_ATTR setup_game()
{
    currentpiece = NULL;
    nextpiece=getRandomPiece();
    score = 0;
}

static piecedef * ICACHE_FLASH_ATTR get_current_piecedef()
{
    return currentpiece->layout[currentrotate];
}


static void ICACHE_FLASH_ATTR rotatepiece(gfxinfo_t*gfx)
{
    uint8_t nextrotate = (currentrotate+1)&0x3;
    DEBUG("Current rotate %d next %d\n", currentrotate, nextrotate);
    if (can_place(px,py,currentpiece, nextrotate)) {
        draw_piece( gfx,px, py, currentpiece->size, get_current_piecedef(), OPERATION_CLEAR);
        currentrotate = nextrotate;
        draw_piece( gfx,px, py, currentpiece->size, get_current_piecedef(), OPERATION_DRAW);
    } else {
        DEBUG("Cannot rotate\n");
    }
}

static int ICACHE_FLASH_ATTR did_make_line(int start)
{
	int x,y;
	for (y=start; y>=0;y--) {
		int count = 0;

		for (x=0;x<BLOCKS_X;x++) {
			if (area[x][y])
				count++;
		}
		if (count==BLOCKS_X) {
                    return y;
		}
	}
	return -1;
}

static void ICACHE_FLASH_ATTR draw_rect(gfxinfo_t*gfx, int x, int y, int w, int h, color_t color)
{
    int i,j;
    for (i=x; i<x+w; i++) {
        for (j=y;j<y+h;j++) {
            gfx->drawPixel(j,i,color);
        }
    }
}

static void ICACHE_FLASH_ATTR draw_hline(gfxinfo_t*gfx, int x, int y, int w, color_t color)
{
    int i;
    for (i=x; i<x+w; i++) {
        gfx->drawPixel(y,i,color);
    }
}

static void ICACHE_FLASH_ATTR special_effects(gfxinfo_t*gfx)
{
    int8_t *line = &completed_lines[0];
    color_t color;

    while (*line>0) {
        unsigned offsety = board_y0 + ((*line) * BLOCKSIZE);
        if (linedone_counter & LINEDONE_SELECTOR) {
            color = CRGB(0xFF,0xFF,0x00);
        } else {
            color = CRGB(0xFF,0x00,0x00);
        }
        draw_rect( gfx, board_x0, offsety, BLOCKS_X*BLOCKSIZE, BLOCKSIZE, color);
        line++;
    }
}

static void ICACHE_FLASH_ATTR draw_block(gfxinfo_t *gfx, int x, int y, color_t color_index)
{

    color_t color = colors[color_index & 0xf];

#ifdef SMALL_PIECES
    gfx->drawPixel(y, x, color);
    gfx->drawPixel(y+1, x, color);
    gfx->drawPixel(y, x+1, color);
    gfx->drawPixel(y+1, x+1, color);
#else
    gfx->drawPixel(y,   x, color);
    gfx->drawPixel(y+1, x, color);
    gfx->drawPixel(y+2, x, color);
    gfx->drawPixel(y,   x+1, color);
    gfx->drawPixel(y+1, x+1, color);
    gfx->drawPixel(y+2, x+1, color);
    gfx->drawPixel(y,   x+2, color);
    gfx->drawPixel(y+1, x+2, color);
    gfx->drawPixel(y+2, x+2, color);

#endif

}

static void ICACHE_FLASH_ATTR draw_block_small(gfxinfo_t *gfx, int x, int y, color_t color_index)
{
    color_t color = colors[color_index & 0xf];
#ifdef SMALL_PIECES
    gfx->drawPixel(y, x, color);
#else
    gfx->drawPixel(y,   x, color);
    gfx->drawPixel(y+1, x, color);
    gfx->drawPixel(y,   x+1, color);
    gfx->drawPixel(y+1, x+1, color);
#endif
}


static void ICACHE_FLASH_ATTR draw_table(gfxinfo_t *gfx)
{
    board_draw(gfx);
    update_score(gfx);
#if 0
    int x, y;
    for (x=0; x<BLOCKS_X; x++) {

        for (y=0; y<BLOCKS_X; y++) {

            int piece = area[x][y];

            if (piece==0)
                continue;

            draw_block( gfx, x, y, piece);

        }
    }
#endif
}

static void ICACHE_FLASH_ATTR checklines()
{
    int y = BLOCKS_Y;
    int s = 30;
    int l = 0;
    while ((y=did_make_line(y))!=-1) {
        score+=s;
        s=s*2;
        //line_done(y);
        completed_lines[l] = y;
        DEBUG("Line completed at %d\n",y);
        l++;
        y--;
        if (y<0)
            break;
    }
    completed_lines[l] = -1;
    lines_made = l;

    if (l>0) {
        DEBUG("Lines completed: %d",l);
        linedone_counter = LINEDONE_COUNTER_MAX;
        game_state = LINEDONE;
    }
}

static void ICACHE_FLASH_ATTR draw_next_piece(gfxinfo_t *gfx)
{
    draw_piece_impl(gfx, 11, -3, nextpiece->size, nextpiece->layout[0], OPERATION_DRAW, 2, &draw_block_small);
}

static void ICACHE_FLASH_ATTR processEvent( gfxinfo_t *gfx, enum event_t ev )
{
    int nextx,nexty;
    bool checkcollision;

    checkcollision=false;

    nextx = px;
    nexty = py;

    if (ev==event_rotate) {
        rotatepiece(gfx);
    }
    if (ev==event_left) {
        nextx=px-1;
        DEBUG("left\n");
    }
    if (ev==event_right) {
        nextx=px+1;
        DEBUG("right\n");
    }
    if (ev==event_down || tick==tickmax) {
        nexty=py+1;
        DEBUG("down\n");
        checkcollision=true;
    }

    if (can_place(nextx,nexty, currentpiece, currentrotate)) {
        draw_piece( gfx,px, py, currentpiece->size, get_current_piecedef(), OPERATION_CLEAR);
        py=nexty;
        px=nextx;
        draw_piece( gfx,px, py, currentpiece->size, get_current_piecedef(), OPERATION_DRAW);
    } else {
        if (checkcollision) {
            DEBUG("Piece is no longer\n");
            score+=7;
            do_place(px,py, currentpiece->size, get_current_piecedef());
            checklines();
            currentpiecevalid=false;
            py=0;
        }
    }

}

static void ICACHE_FLASH_ATTR end_of_game(gfxinfo_t*gfx)
{
    DEBUG("End of game\n");
    game_state = START;
}

static void ICACHE_FLASH_ATTR pre_game_play(gfxinfo_t*gfx)
{
    currentpiecevalid=false;

    area_init();
    board_draw(gfx);
    DEBUG("Pre game\n");
    score=0;
    lines=0;
    lines_total=0;
}

static void ICACHE_FLASH_ATTR line_done(int y)
{
    int x,py;

    DEBUG("Line done at %d\n",y);
    // Shift down area
    for (py=y;py>=0;py--) {
        for (x=0;x<BLOCKS_X;x++) {
            area[x][py] = py>0 ? area[x][py-1] : 0; // Clear
        }
    }
}

static void ICACHE_FLASH_ATTR game_linedone(gfxinfo_t*gfx)
{
    DEBUG("Doing SFX %u\n", linedone_counter);
    special_effects(gfx);
    linedone_counter--;
    if (linedone_counter==0) {
        int y = BLOCKS_Y;
        while ((y=did_make_line(y))!=-1) {
            line_done(y);
        }
        game_state = PLAY;
    }
}

static void ICACHE_FLASH_ATTR game_start(gfxinfo_t*gfx)
{
    enum event_t ev = hasEvent();
    if (1 || ev==event_rotate) {
        DEBUG("Starting game\n");
        game_state = PLAY;
        pre_game_play(gfx);
    }
}

static void ICACHE_FLASH_ATTR game_play(gfxinfo_t*gfx)
{
    if (!currentpiecevalid) {

        py=0;

        currentpiece=nextpiece;
        currentrotate=0;

        nextpiece=getRandomPiece();
        currentpiecevalid=true;

        px=(BLOCKS_X/2)-2 + nextpiece->x_offset;

        if (!can_place(px,py,currentpiece,currentrotate)) {
            // End of game
            end_of_game(gfx);
            return;
        }
        draw_piece( gfx, px, py, currentpiece->size, get_current_piecedef(), OPERATION_DRAW);
    } else {
        draw_piece( gfx, px, py, currentpiece->size, get_current_piecedef(), OPERATION_DRAW);
    }

    draw_next_piece(gfx);

    enum event_t ev = hasEvent();
    tick++;

    if (tick==tickmax) {
        tick=0;
        processEvent(gfx,event_down);
    } else {

        if (ev!=event_none) {
            processEvent(gfx,ev);
        }
    }
}

static void draw_pallete(gfxinfo_t*gfx)
{
    int x, y;
    color_t color = 0;
    for (x=0; x<32;x+=2) {
        for (y=0; y<32; y+=2) {
            gfx->drawPixel( board_y0+y,  x, color);
            gfx->drawPixel( board_y0+y+1, x, color);
            gfx->drawPixel( board_y0+y,   x+1, color);
            gfx->drawPixel( board_y0+y+1,x+1, color);
            color++;
        }
    }
    enum event_t ev = hasEvent();
    if (1 || (ev != event_none)) {
        game_state = START;
    }
}


static loopfunc loop_functions[] =
{
    &draw_pallete,
    &game_start,
    &game_play,
    &game_linedone
};

void game_loop(gfxinfo_t*gfx)
{
    //gfx->clear();
    draw_table(gfx);
    loop_functions[game_state](gfx);
}
