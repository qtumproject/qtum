#ifndef QTUMDGP_H
#define QTUMDGP_H

#include "qtumstate.h"
#include "primitives/block.h"
#include "validation.h"
#include "utilstrencodings.h"

static const dev::Address GasScheduleDGP = dev::Address("0000000000000000000000000000000000000080");
static const dev::Address BlockSizeDGP = dev::Address("0000000000000000000000000000000000000081");
static const dev::Address GasPriceDGP = dev::Address("0000000000000000000000000000000000000082");
static const dev::Address DGPCONTRACT4 = dev::Address("0000000000000000000000000000000000000083");
static const dev::Address DGPCONTRACT5 = dev::Address("0000000000000000000000000000000000000084");

class QtumDGP {
    
public:

    QtumDGP(QtumState* _state, bool _dgpevm = true) : dgpevm(_dgpevm), state(_state) {}

    dev::eth::EVMSchedule getGasSchedule(unsigned int blockHeight);

    uint32_t getBlockSize(unsigned int blockHeight);

    uint32_t getMinGasPrice(unsigned int blockHeight);

private:

    bool initStorages(const dev::Address& addr, unsigned int blockHeight, std::vector<unsigned char> data = std::vector<unsigned char>());

    void initStorageDGP(const dev::Address& addr);

    void initStorageTemplate(const dev::Address& addr);

    void initDataTemplate(const dev::Address& addr, std::vector<unsigned char>& data);

    void createParamsInstance();

    std::vector<ResultExecute> callContract(const dev::Address& addrContract, std::vector<unsigned char> opcode);

    dev::Address getAddressForBlock(unsigned int blockHeight);

    void parseStorageScheduleContract(std::vector<uint32_t>& uint32Values);
    
    void parseDataScheduleContract(std::vector<uint32_t>& uint32Values);

    void parseStorageOneUint32(uint32_t& blockSize);

    void parseDataOneUint32(uint32_t& value);

    dev::eth::EVMSchedule createEVMSchedule();

    void clear();



    bool dgpevm;

    const QtumState* state;

    dev::Address templateContract;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageDGP;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageTemplate;

    std::vector<unsigned char> dataTemplate;

    std::vector<std::pair<unsigned int, dev::Address>> paramsInstance;

};
#endif
