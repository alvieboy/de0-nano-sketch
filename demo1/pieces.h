#include "color_defs.h"

#undef C
#define C 1

static piecedef piece1_1 = {
    {C,C,C,0},
    {0,C,0,0},
    {0,0,0,0},
    {0,0,0,0}
};

static piecedef piece1_2 = {
    {C,0,0,0},
    {C,C,0,0},
    {C,0,0,0},
    {0,0,0,0}
};

static piecedef piece1_3 = {
    {0,C,0,0},
    {C,C,C,0},
    {0,0,0,0},
    {0,0,0,0}
};

static piecedef piece1_4 = {
    {0,C,0,0},
    {C,C,0,0},
    {0,C,0,0},
    {0,0,0,0}
};

#undef C
#define C 2

static piecedef piece2_1 = {
    {C,C,C,0},
    {C,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece2_2 = {
    {C,0,0,0},
    {C,0,0,0},
    {C,C,0,0},
    {0,0,0,0}
};
static piecedef piece2_3 = {
    {0,0,C,0},
    {C,C,C,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece2_4 = {
    {C,C,0,0},
    {0,C,0,0},
    {0,C,0,0},
    {0,0,0,0}
};

#undef C
#define C 3

static piecedef piece3_1 = {
    {C,C,C,0},
    {0,0,C,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece3_2 = {
    {C,C,0,0},
    {C,0,0,0},
    {C,0,0,0},
    {0,0,0,0}
};
static piecedef piece3_3 = {
    {C,0,0,0},
    {C,C,C,0},
    {0,0,0,0},
    {0,0,0,0}
};

static piecedef piece3_4 = {
    {0,C,0,0},
    {0,C,0,0},
    {C,C,0,0},
    {0,0,0,0}
};


#undef C
#define C 4

static piecedef piece4_1 = {
    {C,C,0,0},
    {0,C,C,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece4_2 = {
    {0,C,0,0},
    {C,C,0,0},
    {C,0,0,0},
    {0,0,0,0}
};
static piecedef piece4_3 = {
    {C,C,0,0},
    {0,C,C,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece4_4 = {
    {0,C,0,0},
    {C,C,0,0},
    {C,0,0,0},
    {0,0,0,0}
};

#undef C
#define C 5

static piecedef piece5_1 = {
    {0,C,C,0},
    {C,C,0,0},
    {0,0,0,0},
    {0,0,0,0}
};

static piecedef piece5_2 = {
    {C,0,0,0},
    {C,C,0,0},
    {0,C,0,0},
    {0,0,0,0}
};
static piecedef piece5_3 = {
    {0,C,C,0},
    {C,C,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece5_4 = {
    {C,0,0,0},
    {C,C,0,0},
    {0,C,0,0},
    {0,0,0,0}
};

#undef C
#define C 6

static piecedef piece6 = {
    {C,C,0,0},
    {C,C,0,0},
    {0,0,0,0},
    {0,0,0,0}
};

#undef C
#define C 7

static piecedef piece7_1 = {
    {C,C,C,C},
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0}
};
static piecedef piece7_2 = {
    {0,C,0,0},
    {0,C,0,0},
    {0,C,0,0},
    {0,C,0,0}
};


static struct piece allpieces[] = {
    {
        3, 0, 0, 0,
        {
            &piece1_1,
            &piece1_2,
            &piece1_3,
            &piece1_4
        }
    },
    {
        3, 0, 0, 0,
        {
            &piece2_1,
            &piece2_2,
            &piece2_3,
            &piece2_4
        },
    },
    {
        3, 0, 0, 2,
        {
            &piece3_1,
            &piece3_2,
            &piece3_3,
            &piece3_4
        }
    },
    {
        3, 0, 0, 3,
        {
            &piece4_1,
            &piece4_2,
            &piece4_3,
            &piece4_4
        }
    },
    {
        3, 0, 0, 4,
        {
            &piece5_1,
            &piece5_2,
            &piece5_3,
            &piece5_4
        }
    },
    {
        2, 0, 0, 5,
        {
            &piece6,
            &piece6,
            &piece6,
            &piece6
        }
    },
    {
        4, 0, 0, 6,
        {
            &piece7_1,
            &piece7_2,
            &piece7_1,
            &piece7_2
        }
    },
};

#undef C

