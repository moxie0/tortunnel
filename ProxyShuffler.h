#ifndef __PROXY_SHUFFLER_H__
#define __PROXY_SHUFFLER_H__

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


#include "ShuffleStream.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#define PROXY_BUF_SIZE 512

/***********
 *
 * This class shuffles data back and forth between two ShuffleStreams.
 *
 **********/


class ProxyShuffler : public boost::enable_shared_from_this<ProxyShuffler> {
  
 private:
  boost::shared_ptr<ShuffleStream> socks;
  boost::shared_ptr<ShuffleStream> node;

  unsigned char socksReadBuffer[PROXY_BUF_SIZE];
  unsigned char nodeReadBuffer[PROXY_BUF_SIZE];

  bool closed;

  void readComplete(boost::shared_ptr<ShuffleStream> thisStream,
		    boost::shared_ptr<ShuffleStream> thatStream,
		    unsigned char* thisBuffer,
		    unsigned char* buf, std::size_t transferred);

  void writeComplete(boost::shared_ptr<ShuffleStream> thisStream,
		     boost::shared_ptr<ShuffleStream> thatStream,
		     unsigned char *buf,
		     const boost::system::error_code &err);

  void close();


 public:
 ProxyShuffler(boost::shared_ptr<ShuffleStream> socks,
	       boost::shared_ptr<ShuffleStream> node);

 void shuffle();
  
};


#endif
