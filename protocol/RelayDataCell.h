#ifndef __RELAY_DATA_CELL_H__
#define __RELAY_DATA_CELL_H__

#include "RelayCell.h"

#define DATA_TYPE 2

class RelayDataCell : public RelayCell {

 public:
 RelayDataCell(uint16_t circuitId, uint16_t streamId, unsigned char *data, int length) :
  RelayCell(circuitId, streamId, DATA_TYPE, data, length) {}
  
 RelayDataCell() : RelayCell() {}
};


#endif
