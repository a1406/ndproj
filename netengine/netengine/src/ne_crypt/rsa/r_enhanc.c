/* R_ENHANC.C - cryptographic enhancements for RSAREF
 */

/* Copyright (C) RSA Laboratories, a division of RSA Data Security,
     Inc., created 1991. All rights reserved.
 */
#include "ne_common/ne_os.h"
#include "ne_crypt/rsah/global.h"
#include "ne_crypt/rsah/rsaref.h"
#include "ne_crypt/rsah/r_random.h"
#include "ne_crypt/rsah/rsa.h"

/* DigestInfo encoding is DIGEST_INFO_A, then 2 or 5 (for MD2/MD5),
   then DIGEST_INFO_B, then 16-byte message digest.
 */

static NEUINT8 DIGEST_INFO_A[] = {
  0x30, 0x20, 0x30, 0x0c, 0x06, 0x08, 0x2a, 0x86, 0x48, 0x86, 0xf7,
  0x0d, 0x02
};
#define DIGEST_INFO_A_LEN sizeof (DIGEST_INFO_A)

static NEUINT8 DIGEST_INFO_B[] = { 0x05, 0x00, 0x04, 0x10 };
#define DIGEST_INFO_B_LEN sizeof (DIGEST_INFO_B)

#define DIGEST_INFO_LEN (DIGEST_INFO_A_LEN + 1 + DIGEST_INFO_B_LEN + 16)

static NEUINT8 *PADDING[] = {
  (NEUINT8 *)"", (NEUINT8 *)"\001", (NEUINT8 *)"\002\002",
  (NEUINT8 *)"\003\003\003", (NEUINT8 *)"\004\004\004\004",
  (NEUINT8 *)"\005\005\005\005\005",
  (NEUINT8 *)"\006\006\006\006\006\006", 
  (NEUINT8 *)"\007\007\007\007\007\007\007",
  (NEUINT8 *)"\010\010\010\010\010\010\010\010"
};

#define MAX_ENCRYPTED_KEY_LEN MAX_RSA_MODULUS_LEN

static void R_EncodeDigestInfo PROTO_LIST
  ((NEUINT8 *, NEINT32, NEUINT8 *));
static void EncryptPEMUpdateFinal PROTO_LIST
  ((R_ENVELOPE_CTX *, NEUINT8 *, NEUINT32 *, NEUINT8 *,
    NEUINT32));
static NEINT32 DecryptPEMUpdateFinal PROTO_LIST
  ((R_ENVELOPE_CTX *, NEUINT8 *, NEUINT32 *, NEUINT8 *,
    NEUINT32));
static NEINT32 CipherInit PROTO_LIST
  ((R_ENVELOPE_CTX *, NEINT32, NEUINT8 *, NEUINT8 *, NEINT32));
static void CipherUpdate PROTO_LIST
  ((R_ENVELOPE_CTX *, NEUINT8 *, NEUINT8 *, NEUINT32));
static void CipherRestart PROTO_LIST ((R_ENVELOPE_CTX *));

NEINT32 R_DigestInit (context, digestAlgorithm)
R_DIGEST_CTX *context;                                       /* new context */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
{
  context->digestAlgorithm = digestAlgorithm;

  switch (digestAlgorithm) {
  case DA_MD2:
    MD2Init (&context->context.md2);
    break;

  case DA_MD5:
    MD5Init (&context->context.md5);
    break;
  
  default:
    return (RE_DIGEST_ALGORITHM);
  }

  return (0);
}

NEINT32 R_DigestUpdate (context, partIn, partInLen)
R_DIGEST_CTX *context;                                           /* context */
NEUINT8 *partIn;                                    /* next data part */
NEUINT32 partInLen;                         /* length of next data part */
{
  if (context->digestAlgorithm == DA_MD2)
    MD2Update (&context->context.md2, partIn, partInLen);
  else
    MD5Update (&context->context.md5, partIn, partInLen);
  return (0);
}

NEINT32 R_DigestFinal (context, digest, digestLen)
R_DIGEST_CTX *context;                                           /* context */
NEUINT8 *digest;                                    /* message digest */
NEUINT32 *digestLen;                        /* length of message digest */
{
  *digestLen = 16;
  if (context->digestAlgorithm == DA_MD2)
    MD2Final (digest, &context->context.md2);
  else
    MD5Final (digest, &context->context.md5);

  return (0);
}

