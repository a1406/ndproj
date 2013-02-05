/* DES.H - header file for DESC.C
 */

#ifndef _DES_H_
#define _DES_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  UINT4 subkeys[32];                                             /* subkeys */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 originalIV[2];                        /* for restarting the context */
  NEINT32 encrypt;                                               /* encrypt flag */
} DES_CBC_CTX;

typedef struct {
  UINT4 subkeys[32];                                             /* subkeys */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 inputWhitener[2];                                 /* input whitener */
  UINT4 outputWhitener[2];                               /* output whitener */
  UINT4 originalIV[2];                        /* for restarting the context */
  NEINT32 encrypt;                                              /* encrypt flag */
} DESX_CBC_CTX;

typedef struct {
  UINT4 subkeys[3][32];                     /* subkeys for three operations */
  UINT4 iv[2];                                       /* initializing vector */
  UINT4 originalIV[2];                        /* for restarting the context */
  NEINT32 encrypt;                                              /* encrypt flag */
} DES3_CBC_CTX;

void DES_CBCInit PROTO_LIST 
  ((DES_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEINT32));
NEINT32 DES_CBCUpdate PROTO_LIST
  ((DES_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEUINT32));
void DES_CBCRestart PROTO_LIST ((DES_CBC_CTX *));

void DESX_CBCInit PROTO_LIST 
  ((DESX_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEINT32));
NEINT32 DESX_CBCUpdate PROTO_LIST
  ((DESX_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEUINT32));
void DESX_CBCRestart PROTO_LIST ((DESX_CBC_CTX *));

void DES3_CBCInit PROTO_LIST 
  ((DES3_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEINT32));
NEINT32 DES3_CBCUpdate PROTO_LIST
  ((DES3_CBC_CTX *, NEUINT8 *, NEUINT8 *, NEUINT32));
void DES3_CBCRestart PROTO_LIST ((DES3_CBC_CTX *));

#ifdef __cplusplus
}
#endif

#endif
