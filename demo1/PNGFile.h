#ifndef __PNGFILE__
#define __PNGFILE__

#include <Arduino.h>
#include <zlib.h>
#include <png.h>

extern const unsigned char linearTable[256];

class PNGFile
{
public:
    int width, height;

    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep * row_pointers;
    png_bytep getRow(int index) { return row_pointers[index]; }

#define COLOR_BYTES 2
#define COLOR_WEIGHT_R 5
#define COLOR_WEIGHT_G 5
#define COLOR_WEIGHT_B 5

#define COLOR_SHIFT_R (COLOR_WEIGHT_B+COLOR_WEIGHT_G)
#define COLOR_SHIFT_G (COLOR_WEIGHT_B)
#define COLOR_SHIFT_B 0


    unsigned short *imgbuf;
public:

    int read(const char*name)
    {
        unsigned char header[8];
        int y;

        FILE *f = fopen(name, "rb");

        if (!f) {
            perror("fopen");
            return -1;
        }

        fread(header, 1, 8, f);

        if (png_sig_cmp(header, 0, 8)) {
            fprintf(stderr,"Invalid PNG file");
            fclose(f);
            return -1;
        }
        fprintf(stderr,"Initialize read struct\r\n");
        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        fprintf(stderr,"Initialized read struct\r\n");

        if (!png_ptr) {
            fprintf(stderr,"Cannot read struct\n");
            return -1;
        }

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            fprintf(stderr,"Cannot create struct\n");
            return -1;
        }

        png_init_io(png_ptr, f);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
            row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);
        printf("All read\r\n");
        printf("Image information:\r\n\r\n");
        printf("Width:     %d\r\n", width);
        printf("Height:    %d\r\n", height);
        printf("ColorType: %d\r\n", color_type);
        printf("Bit Depth: %d\r\n", bit_depth);
        fclose(f);


#if 0
        printf("Converting to packed pixel, please wait....");

        int x;
        unsigned short v;

        for (y=0; y<height; y++) {
            png_byte* row = row_pointers[y];
            png_byte* dptr = row_pointers[y];
            for (x=0; x<width; x++) {
                unsigned red = *row++;
                unsigned green = *row++;
                unsigned blue = *row++;

                /* weight each of the RGB. We dont support 8 BPP */

                red   <<=2; red   &= (0x1f<<5);
                green <<=7; green &= (0x1f<<10);
                blue >>=3;
                blue+=green;
                blue+=red;

                /* Pack it into the destination */
                *dptr++ = blue>>8;
                *dptr++ = blue;
            }
        }

        printf("Conversion done\n");
#endif
        return 0;
    }

    void copyToArea(int sx, int sy, int w, int h, uint16_t *dest, int stride)
    {
        while (h--) {
            /* Set up the row pointer */
            uint16_t *rp = (uint16_t*)row_pointers[sy] + sx;
            int z = w;
            while (z--) {
                *dest++=*rp++;
            }
            /* Move dest forward, to next line */
            dest+=stride-w;
            sy++;
        }
    }
    void copyToArea32(int sx, int sy, int w, int h, uint32_t *dest, int stride)
    {
        while (h--) {
            /* Set up the row pointer */
            uint32_t *rp = (uint32_t*)row_pointers[sy] + sx;
            int z = w;
            while (z--) {
                *dest++=*rp++;
            }
            /* Move dest forward, to next line */
            dest+=stride-w;
            sy++;
        }
    }

    
    void put32(uint32_t *dest, int stride)
    {
        unsigned x,y;
        uint32_t *d = dest;
        uint32_t *sd;
        for (y=0; y<height; y++) {
            png_byte* row = row_pointers[y];
            sd=d;

            for (x=0; x<width; x++) {
                unsigned red = linearTable[*row++];
                unsigned green = linearTable[*row++];
                unsigned blue = linearTable[*row++];

                unsigned val = (red<<16) + (green<<8) + blue;

                *d++=val;
            }

            d=sd+stride;
        }

    }


};

#endif