NEINT32 R_SignInit (context, digestAlgorithm)
R_SIGNATURE_CTX *context;                                    /* new context */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
{
  return (R_DigestInit (&context->digestContext, digestAlgorithm));
}

NEINT32 R_SignUpdate (context, partIn, partInLen)
R_SIGNATURE_CTX *context;                                        /* context */
NEUINT8 *partIn;                                    /* next data part */
NEUINT32 partInLen;                         /* length of next data part */
{
  return (R_DigestUpdate (&context->digestContext, partIn, partInLen));
}

NEINT32 R_SignFinal (context, signature, signatureLen, privateKey)
R_SIGNATURE_CTX *context;                                        /* context */
NEUINT8 *signature;                                      /* signature */
NEUINT32 *signatureLen;                          /* length of signature */
R_RSA_PRIVATE_KEY *privateKey;                  /* signer's RSA private key */
{
  NEINT32 status;
  NEUINT8 digest[MAX_DIGEST_LEN], digestInfo[DIGEST_INFO_LEN];
  NEUINT32 digestLen;

  do {
    if ((status = R_DigestFinal (&context->digestContext, digest, &digestLen))
        != 0)
      break;

    R_EncodeDigestInfo
      (digestInfo, context->digestContext.digestAlgorithm, digest);
    
    if (RSAPrivateEncrypt
        (signature, signatureLen, digestInfo, DIGEST_INFO_LEN, privateKey)
        != 0) {
      status = RE_PRIVATE_KEY;
      break;
    }

    /* Reset for another verification. Assume Init won't fail */
    R_DigestInit
      (&context->digestContext, context->digestContext.digestAlgorithm);
  } while (0);
  
  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)digest, 0, sizeof (digest));
  R_memset ((POINTER)digestInfo, 0, sizeof (digestInfo));

  return (status);
}

NEINT32 R_VerifyInit (context, digestAlgorithm)
R_SIGNATURE_CTX *context;                                    /* new context */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
{
  return (R_DigestInit (&context->digestContext, digestAlgorithm));
}

NEINT32 R_VerifyUpdate (context, partIn, partInLen)
R_SIGNATURE_CTX *context;                                        /* context */
NEUINT8 *partIn;                                    /* next data part */
NEUINT32 partInLen;                         /* length of next data part */
{
  return (R_DigestUpdate (&context->digestContext, partIn, partInLen));
}

NEINT32 R_VerifyFinal (context, signature, signatureLen, publicKey)
R_SIGNATURE_CTX *context;                                        /* context */
NEUINT8 *signature;                                      /* signature */
NEUINT32 signatureLen;                           /* length of signature */
R_RSA_PUBLIC_KEY *publicKey;                     /* signer's RSA public key */
{
  NEINT32 status;
  NEUINT8 digest[MAX_DIGEST_LEN], digestInfo[DIGEST_INFO_LEN],
    originalDigestInfo[MAX_SIGNATURE_LEN];
  NEUINT32 originalDigestInfoLen, digestLen;
  
  if (signatureLen > MAX_SIGNATURE_LEN)
    return (RE_LEN);

  status = 0;
  do {
    if ((status = R_DigestFinal (&context->digestContext, digest, &digestLen))
        != 0)
      break;

    R_EncodeDigestInfo
      (digestInfo, context->digestContext.digestAlgorithm, digest);
    
    if (RSAPublicDecrypt
        (originalDigestInfo, &originalDigestInfoLen, signature, signatureLen, 
         publicKey) != 0) {
      status = RE_PUBLIC_KEY;
      break;
    }
    
    if ((originalDigestInfoLen != DIGEST_INFO_LEN) ||
        (R_memcmp 
         ((POINTER)originalDigestInfo, (POINTER)digestInfo,
          DIGEST_INFO_LEN))) {
      status = RE_SIGNATURE;
      break;
    }

    /* Reset for another verification. Assume Init won't fail */
    R_DigestInit
      (&context->digestContext, context->digestContext.digestAlgorithm);
  } while (0);
  
  /* Zeroize potentially sensitive information.
   */
  R_memset ((POINTER)digest, 0, sizeof (digest));
  R_memset ((POINTER)digestInfo, 0, sizeof (digestInfo));
  R_memset ((POINTER)originalDigestInfo, 0, sizeof (originalDigestInfo));

  return (status);
}

/* Caller must ASCII recode the encrypted keys if desired.
 */
NEINT32 R_SealInit
  (context, encryptedKeys, encryptedKeyLens, iv, publicKeyCount, publicKeys,
   encryptionAlgorithm, randomStruct)
