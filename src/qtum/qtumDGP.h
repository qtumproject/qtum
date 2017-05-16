#include "qtumstate.h"

class QtumDGP {
    
public:

    QtumDGP(const dev::Address _dgpContract) : dgpContract(_dgpContract){}

    dev::eth::EVMSchedule getGasSchedule(QtumState* state, unsigned int blockHeight);

private:

    void initStorageDGP(QtumState* state);

    void initStorageSchedule(QtumState* state, const dev::Address& addr);

    void createParamsInstance();

    dev::Address getAddressForBlock(unsigned int blockHeight);

    void parseStorageScheduleContract(std::vector<uint32_t>& uint32Values);

    dev::eth::EVMSchedule createEVMSchedule();

    const dev::Address dgpContract;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageDGP;

    std::map<dev::h256, std::pair<dev::u256, dev::u256>> storageSchedule;

    std::vector<std::pair<unsigned int, dev::Address>> paramsInstance;

};
