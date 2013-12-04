#ifndef __SHA_H_

/* Useful defines/typedefs */

typedef unsigned char   BYTE;
typedef unsigned int    LONG;	/* Teeny assumes ints are 32bits. */

/* The SHS block size and message digest sizes, in bytes */

#define SHS_BLOCKSIZE   64
#define SHS_DIGESTSIZE  20

/* The structure for storing SHS info */

typedef struct {
	       LONG digest[ 5 ];            /* Message digest */
	       LONG countLo, countHi;       /* 64-bit bit count */
	       LONG data[ 16 ];             /* SHS data buffer */
	       } SHS_INFO;

/* Prototypes */
extern void shsInit _ANSI_ARGS_((SHS_INFO *));
extern void shsTransform _ANSI_ARGS_((SHS_INFO *));
extern void shsUpdate _ANSI_ARGS_((SHS_INFO *, BYTE *, int));
extern void shsFinal _ANSI_ARGS_((SHS_INFO *));

#define __SHA_H_
#endif			/* __SHA_H_ */