R_ENVELOPE_CTX *context;                                     /* new context */
NEUINT8 **encryptedKeys;                            /* encrypted keys */
NEUINT32 *encryptedKeyLens;                /* lengths of encrypted keys */
NEUINT8 iv[8];                               /* initialization vector */
NEUINT32 publicKeyCount;                       /* number of public keys */
R_RSA_PUBLIC_KEY **publicKeys;                               /* public keys */
NEINT32 encryptionAlgorithm;                       /* data encryption algorithm */
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
{
  NEINT32 status;
  NEUINT8 key[24];
  NEUINT32 keyLen, i;
  
  do {
    context->encryptionAlgorithm = encryptionAlgorithm;
    
    keyLen = (encryptionAlgorithm == EA_DES_CBC) ? 8 : 24;
    if ((status = R_GenerateBytes (key, keyLen, randomStruct)) != 0)
      break;
    if ((status = R_GenerateBytes (iv, 8, randomStruct)) != 0)
      break;

    if (encryptionAlgorithm == EA_DES_EDE2_CBC)
      /* Make both E keys the same */
      R_memcpy ((POINTER)(key + 16), (POINTER)key, 8);

    if ((status = CipherInit (context, encryptionAlgorithm, key, iv, 1)) != 0)
      break;

    for (i = 0; i < publicKeyCount; ++i) {
      if (RSAPublicEncrypt
          (encryptedKeys[i], &encryptedKeyLens[i], key, keyLen,
           publicKeys[i], randomStruct)) {
        status = RE_PUBLIC_KEY;
        break;
      }
    }
    if (status != 0)
      break;

    context->bufferLen = 0;
  } while (0);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)key, 0, sizeof (key));

  return (status);
}

/* Assume partOut buffer is at least partInLen + 7, since this may flush
     buffered input.
 */
NEINT32 R_SealUpdate (context, partOut, partOutLen, partIn, partInLen)
R_ENVELOPE_CTX *context;                                         /* context */
NEUINT8 *partOut;                         /* next encrypted data part */
NEUINT32 *partOutLen;             /* length of next encrypted data part */
NEUINT8 *partIn;                                    /* next data part */
NEUINT32 partInLen;                         /* length of next data part */
{
  NEUINT32 tempLen;

  tempLen = 8 - context->bufferLen;
  if (partInLen < tempLen) {
    /* Just accumulate into buffer.
     */
    R_memcpy
      ((POINTER)(context->buffer + context->bufferLen), (POINTER)partIn,
       partInLen);
    context->bufferLen += partInLen;
    *partOutLen = 0;
    return (0);
  }

  /* Fill the buffer and encrypt.
   */
  R_memcpy
    ((POINTER)(context->buffer + context->bufferLen), (POINTER)partIn,
     tempLen);
  CipherUpdate (context, partOut, context->buffer, 8);
  partIn += tempLen;
  partInLen -= tempLen;
  partOut += 8;
  *partOutLen = 8;

  /* Encrypt as many 8-byte blocks as possible.
   */
  tempLen = 8 * (partInLen / 8);
  CipherUpdate (context, partOut, partIn, tempLen);
  partIn += tempLen;
  partInLen -= tempLen;
  *partOutLen += tempLen;

  /* Length is now less than 8, so copy remainder to buffer.
   */
  R_memcpy
    ((POINTER)context->buffer, (POINTER)partIn,
     context->bufferLen = partInLen);

  return (0);
}

/* Assume partOut buffer is at least 8 bytes.
 */
NEINT32 R_SealFinal (context, partOut, partOutLen)
R_ENVELOPE_CTX *context;                                         /* context */
NEUINT8 *partOut;                         /* last encrypted data part */
NEUINT32 *partOutLen;             /* length of last encrypted data part */
{
  NEUINT32 padLen;

  /* Pad and encrypt final block.
   */
  padLen = 8 - context->bufferLen;
  R_memset
    ((POINTER)(context->buffer + context->bufferLen), (NEINT32)padLen, padLen);
  CipherUpdate (context, partOut, context->buffer, 8);
  *partOutLen = 8;

  /* Restart the context.
   */
  CipherRestart (context);
  context->bufferLen = 0;

  return (0);
}

/* Assume caller has already ASCII decoded the encryptedKey if necessary.
 */
NEINT32 R_OpenInit
  (context, encryptionAlgorithm, encryptedKey, encryptedKeyLen, iv, privateKey)
