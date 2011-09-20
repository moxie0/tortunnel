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

#include "Connection.h"
#include "../util/Util.h"
#include "Cell.h"

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <stdint.h>
#include <time.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <boost/bind.hpp>

using namespace boost::asio;

Connection::Connection(io_service &io_service, string &host, string &port) 
  : socket(io_service), host(host), port(port)
{}

void Connection::connect(ConnectHandler handler) {
  initializeSSL();
  initiateConnection(host, atoi(port.c_str()), handler);
}

void Connection::initializeSSL() {
  SSL_load_error_strings();
  SSL_library_init();
  
  ctx = SSL_CTX_new(SSLv23_client_method());
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
  
  readBio  = BIO_new(BIO_s_mem());
  writeBio = BIO_new(BIO_s_mem());
  ssl      = SSL_new(ctx);

  SSL_set_bio(ssl, readBio, writeBio);    
  SSL_set_connect_state(ssl);
}

void Connection::writeCell(Cell &cell, ConnectHandler handler) {
  unsigned char *buffer = cell.getBuffer();
  int len               = cell.getBufferSize();

//   std::cerr << "Writing Cell: " << std::endl;
//   Util::hexDump(buffer, len);

  writeFully(buffer, len, handler, boost::system::error_code());
}

void Connection::readCell(boost::shared_ptr<Cell> cell, ConnectHandler handler) {
  unsigned char *buffer = cell->getBuffer();
  int len               = cell->getBufferSize();

  readFully(buffer, len, handler, boost::system::error_code());
}

void Connection::readFully(unsigned char *buf, int len, 
			   ConnectHandler handler, 
			   const boost::system::error_code err) 
{

  if(err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }
  
  while (len > 0) {
    int count = SSL_read(ssl, buf, len);

    switch (SSL_get_error(ssl, count)) {
    case SSL_ERROR_NONE:
//       std::cerr << "Partial Read (" << count << "): " << std::endl;
//       Util::hexDump(buf, count);
      
      len -= count;
      buf += count;      
      break;	
    case SSL_ERROR_WANT_READ:
      readIntoBuffer(boost::bind(&Connection::readFully, this, buf, len, 
				 handler, placeholders::error));
      return;
    case SSL_ERROR_WANT_WRITE:
      writeFromBuffer(boost::bind(&Connection::readFully, this, buf, len,
				  handler, placeholders::error));
      return;
    default:
      socket.get_io_service().post(boost::bind(handler, boost::asio::error::bad_descriptor));
      return;
    }
  }

  socket.get_io_service().post(boost::bind(handler, err));
}

void Connection::writeFully(unsigned char *buf, int len, ConnectHandler handler, 
			    const boost::system::error_code &err) 
{

  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  while (len > 0) {
    int count = SSL_write(ssl, buf, len);

    switch (SSL_get_error(ssl, count)) {
    case SSL_ERROR_NONE:
      len -= count;
      buf += count;
      break;
    case SSL_ERROR_WANT_READ:
      readIntoBuffer(boost::bind(&Connection::writeFully, this, buf, len, 
				 handler, placeholders::error));
      return;
    case SSL_ERROR_WANT_WRITE:
      writeFromBuffer(boost::bind(&Connection::readFully, this, buf, len,
				  handler, placeholders::error));
      return;
    default:
      socket.get_io_service().post(boost::bind(handler, boost::asio::error::bad_descriptor));
      return;
    }
  }

  writeFromBuffer(boost::bind(&Connection::dummyWrite, this, placeholders::error));
  socket.get_io_service().post(boost::bind(handler, err));
}

void Connection::initiateConnection(std::string &host, int port, ConnectHandler handler) {
  ip::tcp::endpoint endpoint(ip::address::from_string(host.c_str()), port);
  socket.async_connect(endpoint, boost::bind(&Connection::initiateConnectionComplete,
					     this, handler, placeholders::error));
}

void Connection::initiateConnectionComplete(ConnectHandler handler, 
					    const boost::system::error_code& err) 
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  handshake(boost::bind(&Connection::renegotiateCiphers, this, handler, placeholders::error), 
	    err);
}

void Connection::renegotiateCiphers(ConnectHandler handler, 
				    const boost::system::error_code &err) 
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  SSL_set_cipher_list(ssl, "DHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA:DES-CBC3-SHA");
  SSL_renegotiate(ssl);

  handshake(boost::bind(&Connection::exchangeVersions, this, handler, placeholders::error), err);
}

void Connection::handshake(ConnectHandler handler, const boost::system::error_code& err) {
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  int status = SSL_do_handshake(ssl);
  int res;

  writeFromBuffer(boost::bind(&Connection::dummyWrite, this, placeholders::error));

  switch ((res = SSL_get_error(ssl, status))) {
  case SSL_ERROR_NONE:
    socket.get_io_service().post(boost::bind(handler, boost::system::error_code()));
    break;
  case SSL_ERROR_WANT_READ:
    readIntoBuffer(boost::bind(&Connection::handshake, this, handler, placeholders::error));
    break;
  case SSL_ERROR_WANT_WRITE:
    writeFromBuffer(boost::bind(&Connection::handshake, this, handler, placeholders::error));
    break;
  default:
    socket.get_io_service().post(boost::bind(handler, boost::asio::error::bad_descriptor));
    break;
  }
}

void Connection::dummyWrite(const boost::system::error_code &error) {}

void Connection::readIntoBuffer(ConnectHandler handler) {
  socket.async_read_some(boost::asio::buffer(readBuffer, sizeof(readBuffer)),
			 boost::bind(&Connection::readIntoBufferComplete,
				     this, handler, placeholders::error, 
				     placeholders::bytes_transferred));
}

