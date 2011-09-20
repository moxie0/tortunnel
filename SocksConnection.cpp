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

#include "SocksConnection.h"

using namespace boost::asio;

SocksConnection::SocksConnection(boost::shared_ptr<ip::tcp::socket> socket) 
  : socket(socket)
{}
				 
void SocksConnection::getRequest(SocksRequestHandler handler) {
  boost::asio::async_read(*socket, buffer(data, VERSION_HEADER_LEN),
			  boost::bind(&SocksConnection::readVersionHeaderComplete,
				      shared_from_this(), handler, 
				      placeholders::error,
				      placeholders::bytes_transferred));
}

void SocksConnection::readVersionHeaderComplete(SocksRequestHandler handler, 
						const boost::system::error_code &err, 
						std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == VERSION_HEADER_LEN);

  version     = data[0];
  methodCount = data[1];

  if (version != 0x05) {
    handler(host, port, boost::asio::error::access_denied);
    return;
  }

  assert(methodCount < sizeof(data));

  boost::asio::async_read(*socket, buffer(data, methodCount),
			  boost::bind(&SocksConnection::readMethodComplete,
				      shared_from_this(), handler, 
				      placeholders::error,
				      placeholders::bytes_transferred));  
}

void SocksConnection::readMethodComplete(SocksRequestHandler handler, 
					 const boost::system::error_code &err, 
					 std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }
  
  assert(transferred == methodCount);

  int i;
  for (i=0;i<methodCount;i++) {
    if (data[i] == 0x00) {
      sendValidMethodResponse(handler);
      return;
    }
  }

  handler(host, port, boost::asio::error::access_denied);  
}

void SocksConnection::sendValidMethodResponse(SocksRequestHandler handler) {
  data[0] = 0x05;
  data[1] = 0x00;

  async_write(*socket, boost::asio::buffer(data, METHOD_RESPONSE_LEN),
	      boost::bind(&SocksConnection::methodResponseComplete, shared_from_this(),
			  handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksConnection::methodResponseComplete(SocksRequestHandler handler, 
					     const boost::system::error_code &err,
					     std::size_t transferred) 
{
  

  if (err) handler(host, port, err);
  else     getConnectionRequest(handler);
}


void SocksConnection::getConnectionRequest(SocksRequestHandler handler) {
  async_read(*socket, boost::asio::buffer(data, REQUEST_HEADER_LEN),
	     boost::bind(&SocksConnection::readRequestComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}


void SocksConnection::readRequestComplete(SocksRequestHandler handler,
					  const boost::system::error_code &err,
					  std::size_t transferred) 
{
  if (err) {
    handler(host, port, err);
    return;
  }
  
  assert(transferred == REQUEST_HEADER_LEN);
  assert(data[0] == 0x05);
  assert(data[2] == 0x00);

  command     = data[1];
  addressType = data[3];
  
  if (command != 0x01) {
    handler(host, port, boost::asio::error::access_denied);
    return;
  }  

  if      (addressType == 0x01) readIPv4Address(handler);
  else if (addressType == 0x03) readDomainAddress(handler);
  else                          handler(host, port, boost::asio::error::access_denied);  
}

void SocksConnection::readDomainAddressComplete(SocksRequestHandler handler,
						const boost::system::error_code &err,
						std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == domainAddressLength + 2);

  host = std::string((const char*)data, (std::size_t)domainAddressLength);
  port = Util::bigEndianArrayToShort(data+domainAddressLength);

  handler(host, port, err);
}

void SocksConnection::readDomainAddressHeaderComplete(SocksRequestHandler handler,
						      const boost::system::error_code &err,
						      std::size_t transferred)
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == DOMAIN_ADDRESS_HEADER_LEN);

  domainAddressLength = data[0];

  async_read(*socket, buffer(data, domainAddressLength + 2),
	     boost::bind(&SocksConnection::readDomainAddressComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksConnection::readDomainAddress(SocksRequestHandler handler) {
  async_read(*socket, buffer(data, DOMAIN_ADDRESS_HEADER_LEN), 
	     boost::bind(&SocksConnection::readDomainAddressHeaderComplete, shared_from_this(),
			 handler, placeholders::error, placeholders::bytes_transferred));
}

void SocksConnection::readIPv4Address(SocksRequestHandler handler) {
  async_read(*socket, buffer(data, IPV4_ADDRESS_LEN), 
	     boost::bind(&SocksConnection::readIPv4AddressComplete,
			 shared_from_this(), handler, 
			 placeholders::error, 
			 placeholders::bytes_transferred));
}
				      
void SocksConnection::readIPv4AddressComplete(SocksRequestHandler handler,
					      const boost::system::error_code &err,
					      std::size_t transferred) 
{
  if (err) {
    handler(host, port, err);
    return;
  }

  assert(transferred == IPV4_ADDRESS_LEN);

  boost::array<unsigned char, 4> hostBytes = {data[0], data[1], data[2], data[3]};
  host                                     = ip::address_v4(hostBytes).to_string();
  port                                     = Util::bigEndianArrayToShort(data+4);

  handler(host, port, err);
}

void SocksConnection::read(StreamReadHandler handler) {
  socket->async_read_some(buffer(data, sizeof(data)), 
			  boost::bind(&SocksConnection::dataReadComplete, shared_from_this(), 
				      handler, 
				      placeholders::bytes_transferred, 
				      placeholders::error));
}

void SocksConnection::dataReadComplete(StreamReadHandler handler, 
				       std::size_t transferred,
				       const boost::system::error_code &err)
{
  if (err) handler(data, -1);
  else     handler(data, transferred);
}

void SocksConnection::write(unsigned char* buf, int length, StreamWriteHandler handler)
{
  async_write(*socket, buffer(buf, length), boost::bind(handler, placeholders::error));
}

void SocksConnection::close() {
  socket->close();
}

void SocksConnection::respondConnectedComplete(const boost::system::error_code &err) {}

void SocksConnection::respondConnected(ip::tcp::endpoint local) {
  data[0] = 0x05;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x01;

  boost::array< unsigned char, 4 > localAddress = local.address().to_v4().to_bytes();
  data[4] = localAddress[0];
  data[5] = localAddress[1];
  data[6] = localAddress[2];
  data[7] = localAddress[3];

  Util::int16ToArrayBigEndian(data+8, local.port());

  async_write(*socket, buffer(data, 10), 
	      boost::bind(&SocksConnection::respondConnectedComplete,
			  shared_from_this(), placeholders::error));  
}


void SocksConnection::respondConnectErrorComplete(const boost::system::error_code &err) {}

void SocksConnection::respondConnectError() {
  data[0] = 0x05;
  data[1] = 0x04;
  data[2] = 0x00;
  data[3] = 0x01;
  data[4] = 0x7F;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x01;
  data[8] = 0x04;
  data[9] = 0x00;

  async_write(*socket, buffer(data, 10), 
	      boost::bind(&SocksConnection::respondConnectErrorComplete,
			  shared_from_this(), placeholders::error));
}