R_ENVELOPE_CTX *context;                                     /* new context */
NEINT32 encryptionAlgorithm;                       /* data encryption algorithm */
NEUINT8 *encryptedKey;               /* encrypted data encryption key */
NEUINT32 encryptedKeyLen;                    /* length of encrypted key */
NEUINT8 iv[8];                               /* initialization vector */
R_RSA_PRIVATE_KEY *privateKey;               /* recipient's RSA private key */
{
  NEINT32 status;
  NEUINT8 key[MAX_ENCRYPTED_KEY_LEN];
  NEUINT32 keyLen;
  
  if (encryptedKeyLen > MAX_ENCRYPTED_KEY_LEN)
    return (RE_LEN);
  
  do {
    context->encryptionAlgorithm = encryptionAlgorithm;

    if (RSAPrivateDecrypt
        (key, &keyLen, encryptedKey, encryptedKeyLen, privateKey)) {
      status = RE_PRIVATE_KEY;
      break;
    }

    if (encryptionAlgorithm == EA_DES_CBC) {    
      if (keyLen != 8) {
        status = RE_PRIVATE_KEY;
        break;
      }
    }
    else {
      if (keyLen != 24) {
        status = RE_PRIVATE_KEY;
        break;
      }
    }
    
    if ((status = CipherInit (context, encryptionAlgorithm, key, iv, 0)) != 0)
      break;

    context->bufferLen = 0;
  } while (0);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)key, 0, sizeof (key));

  return (status);
}

/* Assume partOut buffer is at least partInLen + 7, since this may flush
     buffered input. Always leaves at least one byte in buffer.
 */
NEINT32 R_OpenUpdate (context, partOut, partOutLen, partIn, partInLen)
R_ENVELOPE_CTX *context;                                         /* context */
NEUINT8 *partOut;                         /* next recovered data part */
NEUINT32 *partOutLen;             /* length of next recovered data part */
NEUINT8 *partIn;                          /* next encrypted data part */
NEUINT32 partInLen;               /* length of next encrypted data part */
{
  NEUINT32 tempLen;

  tempLen = 8 - context->bufferLen;
  if (partInLen <= tempLen) {
    /* Just accumulate into buffer.
     */
    R_memcpy
      ((POINTER)(context->buffer + context->bufferLen), (POINTER)partIn,
       partInLen);
    context->bufferLen += partInLen;
    *partOutLen = 0;
    return (0);
  }

  /* Fill the buffer and decrypt.  We know that there will be more left
       in partIn after decrypting the buffer.
   */
  R_memcpy
    ((POINTER)(context->buffer + context->bufferLen), (POINTER)partIn,
     tempLen);
  CipherUpdate (context, partOut, context->buffer, 8);
  partIn += tempLen;
  partInLen -= tempLen;
  partOut += 8;
  *partOutLen = 8;

  /* Decrypt as many 8 byte blocks as possible, leaving at least one byte
       in partIn.
   */
  tempLen = 8 * ((partInLen - 1) / 8);
  CipherUpdate (context, partOut, partIn, tempLen);
  partIn += tempLen;
  partInLen -= tempLen;
  *partOutLen += tempLen;

  /* Length is between 1 and 8, so copy into buffer.
   */
  R_memcpy
    ((POINTER)context->buffer, (POINTER)partIn,
     context->bufferLen = partInLen);

  return (0);
}

/* Assume partOut buffer is at least 7 bytes.
 */
NEINT32 R_OpenFinal (context, partOut, partOutLen)
R_ENVELOPE_CTX *context;                                         /* context */
NEUINT8 *partOut;                         /* last recovered data part */
NEUINT32 *partOutLen;             /* length of last recovered data part */
{
  NEINT32 status;
  NEUINT8 lastPart[8];
  NEUINT32 padLen;

  status = 0;
  do {
    if (context->bufferLen == 0)
      /* There was no input data to decrypt */
      *partOutLen = 0;
    else {
      if (context->bufferLen != 8) {
        status = RE_KEY;
        break;
      }

      /* Decrypt and strip padding from final block which is in buffer.
       */
      CipherUpdate (context, lastPart, context->buffer, 8);
    
      padLen = lastPart[7];
      if (padLen == 0 || padLen > 8) {
        status = RE_KEY;
        break;
      }
      if (R_memcmp 
          ((POINTER)&lastPart[8 - padLen], PADDING[padLen], padLen) != 0) {
        status = RE_KEY;
        break;
      }
      
      R_memcpy ((POINTER)partOut, (POINTER)lastPart, *partOutLen = 8 - padLen);
    }

    /* Restart the context.
     */
    CipherRestart (context);
    context->bufferLen = 0;
  } while (0);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)lastPart, 0, sizeof (lastPart));

  return (status);
}

