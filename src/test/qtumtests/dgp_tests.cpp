#include <boost/test/unit_test.hpp>
#include <qtumtests/test_utils.h>
#include <script/standard.h>

namespace dgpTest{

std::vector<valtype> code = {
    /*setInitialAdmin()*/
    valtype(ParseHex("6fb81cbb")),
    /*
        contract gasSchedule{
        uint32[39] _gasSchedule=[
            10, //0: tierStepGas0
            10, //1: tierStepGas1
            10, //2: tierStepGas2
            10, //3: tierStepGas3
            10, //4: tierStepGas4
            10, //5: tierStepGas5
            10, //6: tierStepGas6
            10, //7: tierStepGas7
            10, //8: expGas
            50, //9: expByteGas
            30, //10: sha3Gas
            6, //11: sha3WordGas
            200, //12: sloadGas
            20000, //13: sstoreSetGas
            5000, //14: sstoreResetGas
            15000, //15: sstoreRefundGas
            1, //16: jumpdestGas
            375, //17: logGas
            8, //18: logDataGas
            375, //19: logTopicGas
            32000, //20: createGas
            700, //21: callGas
            2300, //22: callStipend
            9000, //23: callValueTransferGas
            25000, //24: callNewAccountGas
            24000, //25: suicideRefundGas
            3, //26: memoryGas
            512, //27: quadCoeffDiv
            200, //28: createDataGas
            21000, //29: txGas
            53000, //30: txCreateGas
            4, //31: txDataZeroGas
            68, //32: txDataNonZeroGas
            3, //33: copyGas
            700, //34: extcodesizeGas
            700, //35: extcodecopyGas
            400, //36: balanceGas
            5000, //37: suicideGas
            24576 //38: maxCodeSize
        ];
        function getSchedule() constant returns(uint32[39] _schedule){
            return _gasSchedule;
        }
        }
    */
    valtype(ParseHex("60606040526104e060405190810160405280600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161138861ffff16815260200161600061ffff1681525060009060278260276007016008900481019282156102645791602002820160005b8382111561023257835183826101000a81548163ffffffff021916908361ffff16021790555092602001926004016020816003010492830192600103026101f0565b80156102625782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610232565b505b50905061029791905b8082111561029357600081816101000a81549063ffffffff02191690555060010161026d565b5090565b505034610000575b61015f806102ae6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003e575b610000565b346100005761004b610097565b6040518082602760200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b6104e0604051908101604052806027905b600063ffffffff168152602001906001900390816100a8579050506000602780602002604051908101604052809291908260278015610128576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100eb5790505b505050505090505b905600a165627a7a72305820d97944c56fa577d97d4b111b48437da64662431b4792b937dbe94f4e07d8e2380029")),
    /*addAddressProposal(address,uint256)*/
    valtype(ParseHex("bf5f1e83000000000000000000000000c4c1d7375918557df2ef8f1d1f0b2329cb248a150000000000000000000000000000000000000000000000000000000000000002")),
    /*
        contract gasSchedule{
        uint32[39] _gasSchedule=[
            13, //0: tierStepGas0
            10, //1: tierStepGas1
            10, //2: tierStepGas2
            10, //3: tierStepGas3
            10, //4: tierStepGas4
            10, //5: tierStepGas5
            10, //6: tierStepGas6
            10, //7: tierStepGas7
            10, //8: expGas
            50, //9: expByteGas
            30, //10: sha3Gas
            6, //11: sha3WordGas
            200, //12: sloadGas
            20000, //13: sstoreSetGas
            5000, //14: sstoreResetGas
            15000, //15: sstoreRefundGas
            1, //16: jumpdestGas
            375, //17: logGas
            8, //18: logDataGas
            375, //19: logTopicGas
            32000, //20: createGas
            700, //21: callGas
            2300, //22: callStipend
            9000, //23: callValueTransferGas
            25000, //24: callNewAccountGas
            24000, //25: suicideRefundGas
            3, //26: memoryGas
            512, //27: quadCoeffDiv
            200, //28: createDataGas
            21000, //29: txGas
            53000, //30: txCreateGas
            4, //31: txDataZeroGas
            68, //32: txDataNonZeroGas
            3, //33: copyGas
            700, //34: extcodesizeGas
            700, //35: extcodecopyGas
            400, //36: balanceGas
            5000, //37: suicideGas
            300 //38: maxCodeSize
        ];
        function getSchedule() constant returns(uint32[39] _schedule){
            return _gasSchedule;
        }
        }
    */
    valtype(ParseHex("60606040526104e060405190810160405280600d61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161138861ffff16815260200161012c61ffff1681525060009060276101df9291906101f0565b5034156101eb57600080fd5b6102c4565b8260276007016008900481019282156102805791602002820160005b8382111561024e57835183826101000a81548163ffffffff021916908361ffff160217905550926020019260040160208160030104928301926001030261020c565b801561027e5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261024e565b505b50905061028d9190610291565b5090565b6102c191905b808211156102bd57600081816101000a81549063ffffffff021916905550600101610297565b5090565b90565b610163806102d36000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082602760200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006027806020026040519081016040528092919082602780156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6104e0604051908101604052806027905b600063ffffffff1681526020019060019003908161011957905050905600a165627a7a7230582079c1e5b0792e2fe1427cf30f190dfc698bbda4a883da02b06f5baf0f9151e11d0029")),
    /*addAddressProposal(address,uint256)*/
    valtype(ParseHex("bf5f1e83000000000000000000000000c943d775eab8664c7b13f04ffa058bbc718329890000000000000000000000000000000000000000000000000000000000000002")),
    /*
        contract gasSchedule{
        uint32[39] _gasSchedule=[
            13, //0: tierStepGas0
            13, //1: tierStepGas1
            10, //2: tierStepGas2
            10, //3: tierStepGas3
            10, //4: tierStepGas4
            10, //5: tierStepGas5
            10, //6: tierStepGas6
            10, //7: tierStepGas7
            10, //8: expGas
            50, //9: expByteGas
            30, //10: sha3Gas
            6, //11: sha3WordGas
            200, //12: sloadGas
            20000, //13: sstoreSetGas
            5000, //14: sstoreResetGas
            15000, //15: sstoreRefundGas
            1, //16: jumpdestGas
            375, //17: logGas
            8, //18: logDataGas
            375, //19: logTopicGas
            32000, //20: createGas
            700, //21: callGas
            2300, //22: callStipend
            9000, //23: callValueTransferGas
            25000, //24: callNewAccountGas
            24000, //25: suicideRefundGas
            3, //26: memoryGas
            512, //27: quadCoeffDiv
            200, //28: createDataGas
            21000, //29: txGas
            53000, //30: txCreateGas
            4, //31: txDataZeroGas
            68, //32: txDataNonZeroGas
            3, //33: copyGas
            700, //34: extcodesizeGas
            700, //35: extcodecopyGas
            400, //36: balanceGas
            600, //37: suicideGas
            300 //38: maxCodeSize
        ];
        function getSchedule() constant returns(uint32[39] _schedule){
            return _gasSchedule;
        }
        }
    */
    valtype(ParseHex("60606040526104e060405190810160405280600d61ffff168152602001600d61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161025861ffff16815260200161012c61ffff1681525060009060276101df9291906101f0565b5034156101eb57600080fd5b6102c4565b8260276007016008900481019282156102805791602002820160005b8382111561024e57835183826101000a81548163ffffffff021916908361ffff160217905550926020019260040160208160030104928301926001030261020c565b801561027e5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261024e565b505b50905061028d9190610291565b5090565b6102c191905b808211156102bd57600081816101000a81549063ffffffff021916905550600101610297565b5090565b90565b610163806102d36000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082602760200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006027806020026040519081016040528092919082602780156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6104e0604051908101604052806027905b600063ffffffff1681526020019060019003908161011957905050905600a165627a7a723058205c544799034ae8d28755e0042259700a20f21c66a2c5131334ce69d08d36df270029")),
    /*addAddressProposal(address,uint256)*/
    valtype(ParseHex("bf5f1e8300000000000000000000000012feb9a596fadb1070d3aa60633658735ef34b790000000000000000000000000000000000000000000000000000000000000002")),
    /*
        contract blockSize{
            uint32[1] _blockSize=[
                1000000 //block size in bytes
            ];
            function getBlockSize() constant returns(uint32[1] _size){
                return _blockSize;
            }
        }
    */
    valtype(ParseHex("6060604052602060405190810160405280620f424062ffffff16815250600090600161002c92919061003a565b50341561003557fe5b61010f565b8260016007016008900481019282156100cb5791602002820160005b8382111561009957835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610056565b80156100c95782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610099565b505b5090506100d891906100dc565b5090565b61010c91905b8082111561010857600081816101000a81549063ffffffff0219169055506001016100e2565b5090565b90565b6101698061011e6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003b575bfe5b341561004357fe5b61004b610097565b6040518082600160200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000600180602002604051908101604052809291908260018015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011f57905050905600a165627a7a723058200b657beb7573194625eb5be9fc9072280a82296a70274ce97ad54d6a9ad0ceb30029")),
    /*
        contract blockSize{
            uint32[1] _blockSize=[
                2000000 //block size in bytes
            ];
            function getBlockSize() constant returns(uint32[1] _size){
                return _blockSize;
            }
        }
    */
    valtype(ParseHex("6060604052602060405190810160405280621e848062ffffff16815250600090600161002c92919061003a565b50341561003557fe5b61010f565b8260016007016008900481019282156100cb5791602002820160005b8382111561009957835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610056565b80156100c95782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610099565b505b5090506100d891906100dc565b5090565b61010c91905b8082111561010857600081816101000a81549063ffffffff0219169055506001016100e2565b5090565b90565b6101698061011e6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003b575bfe5b341561004357fe5b61004b610097565b6040518082600160200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000600180602002604051908101604052809291908260018015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011f57905050905600a165627a7a72305820b849e0a25b57c96e2ae1c2acf855080420b2c6123d3bd477695e3d189d047db20029")),
    /*
        contract blockSize{
            uint32[1] _blockSize=[
                500123 //block size in bytes
            ];
            function getBlockSize() constant returns(uint32[1] _size){
                return _blockSize;
            }
        }
    */
    valtype(ParseHex("60606040526020604051908101604052806207a19b62ffffff16815250600090600161002c92919061003d565b50341561003857600080fd5b610112565b8260016007016008900481019282156100ce5791602002820160005b8382111561009c57835183826101000a81548163ffffffff021916908362ffffff1602179055509260200192600401602081600301049283019260010302610059565b80156100cc5782816101000a81549063ffffffff021916905560040160208160030104928301926001030261009c565b505b5090506100db91906100df565b5090565b61010f91905b8082111561010b57600081816101000a81549063ffffffff0219169055506001016100e5565b5090565b90565b610162806101216000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806392ac3c621461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a72305820ee7f7c9b6420f2f5b76e514b8e4fbda9d2157592281d2be78d1321b6c4428f100029")),
    /*
        contract minGasPrice{
            uint32[1] _minGasPrice=[
                13 //min gas price in satoshis
            ];
            function getMinGasPrice() constant returns(uint32[1] _gasPrice){
                return _minGasPrice;
            }
        }
    */
    valtype(ParseHex("6060604052602060405190810160405280600d60ff168152506000906001610028929190610036565b50341561003157fe5b610109565b8260016007016008900481019282156100c55791602002820160005b8382111561009357835183826101000a81548163ffffffff021916908360ff1602179055509260200192600401602081600301049283019260010302610052565b80156100c35782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610093565b505b5090506100d291906100d6565b5090565b61010691905b8082111561010257600081816101000a81549063ffffffff0219169055506001016100dc565b5090565b90565b610169806101186000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633fb588191461003b575bfe5b341561004357fe5b61004b610097565b6040518082600160200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000600180602002604051908101604052809291908260018015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011f57905050905600a165627a7a72305820cc36549938177e1eb1f75e1c29460e706caac3ba259710e09610baec608493650029")),
    /*
        contract minGasPrice{
            uint32[1] _minGasPrice=[
                9850 //min gas price in satoshis
            ];
            function getMinGasPrice() constant returns(uint32[1] _gasPrice){
                return _minGasPrice;
            }
        }
    */
    valtype(ParseHex("606060405260206040519081016040528061267a61ffff16815250600090600161002a92919061003b565b50341561003657600080fd5b61010f565b8260016007016008900481019282156100cb5791602002820160005b8382111561009957835183826101000a81548163ffffffff021916908361ffff1602179055509260200192600401602081600301049283019260010302610057565b80156100c95782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610099565b505b5090506100d891906100dc565b5090565b61010c91905b8082111561010857600081816101000a81549063ffffffff0219169055506001016100e2565b5090565b90565b6101628061011e6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633fb588191461003e575b600080fd5b341561004957600080fd5b610051610090565b6040518082600160200280838360005b8381101561007d5780820151818401525b602081019050610061565b5050505090500191505060405180910390f35b610098610108565b60006001806020026040519081016040528092919082600180156100fd576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c05790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011857905050905600a165627a7a72305820f01192afe1c50ff7ea0b6f684092bf05620c47a1a48583935d04610d47c384a40029")),
    /*
        contract minGasPrice{
            uint32[1] _minGasPrice=[
                123 //min gas price in satoshis
            ];
            function getMinGasPrice() constant returns(uint32[1] _gasPrice){
                return _minGasPrice;
            }
        }
    */
    valtype(ParseHex("6060604052602060405190810160405280607b60ff168152506000906001610028929190610036565b50341561003157fe5b610109565b8260016007016008900481019282156100c55791602002820160005b8382111561009357835183826101000a81548163ffffffff021916908360ff1602179055509260200192600401602081600301049283019260010302610052565b80156100c35782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610093565b505b5090506100d291906100d6565b5090565b61010691905b8082111561010257600081816101000a81549063ffffffff0219169055506001016100dc565b5090565b90565b610169806101186000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff1680633fb588191461003b575bfe5b341561004357fe5b61004b610097565b6040518082600160200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b61009f61010f565b6000600180602002604051908101604052809291908260018015610104576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100c75790505b505050505090505b90565b6020604051908101604052806001905b600063ffffffff1681526020019060019003908161011f57905050905600a165627a7a72305820bfd8ad217275ac01e0ff45cf88c44acf60c31d36f6cf200711eb17d6831a54cc0029"))
};

struct EVMScheduleCustom : public dev::eth::EVMSchedule{
    EVMScheduleCustom(bool v1,bool v2,bool v3,bool v4,std::array<unsigned, 8> v5,unsigned v6,unsigned v7,unsigned v8,unsigned v9,unsigned v10,unsigned v11,
                      unsigned v12,unsigned v13,unsigned v14,unsigned v15,unsigned v16,unsigned v17,unsigned v18,unsigned v19,unsigned v20,unsigned v21,
                      unsigned v22,unsigned v23,unsigned v24,unsigned v25,unsigned v26,unsigned v27,unsigned v28,unsigned v29,unsigned v30,unsigned v31,
                      unsigned v32,unsigned v33,unsigned v34,unsigned v35,unsigned v36)
    {
        exceptionalFailedCodeDeposit = v1;
        haveDelegateCall = v2;
        eip150Mode = v3;
        eip158Mode = v4;
        tierStepGas = v5;
        expGas = v6;
        expByteGas = v7;
        sha3Gas = v8;
        sha3WordGas = v9;
        sloadGas = v10;
        sstoreSetGas = v11;
        sstoreResetGas = v12;
        sstoreRefundGas = v13;
        jumpdestGas = v14;
        logGas = v15;
        logDataGas = v16;
        logTopicGas = v17;
        createGas = v18;
        callGas = v19;
        callStipend = v20;
        callValueTransferGas = v21;
        callNewAccountGas = v22;
        suicideRefundGas = v23;
        memoryGas = v24;
        quadCoeffDiv = v25;
        createDataGas = v26;
        txGas = v27;
        txCreateGas = v28;
        txDataZeroGas = v29;
        txDataNonZeroGas = v30;
        copyGas = v31;
        extcodesizeGas = v32;
        extcodecopyGas = v33;
        balanceGas = v34;
        suicideGas = v35;
        maxCodeSize = v36;
    }
};

EVMScheduleCustom EVMScheduleContractGasSchedule(true,true,true,true,{{10,10,10,10,10,10,10,10}},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,5000,24576);
EVMScheduleCustom EVMScheduleContractGasSchedule2(true,true,true,true,{{13,10,10,10,10,10,10,10}},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,5000,300);
EVMScheduleCustom EVMScheduleContractGasSchedule3(true,true,true,true,{{13,13,10,10,10,10,10,10}},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,600,300);

dev::h256 hash = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

void contractLoading(){
    dev::eth::ChainParams cp((dev::eth::genesisInfo(dev::eth::Network::qtumMainNetwork)));
    globalState->populateFrom(cp.genesisState);
    globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());
    globalState->db().commit();
}

