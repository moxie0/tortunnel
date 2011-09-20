/*-
 * Copyright (c) 2009, Moxie Marlinspike
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of this program nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "HybridEncryption.h"

#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define PADDING_OVERHEAD 42
#define AES_KEY_SIZE     128/8

void HybridEncryption::AES_encrypt(unsigned char *keyMaterial, int keyLength,
				   unsigned char *out, unsigned char *in, int len) 
{
  AES_KEY key;
  unsigned char ivec[AES_BLOCK_SIZE];
  unsigned char ecount_buf[AES_BLOCK_SIZE];
  unsigned int num = 0;

  memset(ivec, 0, sizeof(ivec));
  memset(ecount_buf, 0, sizeof(ecount_buf));

  AES_set_encrypt_key(keyMaterial, keyLength*8, &key);
  AES_ctr128_encrypt(in, out, len, &key, ivec, ecount_buf, &num);
}

void HybridEncryption::encryptInHybridChunk(unsigned char* plaintext, int plaintextLength,
					    unsigned char** encrypted, int *encryptedLength,
					    RSA *rsa)
{
  int pkSize            = RSA_size(rsa);
  int firstChunkLength  = pkSize - PADDING_OVERHEAD - AES_KEY_SIZE;
  int secondChunkLength = plaintextLength - firstChunkLength;

  *encryptedLength = pkSize + secondChunkLength;
  *encrypted       = (unsigned char*)malloc(*encryptedLength);

  unsigned char symmetricKey[AES_KEY_SIZE];
  RAND_bytes(symmetricKey, sizeof(symmetricKey));

  unsigned char *rsaEnvelope = (unsigned char*)malloc(pkSize - PADDING_OVERHEAD);
  memcpy(rsaEnvelope, symmetricKey, sizeof(symmetricKey));
  memcpy(rsaEnvelope+sizeof(symmetricKey), plaintext, firstChunkLength);

  RSA_public_encrypt(pkSize - PADDING_OVERHEAD, rsaEnvelope, 
		     *encrypted, rsa, RSA_PKCS1_OAEP_PADDING);

  AES_encrypt(symmetricKey, sizeof(symmetricKey), (*encrypted) + pkSize,
	      plaintext + firstChunkLength, secondChunkLength);

  free(rsaEnvelope);
}

void HybridEncryption::encryptInSingleChunk(unsigned char* plaintext, int plaintextLength,
					    unsigned char** encrypted, int *encryptedLength,
					    RSA *rsa)
{
  *encryptedLength = RSA_size(rsa);
  *encrypted       = (unsigned char*)malloc(*encryptedLength);
  
  RSA_public_encrypt(plaintextLength, plaintext, *encrypted, rsa, RSA_PKCS1_OAEP_PADDING);
}

void HybridEncryption::encrypt(unsigned char *plaintext, int plaintextLength,
			       unsigned char **encrypted, int *encryptedLength,
			       RSA *rsa) 
{
  if (plaintextLength < (RSA_size(rsa) - PADDING_OVERHEAD)) {
    encryptInSingleChunk(plaintext, plaintextLength, encrypted, encryptedLength, rsa);
  } else {
    encryptInHybridChunk(plaintext, plaintextLength, encrypted, encryptedLength, rsa);
  }
}
