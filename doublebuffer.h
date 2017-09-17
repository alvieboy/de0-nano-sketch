#ifndef __DOUBLEBUFFER_H__
#define __DOUBLEBUFFER_H__

#include "samplingbuffer.h"

template<unsigned int SZ>
    struct DoubleBuffer {
        inline void reset() {
            buffers[0].reset();
            buffers[1].reset();
            currentReadBuffer=0;
            currentWriteBuffer=0;
        }
        inline int isWriteFull() const { return buffers[currentWriteBuffer].isFull(); }
        inline int isReady() const  { return buffers[currentReadBuffer].isFull(); }
        inline const SamplingBuffer<SZ> &getCurrentReadBuffer() const {
            return buffers[currentReadBuffer];
        }
        inline void advanceReadBuffer() {
            buffers[currentReadBuffer].reset();
            currentReadBuffer++;
            if (currentReadBuffer==2)
                currentReadBuffer=0;
        }
        inline void advanceWriteBuffer() {
            currentWriteBuffer++;
            if (currentWriteBuffer==2)
                currentWriteBuffer=0;
        }
        inline void append(int value) {
            SamplingBuffer<SZ> &b = buffers[currentWriteBuffer];
            b.append(value);
            if (b.isFull()) {
                advanceWriteBuffer();
            }
        }
    protected:
        int currentWriteBuffer;
        int currentReadBuffer;
        SamplingBuffer<SZ> buffers[2];
    };

#endif
