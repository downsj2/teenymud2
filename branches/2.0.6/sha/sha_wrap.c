/* sha_wrap.c */

#include "config.h"

/*
 *		       This file is part of TeenyMUD II.
 *		 Copyright(C) 1993, 1994, 1995 by Jason Downs.
 *                           All rights reserved.
 * 
 * TeenyMUD II is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * TeenyMUD II is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file 'COPYING'); if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
 */

#include <stdio.h>
#include <sys/types.h>

#include "sha.h"
#include "sha_wrap.h"

/* SHA wrapper for TeenyMUD */

static void sha_alpha _ANSI_ARGS_((SHS_INFO, char *));

char *sha_crypt(str)
    char *str;
{
  unsigned char buffer[SHS_BLOCKSIZE];
  register char *ptr, *optr;
  register unsigned char *bptr;
  SHS_INFO shsInfo;
  static char ret[28];

  /* sanity check. */
  if((str == (char *)NULL) || (str[0] == '\0'))
    return((char *)NULL);

  /* initialize SHA. */
  shsInit(&shsInfo);

  /* pass str to SHA in chunks. */
  optr = str;
  do {
    ptr = optr;
    bptr = buffer;

    while((*ptr != '\0') && ((ptr - optr) < sizeof(buffer))) {
      *bptr++ = *ptr++;
    }
    shsUpdate(&shsInfo, buffer, (ptr - optr));

    optr = ptr;
  } while(*ptr != '\0');

  shsFinal(&shsInfo);

  /* convert into something text-like. */
  sha_alpha(shsInfo, ret);

  return(ret);
}

/* conversions for every six bits of the SHA code. */
static const char _alphatab[] = {
  '.', '(', ')', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
};

static void sha_alpha(shsInfo, buff)
    SHS_INFO shsInfo;
    char *buff;
{
  register int idx;
  int left = 0;

  for(idx = 0; idx < 5; idx++) {
    *buff++ = _alphatab[shsInfo.digest[idx] & 0x3F];
    *buff++ = _alphatab[(shsInfo.digest[idx] >> 6) & 0x3F];
    *buff++ = _alphatab[(shsInfo.digest[idx] >> 12) & 0x3F];
    *buff++ = _alphatab[(shsInfo.digest[idx] >> 18) & 0x3F];
    *buff++ = _alphatab[(shsInfo.digest[idx] >> 24) & 0x3F];
    left += ((shsInfo.digest[idx] >> 30) & 0x03);
  }
  *buff++ = _alphatab[left & 0x3F];
  *buff++ = _alphatab[left & ~0x3F];
  *buff = '\0';
}
