#ifndef __FONT_H__
#define __FONT_H__

#include <Arduino.h>
#include <Adafruit_GFX.h>

typedef unsigned int color_t;

typedef struct {
    uint8_t w;
    uint8_t h;
    uint8_t start;
    uint8_t end;
} fonthdr_t;

typedef struct font {
    fonthdr_t hdr;
    char name[10];
    uint8_t *bitmap;
    uint8_t ref;
    struct font *next;
} font_t;


const font_t *font_find(const char *name);

typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
} alignment_t;

typedef struct {
    const font_t *font;
    int8_t w; // width clip
    int8_t h; // height clip
    alignment_t align;
    uint8_t wrap:1; // Wrap or not text
    uint8_t direction:2; // Rotate and flip
} textrendersettings_t;

#define T_ROTATE 1
#define T_FLIP 2

void drawText(Adafruit_GFX_32  *gfx, const textrendersettings_t *s, int x, int y, const char *str, color_t color, color_t bg);
void font_init();

#endif
