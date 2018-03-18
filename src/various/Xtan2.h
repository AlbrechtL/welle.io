//
//  This LUT implementation of atan2 is a C++ translation of
//  a Java discussion on the net
//  http://www.java-gaming.org/index.php?topic=14647.0

#ifndef     __COMP_ATAN
#define     __COMP_ATAN

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <limits>
#include <vector>
#include "dab-constants.h"

class compAtan
{
    public:
        compAtan(void);
        float   atan2(float y, float x);
        float   argX(DSPCOMPLEX);
    private:
        std::vector<float> ATAN2_TABLE_PPY;
        std::vector<float> ATAN2_TABLE_PPX;
        std::vector<float> ATAN2_TABLE_PNY;
        std::vector<float> ATAN2_TABLE_PNX;
        std::vector<float> ATAN2_TABLE_NPY;
        std::vector<float> ATAN2_TABLE_NPX;
        std::vector<float> ATAN2_TABLE_NNY;
        std::vector<float> ATAN2_TABLE_NNX;
        float   Stretch;
};

#endif
