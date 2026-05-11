#include <climits>
#include <iomanip>

#include "BitReader.hpp"

namespace pod {

    BitReader::BitReader(std::span<const uint8_t> data, Endianness endian)
        : m_data(data), m_endian(endian)
    {
    }

    BitReader::BitReader(std::span<const uint8_t> parent,
        size_t offset,
        size_t length,
        Endianness endian)
        : m_endian(endian)
    {
        if (offset > parent.size()) {
            throw std::out_of_range("Span offset out of bounds");
        }

        if (offset + length > parent.size()) {
            throw std::out_of_range("Span length out of bounds");
        }
        m_data = parent.subspan(offset, length);
    }
    
    BitReader::BitReader(const BitReader& parent,
        size_t offset,
        size_t length,
        Endianness endian)
        : m_endian(endian)
    {
        if (offset > parent.m_data.size()) {
            throw std::out_of_range("Span offset out of bounds");
        }

        if (offset + length > parent.m_data.size()) {
            throw std::out_of_range("Span length out of bounds");
        }
        m_data = parent.m_data.subspan(offset, length);
    }

    void BitReader::validate(size_t bitOffset, size_t bitLength) const
    {
        if (bitLength == 0)
            throw std::invalid_argument("bitLength must be > 0");

        const size_t totalBits = m_data.size() * CHAR_BIT;
        if (bitOffset + bitLength > totalBits)
            throw std::out_of_range("Bit range out of bounds");
    }

    bool BitReader::bit(size_t bitOffset) const
    {
        validate(bitOffset, 1);

        const size_t byteIndex = bitOffset / CHAR_BIT;
        const size_t bitInByte = bitOffset % CHAR_BIT;

        // MSB first
        return (m_data[byteIndex] >> (7 - bitInByte)) & 1;
    }

    template<typename T>
    T BitReader::readUnsigned(size_t bitOffset, size_t bitLength) const
    {
        validate(bitOffset, bitLength);

        if (bitLength > sizeof(T) * CHAR_BIT)
            throw std::invalid_argument("bitLength exceeds type width");

        T result = 0;

        for (size_t i = 0; i < bitLength; ++i) {
            const bool b = bit(bitOffset + i);

            if (!b)
                continue;

            if (m_endian == Endianness::Big) {
                result |= (T(1) << (bitLength - 1 - i));
            }
            else {
                result |= (T(1) << i);
            }
        }

        return result;
    }

    uint8_t BitReader::u8(size_t o, size_t n)  const { return readUnsigned<uint8_t >(o, n); }
    uint16_t BitReader::u16(size_t o, size_t n) const { return readUnsigned<uint16_t>(o, n); }
    uint32_t BitReader::u32(size_t o, size_t n) const { return readUnsigned<uint32_t>(o, n); }
    uint64_t BitReader::u64(size_t o, size_t n) const { return readUnsigned<uint64_t>(o, n); }

    int32_t BitReader::i32(size_t bitOffset, size_t bitLength) const
    {
        if (bitLength > 32)
            throw std::invalid_argument("bitLength exceeds int32");

        uint32_t u = readUnsigned<uint32_t>(bitOffset, bitLength);

        // two's complement sign extension
        if (u & (1u << (bitLength - 1))) {
            const uint32_t mask = (bitLength == 32)
                ? 0xFFFFFFFFu
                : ((1u << bitLength) - 1);
            return static_cast<int32_t>(u | ~mask);
        }

        return static_cast<int32_t>(u);
    }

    std::string BitReader::str(size_t bitOffset, size_t bitLength) const
    {
        if (bitLength % 8 != 0)
            throw std::invalid_argument("String must be byte aligned");

        validate(bitOffset, bitLength);

        const size_t count = bitLength / 8;
        std::string s;
        s.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            s.push_back(static_cast<char>(
                u8(bitOffset + i * 8, 8)
                ));
        }

        return s;
    }

    std::ostream& operator<<(std::ostream& os, const BitReader& br)
    {
        const auto data = br.m_data;

        os << std::hex << std::setfill('0');

        for (size_t i = 0; i < data.size(); ++i) {
            if (i > 0)
                os << ' ';

            os << "0x"
                << std::setw(2)
                << static_cast<unsigned>(data[i]);
        }

        os << std::dec;
        return os;
    }
} // namespace pod
