#ifndef __CELL_H__
#define __CELL_H__

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

#include <string>
#include <stdint.h>

#define NETINFO 0x08

/*
 * This class implements the basic Cell unit that is transmitted through
 * a Tor Tunnel.
 *
 */

using namespace std;

class Cell {
 protected:
  unsigned char buffer[512];
  int index;

 public:
  static const int PADDING_TYPE = 0;
  static const int CREATED_TYPE = 2;
  static const int RELAY_TYPE   = 3;
  static const int DESTROY_TYPE = 4;

  Cell(uint16_t id, unsigned char type);
  Cell();

  void append(uint16_t val);
  void append(uint32_t val);
  void append(unsigned char val);
  void append(string &val);
  void append(unsigned char *segment, int length);

  uint32_t readInt();
  unsigned char readByte();
  string readString();

  unsigned char* getBuffer();
  int getBufferSize();

  unsigned char* getPayload();
  int getPayloadSize();

  unsigned char getType();
  bool isRelayCell();
  bool isPaddingCell();

  virtual ~Cell() {}
};

#endif
