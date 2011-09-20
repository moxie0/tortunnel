#ifndef __TOR_TUNNEL_H__
#define __TOR_TUNNEL_H__

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

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "ShuffleStream.h"
#include "protocol/Directory.h"
#include "protocol/ServerListing.h"
#include "protocol/Connection.h"
#include "protocol/Circuit.h"

using namespace boost::asio;

class TorTunnelStream;

typedef boost::function<void (const boost::system::error_code &error)> TunnelConnectHandler;
typedef boost::function<void (boost::shared_ptr<TorTunnelStream> stream, const boost::system::error_code &error)> TunnelStreamHandler;
typedef boost::function<void (const boost::system::error_code &error)> TorTunnelErrorHandler;

class TorTunnel : public CircuitErrorListener {

  friend class TorTunnelStream;

 private:
  boost::asio::io_service &io_service;
  boost::shared_ptr<ServerListing> serverListing;
  boost::shared_ptr<Circuit> circuit;

  TorTunnelErrorHandler errorHandler;

  void nodeConnectionComplete(TunnelConnectHandler handler,
			      const boost::system::error_code &err);

  void openStreamComplete(TunnelStreamHandler handler, uint16_t streamId,
			  const boost::system::error_code &err);

  void directoryListingComplete(TunnelConnectHandler handler, 
				const boost::system::error_code &err);

 public:
  TorTunnel(boost::asio::io_service &io_service, 
	    boost::shared_ptr<ServerListing> serverListing, 
	    TorTunnelErrorHandler errorHandler);

  void close();
  void connect(TunnelConnectHandler handler);
  void openStream(std::string &host, uint16_t port, TunnelStreamHandler handler);
  void handleConnectionError(const boost::system::error_code &err);    
  void handleCircuitDestroyed();

  Connection nodeConnection;

};

class TorTunnelStream : public ShuffleStream {

 private:
  TorTunnel *tunnel;
  uint16_t streamId;

 public:
  TorTunnelStream(uint16_t streamId, TorTunnel *tunnel) 
    : streamId(streamId), tunnel(tunnel)
  {}

  void write(unsigned char* buf, int len, StreamWriteHandler handler) {
    tunnel->circuit->write(streamId, buf, len, handler);
  }

  void read(StreamReadHandler handler) {
    tunnel->circuit->read(streamId, handler);
  }

  void close() {
    tunnel->circuit->close(streamId);
  }

  std::string getRemoteNodeAddress() {
    return tunnel->circuit->getRemoteNodeAddress();
  }

  ip::tcp::endpoint getLocalEndpoint() {
    return tunnel->circuit->getLocalEndpoint();
  }

};


#endif
