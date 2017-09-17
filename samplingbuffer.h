#ifndef __SAMPLINGBUFFER_H__
#define __SAMPLINGBUFFER_H__

#include <Arduino.h>

template<unsigned int SZ>
    struct SamplingBuffer {
        static const unsigned SIZE = SZ;
        unsigned int ptr;
        volatile int done;
        int buffer[SZ];
        inline void reset()    { ptr=0; done=0; }
        inline int canAppend() const { return done==0; }
        inline int isFull() const   { return done; }
        inline int sample(int index) const { return buffer[index]; }
        inline int operator[](int index) const { return buffer[index]; }
        inline const int*getBuffer()const {return &buffer[0]; }
        // append a Q16.
        inline void append(int value) {
            buffer[ptr++]=value;
            if (ptr==SZ) {
                done=1;
                ptr=0;
            }
        }
    };

#endif