bool compareEVMSchedule(const dev::eth::EVMSchedule& a, const dev::eth::EVMSchedule& b){
    if(a.tierStepGas == b.tierStepGas && a.expGas == b.expGas && a.expByteGas == b.expByteGas &&
    a.sha3Gas == b.sha3Gas && a.sha3WordGas == b.sha3WordGas && a.sloadGas == b.sloadGas &&
    a.sstoreSetGas == b.sstoreSetGas && a.sstoreResetGas == b.sstoreResetGas && a.sstoreRefundGas == b.sstoreRefundGas &&
    a.jumpdestGas == b.jumpdestGas && a.logGas == b.logGas && a.logDataGas == b.logDataGas &&
    a.logTopicGas == b.logTopicGas && a.createGas == b.createGas && a.callGas == b.callGas &&
    a.callStipend == b.callStipend && a.callValueTransferGas == b.callValueTransferGas &&
    a.callNewAccountGas == b.callNewAccountGas && a.suicideRefundGas == b.suicideRefundGas &&
    a.memoryGas == b.memoryGas && a.quadCoeffDiv == b.quadCoeffDiv && a.createDataGas == b.createDataGas &&
    a.txGas == b.txGas && a.txCreateGas == b.txCreateGas && a.txDataZeroGas == b.txDataZeroGas &&
    a.txDataNonZeroGas == b.txDataNonZeroGas && a.copyGas == b.copyGas && a.extcodesizeGas == b.extcodesizeGas &&
    a.extcodecopyGas == b.extcodecopyGas && a.balanceGas == b.balanceGas && a.suicideGas == b.suicideGas &&
    a.maxCodeSize == b.maxCodeSize && a.exceptionalFailedCodeDeposit == b.exceptionalFailedCodeDeposit &&
    a.haveDelegateCall == b.haveDelegateCall && a.eip150Mode == b.eip150Mode && a.eip158Mode == b.eip158Mode)
        return true;
    return false;
}