NEINT32 R_SignPEMBlock 
  (encodedContent, encodedContentLen, encodedSignature, encodedSignatureLen,
   content, contentLen, recode, digestAlgorithm, privateKey)
NEUINT8 *encodedContent;                           /* encoded content */
NEUINT32 *encodedContentLen;               /* length of encoded content */
NEUINT8 *encodedSignature;                       /* encoded signature */
NEUINT32 *encodedSignatureLen;           /* length of encoded signature */
NEUINT8 *content;                                          /* content */
NEUINT32 contentLen;                               /* length of content */
NEINT32 recode;                                                /* recoding flag */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
R_RSA_PRIVATE_KEY *privateKey;                  /* signer's RSA private key */
{
  NEINT32 status;
  NEUINT8 signature[MAX_SIGNATURE_LEN];
  NEUINT32 signatureLen;
  
  if ((status = R_SignBlock
       (signature, &signatureLen, content, contentLen, digestAlgorithm,
        privateKey)) != 0)
    return (status);

  R_EncodePEMBlock 
    (encodedSignature, encodedSignatureLen, signature, signatureLen);

  if (recode)
    R_EncodePEMBlock
    (encodedContent, encodedContentLen, content, contentLen);

  return (0);
}

NEINT32 R_SignBlock
  (signature, signatureLen, block, blockLen, digestAlgorithm, privateKey)
NEUINT8 *signature;                                      /* signature */
NEUINT32 *signatureLen;                          /* length of signature */
NEUINT8 *block;                                              /* block */
NEUINT32 blockLen;                                   /* length of block */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
R_RSA_PRIVATE_KEY *privateKey;                  /* signer's RSA private key */
{
  R_SIGNATURE_CTX context;
  NEINT32 status;

  do {
    if ((status = R_SignInit (&context, digestAlgorithm)) != 0)
      break;
    if ((status = R_SignUpdate (&context, block, blockLen)) != 0)
      break;
    if ((status = R_SignFinal (&context, signature, signatureLen, privateKey))
        != 0)
      break;
  } while (0);

  /* Zeroize sensitive information. */
  R_memset ((POINTER)&context, 0, sizeof (context));

  return (status);
}

NEINT32 R_VerifyPEMSignature 
  (content, contentLen, encodedContent, encodedContentLen, encodedSignature,
   encodedSignatureLen, recode, digestAlgorithm, publicKey)
NEUINT8 *content;                                          /* content */
NEUINT32 *contentLen;                              /* length of content */
NEUINT8 *encodedContent;                /* (possibly) encoded content */
NEUINT32 encodedContentLen;                /* length of encoded content */
NEUINT8 *encodedSignature;                       /* encoded signature */
NEUINT32 encodedSignatureLen;            /* length of encoded signature */
NEINT32 recode;                                                /* recoding flag */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
R_RSA_PUBLIC_KEY *publicKey;                     /* signer's RSA public key */
{
  NEUINT8 signature[MAX_SIGNATURE_LEN];
  NEUINT32 signatureLen;
  
  if (encodedSignatureLen > MAX_PEM_SIGNATURE_LEN)
    return (RE_SIGNATURE_ENCODING);
  
  if (recode) {
    if (R_DecodePEMBlock
        (content, contentLen, encodedContent, encodedContentLen))
      return (RE_CONTENT_ENCODING);
  }
  else {
    content = encodedContent;
    *contentLen = encodedContentLen;
  }
    
  if (R_DecodePEMBlock
      (signature, &signatureLen, encodedSignature, encodedSignatureLen))
    return (RE_SIGNATURE_ENCODING);
  
  return (R_VerifyBlockSignature 
          (content, *contentLen, signature, signatureLen, digestAlgorithm,
           publicKey));
}

NEINT32 R_VerifyBlockSignature 
  (block, blockLen, signature, signatureLen, digestAlgorithm, publicKey)
