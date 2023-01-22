#ifndef PACKETPARSER_H
#define PACKETPARSER_H

#include "eth_rec_common.h"

#include <QByteArray>

#include <array>


class PacketParser
{
public:
    PacketParser();

    void reset();

    void parseRawStream(const QByteArray& stream);

    size_t receivedPackets() const {return receivedPackets_;}

    size_t errorBytes() const {return errorBytes_;}

private:
    enum State
    {
        FIND_SYNC = 0,
        PARSE_HEADER,
        PARSE_PACKET,
    };

    union Buffer
    {
        EthRecHeader header;
        uint32_t syncWord;
    };

    void resetParsing();

    Buffer buffer_;

    State state_;
    size_t bufferValidBytes_{0};
    size_t packetBytesToSkip_{0};

    size_t errorBytes_{0};
    size_t receivedPackets_{0};
};

#endif // PACKETPARSER_H
