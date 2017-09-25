#ifndef __WINDOW_H__
#define __WINDOW_H__

template<unsigned int LEN>
    struct Window
{
    static const unsigned int v[LEN];
    int operator[](int index) const { return v[index]; }
};

#endif

