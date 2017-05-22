#include "qtumDGP.h"

dev::eth::EVMSchedule QtumDGP::getGasSchedule(QtumState* state, unsigned int blockHeight){
    clear();
    dev::eth::EVMSchedule schedule = dev::eth::EIP158Schedule;
    initStorageDGP(state, DGPCONTRACT1);
    createParamsInstance();
    dev::Address address = getAddressForBlock(blockHeight);
    if(address != dev::Address()){
        initStorageTemplate(state, address);
        schedule = createEVMSchedule();
    }
    return schedule;
}

uint32_t QtumDGP::getBlockSize(QtumState* state, unsigned int blockHeight){
    clear();
    uint32_t blockSize = 0;
    initStorageDGP(state, DGPCONTRACT2);
    createParamsInstance();
    dev::Address address = getAddressForBlock(blockHeight);
    if(address != dev::Address()){
        initStorageTemplate(state, address);
        parseStorageBlockSizeContract(blockSize);
    }
    return blockSize;
}

void QtumDGP::initStorageDGP(QtumState* state, const dev::Address& addr){
    storageDGP = state->storage(addr);
}

void QtumDGP::initStorageTemplate(QtumState* state, const dev::Address& addr){
    storageTemplate = state->storage(addr);
}

void QtumDGP::createParamsInstance(){
    dev::h256 paramsInstanceHash = sha3(dev::h256("0000000000000000000000000000000000000000000000000000000000000000"));
    if(storageDGP.count(paramsInstanceHash)){
        dev::u256 paramsInstanceSize = storageDGP.find(paramsInstanceHash)->second.second;
        for(size_t i = 0; i < size_t(paramsInstanceSize); i++){
            std::pair<unsigned int, dev::Address> params;
            params.first = uint64_t(storageDGP.find(sha3(paramsInstanceHash))->second.second);
            ++paramsInstanceHash;
            params.second = dev::right160(dev::h256(storageDGP.find(sha3(paramsInstanceHash))->second.second));
            ++paramsInstanceHash;
            paramsInstance.push_back(params);
        }
    }
}

dev::Address QtumDGP::getAddressForBlock(unsigned int blockHeight){
    for(auto i = paramsInstance.rbegin(); i != paramsInstance.rend(); i++){
        if(i->first <= blockHeight)
            return i->second;
    }
    return dev::Address();
}

void QtumDGP::parseStorageScheduleContract(std::vector<uint32_t>& uint32Values){
    std::vector<std::pair<dev::u256, dev::u256>> data;
    for(size_t i = 0; i < 5; i++){
        dev::h256 gasScheduleHash = sha3(dev::h256(dev::u256(i)));
        if(storageTemplate.count(gasScheduleHash)){
            dev::u256 key = storageTemplate.find(gasScheduleHash)->second.first;
            dev::u256 value = storageTemplate.find(gasScheduleHash)->second.second;
            data.push_back(std::make_pair(key, value));
        }
    }

    std::sort(data.begin(), data.end(), [&data](std::pair<dev::u256, dev::u256>& a, std::pair<dev::u256, dev::u256>& b){
        return a.first < b.first;
    });

    for(std::pair<dev::u256, dev::u256> d : data){
        dev::u256 value = d.second;
        for(size_t i = 0; i < 4; i++){
            uint64_t uint64Value = uint64_t(value);
            value = value >> 64;

            uint32Values.push_back(uint32_t(uint64Value));
            uint64Value = uint64Value >> 32;
            uint32Values.push_back(uint32_t(uint64Value));
        }
    }
}

void QtumDGP::parseStorageBlockSizeContract(uint32_t& blockSize){
    dev::h256 blockSizeHash = sha3(dev::h256(dev::u256(0)));
    if(storageTemplate.count(blockSizeHash)){
        blockSize = uint32_t(storageTemplate.find(blockSizeHash)->second.second);
    }
}

dev::eth::EVMSchedule QtumDGP::createEVMSchedule(){
    dev::eth::EVMSchedule schedule = dev::eth::EIP158Schedule;
    std::vector<uint32_t> uint32Values;
    parseStorageScheduleContract(uint32Values);
    if(uint32Values.size() >= 39){
        schedule.tierStepGas = {uint32Values[0], uint32Values[1], uint32Values[2], uint32Values[3],
                                uint32Values[4], uint32Values[5], uint32Values[6], uint32Values[7]};
        schedule.expGas = uint32Values[8];
        schedule.expByteGas = uint32Values[9];
        schedule.sha3Gas = uint32Values[10];
        schedule.sha3WordGas = uint32Values[11];
        schedule.sloadGas = uint32Values[12];
        schedule.sstoreSetGas = uint32Values[13];
        schedule.sstoreResetGas = uint32Values[14];
        schedule.sstoreRefundGas = uint32Values[15];
        schedule.jumpdestGas = uint32Values[16];
        schedule.logGas = uint32Values[17];
        schedule.logDataGas = uint32Values[18];
        schedule.logTopicGas = uint32Values[19];
        schedule.createGas = uint32Values[20];
        schedule.callGas = uint32Values[21];
        schedule.callStipend = uint32Values[22];
        schedule.callValueTransferGas = uint32Values[23];
        schedule.callNewAccountGas = uint32Values[24];
        schedule.suicideRefundGas = uint32Values[25];
        schedule.memoryGas = uint32Values[26];
        schedule.quadCoeffDiv = uint32Values[27];
        schedule.createDataGas = uint32Values[28];
        schedule.txGas = uint32Values[29];
        schedule.txCreateGas = uint32Values[30];
        schedule.txDataZeroGas = uint32Values[31];
        schedule.txDataNonZeroGas = uint32Values[32];
        schedule.copyGas = uint32Values[33];
        schedule.extcodesizeGas = uint32Values[34];
        schedule.extcodecopyGas = uint32Values[35];
        schedule.balanceGas = uint32Values[36];
        schedule.suicideGas = uint32Values[37];
        schedule.maxCodeSize = uint32Values[38];
    }
    return schedule;
}

void QtumDGP::clear(){
    templateContract = dev::Address();
    storageDGP.clear();
    storageTemplate.clear();
    paramsInstance.clear();
}
