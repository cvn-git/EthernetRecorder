#include "packetparser.h"

#include <stdexcept>


PacketParser::PacketParser()
{
    if (sizeof(buffer_) != ETH_REC_HEADER_BYTES)
    {
        throw std::invalid_argument("PacketParser::PacketParser(): Invalid buffer size");
    }

    reset();
}

void PacketParser::reset()
{
    resetParsing();

    errorBytes_ = 0;
    receivedPackets_ = 0;
}

void PacketParser::resetParsing()
{
    state_ = FIND_SYNC;
    bufferValidBytes_ = 0;
}

void PacketParser::parseRawStream(const QByteArray& stream)
{
    constexpr size_t syncWordSize = sizeof(buffer_.syncWord);

    auto inputData = reinterpret_cast<const uint8_t*>(stream.data());
    auto numInputBytes = static_cast<size_t>(stream.size());

    const auto bufferData = reinterpret_cast<uint8_t*>(&buffer_);

    while (numInputBytes > 0)
    {
        if (state_ != PARSE_PACKET)
        {
            const size_t expectedBytes = (state_ == FIND_SYNC)? syncWordSize : ETH_REC_HEADER_BYTES;
            const size_t bytesToRead = std::min(expectedBytes - bufferValidBytes_, numInputBytes);
            if (bytesToRead > 0)
            {
                memcpy(bufferData + bufferValidBytes_, inputData, bytesToRead);
                bufferValidBytes_ += bytesToRead;
                inputData += bytesToRead;
                numInputBytes -= bytesToRead;
            }

            if (bufferValidBytes_ >= expectedBytes)
            {
                if (state_ == FIND_SYNC)
                {
                    // Enough data to process sync word
                    if (buffer_.syncWord == ETH_REC_SYNC_WORD)
                    {
                        // Sync word found
                        state_ = PARSE_HEADER;
                    }
                    else
                    {
                        // Wrong sync word, shift left one byte
                        buffer_.syncWord >>= 8;     // little-endian
                        --bufferValidBytes_;
                        ++errorBytes_;
                    }
                }
                else
                {
                    // Enough data to process header

                    state_ = PARSE_PACKET;
                    packetBytesToSkip_ = buffer_.header.numBytes;
                }
            }
        }
        else
        {
            // Skip input bytes for the whole packet body
            const size_t bytesToRead = std::min(packetBytesToSkip_, numInputBytes);
            inputData += bytesToRead;
            numInputBytes -= bytesToRead;
            packetBytesToSkip_ -= bytesToRead;

            if (packetBytesToSkip_ == 0)
            {
                ++receivedPackets_;
                resetParsing();
            }
        }
    }
}
