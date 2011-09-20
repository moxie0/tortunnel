#ifndef __CIRCUIT_H__
#define __CIRCUIT_H__

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


#define SAFE_PRIME "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381FFFFFFFFFFFFFFFF"

#include <openssl/bn.h>
#include <openssl/dh.h>
#include "Connection.h"

#include <openssl/rsa.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "RelayDataCell.h"
#include "RelayBeginCell.h"
#include "RelayEndCell.h"
#include "RelaySendMeCell.h"
#include "CreateCell.h"
#include "CreatedCell.h"
#include "CellEncrypter.h"
#include "Cell.h"

#include "RelayCellDispatcher.h"
#include "CellConsumer.h"
#include "CellListener.h"

/*
 * This class implements a Tor Circuit.
 */

typedef boost::function<void (const boost::system::error_code &error)> CircuitWriteHandler;

class CircuitErrorListener {

 public:
  virtual void handleConnectionError(const boost::system::error_code &err) = 0;
  virtual void handleCircuitDestroyed() = 0;
};

class Circuit : public CellListener {

 private:
  BIGNUM *p;
  BIGNUM *g;
  DH *dh;
  RSA *onionKey;
  uint16_t circuitId;
  uint32_t circuitWindow;

  CircuitErrorListener *errorListener;
  Connection &connection;
  CellEncrypter cellEncrypter;
  CellConsumer cellConsumer;
  RelayCellDispatcher dispatcher;
  std::map<uint16_t, uint32_t> streamWindows;
  

  void initializeDhParameters();

  void sendCreateCell(RSA *onionKey, CircuitConnectHandler handler);
  void sendCreateCellComplete(CircuitConnectHandler handler, 
			      boost::shared_ptr<CreateCell> create,
			      const boost::system::error_code &err);


  void readCreatedCell(CircuitConnectHandler handler);
  void readCreatedCellComplete(CircuitConnectHandler handler, 
			       boost::shared_ptr<CreatedCell> response,
			       const boost::system::error_code &err);


  void sendBeginCell(uint16_t streamId, std::string &address, CircuitConnectHandler handler);
  void sendBeginCellComplete(CircuitConnectHandler handler,
			     uint16_t streamId, 
			     boost::shared_ptr<RelayBeginCell> beginCell,
			     const boost::system::error_code &err);

  void handleConnectionError(const boost::system::error_code &err);
  void handleDestroyCell(boost::shared_ptr<Cell> cell);
  void handleUnknownCell(boost::shared_ptr<Cell> cell);
  void handleConnected(boost::shared_ptr<RelayCell> cell);
  void handleDataCell(boost::shared_ptr<RelayCell> cell);
  void handleSendMe(boost::shared_ptr<RelayCell> cell);
  void handleCryptoException(boost::shared_ptr<RelayCell> cell);

  void writeComplete(boost::shared_ptr<RelayDataCell> dataCell,
		     const boost::system::error_code &err);
  void readComplete(CircuitReadHandler handler, 
		    boost::shared_ptr<RelayDataCell> dataCell,
		    uint16_t streamId, unsigned char* buf,
		    const boost::system::error_code &err);

  void closeComplete(boost::shared_ptr<RelayEndCell> cell,
		     const boost::system::error_code &err);

  void sendWindowUpdateComplete(boost::shared_ptr<RelaySendMeCell> cell,
				const boost::system::error_code &err);
  void sendWindowUpdate(uint16_t streamId);
  void decrementWindows(uint16_t streamId);

 public:
  Circuit(Connection &connection, RSA *onionKey, uint16_t circuitId, 
	  CircuitErrorListener *errorListener);
  void connect(uint16_t streamId, std::string &address, CircuitConnectHandler handler);
  void create(CircuitConnectHandler handler);

  void write(uint16_t streamId, unsigned char *buf, int length, CircuitWriteHandler handler);
  void read(uint16_t streamId, CircuitReadHandler handler);

  void close(uint16_t streamId);
  void close();

  std::string& getRemoteNodeAddress();
  ip::tcp::endpoint getLocalEndpoint();
  ~Circuit();
};


#endif
