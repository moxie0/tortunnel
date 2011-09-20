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

#include "Util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iomanip>

#include <openssl/rand.h>
#include <boost/assert.hpp>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>

#undef DEBUG

#define X 255
#define SP 64
#define PAD 65

static const uint8_t base64_decode_table[256] = {
  X, X, X, X, X, X, X, X, X, SP, SP, SP, X, SP, X, X, /* */
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  SP, X, X, X, X, X, X, X, X, X, X, 62, X, X, X, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, X, X, X, PAD, X, X,
  X, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, X, X, X, X, X,
  X, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,
};

uint16_t Util::bigEndianArrayToShort(unsigned char *buf) {
  return (uint16_t)((buf[0] & 0xff) << 8 | (buf[1] & 0xff));
}

uint32_t Util::bigEndianArrayToInt(unsigned char *buf) {
  return (uint32_t)((buf[0] & 0xff) << 24 | (buf[1] & 0xff) << 16 | 
		    (buf[2] & 0xff) << 8 | (buf[3] & 0xff));
}

void Util::int64ToArrayBigEndian(unsigned char *a, uint64_t i) {
  a[0] = (i >> 56) & 0xFF;
  a[1] = (i >> 48) & 0xFF;
  a[2] = (i >> 40) & 0xFF;
  a[3] = (i >> 32) & 0xFF;
  a[4] = (i >> 24) & 0xFF;
  a[5] = (i >> 16) & 0xFF;
  a[6] = (i >> 8)  & 0xFF;
  a[7] = i         & 0xFF;
}


void Util::int32ToArrayBigEndian(unsigned char *a, uint32_t i) {
  a[0] = (i >> 24) & 0xFF;
  a[1] = (i >> 16) & 0xFF;
  a[2] = (i >> 8)  & 0xFF;
  a[3] = i         & 0xFF;
}

void Util::int16ToArrayBigEndian(unsigned char *a, uint32_t i) {
  a[0] = (i >> 8) & 0xff;
  a[1] = i        & 0xff;
}

void Util::hexDump(unsigned char *buf, int length) {
  char *output = (char*)malloc((length * 3) + 1);
  int i;
  for (i=0;i<length;i++) {
    sprintf(output+(i*3), "%02X ", *(buf+i));
  }

  fprintf(stderr, "%s\n", output);
  free(output);
}

void Util::hexStringToChar(unsigned char *buffer, int len, std::string &hexString) {
  assert(hexString.length() / 2 == len);

  char nibble[9];
  nibble[8] = '\0';

  uint32_t nibbleConversion;

  for (int i=0;i<len/4;i++) {
    memcpy(nibble, hexString.c_str()+(i*8), 8);
    sscanf(nibble, "%x", &nibbleConversion);
    Util::int32ToArrayBigEndian(buffer+(i*4), nibbleConversion);
  }

  memset(nibble, 0, sizeof(nibble));
  nibbleConversion = 0;
}

std::string Util::charToHexString(const unsigned char* buffer, size_t size) {
  std::stringstream str;
  str.setf(std::ios_base::hex, std::ios::basefield);
  str.fill('0');

  for (size_t i=0;i<size;++i) {
    str << std::setw(2) << (unsigned short)(unsigned char)buffer[i];
  }

  return str.str();
}

void Util::base16_encode(char *dest, size_t destlen, const char *src, size_t srclen)
{
  const char *end;
  char *cp;

  assert(destlen >= srclen*2+1);

  cp = dest;
  end = src+srclen;
  while (src<end) {
    *cp++ = "0123456789ABCDEF"[ (*(const uint8_t*)src) >> 4 ];
    *cp++ = "0123456789ABCDEF"[ (*(const uint8_t*)src) & 0xf ];
    ++src;
  }
  *cp = '\0';
}

int Util::base64_decode(char* dest, int destlen, const char *input, int length)
{
  const char *eos = input+length;
  uint32_t n=0;
  int n_idx=0;
  char *dest_orig = dest;

  /* Max number of bits == length*6.
   * Number of bytes required to hold all bits == (length*6)/8.
   * Yes, we want to round down: anything that hangs over the end of a
   * byte is padding. */
  if (destlen < (length*3)/4)
    return -1;

  /* Iterate over all the bytes in src.  Each one will add 0 or 6 bits to the
   * value we're decoding.  Accumulate bits in <b>n</b>, and whenever we have
   * 24 bits, batch them into 3 bytes and flush those bytes to dest.
   */
  for ( ; input < eos; ++input) {
    unsigned char c = (unsigned char) *input;
    uint8_t v = base64_decode_table[c];
    switch (v) {
      case X:
        /* This character isn't allowed in base64. */
        return -1;
      case SP:
        /* This character is whitespace, and has no effect. */
        continue;
      case PAD:
        /* We've hit an = character: the data is over. */
        goto end_of_loop;
      default:
        /* We have an actual 6-bit value.  Append it to the bits in n. */
        n = (n<<6) | v;
        if ((++n_idx) == 4) {
          /* We've accumulated 24 bits in n. Flush them. */
          *dest++ = (n>>16);
          *dest++ = (n>>8) & 0xff;
          *dest++ = (n) & 0xff;
          n_idx = 0;
          n = 0;
        }
    }
  }
 end_of_loop:
  /* If we have leftover bits, we need to cope. */
  switch (n_idx) {
    case 0:
    default:
      /* No leftover bits.  We win. */
      break;
    case 1:
      /* 6 leftover bits. That's invalid; we can't form a byte out of that. */
      return -1;
    case 2:
      /* 12 leftover bits: The last 4 are padding and the first 8 are data. */
      *dest++ = n >> 4;
      break;
    case 3:
      /* 18 leftover bits: The last 2 are padding and the first 16 are data. */
      *dest++ = n >> 10;
      *dest++ = n >> 2;
  }

  assert((dest-dest_orig) <= (ssize_t)destlen);

  return (int)(dest-dest_orig);
}


void Util::tokenizeString(std::string &str, 
			  std::string &delimiters, 
			  std::vector<std::string> &tokens) 
{
  std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
  std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
  
  while (std::string::npos != pos || std::string::npos != lastPos) {
    tokens.push_back(str.substr(lastPos, pos - lastPos));
    lastPos = str.find_first_not_of(delimiters, pos);
    pos     = str.find_first_of(delimiters, lastPos);
  }
  
  if (lastPos != std::string::npos)
    tokens.push_back(str.substr(lastPos)); 
}

uint16_t Util::getRandomId() {
  unsigned char id[2];
  RAND_bytes(id, sizeof(id));  
  
  return bigEndianArrayToShort(id);
}

uint32_t Util::getRandom() {
  unsigned char bytes[4];
  RAND_bytes(bytes, sizeof(bytes));

  return bigEndianArrayToInt(bytes);
}
