/* RSA.H - header file for RSA.C
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */

NEINT32 RSAPublicEncrypt PROTO_LIST 
  ((NEUINT8 *, NEUINT32 *, NEUINT8 *, NEUINT32,
    R_RSA_PUBLIC_KEY *, R_RANDOM_STRUCT *));
NEINT32 RSAPrivateEncrypt PROTO_LIST
  ((NEUINT8 *, NEUINT32 *, NEUINT8 *, NEUINT32,
    R_RSA_PRIVATE_KEY *));
NEINT32 RSAPublicDecrypt PROTO_LIST 
  ((NEUINT8 *, NEUINT32 *, NEUINT8 *, NEUINT32,
    R_RSA_PUBLIC_KEY *));
NEINT32 RSAPrivateDecrypt PROTO_LIST
  ((NEUINT8 *, NEUINT32 *, NEUINT8 *, NEUINT32,
    R_RSA_PRIVATE_KEY *));
