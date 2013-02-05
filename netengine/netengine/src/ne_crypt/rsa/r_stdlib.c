/* R_STDLIB.C - platform-specific C library routines for RSAREF
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */
#include "ne_common/ne_os.h"
#include <string.h>
#include "ne_crypt/rsah/global.h"
#include "ne_crypt/rsah/rsaref.h"

void R_memset (output, value, len)
POINTER output;                                             /* output block */
NEINT32 value;                                                         /* value */
NEUINT32 len;                                        /* length of block */
{
  if (len)
    memset (output, value, len);
}

void R_memcpy (output, input, len)
POINTER output;                                             /* output block */
POINTER input;                                               /* input block */
NEUINT32 len;                                       /* length of blocks */
{
  if (len)
    memcpy (output, input, len);
}

NEINT32 R_memcmp (firstBlock, secondBlock, len)
POINTER firstBlock;                                          /* first block */
POINTER secondBlock;                                        /* second block */
NEUINT32 len;                                       /* length of blocks */
{
  if (len)
    return (memcmp (firstBlock, secondBlock, len));
  else
    return (0);
}
