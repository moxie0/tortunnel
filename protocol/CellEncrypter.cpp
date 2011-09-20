
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


#include "CellEncrypter.h"
#include <cassert>
#include <openssl/sha.h>

#include "../util/Util.h"

#define TOTAL_KEY_MATERIAL (20*3+16*2)
#define DIGEST_LEN 20
#define KEY_LEN 128/8

#define MIN(a,b) ((a)<(b)?(a):(b))

CellEncrypter::CellEncrypter() : forwardAesNum(0), backAesNum(0) {
  memset(forwardIV, 0, sizeof(forwardIV));
  memset(forwardEC, 0, sizeof(forwardEC));
  memset(backIV, 0, sizeof(backIV));
  memset(backEC, 0, sizeof(backEC));

  SHA1_Init(&forwardDigest);
  SHA1_Init(&backDigest);
}

void CellEncrypter::expandKeyMaterial(unsigned char* keyMaterial, int keyMaterialLength,
				      unsigned char* expanded, int expandedLength)
{
  unsigned char *buf = (unsigned char*)malloc(keyMaterialLength + 1);
  unsigned char digest[DIGEST_LEN];
  unsigned char *expandedPointer;
  int i;

  memcpy(buf, keyMaterial, keyMaterialLength);

  for (expandedPointer = expanded, i=0; 
       expandedPointer < expanded + expandedLength; 
       ++i, expandedPointer += DIGEST_LEN) 
    {
      buf[keyMaterialLength] = i;
      SHA1(buf, keyMaterialLength + 1, digest);

//       std::cerr << "Finished expansion hash: " << std::endl;

//       Util::hexDump(digest, sizeof(digest));

      memcpy(expandedPointer, digest, 
	     MIN(DIGEST_LEN, expandedLength-(expandedPointer-expanded)));
    }

  memset(buf, 0, keyMaterialLength + 1);
  memset(digest, 0, sizeof(digest));

  free(buf);
}

void CellEncrypter::verifyKeyMaterial(unsigned char *material, unsigned char *challenge) {
  if (memcmp(material, challenge, DIGEST_LEN)) {
    throw CryptoMismatchException();
  }
}

void CellEncrypter::initKeyMaterial(unsigned char *material) {
  SHA1_Update(&forwardDigest, material+DIGEST_LEN, DIGEST_LEN);
  SHA1_Update(&backDigest, material+(DIGEST_LEN*2), DIGEST_LEN);
  
  AES_set_encrypt_key(material+(DIGEST_LEN*3), KEY_LEN*8, &forwardKey);
  AES_set_encrypt_key(material+(DIGEST_LEN*3)+KEY_LEN, KEY_LEN*8, &backKey);
}

void CellEncrypter::setKeyMaterial(unsigned char *keyMaterial, int keyMaterialLength,
				   unsigned char *challenge) 
{
  unsigned char expanded[TOTAL_KEY_MATERIAL];

//   std::cerr << "Expanding key material to length: " << TOTAL_KEY_MATERIAL << std::endl;
  
  expandKeyMaterial(keyMaterial, keyMaterialLength, expanded, sizeof(expanded));
  verifyKeyMaterial(expanded, challenge);  
  initKeyMaterial(expanded);
}

void CellEncrypter::aesOperate(Cell &cell,
			       AES_KEY *key,
			       unsigned char* iv, 
			       unsigned char *ec, 
			       unsigned int *num) 
{
  unsigned char buf[512];

  unsigned char *cellPayload = cell.getPayload();
  int cellPayloadLength      = cell.getPayloadSize();
  
  assert(cellPayloadLength < sizeof(buf));

  AES_ctr128_encrypt(cellPayload, buf, cellPayloadLength, key, iv, ec, num);  
  memcpy(cellPayload, buf, cellPayloadLength);
}

void CellEncrypter::calculateDigest(SHA_CTX *digest, 
				    RelayCell &cell,
				    unsigned char *result) 
{
  SHA1_Update(digest, cell.getPayload(), cell.getPayloadSize());

  SHA_CTX temp;
  memcpy(&temp, digest, sizeof(SHA_CTX));

  SHA1_Final(result, &temp);
}

void CellEncrypter::setDigestForCell(RelayCell &cell) {
  unsigned char buf[DIGEST_LEN];
    
  calculateDigest(&forwardDigest, cell, buf);
  cell.setDigest(buf);
}

void CellEncrypter::verifyDigestForCell(RelayCell &cell) {
  unsigned char zeroDigest[4];
  unsigned char receivedDigest[4];
  unsigned char calculatedDigest[DIGEST_LEN];

  memset(zeroDigest, 0, sizeof(zeroDigest));
  cell.getDigest(receivedDigest);
  cell.setDigest(zeroDigest);

  calculateDigest(&backDigest, cell, calculatedDigest);

  if (memcmp(receivedDigest, calculatedDigest, sizeof(receivedDigest)))
    throw CryptoMismatchException();
}

void CellEncrypter::encrypt(RelayCell &cell) {
  setDigestForCell(cell);

//   std::cerr << "Cell Before Encryption: " << std::endl;
//   Util::hexDump(cell.getBuffer(), cell.getBufferSize());

  aesOperate(cell, &forwardKey, forwardIV, forwardEC, &forwardAesNum);
}

void CellEncrypter::decrypt(RelayCell &cell) {
  aesOperate(cell, &backKey, backIV, backEC, &backAesNum);
  verifyDigestForCell(cell);
}
