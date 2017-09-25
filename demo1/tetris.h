#include <Arduino.h>
#include <Adafruit_GFX.h>

#ifdef SMALLPIECES
// Panel is 64*32. Reserve 12 pix at top.

#define BLOCKS_X 16
#define BLOCKS_Y 26
#define BLOCKSIZE 2
static const int board_x0 = 0;
static const int board_y0 = 12;

#else

#define BLOCKS_X 10
#define BLOCKS_Y 29
#define BLOCKSIZE 3

static const int board_x0 = 1;
static const int board_y0 = 9;


#endif

static const int board_width = (BLOCKS_X * BLOCKSIZE);
static const int board_height = (BLOCKS_Y * BLOCKSIZE);

#define PIECESIZE_MAX 4

enum event_t {
    event_none,
    event_left,
    event_right,
    event_down,
    event_rotate
};

typedef unsigned char piecedef[PIECESIZE_MAX][PIECESIZE_MAX];

struct piece {
    uint8_t size;
    uint8_t x_offset;
    uint8_t y_offset;
    uint8_t rsvd;
    piecedef *layout[4];
};

typedef uint8_t color_type;//[BLOCKSIZE*BLOCKSIZE];

struct level {
    unsigned lines_required;
    unsigned char area[BLOCKS_X][BLOCKS_Y];
};

typedef Adafruit_GFX_32 gfxinfo_t;

void setup_game(void);
void game_loop(gfxinfo_t *gfx);


#define TODO
