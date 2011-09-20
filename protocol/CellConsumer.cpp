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

#include "CellConsumer.h" 

#include <cassert>

CellConsumer::CellConsumer(Connection &connection, 
			   CellEncrypter &encrypter,
			   CellListener &listener) :
  connection(connection), encrypter(encrypter), listener(listener), closed(false)
{}

void CellConsumer::close() {
  closed = true;
}

void CellConsumer::consume() {
  if (closed) return;

  boost::shared_ptr<Cell> cell(new Cell());
  connection.readCell(cell, boost::bind(&CellConsumer::readCellComplete, this,
					cell, placeholders::error));
}

void CellConsumer::readCellComplete( boost::shared_ptr<Cell> cell,
				     const boost::system::error_code &err)
{
  if (closed) return;

  if (err) {
    listener.handleConnectionError(err);
    return;
  }

  switch (cell->getType()) {
  case Cell::PADDING_TYPE: break;
  case Cell::RELAY_TYPE:
    handleRelayCell(boost::shared_ptr<RelayCell>(new RelayCell(*cell))); 
    break;
  case Cell::DESTROY_TYPE: listener.handleDestroyCell(cell);                             break;
  default:                 listener.handleUnknownCell(cell);                             break;
  }  

  consume();
}


void CellConsumer::handleRelayCell(boost::shared_ptr<RelayCell> cell) {
  try {
    encrypter.decrypt(*cell);
    
    switch (cell->getRelayType()) {
    case RelayCell::DATA_TYPE:
    case RelayCell::END_TYPE:       listener.handleDataCell(cell);    break;
    case RelayCell::CONNECTED_TYPE: listener.handleConnected(cell);   break;
    case RelayCell::SENDME_TYPE:    listener.handleSendMe(cell);      break;
    case RelayCell::DROP_TYPE:                                        break;
    }
  } catch (CryptoMismatchException &e) {
    listener.handleCryptoException(cell);
  }
}
