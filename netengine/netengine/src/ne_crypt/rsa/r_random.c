/* R_RANDOM.C - random objects for RSAREF
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */
#include "ne_common/ne_os.h"
#include "ne_crypt/rsah/global.h"
#include "ne_crypt/rsah/rsaref.h"
#include "ne_crypt/rsah/r_random.h"
#include "ne_crypt/rsah/md5.h"

#define RANDOM_BYTES_NEEDED 256

NEINT32 R_RandomInit (randomStruct)
R_RANDOM_STRUCT *randomStruct;                      /* new random structure */
{
  randomStruct->bytesNeeded = RANDOM_BYTES_NEEDED;
  R_memset ((POINTER)randomStruct->state, 0, sizeof (randomStruct->state));
  randomStruct->outputAvailable = 0;
  
  return (0);
}

NEINT32 R_RandomUpdate (randomStruct, block, blockLen)
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
NEUINT8 *block;                          /* block of values to mix in */
NEUINT32 blockLen;                                   /* length of block */
{
  MD5_CTX context;
  NEUINT8 digest[16];
  NEUINT32 i, x;
  
  MD5Init (&context);
  MD5Update (&context, block, blockLen);
  MD5Final (digest, &context);

  /* add digest to state */
  x = 0;
  for (i = 0; i < 16; i++) {
    x += randomStruct->state[15-i] + digest[15-i];
    randomStruct->state[15-i] = (NEUINT8)x;
    x >>= 8;
  }
  
  if (randomStruct->bytesNeeded < blockLen)
    randomStruct->bytesNeeded = 0;
  else
    randomStruct->bytesNeeded -= blockLen;
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)digest, 0, sizeof (digest));
  x = 0;
  
  return (0);
}

NEINT32 R_GetRandomBytesNeeded (bytesNeeded, randomStruct)
NEUINT32 *bytesNeeded;                 /* number of mix-in bytes needed */
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
{
  *bytesNeeded = randomStruct->bytesNeeded;
  
  return (0);
}

NEINT32 R_GenerateBytes (block, blockLen, randomStruct)
NEUINT8 *block;                                              /* block */
NEUINT32 blockLen;                                   /* length of block */
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
{
  MD5_CTX context;
  NEUINT32 available, i;
  
  if (randomStruct->bytesNeeded)
    return (RE_NEED_RANDOM);
  
  available = randomStruct->outputAvailable;
  
  while (blockLen > available) {
    R_memcpy
      ((POINTER)block, (POINTER)&randomStruct->output[16-available],
       available);
    block += available;
    blockLen -= available;

    /* generate new output */
    MD5Init (&context);
    MD5Update (&context, randomStruct->state, 16);
    MD5Final (randomStruct->output, &context);
    available = 16;

    /* increment state */
    for (i = 0; i < 16; i++)
      if (randomStruct->state[15-i]++)
        break;
  }

  R_memcpy 
    ((POINTER)block, (POINTER)&randomStruct->output[16-available], blockLen);
  randomStruct->outputAvailable = available - blockLen;

  return (0);
}

void R_RandomFinal (randomStruct)
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
{
  R_memset ((POINTER)randomStruct, 0, sizeof (*randomStruct));
}
