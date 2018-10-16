#include "x86lib.h"
#include "x86test.h"


using namespace std;
using namespace x86Lib;

TEST_CASE("encoding test", "[Encoding]" ){
    uint8_t tmp[] = {0x00, 0x91, 0x00, 0x00, 0x00, 0x05, 0x55, 0x12, 0x00, 0x1f};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumCompressPayload(payload);
    REQUIRE(result.size() == 11);
    REQUIRE(result[0] == 0x00);
    REQUIRE(result[1] == 0x01);
    REQUIRE(result[2] == 0x91);
    REQUIRE(result[3] == 0x00);
    REQUIRE(result[4] == 0x03);
}

TEST_CASE("Big encode test", "[Encoding]" ){
    std::vector<uint8_t> payload;
    payload.push_back(0x53);
    payload.push_back(0x11);
    for(int i = 0; i < 500 ; i++){
        payload.push_back(0);
    }
    payload.push_back(0xff);
    //expected: 53 11 00 ff 00 ff 00 10 ff
    std::vector<uint8_t> result = qtumCompressPayload(payload);
    REQUIRE(result.size() == 7);
    REQUIRE(result[0] == 0x53);
    REQUIRE(result[1] == 0x11);
    REQUIRE(result[2] == 0x00);
    REQUIRE(result[3] == 0xff);
    REQUIRE(result[4] == 0x00);
    REQUIRE(((int)result[5]) == 0xf5);
    REQUIRE(result[6] == 0xff);
}

TEST_CASE("decoding test", "[Encoding" ){
    //00 55 00 00 ff fc 00 00
    //size: 8
    uint8_t tmp[] = {0x00, 0x01, 0x55, 0x00, 0x01, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x02};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(8, payload);

    REQUIRE(result.size() == 8);
    REQUIRE(result[0] == 0x00);
    REQUIRE(result[1] == 0x55);
    REQUIRE(result[2] == 0x00);
    REQUIRE(result[3] == 0x00);
    REQUIRE(result[4] == 0xff);
    REQUIRE(result[5] == 0xfc);
    REQUIRE(result[6] == 0x00);
    REQUIRE(result[7] == 0x00);
}

TEST_CASE("decoding error test", "[Encoding" ){
    {
        //double zero error
        uint8_t tmp[] = {0x00, 0x01, 0x55, 0x00, 0x00};
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(10, payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size too big error
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(5, payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size too small error
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(3, payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size sanity check
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(4, payload);
        REQUIRE(result.size() == 4);
    }
}

TEST_CASE("Big decode test", "[Encoding]") {
    //actual size: 255 + 255 + 16 + 3 = 529
    uint8_t tmp[] = {0x00, 0xff, 0x00, 0xff, 0x00, 0x10, 0x55, 0xff, 0x00, 0x01};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(529, payload);
    REQUIRE(result.size() == 529);
    for(int i = 0; i < 255+255+16; i++){
        REQUIRE(result[i] == 0);
    }
    REQUIRE((int)result[255 + 255 + 16] == 0x55);
}

TEST_CASE("Big decode overrun test", "[Encoding]") {
    //actual size: 256 + 256 + 16 + 3 = 531
    uint8_t tmp[] = {0x00, 0xff, 0x00, 0xff, 0x00, 0x10, 0x55, 0xff, 0x00, 0x01};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(100, payload);
    REQUIRE(result.size() == 0);
}

TEST_CASE("Encode Decode test", "[Encoding]"){
    uint8_t tmp[] = {0x00, 0x91, 0x00, 0x00, 0x00, 0x05, 0x55, 0x12, 0x00, 0x1f, 0x00, 0x10, 0xff};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> compressed = qtumCompressPayload(payload);
    std::vector<uint8_t> decompressed = qtumDecompressPayload(payload.size(), compressed);
    REQUIRE(decompressed == payload);
}


