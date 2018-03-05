/*
 * Simple interface to the Fly Sky IBus RC system.
 */

#include <inttypes.h>

class HardwareSerial;
class Stream;

class FlySkyIBus
{
public:
  void begin(HardwareSerial& serial);
  void begin(Stream& stream);
  void writeLoop(void);
  void readLoop(void);
  uint16_t readChannel(uint8_t channelNr);
  void writeToChannel(uint8_t channelNr, uint16_t value);

private:
  // Used as a state machine
  enum State
  {
    GET_LENGTH,
    GET_DATA,
    GET_CHKSUML,
    GET_CHKSUMH,
    DISCARD,
  };

  static const uint8_t PROTOCOL_CHANNELS = 10;

  static const uint8_t PROTOCOL_LENGTH = 0x20; // 32 bytes
  static const uint8_t PROTOCOL_OVERHEAD = 3; // <len><cmd><data....><chkl><chkh>
  static const uint8_t PROTOCOL_TIME = 7; // Packets are received very ~7ms
  static const uint8_t PROTOCOL_TIMEGAP = 3; // Packets are received very ~7ms so use ~half that for the gap
  static const uint8_t PROTOCOL_COMMAND40 = 0x40; // Command is always 0x40

  static const uint16_t LOW_VALUE = 0x3E8;
  static const uint16_t HIGH_VALUE = 0x7D0;
  static const uint16_t HALF_VALUE = 0x5DC;


  Stream* stream;
  uint8_t state;
  uint8_t w_state;
  uint32_t last;
  uint8_t ptr;
  uint8_t len;
  uint8_t buffer[PROTOCOL_LENGTH];
  uint8_t w_buffer[PROTOCOL_LENGTH];
  uint16_t chksum;
  uint16_t w_chksum;
  uint8_t lchksum;
  
  uint16_t channel[PROTOCOL_CHANNELS];

  void initWriteBuffer();
  
};

extern FlySkyIBus IBus;