NEUINT8 *block;                                              /* block */
NEUINT32 blockLen;                                   /* length of block */
NEUINT8 *signature;                                      /* signature */
NEUINT32 signatureLen;                           /* length of signature */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
R_RSA_PUBLIC_KEY *publicKey;                     /* signer's RSA public key */
{
  R_SIGNATURE_CTX context;
  NEINT32 status;

  do {
    if ((status = R_VerifyInit (&context, digestAlgorithm)) != 0)
      break;
    if ((status = R_VerifyUpdate (&context, block, blockLen)) != 0)
      break;
    if ((status = R_VerifyFinal (&context, signature, signatureLen, publicKey))
        != 0)
      break;
  } while (0);

  /* Zeroize sensitive information. */
  R_memset ((POINTER)&context, 0, sizeof (context));

  return (status);
}

NEINT32 R_SealPEMBlock 
  (encryptedContent, encryptedContentLen, encryptedKey, encryptedKeyLen,
   encryptedSignature, encryptedSignatureLen, iv, content, contentLen,
   digestAlgorithm, publicKey, privateKey, randomStruct)
NEUINT8 *encryptedContent;              /* encoded, encrypted content */
NEUINT32 *encryptedContentLen;                                /* length */
NEUINT8 *encryptedKey;                      /* encoded, encrypted key */
NEUINT32 *encryptedKeyLen;                                    /* length */
NEUINT8 *encryptedSignature;          /* encoded, encrypted signature */
NEUINT32 *encryptedSignatureLen;                              /* length */
NEUINT8 iv[8];                           /* DES initialization vector */
NEUINT8 *content;                                          /* content */
NEUINT32 contentLen;                               /* length of content */
NEINT32 digestAlgorithm;                           /* message-digest algorithms */
R_RSA_PUBLIC_KEY *publicKey;                  /* recipient's RSA public key */
R_RSA_PRIVATE_KEY *privateKey;                  /* signer's RSA private key */
R_RANDOM_STRUCT *randomStruct;                          /* random structure */
{
  R_ENVELOPE_CTX context;
  R_RSA_PUBLIC_KEY *publicKeys[1];
  NEINT32 status;
  NEUINT8 encryptedKeyBlock[MAX_ENCRYPTED_KEY_LEN],
    signature[MAX_SIGNATURE_LEN], *encryptedKeys[1];
  NEUINT32 signatureLen, encryptedKeyBlockLen;
  
  do {
    if ((status = R_SignBlock
         (signature, &signatureLen, content, contentLen, digestAlgorithm,
          privateKey)) != 0)
      break;

    publicKeys[0] = publicKey;
    encryptedKeys[0] = encryptedKeyBlock;
    if ((status = R_SealInit
         (&context, encryptedKeys, &encryptedKeyBlockLen, iv, 1, publicKeys,
          EA_DES_CBC, randomStruct)) != 0)
      break;

    R_EncodePEMBlock 
      (encryptedKey, encryptedKeyLen, encryptedKeyBlock,
       encryptedKeyBlockLen);

    EncryptPEMUpdateFinal
      (&context, encryptedContent, encryptedContentLen, content,
       contentLen);
    
    EncryptPEMUpdateFinal
      (&context, encryptedSignature, encryptedSignatureLen, signature,
       signatureLen);
  } while (0);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)&context, 0, sizeof (context));
  R_memset ((POINTER)signature, 0, sizeof (signature));

  return (status);
}

NEINT32 R_OpenPEMBlock
  (content, contentLen, encryptedContent, encryptedContentLen, encryptedKey,
   encryptedKeyLen, encryptedSignature, encryptedSignatureLen,
   iv, digestAlgorithm, privateKey, publicKey)
