#include "x86lib.h"
#include "x86test.h"


using namespace std;
using namespace x86Lib;

TEST_CASE("encoding test", "[Encoding]" ){
    uint8_t tmp[] = {0x00, 0x91, 0x00, 0x00, 0x00, 0x05, 0x55, 0x12, 0x00, 0x1f};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumCompressPayload(payload);
    REQUIRE(result.size() == 11 + 4);
    uint32_t size = 0;
    memcpy(&size, result.data(), sizeof(uint32_t));
    REQUIRE(size == payload.size());
    REQUIRE(result[4] == 0x00);
    REQUIRE(result[5] == 0x01);
    REQUIRE(result[6] == 0x91);
    REQUIRE(result[7] == 0x00);
    REQUIRE(result[8] == 0x03);
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
    REQUIRE(result.size() == 7 + 4);
    uint32_t size = 0;
    memcpy(&size, result.data(), sizeof(uint32_t));
    REQUIRE(size == payload.size());
    REQUIRE(result[4] == 0x53);
    REQUIRE(result[5] == 0x11);
    REQUIRE(result[6] == 0x00);
    REQUIRE(result[7] == 0xff);
    REQUIRE(result[8] == 0x00);
    REQUIRE(((int)result[9]) == 0xf5);
    REQUIRE(result[10] == 0xff);
}

TEST_CASE("decoding test", "[Encoding" ){
    //00 55 00 00 ff fc 00 00
    //size: 8
    uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x55, 0x00, 0x01, 0x00, 0x01, 0xff, 0xfc, 0x00, 0x02};
    uint32_t size = 8;
    memcpy(tmp, &size, sizeof(uint32_t));
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(payload);

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
        uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x55, 0x00, 0x00};
        uint32_t size = 10;
        memcpy(tmp, &size, sizeof(uint32_t));
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size too big error
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        uint32_t size = 5;
        memcpy(tmp, &size, sizeof(uint32_t));
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size too small error
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        uint32_t size = 3;
        memcpy(tmp, &size, sizeof(uint32_t));
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(payload);
        REQUIRE(result.size() == 0);
    }
    {
        //size sanity check
        //actual size: 4
        uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x55, 0xff, 0x00, 0x01};
        uint32_t size = 4;
        memcpy(tmp, &size, sizeof(uint32_t));
        std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
        std::vector<uint8_t> result = qtumDecompressPayload(payload);
        REQUIRE(result.size() == 4);
    }
}

TEST_CASE("Big decode test", "[Encoding]") {
    //actual size: 255 + 255 + 16 + 3 = 529
    uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0x10, 0x55, 0xff, 0x00, 0x01};
    uint32_t size = 529;
    memcpy(tmp, &size, sizeof(uint32_t));
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(payload);
    REQUIRE(result.size() == 529);
    for(int i = 0; i < 255+255+16; i++){
        REQUIRE(result[i] == 0);
    }
    REQUIRE((int)result[255 + 255 + 16] == 0x55);
}

TEST_CASE("Big decode overrun test", "[Encoding]") {
    //actual size: 256 + 256 + 16 + 3 = 531
    uint8_t tmp[] = {0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x00, 0xff, 0x00, 0x10, 0x55, 0xff, 0x00, 0x01};
    uint32_t size = 100;
    memcpy(tmp, &size, sizeof(uint32_t));
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> result = qtumDecompressPayload(payload);
    REQUIRE(result.size() == 0);
}

TEST_CASE("Encode Decode test", "[Encoding]"){
    uint8_t tmp[] = {0x00, 0x91, 0x00, 0x00, 0x00, 0x05, 0x55, 0x12, 0x00, 0x1f, 0x00, 0x10, 0xff};
    std::vector<uint8_t> payload(tmp, tmp + sizeof tmp / sizeof tmp[0]);
    std::vector<uint8_t> compressed = qtumCompressPayload(payload);
    std::vector<uint8_t> decompressed = qtumDecompressPayload(compressed);
    REQUIRE(decompressed == payload);
}

