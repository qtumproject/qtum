#include <qtum/qtumutils.h>
#include <libdevcore/CommonData.h>
#include <pubkey.h>
#include <util/convert.h>
#include <chainparams.h>
#include <chain.h>

using namespace dev;

bool qtumutils::btc_ecrecover(const dev::h256 &hash, const dev::u256 &v, const dev::h256 &r, const dev::h256 &s, dev::h256 &key)
{
    // Check input parameters
    if(v >= 256)
    {
        // Does not fit into 1 byte
        return false;
    }

    // Convert the data into format usable for btc
    CPubKey pubKey;
    std::vector<unsigned char> vchSig;
    vchSig.push_back((unsigned char)v);
    vchSig += r.asBytes();
    vchSig += s.asBytes();
    uint256 mesage = h256Touint(hash);

    // Recover public key from compact signature (65 bytes)
    // The public key can be compressed (33 bytes) or uncompressed (65 bytes)
    // Pubkeyhash is RIPEMD160 hash of the public key, handled both types
    if(pubKey.RecoverCompact(mesage, vchSig))
    {
        // Get the pubkeyhash
        CKeyID id = pubKey.GetID();
        size_t padding = sizeof(key) - sizeof(id);
        memset(key.data(), 0, padding);
        memcpy(key.data() + padding, id.begin(), sizeof(id));
        return true;
    }

    return false;
}

struct EthChainIdCache
{
    EthChainIdCache() {}
    uint16_t nDefaultPort = 0;
    int beforeShanghaiChainId = 0;
    int afterShanghaiChainId = 0;
};

int qtumutils::eth_getChainId(int blockHeight, int shanghaiHeight, const ChainType& chain)
{
    if (chain == ChainType::MAIN || blockHeight < shanghaiHeight)
        return ChainIdType::MAIN;

    if (chain == ChainType::REGTEST || chain == ChainType::UNITTEST)
        return ChainIdType::REGTEST;

    return ChainIdType::TESTNET;
}

int qtumutils::eth_getChainId(int blockHeight)
{
    const CChainParams& params = Params();
    int shanghaiHeight = params.GetConsensus().nShanghaiHeight;
    uint16_t nDefaultPort = params.GetDefaultPort();
    static EthChainIdCache idCache;
    if(idCache.nDefaultPort != nDefaultPort)
    {
        ChainType chain = params.GetChainType();
        idCache.nDefaultPort = nDefaultPort;
        idCache.beforeShanghaiChainId = eth_getChainId(0, shanghaiHeight, chain);
        idCache.afterShanghaiChainId = eth_getChainId(shanghaiHeight, shanghaiHeight, chain);
    }

    return blockHeight < shanghaiHeight ? idCache.beforeShanghaiChainId : idCache.afterShanghaiChainId;
}

dev::Address qtumutils::eth_getHistoryStorageAddress()
{

    const CChainParams& chainparams = Params();
    dev::Address addr = uintToh160(chainparams.GetConsensus().historyStorageAddress);
    return addr;
}

qtumutils::HistoricalHashes &qtumutils::HistoricalHashes::instance()
{
    // Get instance
    static qtumutils::HistoricalHashes _instance;
    return _instance;
}

void qtumutils::HistoricalHashes::set(CBlockIndex *tip)
{
    if (m_tip != tip)
    {
        clear();
    }
    m_tip = tip;
}

bool qtumutils::HistoricalHashes::get(const dev::u256 &blockHeight, dev::h256 &hash)
{
    // Check the tip
    if (!m_tip)
        return false;
    if (blockHeight > (dev::h256) m_tip->nHeight)
        return false;

    // Update the list of hashes
    update();

    // Get the hash from the list
    int height = (int) blockHeight;
    if (m_hashes.contains(height)) {
        hash = m_hashes[height];
        return true;
    }

    return false;
}

qtumutils::HistoricalHashes::HistoricalHashes()
{
    clear();
}

void qtumutils::HistoricalHashes::clear() {
    m_hashes.clear();
}

void qtumutils::HistoricalHashes::update()
{
    // Check if update is needed
    if (needUpdate()) {
        clear();

        // Get Pectra fork height
        const CChainParams& params = Params();
        int pectraHeight = params.GetConsensus().nPectraHeight;

        // Add the last 8191 hashes, or until Pectra fork is reached, or not enough blocks
        const CBlockIndex *tip = m_tip;
        for(int i = 0; i < m_historyWindow; i++){
            if(!tip)
                break;
            if(tip->nHeight < pectraHeight)
                break;
            m_hashes[tip->nHeight] = uintToh256(*tip->phashBlock);
            tip = tip->pprev;
        }
    }
}

bool qtumutils::HistoricalHashes::needUpdate()
{
    // Update if the tip hash is changed
    bool ret = true;
    if (m_tip && m_hashes.contains(m_tip->nHeight) && uintToh256(*m_tip->phashBlock) == m_hashes[m_tip->nHeight]) {
        ret = false;
    }
    return ret;
}