NEUINT8 *content;                                          /* content */
NEUINT32 *contentLen;                              /* length of content */
NEUINT8 *encryptedContent;              /* encoded, encrypted content */
NEUINT32 encryptedContentLen;                                 /* length */
NEUINT8 *encryptedKey;                      /* encoded, encrypted key */
NEUINT32 encryptedKeyLen;                                     /* length */
NEUINT8 *encryptedSignature;          /* encoded, encrypted signature */
NEUINT32 encryptedSignatureLen;                               /* length */
NEUINT8 iv[8];                           /* DES initialization vector */
NEINT32 digestAlgorithm;                           /* message-digest algorithms */
R_RSA_PRIVATE_KEY *privateKey;               /* recipient's RSA private key */
R_RSA_PUBLIC_KEY *publicKey;                     /* signer's RSA public key */
{
  R_ENVELOPE_CTX context;
  NEINT32 status;
  NEUINT8 encryptedKeyBlock[MAX_ENCRYPTED_KEY_LEN],
    signature[MAX_SIGNATURE_LEN];
  NEUINT32 encryptedKeyBlockLen, signatureLen;
  
  if (encryptedKeyLen > MAX_PEM_ENCRYPTED_KEY_LEN)
    return (RE_KEY_ENCODING);
  
  if (encryptedSignatureLen > MAX_PEM_ENCRYPTED_SIGNATURE_LEN)
    return (RE_SIGNATURE_ENCODING);
  
  do {
    if (R_DecodePEMBlock 
        (encryptedKeyBlock, &encryptedKeyBlockLen, encryptedKey,
         encryptedKeyLen) != 0) {
      status = RE_KEY_ENCODING;
      break;
    }

    if ((status = R_OpenInit
         (&context, EA_DES_CBC, encryptedKeyBlock, encryptedKeyBlockLen,
          iv, privateKey)) != 0)
      break;

    if ((status = DecryptPEMUpdateFinal
         (&context, content, contentLen, encryptedContent,
          encryptedContentLen)) != 0) {
      if ((status == RE_LEN || status == RE_ENCODING))
        status = RE_CONTENT_ENCODING;
      else
        status = RE_KEY;
      break;
    }
    
    if (status = DecryptPEMUpdateFinal
        (&context, signature, &signatureLen, encryptedSignature,
         encryptedSignatureLen)) {
      if ((status == RE_LEN || status == RE_ENCODING))
        status = RE_SIGNATURE_ENCODING;
      else
        status = RE_KEY;
      break;
    }

    if ((status = R_VerifyBlockSignature
         (content, *contentLen, signature, signatureLen, digestAlgorithm,
          publicKey)) != 0)
      break;
  } while (0);
  
  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)&context, 0, sizeof (context));
  R_memset ((POINTER)signature, 0, sizeof (signature));

  return (status);
}

NEINT32 R_DigestBlock (digest, digestLen, block, blockLen, digestAlgorithm)
NEUINT8 *digest;                                    /* message digest */
NEUINT32 *digestLen;                        /* length of message digest */
NEUINT8 *block;                                              /* block */
NEUINT32 blockLen;                                   /* length of block */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
{
  R_DIGEST_CTX context;
  NEINT32 status;

  do {
    if ((status = R_DigestInit (&context, digestAlgorithm)) != 0)
      break;
    if ((status = R_DigestUpdate (&context, block, blockLen)) != 0)
      break;
    if ((status = R_DigestFinal (&context, digest, digestLen)) != 0)
      break;
  } while (0);

  /* Zeroize sensitive information. */
  R_memset ((POINTER)&context, 0, sizeof (context));

  return (status);
}

/* Assumes digestAlgorithm is DA_MD2 or DA_MD5 and digest length is 16.
 */
static void R_EncodeDigestInfo (digestInfo, digestAlgorithm, digest)
NEUINT8 *digestInfo;                           /* DigestInfo encoding */
NEINT32 digestAlgorithm;                            /* message-digest algorithm */
NEUINT8 *digest;                                    /* message digest */
{
  R_memcpy 
    ((POINTER)digestInfo, (POINTER)DIGEST_INFO_A, DIGEST_INFO_A_LEN);
  
  digestInfo[DIGEST_INFO_A_LEN] =
    (digestAlgorithm == DA_MD2) ? (NEUINT8)2 : (NEUINT8)5;

  R_memcpy 
    ((POINTER)&digestInfo[DIGEST_INFO_A_LEN + 1], (POINTER)DIGEST_INFO_B,
     DIGEST_INFO_B_LEN);
  
  R_memcpy 
    ((POINTER)&digestInfo[DIGEST_INFO_A_LEN + 1 + DIGEST_INFO_B_LEN],
     (POINTER)digest, 16);
}

/* Call SealUpdate and SealFinal on the input and ASCII recode.
 */
static void EncryptPEMUpdateFinal
  (context, output, outputLen, input, inputLen)
R_ENVELOPE_CTX *context;
NEUINT8 *output;                          /* encrypted, encoded block */
NEUINT32 *outputLen;                                /* length of output */
NEUINT8 *input;                                   /* block to encrypt */
NEUINT32 inputLen;                                            /* length */
{
  NEUINT8 encryptedPart[24];
  NEUINT32 i, lastPartLen, tempLen, len;

  /* Choose a buffer size of 24 bytes to hold the temporary encrypted output
       which will be encoded.
     Encrypt and encode as many 24-byte blocks as possible.
   */
  for (i = 0; i < inputLen / 24; ++i) {
    /* Assume part out length will equal part in length since it is
         a multiple of 8.  Also assume no error output. */
    R_SealUpdate (context, encryptedPart, &tempLen, &input[24*i], 24);

    /* len is always 32 */
    R_EncodePEMBlock (&output[32*i], &tempLen, encryptedPart, 24);
  }
  
  /* Encrypt the last part into encryptedPart.
   */  
  R_SealUpdate
    (context, encryptedPart, &lastPartLen, &input[24*i], inputLen - 24*i);
  R_SealFinal (context, encryptedPart + lastPartLen, &len);
  lastPartLen += len;

  R_EncodePEMBlock (&output[32*i], &len, encryptedPart, lastPartLen);
  *outputLen = 32*i + len;

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)encryptedPart, 0, sizeof (encryptedPart));
}

