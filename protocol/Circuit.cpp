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
#include "Cell.h"
#include "Circuit.h"
#include "HybridEncryption.h"
#include "../util/Util.h"
#include "RelayBeginCell.h"
#include "RelayDataCell.h"
#include "RelayCell.h"

#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <cassert>

using namespace std;

#define MIN(a,b) ((a)<(b)?(a):(b))

Circuit::Circuit(Connection &conn, RSA *onionKey, uint16_t id, 
		 CircuitErrorListener *errorListener) :
  connection(conn), 
  circuitId(id), 
  onionKey(onionKey), 
  cellConsumer(conn, cellEncrypter, *this),
  circuitWindow(1000),
  errorListener(errorListener)
{
  assert(onionKey != NULL);

  initializeDhParameters();
}

void Circuit::initializeDhParameters() {
  this->p     = BN_new();
  this->g     = BN_new();
  this->dh    = DH_new();

  BN_hex2bn(&(this->p), SAFE_PRIME);
  BN_set_word(this->g, 2);

  this->dh->p = BN_dup(this->p);
  this->dh->g = BN_dup(this->g);

  DH_generate_key(dh);
}

void Circuit::sendCreateCell(RSA *onionKey, CircuitConnectHandler handler) {
  boost::shared_ptr<CreateCell> create(new CreateCell(circuitId, dh, onionKey));
  connection.writeCell(*create, boost::bind(&Circuit::sendCreateCellComplete, this, 
					     handler, create, placeholders::error));
}

void Circuit::sendCreateCellComplete(CircuitConnectHandler handler, 
				     boost::shared_ptr<CreateCell> create,
				     const boost::system::error_code &err) 
{
  if (err) {
    handler(err);
    return;
  }

  readCreatedCell(handler);
}

void Circuit::readCreatedCell(CircuitConnectHandler handler) {
  boost::shared_ptr<CreatedCell> response(new CreatedCell(dh));
  connection.readCell(response, boost::bind(&Circuit::readCreatedCellComplete, this,
					     handler, response, placeholders::error));
}

void Circuit::readCreatedCellComplete(CircuitConnectHandler handler, 
				      boost::shared_ptr<CreatedCell> response,
				      const boost::system::error_code &err)
{
  unsigned char* keyMaterial = NULL;
  unsigned char* verifier    = NULL;

  try {
    if (err || !response->isValid()) {
      std::cerr << "Created Cell Not Valid..." << std::endl;
      handler(err ? err : boost::asio::error::invalid_argument);
      return;
    }    
    
    int keyMaterialLength = response->getKeyMaterial(&keyMaterial, &verifier);    
    cellEncrypter.setKeyMaterial(keyMaterial, keyMaterialLength, verifier);

  } catch (CryptoMismatchException &e) {
    std::cerr << "Got a crypto mismatch exception(" << getRemoteNodeAddress() <<"): " 
	      << e.what() << std::endl;
    if (keyMaterial) free(keyMaterial);
    handler(boost::asio::error::invalid_argument);
    return;
  }

  free(keyMaterial);
  cellConsumer.consume();
  handler(err);
}

void Circuit::sendBeginCell(uint16_t streamId, std::string &address, 
			    CircuitConnectHandler handler) 
{
  boost::shared_ptr<RelayBeginCell> beginCell(new RelayBeginCell(circuitId, streamId, address));

  cellEncrypter.encrypt(*beginCell);
  connection.writeCell(*beginCell, boost::bind(&Circuit::sendBeginCellComplete, this,
						handler, streamId, beginCell, 
						placeholders::error));
}

void Circuit::sendBeginCellComplete(CircuitConnectHandler handler, 
				    uint16_t streamId,
				    boost::shared_ptr<RelayBeginCell> beginCell,
				    const boost::system::error_code &err) 
{
  if (err) {
    std::cerr << "begincell write error" << std::endl;
    handler(err);
    return;
  }
  
  dispatcher.dispatchConnectedCellRequest(streamId, handler);
}

void Circuit::handleConnectionError(const boost::system::error_code &err) {
  std::cerr << "handle connectoin error" << std::endl;
  errorListener->handleConnectionError(err);
}

