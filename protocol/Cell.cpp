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


#include "../util/Util.h"
#include "Cell.h"

Cell::Cell(uint16_t id, unsigned char type) {
  memset(buffer, 0, sizeof(buffer));

  Util::int16ToArrayBigEndian(buffer, id);
  buffer[2] = type;
  index     = 3;
}

Cell::Cell() {
  index = 3;
}

unsigned char Cell::getType() {
  return buffer[2];
}

void Cell::append(uint16_t val) {
  Util::int16ToArrayBigEndian(buffer+index, val);
  index+=2;
}

void Cell::append(uint32_t val) {
  Util::int32ToArrayBigEndian(buffer+index, val);
  index+=4;
}

void Cell::append(unsigned char val) {
  buffer[index++] = val;
}

void Cell::append(string &val) {
  //  buffer[index++] = (unsigned char)val.length();
  memcpy(buffer+index, (unsigned char*)val.c_str(), val.length());
  index+= val.length();
}

void Cell::append(unsigned char *segment, int length) {
  memcpy(buffer+index, segment, length);
  index+=length;
}

uint32_t Cell::readInt() {
  uint32_t val = Util::bigEndianArrayToInt(buffer+index);
  index       += 4;

  return val;
}

unsigned char Cell::readByte() {
  return buffer[index++];
}

string Cell::readString() {
  unsigned char len = buffer[index++];
  string val((const char*)buffer+index, (size_t)len);
  index += (int)len;
  return val;
}

unsigned char* Cell::getPayload() {
  return buffer+3;
}

int Cell::getPayloadSize() {
  return sizeof(buffer)-3;
}

unsigned char* Cell::getBuffer() {
  return buffer;
}

int Cell::getBufferSize() {
  return sizeof(buffer);
}

bool Cell::isRelayCell() {
  return getType() == 0x03;
}

bool Cell::isPaddingCell() {
  return getType() == 0x00;
}
