#ifndef __CONNECTION_H__
#define __CONNECTION_H__

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

#include <iostream>
#include <stdint.h>
#include <openssl/ssl.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "Cell.h"

/*
 * This class implements the basic connnection functionality.  It takes care
 * of the TLS connection and all the reading/writing to and from the wire.
 *
 */

using namespace std;
using namespace boost::asio;

typedef boost::function<void (const boost::system::error_code &error)> ConnectHandler;

class Connection {

 private:
  std::string host;
  std::string port;

  SSL_CTX *ctx;
  SSL *ssl;
  BIO *readBio;
  BIO *writeBio;

  ip::tcp::socket socket;

  unsigned char readBuffer[1024];

  void readFully(unsigned char *buf, int len, 
		 ConnectHandler handler, 
		 const boost::system::error_code err);
    
  void writeFully(unsigned char *buf, int len, ConnectHandler handler, 
		  const boost::system::error_code &err);

  void initiateConnection(std::string &host, int port, ConnectHandler handler);

  void initiateConnectionComplete(ConnectHandler handler, const boost::system::error_code& err);

  void renegotiateCiphers(ConnectHandler handler, const boost::system::error_code &err);

  void handshake(ConnectHandler handler, const boost::system::error_code& err);
  void dummyWrite(const boost::system::error_code &error);

  void readIntoBuffer(ConnectHandler handler);

  void readIntoBufferComplete(ConnectHandler handler, const boost::system::error_code &err,
			      size_t bytesRead);

  void writeFromBuffer(ConnectHandler handler);
  

  void exchangeVersions(ConnectHandler handler, const boost::system::error_code &err);
  void sentVersionComplete(ConnectHandler handler, 
			   unsigned char* versionResponseHeader, 
			   const boost::system::error_code &err);

  void readVersionResponseComplete(ConnectHandler handler, 
				   const boost::system::error_code &err);

  void exchangeNodeInfoReceived(ConnectHandler handler, 
				boost::shared_ptr<Cell> remoteNodeInfo, 
				const boost::system::error_code &err);

  void exchangeNodeInfoSent(ConnectHandler handler, const boost::system::error_code &err);
  void exchangeNodeInfo(ConnectHandler handler);

  void initializeSSL();

 public:
  Connection(io_service &service, string &host, string &port);

  std::string& getRemoteNodeAddress();
  ip::tcp::endpoint getLocalEndpoint();

  void connect(ConnectHandler handler);
  void close();

  void writeCell(Cell &cell, ConnectHandler handler);
  void readCell(boost::shared_ptr<Cell> cell, ConnectHandler handler);
  X509* getCertificate();
  STACK_OF(X509)* getCertificateChain();

  ~Connection();

};

#endif