static NEINT32 DecryptPEMUpdateFinal (context, output, outputLen, input, inputLen)
R_ENVELOPE_CTX *context;
NEUINT8 *output;                          /* decoded, decrypted block */
NEUINT32 *outputLen;                                /* length of output */
NEUINT8 *input;                           /* encrypted, encoded block */
NEUINT32 inputLen;                                            /* length */
{
  NEINT32 status;
  NEUINT8 encryptedPart[24];
  NEUINT32 i, len;
  
  do {
    /* Choose a buffer size of 24 bytes to hold the temporary decoded output
         which will be decrypted.
       Decode and decrypt as many 32-byte input blocks as possible.
     */
    *outputLen = 0;
    for (i = 0; i < inputLen/32; i++) {
      /* len is always 24 */
      if ((status = R_DecodePEMBlock
           (encryptedPart, &len, &input[32*i], 32)) != 0)
        break;

      /* Excpect no error return */
      R_OpenUpdate (context, output, &len, encryptedPart, 24);
      output += len;
      *outputLen += len;
    }
    if (status)
      break;

    /* Decode the last part */  
    if ((status = R_DecodePEMBlock
         (encryptedPart, &len, &input[32*i], inputLen - 32*i)) != 0)
      break;

    /* Decrypt the last part.
     */
    R_OpenUpdate (context, output, &len, encryptedPart, len);
    output += len;
    *outputLen += len;
    if ((status = R_OpenFinal (context, output, &len)) != 0)
      break;
    *outputLen += len;
  } while (0);

  /* Zeroize sensitive information.
   */
  R_memset ((POINTER)&context, 0, sizeof (context));
  R_memset ((POINTER)encryptedPart, 0, sizeof (encryptedPart));

  return (status);
}

static NEINT32 CipherInit (context, encryptionAlgorithm, key, iv, encrypt)
R_ENVELOPE_CTX *context;
NEINT32 encryptionAlgorithm;
NEUINT8 *key;                                              /* DES key */
NEUINT8 *iv;                             /* DES initialization vector */
NEINT32 encrypt;                     /* encrypt flag (1 = encrypt, 0 = decrypt) */
{
  switch (encryptionAlgorithm) {
  case EA_DES_CBC:
    DES_CBCInit (&context->cipherContext.des, key, iv, encrypt);
    return (0);
  case EA_DESX_CBC:
    DESX_CBCInit (&context->cipherContext.desx, key, iv, encrypt);
    return (0);
  case EA_DES_EDE2_CBC:
  case EA_DES_EDE3_CBC:
    DES3_CBCInit (&context->cipherContext.des3, key, iv, encrypt);
    return (0);

  default:
    return (RE_ENCRYPTION_ALGORITHM);
  }
}

/* Assume len is a multiple of 8.
 */
static void CipherUpdate (context, output, input, len)
R_ENVELOPE_CTX *context;
NEUINT8 *output;                                      /* output block */
NEUINT8 *input;                                        /* input block */
NEUINT32 len;                      /* length of input and output blocks */
{
  if (context->encryptionAlgorithm == EA_DES_CBC)
    DES_CBCUpdate (&context->cipherContext.des, output, input, len);
  else if (context->encryptionAlgorithm == EA_DESX_CBC)
    DESX_CBCUpdate (&context->cipherContext.desx, output, input, len);
  else
    DES3_CBCUpdate (&context->cipherContext.des3, output, input, len);
}

static void CipherRestart (context)
R_ENVELOPE_CTX *context;
{
  if (context->encryptionAlgorithm == EA_DES_CBC)
    DES_CBCRestart (&context->cipherContext.des);
  else if (context->encryptionAlgorithm == EA_DESX_CBC)
    DESX_CBCRestart (&context->cipherContext.desx);
  else
    DES3_CBCRestart (&context->cipherContext.des3);
}