/*
 * Simple interface to the Fly Sky IBus RC system.
 */

#include <Arduino.h>
#include "FlySkyIBus.h"

#define DEBUG 0

FlySkyIBus IBus;

void FlySkyIBus::begin(HardwareSerial& serial, HardwareSerial& serialr)
{
  serial.begin(115200);
  serialr.begin(115200);
  begin((Stream&)serial, (Stream&)serialr);
}

void FlySkyIBus::begin(Stream& stream, Stream& streamr)
{
  this->stream = &stream;
  this->streamr = &streamr;
  this->state = DISCARD;
  this->last_r = millis();
  this->last = millis();
  this->ptr = 0;
  this->len = 0;
  this->chksum = 0;
  this->lchksum = 0;

  initWriteBuffer();
}


void FlySkyIBus::initWriteBuffer(){

  uint8_t tmp;
  uint8_t tmp2;

  w_chksum = 0xff9f; // Base checksum value with LEN and CMD substracted

  w_buffer[0] = 0x20; // LEN
  w_buffer[1] = 0x40; // CMD

  for (int i = 1; i < PROTOCOL_MAX_CHANNELS + 1; i++ ) // All channels fill
  {
    if (i == AUX1 || i == AUX2) // AUX1 and AUX2
    {
      tmp = HIGH_VALUE & 0xff;
      tmp2 = HIGH_VALUE >> 8;
    }else{
      tmp = HALF_VALUE & 0xff;
      tmp2 = HALF_VALUE >> 8;
    }

    w_buffer[i*2] = tmp; // +2 because LEN and CMD
    w_chksum -= tmp;

    w_buffer[i*2+1] = tmp2;
    w_chksum -= tmp2;
  }

  w_buffer[PROTOCOL_LENGTH-2] = w_chksum & 0xff;
  w_buffer[PROTOCOL_LENGTH-1] = w_chksum >> 8;

}


void FlySkyIBus::writeToChannel(uint8_t channelNr, uint16_t value){

  uint16_t old_value_tmp; // 16 bits in case that overflows the byte
  uint16_t new_value_tmp; // 16 bits in case that overflows the byte
  uint16_t checksum;

  // get old channel data and checkum
  old_value_tmp = w_buffer[channelNr*2] + w_buffer[channelNr*2+1];
  new_value_tmp = (value & 0xff) + (value >> 8 & 0xff);
  checksum = w_buffer[PROTOCOL_LENGTH-2] | w_buffer[PROTOCOL_LENGTH-1] << 8;

  // calculate new checksum without iterating over all the data again
  checksum = checksum + old_value_tmp - new_value_tmp;

  // value
  w_buffer[channelNr*2] = value & 0xff;
  w_buffer[channelNr*2+1] = value >> 8 & 0xff;

  // checksum
  w_buffer[PROTOCOL_LENGTH-2] = checksum & 0xff;
  w_buffer[PROTOCOL_LENGTH-1] = checksum >> 8 & 0xff;

}

void FlySkyIBus::writeLoop(void){

  uint32_t now = millis();

  if (now - last >= PROTOCOL_TIME)
  {
    last = now;
    stream->write(w_buffer, PROTOCOL_LENGTH);
  }
}

void FlySkyIBus::readLoop(void)
{
  // If serial data
  while (streamr->available() > 0)
  {
    uint32_t now = millis();

    // restart state, after 7/2 ms
    if (now - last_r >= PROTOCOL_TIMEGAP)
    {
      state = GET_LENGTH;
    }

    last_r = now;
    
    uint8_t value = streamr->read(); // Read serial

    // State machine
    switch (state)
    {
      case GET_LENGTH:

        if (value <= PROTOCOL_LENGTH)
        {
          ptr = 0;
          len = value - PROTOCOL_OVERHEAD;
          chksum = 0xFFFF - value;
          state = GET_DATA;
        }
        else
        {
          state = DISCARD;
        }
        break;

      case GET_DATA:

        buffer[ptr++] = value;
        chksum -= value;
        if (ptr == len)
        {
          state = GET_CHKSUML;
        }
        break;
        
      case GET_CHKSUML:

        lchksum = value;
        state = GET_CHKSUMH;
        break;

      case GET_CHKSUMH:

        // Validate checksum
        if (chksum == (value << 8) + lchksum)
        {
          // Execute command - we only know command 0x40
          switch (buffer[0])
          {
            case PROTOCOL_COMMAND40:
              // Valid - extract channel data
              for (uint8_t i = 1; i < PROTOCOL_CHANNELS * 2 + 1; i += 2)
              {
                // Write 16 bits (1000 to 2000 range)
                channel[i / 2] = buffer[i] | (buffer[i + 1] << 8);
              }
              break;

            default:
              break;
          }
        }
        state = DISCARD;
        break;

      case DISCARD:
      default:
        break;
    }
  }
}

uint16_t FlySkyIBus::readChannel(uint8_t channelNr)
{
  if (channelNr < PROTOCOL_CHANNELS)
  {
    return channel[channelNr-1];
  }
  else
  {
    return 0;
  }
}
