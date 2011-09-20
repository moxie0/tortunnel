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

#include "TorTunnel.h"
#include "util/Util.h"

#include <boost/lexical_cast.hpp>

TorTunnel::TorTunnel(boost::asio::io_service &io_service,
		     boost::shared_ptr<ServerListing> serverListing,
		     TorTunnelErrorHandler errorHandler) :
  io_service(io_service), serverListing(serverListing), errorHandler(errorHandler),
  nodeConnection(io_service, serverListing->getAddress(), serverListing->getPort())
{}

void TorTunnel::close() {
  circuit->close();
  nodeConnection.close();
}

void TorTunnel::connect(TunnelConnectHandler handler) {
  nodeConnection.connect(boost::bind(&TorTunnel::nodeConnectionComplete, 
				     this, handler, placeholders::error));
}

void TorTunnel::nodeConnectionComplete(TunnelConnectHandler handler,
				       const boost::system::error_code &err) 
{
  if (err) {
    handler(err);
    return;
  }

  std::cerr << "SSL Connection to node complete.  Setting up circuit." << std::endl;

  RSA *onionKey      = serverListing->getOnionKey();
  uint16_t circuitId = Util::getRandomId();
  circuit            = boost::shared_ptr<Circuit>(new Circuit(nodeConnection, onionKey, 
							      circuitId, this));

  circuit->create(boost::bind(handler, placeholders::error));
}

void TorTunnel::openStream(std::string &host, uint16_t port, TunnelStreamHandler handler) {
  uint16_t streamId  = Util::getRandomId();
  std::string destination(host);
  destination.append(":");
  destination.append(boost::lexical_cast<std::string>(port));

  circuit->connect(streamId, destination, boost::bind(&TorTunnel::openStreamComplete,
						      this, handler, streamId,
						      placeholders::error));
}

void TorTunnel::openStreamComplete(TunnelStreamHandler handler, uint16_t streamId,
				   const boost::system::error_code &err)
{
  boost::shared_ptr<TorTunnelStream> stream;

  if (err) {
    handler(stream, err);
    return;
  }

  stream = boost::shared_ptr<TorTunnelStream>(new TorTunnelStream(streamId, this));
  handler(stream, err);
}

void TorTunnel::handleConnectionError(const boost::system::error_code &err) {
  std::cerr << "Error with connection to Exit Node: " << err << std::endl;
  errorHandler(err);
}

void TorTunnel::handleCircuitDestroyed() {
  std::cerr << "Exit Node circuit destroyed." << std::endl;
  errorHandler(boost::system::error_code());
}