//this caused a bug.. why?
std::string bugString = "00000000b00c00001400000000000000e920000000666666e863010000e82e080000c200000000000000000000000000bc00182000e8deffffffe884040000a1040000d083f801740db800000000e8a6030000cdf0f4b800000000e8ac010000ebf16690669066906690669066906690b8140010003d140010007424b80000000085c0741b5589e583ec146814001000ffd083c410c9c389f68dbc2700000000f3c38db426000000008dbc2700000000b8140010002d14001000c1f80289c2c1ea1f01d0d1f87428ba0000000085d2741f5589e583ec10506814001000ffd283c410c9c38db6000000008dbf00000000f3c38db426000000008dbc2700000000803d1400100000756755a11800100089e55653bb0c001000be0800100081eb08001000c1fb0283eb0139d873178d760083c001a318001000ff1486a11800100039d872ece827ffffffb80000000085c0741083ec0c68ec180000e8b1eeffff83c410c60514001000018d65f85b5e5dc3f3c38db426000000008dbc2700000000b80000000085c074275589e583ec10681c00100068ec180000e872eeffff83c410c9e909ffffff89f68dbc2700000000e9fbfeffff5589e583ec18c745f000000000c745f4000000006a088d45f0506a24ff7508e81103000083c4108b45f08b55f4c9c35589e583ec188b450c8945f08b45108945f46a088d45f0506a24ff7508e8c202000083c41090c9c35589e583ec18c745f0a0860100c745f400000000b8000000d083c00883ec046a0068a086010050e8acffffff83c4108b45f08b55f4b9000000d083c10883ec04525051e86003000083c410b800000000c9c35589e55383ec248b450c8945e08b45108945e4c745f000000000c745f400000000b8000000d083c00883ec0c50e825ffffff83c4108945f08955f48b45f08b55f43955e472173955e477053945e0760d83ec0c6898180000e8f7030000c745e800000000c745ec0000000083ec0cff7508e8e1feffff83c4108945e88955ec8b4de88b5dec8b45e08b55e401c811da8945e88955ec8b45f08b55f42b45e01b55e48945f08955f48b45e88b55ec83ec045250ff7508e8ccfeffff83c4108b45f08b55f4b9000000d083c10883ec04525051e8b0feffff83c4108b45f08b55f4b9000000d083c10883ec04525051e86402000083c4108b45e88b55ec83ec045250ff7508e84e02000083c41083ec086a088d45f050e83904000083c41083ec086a088d45e850e82804000083c410b8000000008b5dfcc9c35589e583ec1883ec0cff7508e80ffeffff83c4108945f08955f48b45f08b55f483ec045250ff7508e8f201000083c41083ec086a088d45f050e8dd03000083c410b800000000c9c35589e583ec08b8000000d083c00883ec0c50e8a1ffffff83c410c9c38d4c240483e4f0ff71fc5589e55183ec44b8000000d08b400485c0740ab800000000e99b000000c645f70083ec086a018d45f750e8f402000083c4100fb645f70fb6c083f801743283f803742685c0756483ec086a248d45c450e8ce02000083c41083ec0c8d45c450e82effffff83c410eb4fe86cffffffeb4883ec086a088d45e850e8a502000083c41083ec086a248d45c450e89402000083c4108b45e88b55ec83ec0452508d45c450e8b5fdffff83c410eb0d83ec0c68aa180000e8f70100008b4dfcc98d61fcc35589e55dc35589e583ec0c6a006a00ff7514ff7510ff750cff75086801100000e838030000c9c35589e583ec0c6a006a00ff7514ff7510ff750cff75086800100000e816030000c9c35589e583ec0c8b45108b551c6a0083e20fc1e00409d00fb6c050ff7518ff7514ff750cff75086a10e8e7020000c9c35531c089e557538b7d0c83cbff89d9f2ae8b7d08f7d18d51ff89d9f2ae89c85151f7d06a045248ff750c6a0450ff7508e89cffffff8d65f85b5f5dc35583c9ff89e5575383ec188b450c8b55088945f08b451089d78945f431c0f2ae6a016a0889c88d4df0f7d048516a045052e85fffffff8d65f85b5f5dc35589e583ec208b450c6a016a088945f08b45108945f48d45f0506a066a24ff7508e832ffffffc9c35589e583ec0c6a006a006a006a006a006a006806300000e828020000c9c35589e583ec0c6a006a006a006a006a006a006809300000e80a020000c9c35589e583ec0c6a006a006a006a006a006a00680a300000e8ec010000c9c35589e583ec0c6a006a006a006a00ff750cff7508680b300000e8cc010000c9c35589e583ec0c6a006a006a006a00ff750cff7508680c300000e8ac010000c9c35589e583ec0c6a006a006a006a006a006a00680e300000e88e010000c9c35589e583ec0c6a006a006a006a006a006a00680f300000e870010000c9c35589e583ec08e8d7ffffff5252ff750868e3180000e86ffeffffc7042406000000e86a0100005589e583ec08837d0800750b83ec0cff750ce8c3ffffffc9c35589e553508b5d1453ff7510ff750cff7508e8e2fdffff83c41039c3c7450cb71800000f94c08b5dfc0fb6c0894508c9e9b2ffffff5531c089e55383ec0c8b5d0c68cc18000085db0f95c050e896ffffff585a53ff7508e8e9feffff83c41039c3c7450cdd1800000f94c08b5dfc0fb6c0894508c9e96dffffff5531c089e55383ec0c8b5d0c68cc18000085db0f95c050e851ffffff585a53ff7508e8c4feffff83c41039c3c7450cdd1800000f94c08b5dfc0fb6c0894508c9e928ffffff5531c089e55383ec0c8b5d0c68cc18000085db0f95c050e80cffffff83c40c6a006a006a006a0053ff7508680d300000e83b00000083c4208b5dfcc9c35589e583ec08e89cfeffff8d450852526a0450e8abffffff5958ff750c68e3180000e827fdffffc7042407000000e8220000005589e55756538b45088b5d0c8b4d108b55148b75188b7d1c8b6d20cd405b5e5f5dc38b442404cdf0faf4669066906690a10000100083f8ff74365589e553bb0000100083ec048d76008dbc270000000083eb04ffd08b0383f8ff75f483c4045b5dc38db426000000008dbc2700000000f3c3e869f8ffffc200000000ff000000000000d2000000d04e6f7420656e6f75676820746f6b656e7300756e6b6e6f776e2063616c6c004d69736d617463686564206c6f61642073697a650073697a65206d757374206265203e203000737461636b206572726f72000000001400000000000000017a5200017c08011b0c0404880100001c0000001c00000099f8ffff2f00000000410e088502420d056bc50c040400001c0000003c000000a8f8ffff2800000000410e088502420d0564c50c040400001c0000005c000000b0f8ffff5200000000410e088502420d05024ec50c040400200000007c000000e2f8ffff3701000000410e088502420d05448303032f01c5c30c04041c000000a0000000f5f9ffff4800000000410e088502420d050244c50c0404001c000000c00000001dfaffff1c00000000410e088502420d0558c50c0404000028000000e000000019faffffca00000000440c0100471005027500430f03757c0602b70c010041c5430c04041c0000000c010000b7faffff0500000000410e088502420d0541c50c040400001c0000002c0100009cfaffff2200000000410e088502420d055ec50c040400001c0000004c0100009efaffff2200000000410e088502420d055ec50c040400001c0000006c010000a0faffff2f00000000410e088502420d056bc50c04040000240000008c010000affaffff3c00000000410e088502440d05428703830472c341c741c50c04040024000000b4010000c3faffff3d00000000410e088502450d0545870383046fc341c741c50c0404001c000000dc010000d8faffff2800000000410e088502420d0564c50c040400001c000000fc010000e0faffff1e00000000410e088502420d055ac50c040400001c0000001c020000defaffff1e00000000410e088502420d055ac50c040400001c0000003c020000dcfaffff1e00000000410e088502420d055ac50c040400001c0000005c020000dafaffff2000000000410e088502420d055cc50c040400001c0000007c020000dafaffff2000000000410e088502420d055cc50c040400001c0000009c020000dafaffff1e00000000410e088502420d055ac50c040400001c000000bc020000d8faffff1e00000000410e088502420d055ac50c0404000018000000dc020000d6faffff2600000000410e088502420d050000001c000000f8020000e0faffff1900000000410e088502420d0555c50c040400002000000018030000d9faffff3500000000410e088502420d054283036bc5c30c04040000200000003c030000eafaffff4500000000410e088502440d0544830377c5c30c0404000020000000600300000bfbffff4500000000410e088502440d0544830377c5c30c0404000020000000840300002cfbffff3d00000000410e088502440d0544830373c5c30c0404000018000000a803000045fbffff3300000000410e088502420d0500000000000000ffffffff00000000ffffffff0000000000000000";
std::vector<uint8_t> HexToBytes(const std::string& hex) {
  std::vector<uint8_t> bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    uint8_t byte = (uint8_t) strtol(byteString.c_str(), NULL, 16);
    bytes.push_back(byte);
  }

  return bytes;
}

TEST_CASE("Bug test 1", "[Encoding]"){
    std::vector<uint8_t> payload = HexToBytes(bugString);
    std::vector<uint8_t> compressed = qtumCompressPayload(payload);
    std::vector<uint8_t> decompressed = qtumDecompressPayload(compressed);
    REQUIRE(decompressed.size() == payload.size());
    REQUIRE(decompressed.size() == 3284);
    REQUIRE(compressed.size() == 3179);
    REQUIRE(decompressed == payload);
}


