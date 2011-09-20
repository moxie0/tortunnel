#ifndef __SOCKS_CONNECTION_H__
#define __SOCKS_CONNECTION_H__

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

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <cassert>
#include <iostream>
#include <string>

#include "util/Util.h"
#include "ShuffleStream.h"

/***********
 *
 * This class implements a SOCKS connection to a requesting client.
 *
 **********/

using namespace boost::asio;

typedef boost::function<void (std::string &host, uint16_t port, const boost::system::error_code &error)> SocksRequestHandler;

class SocksConnection : public ShuffleStream, public boost::enable_shared_from_this<SocksConnection> {

 private:
  boost::shared_ptr<ip::tcp::socket> socket;

  static const int VERSION_HEADER_LEN        = 2;
  static const int METHOD_RESPONSE_LEN       = 2;
  static const int REQUEST_HEADER_LEN        = 4;
  static const int DOMAIN_ADDRESS_HEADER_LEN = 1;
  static const int IPV4_ADDRESS_LEN          = 6;

  unsigned char version;
  unsigned char methodCount;
  unsigned char command;
  unsigned char addressType;
  unsigned char domainAddressLength;
  
  std::string host;
  uint16_t port;
  unsigned char data[512];

  void readVersionHeaderComplete(SocksRequestHandler handler, 
				 const boost::system::error_code &err, 
				 std::size_t transferred);

  void readMethodComplete(SocksRequestHandler handler, 
			  const boost::system::error_code &err, 
			  std::size_t transferred);

  void sendValidMethodResponse(SocksRequestHandler handler);

  void methodResponseComplete(SocksRequestHandler handler, 
			      const boost::system::error_code &err,
			      std::size_t transferred); 
  void readRequestComplete(SocksRequestHandler handler,
			   const boost::system::error_code &err,
			   std::size_t transferred);

  void getConnectionRequest(SocksRequestHandler handler);

  void readDomainAddressComplete(SocksRequestHandler handler,
				 const boost::system::error_code &err,
				 std::size_t transferred);

  void readDomainAddressHeaderComplete(SocksRequestHandler handler,
				       const boost::system::error_code &err,
				       std::size_t transferred);

  void readDomainAddress(SocksRequestHandler handler);
  void readIPv4Address(SocksRequestHandler handler);

  void readIPv4AddressComplete(SocksRequestHandler handler,
			       const boost::system::error_code &err,
			       std::size_t transferred);

  void read(StreamReadHandler handler);

  void dataReadComplete(StreamReadHandler handler, 
			std::size_t transferred,
			const boost::system::error_code &err);

  void write(unsigned char* buf, int length, StreamWriteHandler handler);
  void respondConnectErrorComplete(const boost::system::error_code &err);
  void respondConnectedComplete(const boost::system::error_code &err);

 public:
  SocksConnection(boost::shared_ptr<ip::tcp::socket> socket);
  void getRequest(SocksRequestHandler handler);
  void respondConnectError();
  void respondConnected(ip::tcp::endpoint local);
  void close();
};



#endif
