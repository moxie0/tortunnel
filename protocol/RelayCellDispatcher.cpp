
#include "RelayCellDispatcher.h"
#include <cassert>

void RelayCellDispatcher::addStreamId(uint16_t streamId) {
  dataCells[streamId] =   std::list<boost::shared_ptr<RelayCell> >();
}

void RelayCellDispatcher::removeStreamId(uint16_t streamId) {
  dataCells.erase(streamId);
  connectedListeners.erase(streamId);
  dataListeners.erase(streamId);
}

void RelayCellDispatcher::dispatchConnectedCell(boost::shared_ptr<RelayCell> cell) {
  uint16_t streamId = cell->getStreamId();

  std::map<uint16_t, CircuitConnectHandler>::iterator iter = connectedListeners.find(streamId);
  
  if (iter != connectedListeners.end()) iter->second(boost::system::error_code());
  else                                  dataCells[streamId].push_back(cell);
}

void RelayCellDispatcher::dispatchConnectedCellRequest(uint16_t streamId,
						       CircuitConnectHandler handler)
{
  std::map<uint16_t, std::list<boost::shared_ptr<RelayCell> > >::iterator iter =
    dataCells.find(streamId);

  if (iter != dataCells.end() && !(iter->second.empty())) {
    boost::shared_ptr<RelayCell> cell = iter->second.front();    
    assert(cell->isConnected() || cell->isRelayEnd());
    
    if (cell->isConnected()) handler(boost::system::error_code());
    else                     handler(boost::asio::error::connection_refused);

    iter->second.erase(iter->second.begin());
  } else {
    connectedListeners[streamId] = handler;
  }

}

void RelayCellDispatcher::dispatchDataCell(boost::shared_ptr<RelayCell> cell) {
  uint16_t streamId = cell->getStreamId();

  std::map<uint16_t, CircuitReadHandler>::iterator dataIter       = dataListeners.find(streamId);
  std::map<uint16_t, CircuitConnectHandler>::iterator connectIter = connectedListeners.find(streamId);

  if (dataIter != dataListeners.end()) {
    dataIter->second(cell->getRelayPayload(), cell->getRelayPayloadLength());
  } else if (connectIter != connectedListeners.end()) {
    connectIter->second(boost::asio::error::connection_refused);
  } else {
    dataCells[streamId].push_back(cell);
  }
}

void RelayCellDispatcher::dispatchDataCellRequest(uint16_t streamId, 
						  CircuitReadHandler handler) 
{
  std::map<uint16_t, std::list<boost::shared_ptr<RelayCell> > >::iterator iter = 
    dataCells.find(streamId);
  
  if (iter != dataCells.end() && !(iter->second.empty())) {
    boost::shared_ptr<RelayCell> cell = iter->second.front();
    handler(cell->getRelayPayload(), cell->getRelayPayloadLength());
    iter->second.erase(iter->second.begin());
  } else {
    dataListeners[streamId] = handler;
  }
}
