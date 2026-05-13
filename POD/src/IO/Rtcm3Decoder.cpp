#include "Rtcm3Decoder.hpp"

#include "BitReader.hpp"
#include "RtcmUtils.hpp"

using namespace gnsstk;

namespace pod
{

    void Rtcm3Decoder::run()
    {
        uchar buff[MAX_MSG_LEN];

        std::map<int, rtcm3_msg_uptr> parsers;
        for (auto&& it : msgsToParse)
            parsers[it->getID()] = it->clone();

        while (true)
        {
            uchar c = source->readByte();
            if (c == PREAMBLE)
            {
                std::cout << c << std::endl;
                uchar lenchars[2];
                source->readBytes(lenchars, 2);
                BitReader len_bsp(lenchars, 0, 2);
                std::cout << len_bsp << std::endl;
                ushort len = len_bsp.u32(6, 10);
                // std::memcpy(&len, lenchars, 2);

                // std::cout << (int)(lenchars[0]) <<' '<< (int)lenchars[1] << std::endl;
                // std::cout << (len) << std::endl;
                if (len < MAX_MSG_LEN)
                {
                    source->readBytes(buff, 3, len + 3);
                    buff[0] = PREAMBLE;
                    buff[1] = lenchars[0];
                    buff[2] = lenchars[1];

                    bool check = RtcmUtils::crc24q_check(buff, len + 6);
                    // std::cout << check << ": " << len << std::endl;
                    if (check)
                    {
                        BitReader msg_bsp(buff, 3, len);
                        int msgId = msg_bsp.u32(0, 12);
                        auto msg = parsers.find(msgId);
                        if (msg != parsers.end())
                            msg->second->parse(msg_bsp, *this);
                    }
                }
            }
        }
    }
} // namespace pod