#ifndef __RELAY_CELL_DISPATCHER_H__
#define __RELAY_CELL_DISPATCHER_H__

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <string>
#include <map>
#include <list>

#include "RelayCell.h"

typedef boost::function<void (const boost::system::error_code &error)> CircuitConnectHandler;
typedef boost::function<void (unsigned char* buf, int read)> CircuitReadHandler;

class RelayCellDispatcher {

 private:
  std::map<uint16_t, std::list<boost::shared_ptr<RelayCell> > > dataCells;
  std::map<uint16_t, CircuitConnectHandler> connectedListeners;
  std::map<uint16_t, CircuitReadHandler> dataListeners;

 public:
  void addStreamId(uint16_t streamId);
  void removeStreamId(uint16_t streamId);
  void dispatchConnectedCell(boost::shared_ptr<RelayCell> cell);

  void dispatchConnectedCellRequest(uint16_t streamId, CircuitConnectHandler handler);
  void dispatchDataCell(boost::shared_ptr<RelayCell> cell);
  void dispatchDataCellRequest(uint16_t streamId, CircuitReadHandler handler);

};

#endif