bool compareUint64(const uint64_t& value1, const uint64_t& value2){
    if(value1 == value2)
        return true;
    return false;
}

void createTestContractsAndBlocks(TestChain100Setup* testChain100Setup, valtype& code1, valtype& code2, valtype& code3, dev::Address addr){
    std::function<void(size_t n)> generateBlocks = [&](size_t n){
        dev::h256 oldHashStateRoot = globalState->rootHash();
        dev::h256 oldHashUTXORoot = globalState->rootHashUTXO();
        for(size_t i = 0; i < n; i++)
            testChain100Setup->CreateAndProcessBlock({}, GetScriptForRawPubKey(testChain100Setup->coinbaseKey.GetPubKey()));
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
    };

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, addr, 0));
    txs.push_back(createQtumTransaction(code1, 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, addr, 0));
    auto result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code2, 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[4], 0, dev::u256(500000), dev::u256(1), ++hashTemp, addr, 0));
    result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code3, 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[6], 0, dev::u256(500000), dev::u256(1), ++hashTemp, addr, 0));
    result = executeBC(txs);
}

template <typename T>
void checkValue(T value, T value1, T value2, T value3, T value4, size_t i, std::function<bool(T&,T&)> func){
    if(i > 599)
        BOOST_CHECK(func(value, value4));
    if(599 > i && i > 550)
        BOOST_CHECK(func(value, value3));
    if(550 > i && i > 501)
        BOOST_CHECK(func(value, value2));
    if(501 > i && i > 0) // After initializing the tests, the height of the chain 502
        BOOST_CHECK(func(value, value1));
}