void Circuit::handleDestroyCell(boost::shared_ptr<Cell> cell) {
  std::cerr << "handle destroy cell" << std::endl;
  errorListener->handleCircuitDestroyed();
}

void Circuit::handleUnknownCell(boost::shared_ptr<Cell> cell) {
  std::cerr << "Error: Got unexpected cell type: " << cell->getType() << std::endl;
}

void Circuit::handleConnected(boost::shared_ptr<RelayCell> cell) {
  dispatcher.dispatchConnectedCell(cell);
}

void Circuit::handleDataCell(boost::shared_ptr<RelayCell> cell) {
  decrementWindows(cell->getStreamId());
  dispatcher.dispatchDataCell(cell);
}

void Circuit::handleSendMe(boost::shared_ptr<RelayCell> cell) {
  std::cerr << "Got SendMe, ignoring..." << std::endl;
}

void Circuit::handleCryptoException(boost::shared_ptr<RelayCell> cell) {
  std::cerr << "Got crypto exception!  Continuing with hope..." << std::endl;
}

void Circuit::decrementWindows(uint16_t streamId) {
  if (--circuitWindow <= 900) {
    sendWindowUpdate(0);
    circuitWindow += 100;
  }

  streamWindows[streamId] = streamWindows[streamId] - 1;

  if (streamWindows[streamId] <= 450) {
    sendWindowUpdate(streamId);
    streamWindows[streamId] = 500;
  }
}

void Circuit::sendWindowUpdateComplete(boost::shared_ptr<RelaySendMeCell> cell,
				       const boost::system::error_code &err)
{}

void Circuit::sendWindowUpdate(uint16_t streamId) {
  boost::shared_ptr<RelaySendMeCell> cell(new RelaySendMeCell(circuitId, streamId));
  cellEncrypter.encrypt(*cell);
  connection.writeCell(*cell, boost::bind(&Circuit::sendWindowUpdateComplete,
					   this, cell, placeholders::error));
}

void Circuit::closeComplete(boost::shared_ptr<RelayEndCell> cell,
			    const boost::system::error_code &err)
{}

void Circuit::close() {
  cellConsumer.close();
}

void Circuit::close(uint16_t streamId) {
  std::cerr << "CIRCUIT: Close called..." << std::endl;

  boost::shared_ptr<RelayEndCell> relayEnd(new RelayEndCell(circuitId, streamId));
  cellEncrypter.encrypt(*relayEnd);
  connection.writeCell(*relayEnd, boost::bind(&Circuit::closeComplete, this,
					       relayEnd, placeholders::error));
}

// Public

void Circuit::connect(uint16_t streamId, std::string &address, CircuitConnectHandler handler) {
  dispatcher.addStreamId(streamId);
  streamWindows[streamId] = 500;
  sendBeginCell(streamId, address, handler);
}

void Circuit::create(CircuitConnectHandler handler) {
  circuitWindow = 1000;
  sendCreateCell(onionKey, handler);
}

void Circuit::writeComplete(boost::shared_ptr<RelayDataCell> dataCell,
			    const boost::system::error_code &err) {}

void Circuit::write(uint16_t streamId, 
		    unsigned char* buf, int length, 
		    CircuitWriteHandler handler) 
{
  int i;
  
  for (i=0;i<length;i+=(MAX_PAYLOAD_LENGTH)) {
    boost::shared_ptr<RelayDataCell> dataCell(new RelayDataCell(circuitId, streamId, 
								buf+i, 
								MIN(MAX_PAYLOAD_LENGTH, 
								    length-i)));

    cellEncrypter.encrypt(*dataCell);
    connection.writeCell(*dataCell, ((length-i <= MAX_PAYLOAD_LENGTH) ? handler : boost::bind(&Circuit::writeComplete, this, dataCell, placeholders::error)));
  }
}

void Circuit::read(uint16_t streamId, CircuitReadHandler handler) {
  dispatcher.dispatchDataCellRequest(streamId, handler);
}

std::string& Circuit::getRemoteNodeAddress() {
  return connection.getRemoteNodeAddress();
}

ip::tcp::endpoint Circuit::getLocalEndpoint() {
  return connection.getLocalEndpoint();
}

Circuit::~Circuit() {
  BN_free(p);
  BN_free(g);
  DH_free(dh);
  RSA_free(onionKey);
}
