/*
  CZSysEx.h
  ---------
  Encoder/Decoder fuer Casio CZ-101 / CZ-1000 / CZ-5000 Tone-SysEx-Dumps.

  Struktur und Nibble-Packung wurden gegen drei echte CZ-5000-Dumps verifiziert
  (Round-Trip decode->encode = bitgenau identisch) und stimmen mit den
  oeffentlich dokumentierten Quellen ueberein:
    - https://github.com/ajwills72/cz101/blob/master/docs/sysex.md
    - https://www.kasploosh.com/cz/11800-spelunking/1-which_bits/

  Frame-Layout (264 Bytes gesamt):
    F0 44 00 00 [0x70+channel] 20 [toneNum] [256 nibbles] F7

  128 logische Datenbytes werden als 256 Nibbles uebertragen
  (Low-Nibble zuerst, dann High-Nibble).
*/
#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>

namespace cz
{

//==============================================================================
// Feldoffsets innerhalb der 128 logischen Datenbytes.
// Reihenfolge: DCA -> DCW -> DCO (wichtig! war der urspruengliche Bug)
//==============================================================================
struct FieldRange { int offset; int size; };

namespace Field
{
    static constexpr FieldRange pflag     { 0,   1 };  // line select / octave
    static constexpr FieldRange pds       { 1,   1 };  // detune sign
    static constexpr FieldRange pdl_pdh   { 2,   2 };  // detune range
    static constexpr FieldRange pvk       { 4,   1 };  // vibrato waveform
    static constexpr FieldRange vibDelay  { 5,   3 };
    static constexpr FieldRange vibRate   { 8,   3 };
    static constexpr FieldRange vibDepth  { 11,  3 };
    static constexpr FieldRange mfw       { 14,  2 };  // DCO1 waveform
    static constexpr FieldRange dca1Kf    { 16,  2 };
    static constexpr FieldRange dcw1Kf    { 18,  2 };
    static constexpr FieldRange dca1End   { 20,  1 };
    static constexpr FieldRange dca1Env   { 21, 16 };
    static constexpr FieldRange dcw1End   { 37,  1 };
    static constexpr FieldRange dcw1Env   { 38, 16 };
    static constexpr FieldRange dco1End   { 54,  1 };
    static constexpr FieldRange dco1Env   { 55, 16 };
    static constexpr FieldRange sfw       { 71,  2 };  // DCO2 waveform
    static constexpr FieldRange dca2Kf    { 73,  2 };
    static constexpr FieldRange dcw2Kf    { 75,  2 };
    static constexpr FieldRange dca2End   { 77,  1 };
    static constexpr FieldRange dca2Env   { 78, 16 };
    static constexpr FieldRange dcw2End   { 94,  1 };
    static constexpr FieldRange dcw2Env   { 95, 16 };
    static constexpr FieldRange dco2End   { 111, 1 };
    static constexpr FieldRange dco2Env   { 112,16 };
}

constexpr int kNumDataBytes   = 128;
constexpr int kNumNibbles     = 256;
constexpr int kFrameSize      = 264; // F0 ... F7 inklusive
constexpr uint8_t kManufacturerCasio = 0x44;
constexpr uint8_t kCommandToneDump   = 0x20;

//==============================================================================
/** Haelt die 128 rohen Datenbytes eines CZ-Tones und bietet benannten Zugriff
    auf die einzelnen Felder. */
class CZTone
{
public:
    std::array<uint8_t, kNumDataBytes> raw {};

    CZTone() { raw.fill(0); }

    uint8_t*       field (FieldRange f)       { return raw.data() + f.offset; }
    const uint8_t* field (FieldRange f) const { return raw.data() + f.offset; }

    uint8_t  get (FieldRange f, int idx = 0) const { return raw[f.offset + idx]; }
    void     set (FieldRange f, int idx, uint8_t v) { raw[f.offset + idx] = v; }

    static CZTone makeDefault();
};

std::array<uint8_t, kNumDataBytes> decodePayload (const uint8_t* nibbles256);
std::vector<uint8_t>               encodePayload (const std::array<uint8_t, kNumDataBytes>& data);

struct ParsedFrame
{
    int    channel  = 0;
    int    toneNum  = 0x20;
    CZTone tone;
};

std::vector<uint8_t> buildSysEx (int toneNum, const CZTone& tone, int channel = 0);
ParsedFrame parseSysEx (const std::vector<uint8_t>& frame);
std::vector<std::vector<uint8_t>> extractFrames (const std::vector<uint8_t>& stream);

uint8_t encodeDcaLevel (int level0to99);
int     decodeDcaLevel (uint8_t byteVal);
uint8_t encodeDcaRate  (int rate0to99, bool levelFalls);
void    decodeDcaRate  (uint8_t byteVal, int& rateOut, bool& levelFallsOut);

uint8_t encodeDcwLevel (int level0to99, bool sustainPoint);
void    decodeDcwLevel (uint8_t byteVal, int& levelOut, bool& sustainOut);
inline uint8_t encodeDcwRate (int rate0to99, bool levelFalls) { return encodeDcaRate (rate0to99, levelFalls); }
inline void    decodeDcwRate (uint8_t b, int& r, bool& f)     { decodeDcaRate (b, r, f); }

uint8_t encodeDcoLevel (int level0to99);
int     decodeDcoLevel (uint8_t byteVal);
inline uint8_t encodeDcoRate (int rate0to99, bool levelFalls) { return encodeDcaRate (rate0to99, levelFalls); }
inline void    decodeDcoRate (uint8_t b, int& r, bool& f)     { decodeDcaRate (b, r, f); }

} // namespace cz
