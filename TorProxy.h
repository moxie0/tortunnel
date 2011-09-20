#ifndef __TOR_PROXY_H__
#define __TOR_PROXY_H__

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

#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "TorTunnel.h"
#include "SocksConnection.h"
#include "ProxyShuffler.h"

using namespace boost::asio;

/***********
 *
 * TorProxy builds a tor tunnel directly to an exit node and
 * sets up a SOCKS proxy to shuttle requests into it.  Most useful
 * for running existing applications through a tor tunnel.
 *
 **********/


class TorProxy {

 private:
  ip::tcp::acceptor acceptor;
  TorTunnel *tunnel;

  void acceptIncomingConnection();
  void handleIncomingConnection(boost::shared_ptr<ip::tcp::socket> socket,
				const boost::system::error_code &err);
  
  void handleSocksRequest(boost::shared_ptr<SocksConnection> connection,
			  std::string &host,
			  uint16_t port,
			  const boost::system::error_code &err);

  void handleStreamOpen(boost::shared_ptr<SocksConnection> socks,
			boost::shared_ptr<TorTunnelStream> stream,
			const boost::system::error_code &err);

 public:

  TorProxy(TorTunnel *tunnel, boost::asio::io_service &io_service, int listenPort);
    
};

typedef struct {
  std::string host;
  int port;
  int random;
} Arguments;


#endif
