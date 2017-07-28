#include "consensus.h"
#include "primitives/transaction.h"

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
unsigned int MAX_BLOCK_SERIALIZED_SIZE = 8000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
unsigned int MAX_BLOCK_WEIGHT = 8000000;
/** The maximum allowed size for a block excluding witness data, in bytes (network rule) */
unsigned int MAX_BLOCK_BASE_SIZE = 2000000;

unsigned int MAX_BLOCK_DGP_SIZE = 2000000; // qtum

/** The maximum allowed number of signature check operations in a block (network rule) */
int64_t MAX_BLOCK_SIGOPS_COST = 80000;

unsigned int MAX_PROTOCOL_MESSAGE_LENGTH = 8000000;

void updateBlockSizeParams(unsigned int newBlockSize){
    unsigned int newSizeForParams=WITNESS_SCALE_FACTOR*newBlockSize;
    MAX_BLOCK_SERIALIZED_SIZE=newSizeForParams;
    MAX_BLOCK_WEIGHT=newSizeForParams;
    MAX_BLOCK_BASE_SIZE=newSizeForParams;
    MAX_BLOCK_SIGOPS_COST=(int64_t)(newSizeForParams/100);
    MAX_PROTOCOL_MESSAGE_LENGTH=newSizeForParams;
}
