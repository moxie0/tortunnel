#ifndef __RELAY_CELL_H__
#define __RELAY_CELL_H__

#include "../util/Util.h"
#include "Cell.h"
#include <string>

#define DIGEST_OFFSET 8
#define DIGEST_LENGTH 4
#define RELAY_PAYLOAD_OFFSET 14
#define RELAY_PAYLOAD_LENGTH_OFFSET 12
#define RELAY_TYPE_OFFSET 3
#define STREAM_ID_OFFSET 6

#define MAX_PAYLOAD_LENGTH 512 - 14

#define RELAY_END_TYPE 0x03

class RelayCell : public Cell {

 private:

  void appendData(uint16_t streamId, unsigned char type, int length) {
    append(type);               // type
    append((unsigned char)0x0); // recognized
    append((unsigned char)0x0); // recognized
    append(streamId);           // steram id
    append((uint32_t)0x0);      // digest (0 for now)
    append((uint16_t)length);   // length
  }

 public:

  static const int DATA_TYPE      = 2;
  static const int END_TYPE       = 3;
  static const int CONNECTED_TYPE = 4;
  static const int SENDME_TYPE    = 5;
  static const int DROP_TYPE      = 10;
  
 RelayCell() : Cell() {}
  
 RelayCell(uint16_t circuitId, 
	   uint16_t streamId, 
	   unsigned char type, 
	   unsigned char* data, 
	   int length) 
   : Cell(circuitId, 0x03)
    {
      appendData(streamId, type, length);
      append(data, length);
    }
  
  // Wow, it'd be nice if C++ would let us call one constructor from another...
 RelayCell(uint16_t circuitId, uint16_t streamId, unsigned char type, 
	   std::string &data, bool nullTerminatd) 
   : Cell(circuitId, 0x03)
    {
      appendData(streamId, type, nullTerminatd ? data.length() + 1 : data.length());
      append(data);

      if (nullTerminatd) 
	append((unsigned char)'\0');
    }

  RelayCell(uint16_t circuitId, uint16_t streamId, unsigned char type, unsigned char payload)
    : Cell(circuitId, 0x03)
    {
      appendData(streamId, type, 0x01);
      append(payload);
    }

 RelayCell(Cell &cell) : Cell()
    {
      memcpy(getBuffer(), cell.getBuffer(), getBufferSize());
    }

  void setDigest(unsigned char* digest) {
    memcpy(getBuffer()+DIGEST_OFFSET, digest, DIGEST_LENGTH);
  }

  void getDigest(unsigned char* buf) {
    memcpy(buf, getBuffer()+DIGEST_OFFSET, DIGEST_LENGTH);
  }

  unsigned char* getRelayPayload() {
    return getBuffer() + RELAY_PAYLOAD_OFFSET;
  }

  int getRelayPayloadLength() {
    return isRelayEnd() ? -1 : (int)Util::bigEndianArrayToShort(getBuffer() + RELAY_PAYLOAD_LENGTH_OFFSET);
  }

  unsigned char getRelayType() {
    return getBuffer()[RELAY_TYPE_OFFSET];
  }

  bool isRelayEnd() {
    return getRelayType() == RELAY_END_TYPE;
  }

  uint16_t getStreamId() {
    return Util::bigEndianArrayToShort(getBuffer()+STREAM_ID_OFFSET);
  }

  bool isConnected() {
    return getRelayType() == CONNECTED_TYPE;
  }

};




#endif
