#include <string.h>

static unsigned char qrframe[4096];
static unsigned char framask[4096];
static unsigned char width, widbytes;

#define QRBIT(x,y) ( ( qrframe[((x)>>3) + (y) * widbytes] >> (7-((x) & 7 ))) & 1 )
#define SETQRBIT(x,y) qrframe[((x)>>3) + (y) * widbytes] |= 0x80 >> ((x) & 7)
#define SETFXBIT(x,y) framask[((x)>>3) + (y) * widbytes] |= 0x80 >> ((x) & 7)

#define TOGQRBIT(x,y) qrframe[((x)>>3) + (y) * widbytes] ^= 0x80 >> ((x) & 7)
#define FIXEDBIT(x,y) ( ( framask[((x)>>3) + (y) * widbytes] >> (7-((x) & 7 ))) & 1 )

static void putfind()
{
    unsigned char j, i, k, t;
    for (t = 0; t < 3; t++) {
        k = 0;
        i = 0;
        if (t == 1)
            k = (width - 7);
        if (t == 2)
            i = (width - 7);
        SETQRBIT(i + 3, k + 3);
        for (j = 0; j < 6; j++) {
            SETQRBIT(i + j, k);
            SETQRBIT(i, k + j + 1);
            SETQRBIT(i + 6, k + j);
            SETQRBIT(i + j + 1, k + 6);
        }
        // the outer wall makes this mask unnecessary

#ifndef QUICK
        for (j = 1; j < 5; j++) {
            SETFXBIT(i + j, k + 1);
            SETFXBIT(i + 1, k + j + 1);
            SETFXBIT(i + 5, k + j);
            SETFXBIT(i + j + 1, k + 5);
        }
#endif
        for (j = 2; j < 4; j++) {
            SETQRBIT(i + j, k + 2);
            SETQRBIT(i + 2, k + j + 1);
            SETQRBIT(i + 4, k + j);
            SETQRBIT(i + j + 1, k + 4);
        }
    }
}

static void putalign(int x, int y)
{
    int j;

    SETQRBIT(x, y);
    for (j = -2; j < 2; j++) {
        SETQRBIT(x + j, y - 2);
        SETQRBIT(x - 2, y + j + 1);
        SETQRBIT(x + 2, y + j);
        SETQRBIT(x + j + 1, y + 2);
    }
#ifndef QUICK
    for( j = 0 ; j < 2 ; j++ ) {
        SETFXBIT(x-1, y+j);
        SETFXBIT(x+1, y-j);
        SETFXBIT(x-j, y-1);
        SETFXBIT(x+j, y+1);
    }
#endif
}

static const unsigned char adelta[41] = {
    0, 11, 15, 19, 23, 27, 31,  // force 1 pat
    16, 18, 20, 22, 24, 26, 28, 20, 22, 24, 24, 26, 28, 28, 22, 24, 24,
    26, 26, 28, 28, 24, 24, 26, 26, 26, 28, 28, 24, 26, 26, 26, 28, 28,
};

void doaligns(unsigned char vers)
{
    unsigned char delta, x, y;
    if (vers < 2)
        return;
    delta = adelta[vers];
    y = width - 7;
    for (;;) {
        x = width - 7;
        while (x > delta - 3U) {
            putalign(x, y);
            if (x < delta)
                break;
            x -= delta;
        }
        if (y <= delta + 9U)
            break;
        y -= delta;
        putalign(6, y);
        putalign(y, 6);
    }
}

static const unsigned vpat[] = {
    0xc94, 0x5bc, 0xa99, 0x4d3, 0xbf6, 0x762, 0x847, 0x60d,
    0x928, 0xb78, 0x45d, 0xa17, 0x532, 0x9a6, 0x683, 0x8c9,
    0x7ec, 0xec4, 0x1e1, 0xfab, 0x08e, 0xc1a, 0x33f, 0xd75,
    0x250, 0x9d5, 0x6f0, 0x8ba, 0x79f, 0xb0b, 0x42e, 0xa64,
    0x541, 0xc69
};

static void putvpat(unsigned char vers)
{
    unsigned char x, y, *p, bc;
    unsigned verinfo;
    if (vers < 7)
        return;
    verinfo = vpat[vers - 7];

    p = qrframe + width * (width - 11);
    bc = 17;
    for (x = 0; x < 6; x++)
        for (y = 0; y < 3; y++, bc--)
            if (1&(bc > 11 ? vers >> (bc - 12) : verinfo >> bc))
                SETQRBIT( x,y+width-11);
            else
                SETFXBIT( x,y+width-11);
    bc = 17;
    for (y = 0; y < 6; y++)
        for (x = 0; x < 3; x++, bc--)
            if (1&(bc > 11 ? vers >> (bc - 12) : verinfo >> bc))
                SETQRBIT( x+width-11,y);
            else
                SETFXBIT( x+width-11,y);
}

void initframe(unsigned char vers)
{
    unsigned x, y;
    if (vers > 40)
        return;
    width = 17 + 4 * vers;
    widbytes = (width + 7) / 8;
    memset(qrframe, 0, sizeof(qrframe));
    memset(framask, 0, sizeof(framask));
    // finders
    putfind();
    // alignment blocks
    doaligns(vers);
    // single black
    SETQRBIT(8, width - 8);
    // timing gap - masks only
    for (y = 0; y < 7; y++) {
        SETFXBIT(7, y);
        SETFXBIT(width - 8, y);
        SETFXBIT(7, y + width - 7);
    }
    for (x = 0; x < 8; x++) {
        SETFXBIT(x, 7);
        SETFXBIT(x + width - 8, 7);
        SETFXBIT(x, width - 8);
    }
    // reserve mask-format area
    for (x = 0; x < 9; x++)
        SETFXBIT(x, 8);
    for (x = 0; x < 8; x++) {
        SETFXBIT(x + width - 8, 8);
        SETFXBIT(8, x);
    }
    for (y = 0; y < 7; y++)
        SETFXBIT(8, y + width - 7);
    // timing
    for (x = 0; x < width - 14; x++)
        if (x & 1) {
            SETFXBIT(8 + x, 6);
            SETFXBIT(6, 8 + x);
        } else {
            SETQRBIT(8 + x, 6);
            SETQRBIT(6, 8 + x);
        }

    for (x = 0; x < width * widbytes; x++)
        framask[x] |= qrframe[x];
    // version block
    putvpat(vers);
}

#include <stdio.h>
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int i, j;
    int v;                      // = atoi(argv[1]);
    int vers;
    for (vers = 1; vers < 41; vers++) {
        initframe(vers);
        v = vers * 4 + 17;
        for (j = 0; j < v; j++) {
            for (i = 0; i < v; i++)
                printf("%c", QRBIT(i, j) ? '#' : (FIXEDBIT(i, j) ? 'o' : '.'));
            printf("\n");
        }
    }
    return 0;
}
