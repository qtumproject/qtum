#include "qtumstate.h"

static const dev::Address DGPCONTRACT1 = dev::Address("0000000000000000000000000000000000000080"); // gasSchedule
static const dev::Address DGPCONTRACT2 = dev::Address("0000000000000000000000000000000000000081"); // blockSize
static const dev::Address DGPCONTRACT3 = dev::Address("0000000000000000000000000000000000000082");
static const dev::Address DGPCONTRACT4 = dev::Address("0000000000000000000000000000000000000083");
static const dev::Address DGPCONTRACT5 = dev::Address("0000000000000000000000000000000000000084");

class QtumDGP {
    
public:

    QtumDGP(QtumState* _state) : state(_state){}

    dev::eth::EVMSchedule getGasSchedule(unsigned int blockHeight);

    uint32_t getBlockSize(unsigned int blockHeight);

    uint32_t getMinGasPrice(unsigned int blockHeight);

private:

    bool initStorages(const dev::Address& addr, unsigned int blockHeight);

    void initStorageDGP(const dev::Address& addr);

    void initStorageTemplate(const dev::Address& addr);

    void createParamsInstance();

    dev::Address getAddressForBlock(unsigned int blockHeight);

    void parseStorageScheduleContract(std::vector<uint32_t>& uint32Values);

    void parseStorageOneUint32(uint32_t& blockSize);

    dev::eth::EVMSchedule createEVMSchedule();

    void clear();

    const QtumState* state;

    dev::Address templateContract;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageDGP;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageTemplate;

    std::vector<std::pair<unsigned int, dev::Address>> paramsInstance;

};
