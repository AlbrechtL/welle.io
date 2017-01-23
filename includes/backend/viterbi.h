#
#ifndef	__VITERBI__
#define	__VITERBI__
/*
 * 	Viterbi.h according to the SPIRAL project
 */
#include	"dab-constants.h"

//	For our particular viterbi decoder, we have
#define	RATE		4
#define NUMSTATES	64
#define BITS_PER_BYTE	8

//	decision_t is a BIT vector
typedef union {
	uint8_t t  [NUMSTATES / BITS_PER_BYTE];
	uint32_t w [NUMSTATES / 32];
	uint16_t s [NUMSTATES / 16];
	uint8_t c  [NUMSTATES / 8];
} decision_t __attribute__ ((aligned (16)));

typedef union {
	int16_t t[NUMSTATES];
} metric_t __attribute__ ((aligned (16)));

/*
 *	 State info for instance of Viterbi decoder
 */

struct v {
/* path metric buffer 1 */
	__attribute__ ((aligned (16))) metric_t metrics1;
/* path metric buffer 2 */
	__attribute__ ((aligned (16))) metric_t metrics2;
/* Pointers to path metrics, swapped on every bit */
	metric_t *old_metrics,*new_metrics;
	decision_t *decisions;   /* decisions */
};

class	viterbi {
public:
		viterbi		(int16_t);
		~viterbi	(void);
	void	deconvolve	(int16_t *, uint8_t *);
private:

	struct v	vp;
	int16_t Branchtab	[NUMSTATES / 2 * RATE] __attribute__ ((aligned (16)));
	int16_t	parity		(int16_t);
	void	init_viterbi	(struct v *, int16_t);
	void	update_viterbi_blk_GENERIC	(struct v *, int16_t *,
	                                         int16_t);
	void	chainback_viterbi (struct v *, uint8_t *, int16_t, uint16_t);
	void	BFLY		(int32_t, int, int16_t *,
	                         struct v *, decision_t *);
	uint8_t *data;
	int16_t *symbols;
	int16_t	frameBits;
};

#endif

