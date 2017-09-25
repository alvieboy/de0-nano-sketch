#ifndef __DECAY_H__
#define __DECAY_H__

#include "fixedpoint.h"

typedef fp32_16_16 fp_t;
static const fp_t accel = 0.5;  // 1/2*a

template<unsigned NUMBER_COLUMNS>
    class Decay
{
public:

    Decay()
    {
        int i;
        for (i=0;i<NUMBER_COLUMNS;i++) {
            lastlevel[i] = 0.0;
            decayindex[i] = 0;
        }
    }


    fp_t lastlevel[NUMBER_COLUMNS];// = {0}; // For decay
    int decayindex[NUMBER_COLUMNS];// = {0};

    fp_t operator[](unsigned index) const {
        return lastlevel[index];
    }



    void level(unsigned index, fp_t fp_level)
    {

        if (fp_level >= lastlevel[index]) {
            lastlevel[index] = fp_level;
            decayindex[index]=0;
        }
        else {
            // Decay.

            // X = Xo + vo(t) + 1/2at^2.

            //lastlevel[index] -= ((decayindex[index]&DECAYM)==DECAYM);
            if (lastlevel[index]>0.0) {
                fp_t time = fp_t(decayindex[index]);
                time = time * time;
                time = time * accel;
                // Get previous in time
                fp_t last;
                if (decayindex[index]>0.0) {
                    last = fp_t(decayindex[index]-1);
                    last = last * last;
                    last = last * accel;
                } else {
                    last = 0.0;
                }
                time -= last;
                lastlevel[index] = lastlevel[index] - time;

                if (lastlevel[index]<0.0)
                    lastlevel[index]=0.0;

                //if (decayindex[index]<MAXDECAY)
                decayindex[index]++;
            }
        }
    }
};
#endif
