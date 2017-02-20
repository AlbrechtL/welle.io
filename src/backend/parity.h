/**
 * static inline parity(int x) -- compute parity (up to 32 bits?)
 */

/* Determine parity of argument: 1 = odd, 0 = even */
#ifdef __i386__
static inline int parityb(unsigned char x){
  __asm__ __volatile__ ("test %1,%1;setpo %0" : "=g"(x) : "r" (x));
  return x;
}
#else

#if __cplusplus
extern "C" {
#endif
void partab_init();
#if __cplusplus
};
#endif

static inline int parityb(unsigned char x){
  extern unsigned char Partab[256];
  extern int Parity_initialized;
  if(!Parity_initialized){
    partab_init();
  }
  return Partab[x];
}
#endif

static inline int parity(int x){
  /* Fold down to one byte */
  x ^= (x >> 16);
  x ^= (x >> 8);
  return parityb(x);
}
