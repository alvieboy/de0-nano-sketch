#ifndef __COLORDEFS_H__
#define __COLORDEFS_H__

#define CRGB(r,g,b) ( ( (r) & 0x7 ) + ( ((g)<<3) & (0x7<<3) ) + ( ((b)<<6) & (0x3<<6) ) )

#endif
