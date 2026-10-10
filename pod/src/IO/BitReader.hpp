#pragma once

#include <cstdint>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>

namespace pod
{

    class BitReader
    {
      public:
        enum class Endianness
        {
            Big,
            Little
        };

        BitReader(std::span<const uint8_t> data, Endianness endian = Endianness::Big);

        BitReader(std::span<const uint8_t> parent,
                  size_t offset,
                  size_t length,
                  Endianness endian = Endianness::Big);

        BitReader(const BitReader& parent,
                  size_t offset,
                  size_t length,
                  Endianness endian = Endianness::Big);

        bool bit(size_t bitOffset) const;

        uint8_t u8(size_t bitOffset, size_t bitLength) const;
        uint16_t u16(size_t bitOffset, size_t bitLength) const;
        uint32_t u32(size_t bitOffset, size_t bitLength) const;
        int32_t i32(size_t bitOffset, size_t bitLength) const;
        uint64_t u64(size_t bitOffset, size_t bitLength) const;

        std::string str(size_t bitOffset, size_t bitLength) const;

        friend std::ostream& operator<<(std::ostream& os, const BitReader& br);

      private:
        std::span<const uint8_t> m_data;
        Endianness m_endian;

        void validate(size_t bitOffset, size_t bitLength) const;

        template <typename T>
        T readUnsigned(size_t bitOffset, size_t bitLength) const;
    };

    std::ostream& operator<<(std::ostream& os, const BitReader& br);

} // namespace pod