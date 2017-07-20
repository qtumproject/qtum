#include <libethcore/Transaction.h>


class VersionVM{

public:

    VersionVM(){
        vmFormat = 0;
        rootVM = 1;
        vmVersion = 0;
        flagOptions = 0;
    }

    VersionVM(uint32_t _rawVersion) : rawVersion(_rawVersion){
        expandData();
    }

    uint8_t getVMFormat(){ return vmFormat; }
    uint8_t getRootVM(){ return rootVM; }
    uint8_t getVMVersion(){ return vmVersion; }
    uint8_t getFlagOptions(){ return flagOptions; }

    uint32_t getRawVersion(){ return rawVersion; }

    bool operator!=(VersionVM& v){
        if(this->vmFormat != v.vmFormat || this->rootVM != v.rootVM ||
           this->vmVersion != v.vmVersion || this->flagOptions != v.flagOptions){
            return true;
        }
        return false;
    }

private:

    void expandData();

    uint8_t vmFormat : 2;
    uint8_t rootVM : 6;
    uint8_t vmVersion : 8;
    uint16_t flagOptions : 16;

    uint32_t rawVersion;
};

class QtumTransaction : public dev::eth::Transaction{

public:

    QtumTransaction() : nVout(0) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
		dev::eth::Transaction(_value, _gasPrice, _gas, _data, _nonce) {}

    QtumTransaction(dev::u256 const& _value, dev::u256 const& _gasPrice, dev::u256 const& _gas, dev::Address const& _dest, dev::bytes const& _data, dev::u256 const& _nonce = dev::Invalid256):
		dev::eth::Transaction(_value, _gasPrice, _gas, _dest, _data, _nonce) {}

    void setHashWith(const dev::h256 hash) { m_hashWith = hash; }

    dev::h256 getHashWith() const { return m_hashWith; }

    void setNVout(uint32_t vout) { nVout = vout; }

    uint32_t getNVout() const { return nVout; }

    void setVersion(VersionVM v){
        version=v;
    }
    VersionVM getVersion(){
        return version;
    }
private:

    uint32_t nVout;
    VersionVM version;

};
