
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

#include "ProxyShuffler.h"
#include <boost/enable_shared_from_this.hpp>

using namespace boost::asio;

ProxyShuffler::ProxyShuffler(boost::shared_ptr<ShuffleStream> socks,
			     boost::shared_ptr<ShuffleStream> node)
  : socks(socks), node(node), closed(false)
{}

void ProxyShuffler::shuffle() {
  socks->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(), 
			  socks, node, socksReadBuffer, _1, _2));

  node->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(),
			 node, socks, nodeReadBuffer, _1, _2));
}

void ProxyShuffler::readComplete(boost::shared_ptr<ShuffleStream> thisStream,
				 boost::shared_ptr<ShuffleStream> thatStream,
				 unsigned char* thisBuffer,
				 unsigned char* buf, std::size_t transferred) 
{
  if (closed) return;

  if (transferred == -1) {
    close();
    return;
  }

  assert(transferred <= PROXY_BUF_SIZE);

  memcpy(thisBuffer, buf, transferred);

  thatStream->write(thisBuffer, transferred,
		    boost::bind(&ProxyShuffler::writeComplete, shared_from_this(),
				thisStream, thatStream, thisBuffer, placeholders::error));
}

void ProxyShuffler::writeComplete(boost::shared_ptr<ShuffleStream> thisStream,
				  boost::shared_ptr<ShuffleStream> thatStream,
				  unsigned char *buf,
				  const boost::system::error_code &err)
{
  if (closed) return;

  if (err) {
    close();
    return;
  }

  thisStream->read(boost::bind(&ProxyShuffler::readComplete, shared_from_this(),
			       thisStream, thatStream, buf, _1, _2));
}

void ProxyShuffler::close() {
  if (!closed) {
    node->close();
    socks->close();
    closed = true;
  }
}
