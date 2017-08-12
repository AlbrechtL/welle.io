//
//  This LUT implementation of atan2 is a C++ translation of
//  a Java discussion on the net
//  http://www.java-gaming.org/index.php?topic=14647.0

#include    "Xtan2.h"

#define SIZE        8192
#define EZIS        (-SIZE)

compAtan::compAtan() :
    ATAN2_TABLE_PPY(SIZE + 1),
    ATAN2_TABLE_PPX(SIZE + 1),
    ATAN2_TABLE_PNY(SIZE + 1),
    ATAN2_TABLE_PNX(SIZE + 1),
    ATAN2_TABLE_NPY(SIZE + 1),
    ATAN2_TABLE_NPX(SIZE + 1),
    ATAN2_TABLE_NNY(SIZE + 1),
    ATAN2_TABLE_NNX(SIZE + 1)
{
    Stretch     = M_PI;
    //  private static final int           SIZE                 = 1024;
    //  private static final float         Stretch            = (float)Math.PI;
    // Output will swing from -Stretch to Stretch (default: Math.PI)
    // Useful to change to 1 if you would normally do "atan2(y, x) / Math.PI"

    for (int i = 0; i <= SIZE; i++) {
        float f = (float)i / SIZE;
        ATAN2_TABLE_PPY[i] = atan(f) * Stretch / M_PI;
        ATAN2_TABLE_PPX[i] = Stretch * 0.5f - ATAN2_TABLE_PPY[i];
        ATAN2_TABLE_PNY[i] = -ATAN2_TABLE_PPY[i];
        ATAN2_TABLE_PNX[i] = ATAN2_TABLE_PPY[i] - Stretch * 0.5f;
        ATAN2_TABLE_NPY[i] = Stretch - ATAN2_TABLE_PPY[i];
        ATAN2_TABLE_NPX[i] = ATAN2_TABLE_PPY[i] + Stretch * 0.5f;
        ATAN2_TABLE_NNY[i] = ATAN2_TABLE_PPY[i] - Stretch;
        ATAN2_TABLE_NNX[i] = -Stretch * 0.5f - ATAN2_TABLE_PPY[i];
    }
}

/**
 * ATAN2 : performance degrades due to the many "0" tests
 */
float compAtan::atan2(float y, float x)
{
    if (x == 0) {
        if (y == 0)  return 0;
        //        return std::numeric_limits<float>::infinity ();
        else
            if (y > 0)
                return  M_PI / 2;
            else     // y < 0
                return  - M_PI / 2;
    }

    if (x > 0) {
        if (y >= 0) {
            if (x >= y)
                return ATAN2_TABLE_PPY[(int)(SIZE * y / x + 0.5)];
            else
                return ATAN2_TABLE_PPX[(int)(SIZE * x / y + 0.5)];

        }
        else {
            if (x >= -y)
                return ATAN2_TABLE_PNY[(int)(EZIS * y / x + 0.5)];
            else
                return ATAN2_TABLE_PNX[(int)(EZIS * x / y + 0.5)];
        }
    }
    else {
        if (y >= 0) {
            if (-x >= y)
                return ATAN2_TABLE_NPY[(int)(EZIS * y / x + 0.5)];
            else
                return ATAN2_TABLE_NPX[(int)(EZIS * x / y + 0.5)];
        }
        else {
            if (x <= y) // (-x >= -y)
                return ATAN2_TABLE_NNY[(int)(SIZE * y / x + 0.5)];
            else
                return ATAN2_TABLE_NNX[(int)(SIZE * x / y + 0.5)];
        }
    }
}

float compAtan::argX(DSPCOMPLEX v)
{
    return this->atan2(imag(v), real(v));
}
