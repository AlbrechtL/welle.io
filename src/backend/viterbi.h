#ifndef __VITERBI__
#define __VITERBI__
/*
 *  Viterbi.h according to the SPIRAL project
 */
#include    "dab-constants.h"
#include    "MathHelper.h"

//  For our particular viterbi decoder, we have
#define RATE    4
#define NUMSTATES 64
#define DECISIONTYPE uint32_t
#define DECISIONALIGN 32
#define DECISIONTYPE_BITSIZE (sizeof(DECISIONTYPE) * 8)
#define COMPUTETYPE uint16_t

typedef struct {
    DECISIONTYPE w[NUMSTATES/32];
} decision_t;

typedef union {
    COMPUTETYPE t[NUMSTATES];
} metric_t __attribute__ ((aligned (16)));

/* State info for instance of Viterbi decoder
*/

struct v {
    /* path metric buffer 1 */
    __attribute__ ((aligned (16))) metric_t metrics1;
    /* path metric buffer 2 */
    __attribute__ ((aligned (16))) metric_t metrics2;
    /* Pointers to path metrics, swapped on every bit */
    metric_t   *old_metrics,*new_metrics;
    decision_t *decisions;   /* decisions */
};

class Viterbi
{
    public:
        Viterbi(int16_t);
        ~Viterbi(void);
        Viterbi(const Viterbi& other) = delete;
        Viterbi& operator=(const Viterbi& other) = delete;
        void deconvolve(softbit_t *input, uint8_t *output);

    private:
        struct v    vp;
        COMPUTETYPE Branchtab   [NUMSTATES / 2 * RATE] __attribute__ ((aligned (16)));
        //  int parityb     (uint8_t);
        int parity(int x);
        void partab_init (void);
        //  uint8_t Partab  [256];
        void init_viterbi(struct v *, int16_t starting_state);

        void update_viterbi_blk_GENERIC( struct v *vp,
                                         COMPUTETYPE *syms,
                                         int16_t nbits);

        void chainback_viterbi( struct v *vp,
                                uint8_t *data, /* Decoded output data */
                                int16_t nbits, /* Number of data bits */
                                uint16_t endstate); /*Terminal encoder state */

        void BFLY( int i, int s, COMPUTETYPE * syms, struct v * vp, decision_t * d);

        uint8_t *data;
        COMPUTETYPE *symbols;
        int16_t frameBits;
};

#endif