void Connection::readIntoBufferComplete(ConnectHandler handler, 
					const boost::system::error_code &err,
					size_t bytesRead)
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  BIO_write(readBio, readBuffer, bytesRead);
  socket.get_io_service().post(boost::bind(handler, err));
}

void Connection::writeFromBuffer(ConnectHandler handler) {
  int pending;

  while ((pending = BIO_ctrl_pending(writeBio)) > 0) {
    unsigned char buf[512];
    int bytesToSend = BIO_read(writeBio, buf, sizeof(buf));
    
    if (pending - bytesToSend == 0)
      async_write(socket, boost::asio::buffer(buf, bytesToSend), 
		  boost::bind(handler, placeholders::error));
    else
      async_write(socket, boost::asio::buffer(buf, bytesToSend),
		  boost::bind(&Connection::dummyWrite, this, placeholders::error));
  }
}

void Connection::exchangeVersions(ConnectHandler handler, const boost::system::error_code &err) 
{
  unsigned char versionBytes[]         = {0x00, 0x00, 0x07, 0x00, 0x02, 0x00, 0x02};
  unsigned char *versionResponseHeader = (unsigned char*)malloc(5);

  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  writeFully(versionBytes, sizeof(versionBytes), 
	     boost::bind(&Connection::dummyWrite, this, placeholders::error),
	     err);

  readFully(versionResponseHeader, 5,
	    boost::bind(&Connection::sentVersionComplete, this, handler, 
			versionResponseHeader, placeholders::error), err);

//   std::cout << "Version Response: " << std::endl;
//   Util::hexDump(versionResponsePayload, length);
}

void Connection::sentVersionComplete(ConnectHandler handler, 
				     unsigned char* versionResponseHeader, 
				     const boost::system::error_code &err) 
{
  if (err) {
    free(versionResponseHeader);
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  unsigned char *versionResponsePayload = (unsigned char*)malloc(24);

  if (versionResponseHeader[2] != 0x07) {
    std::cerr << "Warning: received strange version response cell." << std::endl;
    free(versionResponseHeader);
    socket.get_io_service().post(boost::bind(handler, boost::asio::error::bad_descriptor));
    return;
  }

  uint16_t length = Util::bigEndianArrayToShort(versionResponseHeader + 3);

  if (length > sizeof(versionResponsePayload)) {
    std::cerr << "Warning: version response length is strangely long." << std::endl;
    free(versionResponseHeader);
    socket.get_io_service().post(boost::bind(handler, boost::asio::error::bad_descriptor));
    return;
  }

  free(versionResponseHeader);

  readFully(versionResponsePayload, length,
	    boost::bind(&Connection::readVersionResponseComplete, 
			this, handler, placeholders::error), 
	    err);
}

void Connection::readVersionResponseComplete(ConnectHandler handler, 
					     const boost::system::error_code &err) 
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }
  
  exchangeNodeInfo(handler);
}

void Connection::exchangeNodeInfoReceived(ConnectHandler handler, 
					  boost::shared_ptr<Cell> remoteNodeInfo, 
					  const boost::system::error_code &err) 
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  uint32_t remoteTimestamp   = remoteNodeInfo->readInt();    // Remote timestamp
//   cout << "Remote timestamp: " << remoteTimestamp << " Local: " << time(0) << endl;
  unsigned char type         = remoteNodeInfo->readByte();   // Address type
//   cout << "Address type: " << (int)type << endl;
  string address             = remoteNodeInfo->readString(); // Address
//   cout << "Address: " << address << endl;
  unsigned char addressCount = remoteNodeInfo->readByte();   // Address Count
//   cout << "Address Count: " << (int)addressCount << endl;

  for (int i=0;i<addressCount;i++) {
    type    = remoteNodeInfo->readByte();
//     cout << "Address Type: " << (int)type << endl;
    address = remoteNodeInfo->readString();
//     cout << "Address: " << address << endl;
  }  

  socket.get_io_service().post(boost::bind(handler, err));
}

void Connection::exchangeNodeInfoSent(ConnectHandler handler, 
				      const boost::system::error_code &err) 
{
  if (err) {
    socket.get_io_service().post(boost::bind(handler, err));
    return;
  }

  boost::shared_ptr<Cell> remoteNodeInfo(new Cell());
  readCell(remoteNodeInfo, boost::bind(&Connection::exchangeNodeInfoReceived,
				       this, handler, remoteNodeInfo, 
				       placeholders::error));
}

void Connection::exchangeNodeInfo(ConnectHandler handler) {
  unsigned char thisHost[] = {0xc0, 0xa8, 0x01, 0x01}; // Nobody seems to care.
  long thatHost            = socket.remote_endpoint().address().to_v4().to_ulong();

  Cell nodeInfo(0, NETINFO);
  nodeInfo.append((uint32_t)time(0));   // Timestamp
  nodeInfo.append((unsigned char)0x04); // Type (host)
  nodeInfo.append((unsigned char)0x04); // Address Length
  nodeInfo.append((unsigned char*)&(thatHost), 4); // Address

  nodeInfo.append((unsigned char)0x01); // Address Count
  nodeInfo.append((unsigned char)0x04); // Type (ipv4)
  nodeInfo.append((unsigned char)0x04);
  nodeInfo.append(thisHost, 4);            // This Address
  
  writeCell(nodeInfo, boost::bind(&Connection::exchangeNodeInfoSent, this, 
				  handler, placeholders::error));
}

void Connection::close() {
  socket.close();
}

std::string& Connection::getRemoteNodeAddress() {
  return host;
}

ip::tcp::endpoint Connection::getLocalEndpoint() {
  return socket.local_endpoint();
}

Connection::~Connection() {
  SSL_CTX_free(ctx);
  SSL_free(ssl);
}