BOOST_FIXTURE_TEST_SUITE(dgp_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(gas_schedule_default_state_test1){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(100);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(gas_schedule_default_state_test2){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(0);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(gas_schedule_one_paramsInstance_introductory_block_1_test1){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, GasScheduleDGP, 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, GasScheduleDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(0);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(gas_schedule_one_paramsInstance_introductory_block_1_test2){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, GasScheduleDGP, 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, GasScheduleDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(502); // After initializing the tests, the height of the chain 502
    BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule));
}

BOOST_AUTO_TEST_CASE(gas_schedule_passage_from_0_to_130_three_paramsInstance_test){
    initState();
    contractLoading();    
    createTestContractsAndBlocks(this, code[1], code[3], code[5], GasScheduleDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 0; i < 1300; i++){
        dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(i);
        std::function<bool(const dev::eth::EVMSchedule&, const dev::eth::EVMSchedule&)> func = compareEVMSchedule;
        checkValue<dev::eth::EVMSchedule>(schedule, dev::eth::EIP158Schedule, EVMScheduleContractGasSchedule,
            EVMScheduleContractGasSchedule2, EVMScheduleContractGasSchedule3, i, func);
    }
}

BOOST_AUTO_TEST_CASE(gas_schedule_passage_from_130_to_0_three_paramsInstance_test){
    initState();
    contractLoading();
    
    createTestContractsAndBlocks(this, code[1], code[3], code[5], GasScheduleDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 1300; i > 0; i--){
        dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(i);
        std::function<bool(const dev::eth::EVMSchedule&, const dev::eth::EVMSchedule&)> func = compareEVMSchedule;
        checkValue<dev::eth::EVMSchedule>(schedule, dev::eth::EIP158Schedule, EVMScheduleContractGasSchedule,
            EVMScheduleContractGasSchedule2, EVMScheduleContractGasSchedule3, i, func);
    }
}

BOOST_AUTO_TEST_CASE(block_size_default_state_test1){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    uint32_t blockSize = qtumDGP.getBlockSize(100);
    BOOST_CHECK(blockSize == DEFAULT_BLOCK_SIZE_DGP);
}

BOOST_AUTO_TEST_CASE(block_size_default_state_test2){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    uint32_t blockSize = qtumDGP.getBlockSize(0);
    BOOST_CHECK(blockSize == DEFAULT_BLOCK_SIZE_DGP);
}

BOOST_AUTO_TEST_CASE(block_size_one_paramsInstance_introductory_block_1_test1){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, BlockSizeDGP, 0));
    txs.push_back(createQtumTransaction(code[7], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, BlockSizeDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    uint32_t blockSize = qtumDGP.getBlockSize(0);
    BOOST_CHECK(blockSize == DEFAULT_BLOCK_SIZE_DGP);
}

BOOST_AUTO_TEST_CASE(block_size_one_paramsInstance_introductory_block_1_test2){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, BlockSizeDGP, 0));
    txs.push_back(createQtumTransaction(code[7], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, BlockSizeDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    uint32_t blockSize = qtumDGP.getBlockSize(502);
    BOOST_CHECK(blockSize == 1000000);
}

BOOST_AUTO_TEST_CASE(block_size_passage_from_0_to_130_three_paramsInstance_test){
    initState();
    contractLoading();
    
    createTestContractsAndBlocks(this, code[7], code[8], code[9], BlockSizeDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 0; i < 1300; i++){
        uint32_t blockSize = qtumDGP.getBlockSize(i);
        std::function<bool(const uint64_t&, const uint64_t&)> func = compareUint64;
        checkValue<uint64_t>(blockSize, DEFAULT_BLOCK_SIZE_DGP, 1000000, 2000000, 500123, i, func);
    }
}

BOOST_AUTO_TEST_CASE(block_size_passage_from_130_to_0_three_paramsInstance_test){
    initState();
    contractLoading();
    
    createTestContractsAndBlocks(this, code[7], code[8], code[9], BlockSizeDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 1300; i > 0; i--){
        uint32_t blockSize = qtumDGP.getBlockSize(i);
        std::function<bool(const uint64_t&, const uint64_t&)> func = compareUint64;
        checkValue<uint32_t>(blockSize, DEFAULT_BLOCK_SIZE_DGP, 1000000, 2000000, 500123, i, func);
    }
}

BOOST_AUTO_TEST_CASE(min_gas_price_default_state_test1){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    uint64_t minGasPrice = qtumDGP.getMinGasPrice(100);
    BOOST_CHECK(minGasPrice == 1);
}

BOOST_AUTO_TEST_CASE(min_gas_price_default_state_test2){
    initState();
    contractLoading();
    QtumDGP qtumDGP(globalState.get());
    uint64_t minGasPrice = qtumDGP.getMinGasPrice(0);
    BOOST_CHECK(minGasPrice == 1);
}

BOOST_AUTO_TEST_CASE(min_gas_price_one_paramsInstance_introductory_block_1_test1){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, GasPriceDGP, 0));
    txs.push_back(createQtumTransaction(code[10], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, GasPriceDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    uint64_t minGasPrice = qtumDGP.getMinGasPrice(0);
    BOOST_CHECK(minGasPrice == 1);
}

BOOST_AUTO_TEST_CASE(min_gas_price_one_paramsInstance_introductory_block_1_test2){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, GasPriceDGP, 0));
    txs.push_back(createQtumTransaction(code[10], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, GasPriceDGP, 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(globalState.get());
    uint64_t minGasPrice = qtumDGP.getMinGasPrice(502);
    BOOST_CHECK(minGasPrice == 13);
}

BOOST_AUTO_TEST_CASE(min_gas_price_passage_from_0_to_130_three_paramsInstance_test){
    initState();
    contractLoading();
    
    createTestContractsAndBlocks(this, code[10], code[11], code[12], GasPriceDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 0; i < 1300; i++){
        uint64_t minGasPrice = qtumDGP.getMinGasPrice(i);
        std::function<bool(const uint64_t&, const uint64_t&)> func = compareUint64;
        checkValue<uint64_t>(minGasPrice, 1, 13, 9850, 123, i, func);
    }
}

BOOST_AUTO_TEST_CASE(min_gas_price_passage_from_130_to_0_three_paramsInstance_test){
    initState();
    contractLoading();
    
    createTestContractsAndBlocks(this, code[10], code[11], code[12], GasPriceDGP);
    QtumDGP qtumDGP(globalState.get());
    for(size_t i = 1300; i > 0; i--){
        uint64_t minGasPrice = qtumDGP.getMinGasPrice(i);
        std::function<bool(const uint64_t&, const uint64_t&)> func = compareUint64;
        checkValue<uint64_t>(minGasPrice, 1, 13, 9850, 123, i, func);
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
