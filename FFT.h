#include "fixedpoint.h"

#undef IMAGINARY_IN
#define FIXEDPOINT


#ifdef FIXEDPOINT
#define FPTYPE fp32_16_16
#define ASDOUBLE(x) (x.asDouble())
#else
#define FPTYPE float
#define ASDOUBLE(x) (x)
#endif

template<unsigned int M>
	struct FFT
{
	typedef FPTYPE fixed;
	static const int N = 1<<M;
	static const int N_D2 = N>>1;
	static const int N_M1 = N-1;

	void doFFT();
        void doFFTi();
	void doInverseFFT();
	void reorder();
	static const unsigned int sincostable[];

	fixed in_real[N];
	fixed in_im[N];

        int get_magnitude(int index)
        {
            fixed v = in_real[index];
            //v.v>>=2;
            v *= v;
            fixed u = in_im[index];
            //u.v>>=2;
            u *= u;
            v += u;
            // TODO: use hardware acceleration here, we already have a module
            v.v = fsqrt16(v.asNative());
            /*unsigned v1 = v.v;

            v1 = v1/256;
            if (v1>0xff)
            v1=0xff;
            return v1;
            */
            return v.v;
        }

};

typedef FFT<10> FFT_1024;
typedef FFT<9> FFT_512;
typedef FFT<8> FFT_256;
typedef FFT<7> FFT_128;
typedef FFT<6> FFT_64;
typedef FFT<5> FFT_32;
