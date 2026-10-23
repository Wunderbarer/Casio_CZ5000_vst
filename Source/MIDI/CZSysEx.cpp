#include "CZSysEx.h"
#include <algorithm>

namespace cz
{

CZTone CZTone::makeDefault()
{
    CZTone t;
    t.set (Field::pflag, 0, 0x00);
    t.set (Field::pds,   0, 0x00);
    t.set (Field::mfw, 0, 0x00);
    t.set (Field::mfw, 1, 0x00);
    t.set (Field::dca1End, 0, 1);
    t.set (Field::dca1Env, 0, encodeDcaRate (99, false));
    t.set (Field::dca1Env, 1, encodeDcaLevel (99));
    t.set (Field::dca1Env, 2, encodeDcaRate (40, true));
    t.set (Field::dca1Env, 3, encodeDcaLevel (0));
    t.set (Field::dcw1End, 0, 0);
    t.set (Field::dco1End, 0, 0);
    return t;
}

std::array<uint8_t, kNumDataBytes> decodePayload (const uint8_t* nibbles256)
{
    std::array<uint8_t, kNumDataBytes> data {};
    for (int i = 0; i < kNumNibbles; i += 2)
    {
        uint8_t lo = nibbles256[i]     & 0x0F;
        uint8_t hi = nibbles256[i + 1] & 0x0F;
        data[i / 2] = static_cast<uint8_t> ((hi << 4) | lo);
    }
    return data;
}

std::vector<uint8_t> encodePayload (const std::array<uint8_t, kNumDataBytes>& data)
{
    std::vector<uint8_t> nibbles;
    nibbles.reserve (kNumNibbles);
    for (uint8_t b : data)
    {
        nibbles.push_back (b & 0x0F);
        nibbles.push_back ((b >> 4) & 0x0F);
    }
    return nibbles;
}

std::vector<uint8_t> buildSysEx (int toneNum, const CZTone& tone, int channel)
{
    std::vector<uint8_t> frame;
    frame.reserve (kFrameSize);
    frame.push_back (0xF0);
    frame.push_back (kManufacturerCasio);
    frame.push_back (0x00);
    frame.push_back (0x00);
    frame.push_back (static_cast<uint8_t> (0x70 + (channel & 0x0F)));
    frame.push_back (kCommandToneDump);
    frame.push_back (static_cast<uint8_t> (toneNum & 0x7F));

    auto nibbles = encodePayload (tone.raw);
    frame.insert (frame.end(), nibbles.begin(), nibbles.end());
    frame.push_back (0xF7);
    return frame;
}

ParsedFrame parseSysEx (const std::vector<uint8_t>& frame)
{
    if (frame.size() != (size_t) kFrameSize)
        throw std::runtime_error ("CZSysEx: erwartet " + std::to_string (kFrameSize)
                                   + " Bytes, bekommen " + std::to_string (frame.size()));
    if (frame.front() != 0xF0 || frame.back() != 0xF7)
        throw std::runtime_error ("CZSysEx: Frame beginnt nicht mit F0 / endet nicht mit F7");
    if (frame[1] != kManufacturerCasio || frame[2] != 0x00 || frame[3] != 0x00)
        throw std::runtime_error ("CZSysEx: kein Casio-Header (44 00 00)");
    if (frame[5] != kCommandToneDump)
        throw std::runtime_error ("CZSysEx: Command-Byte != 0x20 (kein Tone-Dump)");

    ParsedFrame out;
    out.channel = frame[4] - 0x70;
    out.toneNum = frame[6];

    std::array<uint8_t, kNumNibbles> nibbles {};
    std::copy (frame.begin() + 7, frame.end() - 1, nibbles.begin());
    out.tone.raw = decodePayload (nibbles.data());
    return out;
}

std::vector<std::vector<uint8_t>> extractFrames (const std::vector<uint8_t>& stream)
{
    std::vector<std::vector<uint8_t>> frames;
    std::vector<uint8_t> current;

    for (uint8_t b : stream)
    {
        if (b == 0xF0)
        {
            current.clear();
            current.push_back (b);
            continue;
        }
        if (! current.empty())
        {
            current.push_back (b);
            if (b == 0xF7)
            {
                if (current.size() == (size_t) kFrameSize)
                    frames.push_back (current);
                current.clear();
            }
        }
    }
    return frames;
}

uint8_t encodeDcaLevel (int level0to99)
{
    level0to99 = std::clamp (level0to99, 0, 99);
    return static_cast<uint8_t> ((level0to99 * 127 + 49) / 99);
}

int decodeDcaLevel (uint8_t byteVal)
{
    int v = byteVal & 0x7F;
    return (v * 99 + 63) / 127;
}

uint8_t encodeDcaRate (int rate0to99, bool levelFalls)
{
    rate0to99 = std::clamp (rate0to99, 0, 99);
    uint8_t b = static_cast<uint8_t> (rate0to99 & 0x7F);
    if (levelFalls)
        b |= 0x80;
    return b;
}

void decodeDcaRate (uint8_t byteVal, int& rateOut, bool& levelFallsOut)
{
    rateOut = byteVal & 0x7F;
    levelFallsOut = (byteVal & 0x80) != 0;
}

uint8_t encodeDcwLevel (int level0to99, bool sustainPoint)
{
    uint8_t b = encodeDcaLevel (level0to99);
    if (sustainPoint)
        b |= 0x80;
    return b;
}

void decodeDcwLevel (uint8_t byteVal, int& levelOut, bool& sustainOut)
{
    levelOut = decodeDcaLevel (byteVal & 0x7F);
    sustainOut = (byteVal & 0x80) != 0;
}

uint8_t encodeDcoLevel (int level0to99)
{
    level0to99 = std::clamp (level0to99, 0, 99);
    if (level0to99 <= 63)
        return static_cast<uint8_t> (level0to99);
    return static_cast<uint8_t> (0x44 + (level0to99 - 64));
}

int decodeDcoLevel (uint8_t byteVal)
{
    if (byteVal <= 0x3F)
        return byteVal;
    return 64 + (byteVal - 0x44);
}

} // namespace cz
