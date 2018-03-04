/*
 * Simple interface to the Fly Sky IBus RC system.
 */

#include <Arduino.h>
#include "FlySkyIBus.h"

#define DEBUG 1

FlySkyIBus IBus;

void FlySkyIBus::begin(HardwareSerial& serial)
{
  serial.begin(115200);
  begin((Stream&)serial);
}

void FlySkyIBus::begin(Stream& stream)
{
  this->stream = &stream;
  this->state = DISCARD;
  this->last = millis();
  this->ptr = 0;
  this->len = 0;
  this->chksum = 0;
  this->lchksum = 0;
}

void FlySkyIBus::writeChannel(uint8_t channelNr, uint16_t value){

  uint32_t now = millis();
  uint16_t chksum = 0xff9f; // Len and cmd substracted
  uint8_t tmp;

  //stream->println(now-last);

  if (now - last >= PROTOCOL_TIME)
  {
    state = WRITE_STATE;
    last = now;
  }

  if (state == WRITE_STATE)
  {

    wbuffer[0] = 0x20; // LEN
    wbuffer[1] = 0x40; // CMD

    for (int i = 2; i < PROTOCOL_LENGTH - 2; i += 2 ) // All channels fill
    {
      tmp = LOW_VALUE & 0xff;
      wbuffer[i] = tmp;
      if (i != channelNr * 2)
      {
        chksum -= tmp;
      }

      tmp = LOW_VALUE >> 8;
      wbuffer[i+1] = tmp;
      if (i != channelNr * 2)
      {
        chksum -= tmp;
      }
    }

    tmp = value & 0xff;
    wbuffer[channelNr*2] = tmp;
    chksum -= tmp;

    tmp = value >> 8;
    wbuffer[channelNr*2+1] = tmp;
    chksum -= tmp;

    wbuffer[PROTOCOL_LENGTH-2] = chksum & 0xff;
    wbuffer[PROTOCOL_LENGTH-1] = chksum >> 8;

    // for (int i = 0; i < PROTOCOL_LENGTH; i++)
    // {
    //   char b[16];
    //   sprintf(b, "%x ", wbuffer[i]);
    //   stream->print(b);
    // }
    // stream->println();

    stream->write(wbuffer, PROTOCOL_LENGTH);

  }

}

void FlySkyIBus::loop(void)
{
  // If serial data
  while (stream->available() > 0)
  {
    uint32_t now = millis();

    // restart state, after 7/2 ms
    if (now - last >= PROTOCOL_TIMEGAP)
    {
      state = GET_LENGTH;
    }

    last = now;
    
    uint8_t value = stream->read(); // Read serial

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

        if (DEBUG)
        {
          char b[16];
          sprintf(b, "DTA: 0x%x", value);
          stream->println(b);
        }

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
    return channel[channelNr];
  }
  else
  {
    return 0;
  }
}
