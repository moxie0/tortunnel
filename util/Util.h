#ifndef __UTIL_H__
#define __UTIL_H__

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

#include <stdint.h>
#include <string>
#include <string.h>
#include <vector>

class Util {

 public:
  static void int64ToArrayBigEndian(unsigned char *a, uint64_t i);
  static void int32ToArrayBigEndian(unsigned char *a, uint32_t i);
  static void int16ToArrayBigEndian(unsigned char *a, uint32_t i);

  static uint16_t bigEndianArrayToShort(unsigned char *buf);
  static uint32_t bigEndianArrayToInt(unsigned char *buf);

  static void hexDump(unsigned char *buf, int length);
  static void hexStringToChar(unsigned char *buffer, int len, std::string &hexString);
  static std::string charToHexString(const unsigned char* buffer, size_t size);

  static void base16_encode(char *dest, size_t destlen, const char *src, size_t srclen);
  static int base64_decode(char* dest, int destlen, const char *input, int length);

  static void tokenizeString(std::string &str, 
			     std::string &delimiters, 
			     std::vector<std::string> &tokens);
  static uint16_t getRandomId();
  static uint32_t getRandom();
};

#endif
