#ifndef __CELL_ENCRYPTER_H__
#define __CELL_ENCRYPTER_H__

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


#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include "RelayCell.h"
#include "Cell.h"

class CryptoMismatchException : public std::exception {
public:
  virtual const char* what() const throw() {
    return "Computed digest does not match server's...";
  }
};

class CellEncrypter {

 private:
    AES_KEY forwardKey;
    AES_KEY backKey;

    unsigned char forwardIV[AES_BLOCK_SIZE];
    unsigned char forwardEC[AES_BLOCK_SIZE];
    unsigned int forwardAesNum;

    unsigned char backIV[AES_BLOCK_SIZE];
    unsigned char backEC[AES_BLOCK_SIZE];
    unsigned int backAesNum;
    
    SHA_CTX forwardDigest;
    SHA_CTX backDigest;
    
    void expandKeyMaterial(unsigned char* keyMaterial, int keyMaterialLength,
			   unsigned char* expanded, int expandedLength);

    void verifyKeyMaterial(unsigned char *material, unsigned char *challenge);

    void initKeyMaterial(unsigned char *material);


    void aesOperate(Cell &cell, 
		    AES_KEY *key,
		    unsigned char* iv, 
		    unsigned char *ec, 
		    unsigned int *num);

    void calculateDigest(SHA_CTX *digest, 
			 RelayCell &cell,
			 unsigned char *result);    

    void setDigestForCell(RelayCell &cell);
    void verifyDigestForCell(RelayCell &cell);

 public:
    CellEncrypter();


    void setKeyMaterial(unsigned char *keyMaterial, int keyMaterialLength,
			unsigned char *challenge);

    void encrypt(RelayCell &cell);
    void decrypt(RelayCell &cell);
};



#endif
