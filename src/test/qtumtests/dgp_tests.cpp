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
    valtype(ParseHex("bf5f1e83000000000000000000000000c0f06b47e5597d9aa25e489061b13ab37360b9100000000000000000000000000000000000000000000000000000000000000002")),
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
            13 //38: maxCodeSize
        ];
        function getSchedule() constant returns(uint32[39] _schedule){
            return _gasSchedule;
        }
        }
    */
    valtype(ParseHex("60606040526104e060405190810160405280600d61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff16815260200161138861ffff168152602001600d61ffff1681525060009060278260276007016008900481019282156102635791602002820160005b8382111561023157835183826101000a81548163ffffffff021916908361ffff16021790555092602001926004016020816003010492830192600103026101ef565b80156102615782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610231565b505b50905061029691905b8082111561029257600081816101000a81549063ffffffff02191690555060010161026c565b5090565b505034610000575b61015f806102ad6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003e575b610000565b346100005761004b610097565b6040518082602760200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b6104e0604051908101604052806027905b600063ffffffff168152602001906001900390816100a8579050506000602780602002604051908101604052809291908260278015610128576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100eb5790505b505050505090505b905600a165627a7a72305820de93b75d67c044eb3c99747a95987928bbe7b3c2da028bc2b11fcc9ed6ca26e50029")),
    /*addAddressProposal(address,uint256)*/
    valtype(ParseHex("bf5f1e830000000000000000000000003d2ea7bd95444ca8102d6bccfdfc58e43fb13a120000000000000000000000000000000000000000000000000000000000000002")),
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
            13, //37: suicideGas
            13 //38: maxCodeSize
        ];
        function getSchedule() constant returns(uint32[39] _schedule){
            return _gasSchedule;
        }
        }
    */
    valtype(ParseHex("60606040526104e060405190810160405280600d61ffff168152602001600d61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001600a61ffff168152602001603261ffff168152602001601e61ffff168152602001600661ffff16815260200160c861ffff168152602001614e2061ffff16815260200161138861ffff168152602001613a9861ffff168152602001600161ffff16815260200161017761ffff168152602001600861ffff16815260200161017761ffff168152602001617d0061ffff1681526020016102bc61ffff1681526020016108fc61ffff16815260200161232861ffff1681526020016161a861ffff168152602001615dc061ffff168152602001600361ffff16815260200161020061ffff16815260200160c861ffff16815260200161520861ffff16815260200161cf0861ffff168152602001600461ffff168152602001604461ffff168152602001600361ffff1681526020016102bc61ffff1681526020016102bc61ffff16815260200161019061ffff168152602001600d61ffff168152602001600d61ffff1681525060009060278260276007016008900481019282156102625791602002820160005b8382111561023057835183826101000a81548163ffffffff021916908361ffff16021790555092602001926004016020816003010492830192600103026101ee565b80156102605782816101000a81549063ffffffff0219169055600401602081600301049283019260010302610230565b505b50905061029591905b8082111561029157600081816101000a81549063ffffffff02191690555060010161026b565b5090565b505034610000575b61015f806102ac6000396000f30060606040526000357c0100000000000000000000000000000000000000000000000000000000900463ffffffff16806326fadbe21461003e575b610000565b346100005761004b610097565b6040518082602760200280838360008314610085575b80518252602083111561008557602082019150602081019050602083039250610061565b50505090500191505060405180910390f35b6104e0604051908101604052806027905b600063ffffffff168152602001906001900390816100a8579050506000602780602002604051908101604052809291908260278015610128576020028201916000905b82829054906101000a900463ffffffff1663ffffffff16815260200190600401906020826003010492830192600103820291508084116100eb5790505b505050505090505b905600a165627a7a72305820fef63e975e39c9a940efe66e72db5c50939f66a21b9c6e57ee0f27539fd2b5b20029")),
    /*addAddressProposal(address,uint256)*/
    valtype(ParseHex("bf5f1e83000000000000000000000000a8beea891fa7b7dfbbd2c6429d2253afae8d31640000000000000000000000000000000000000000000000000000000000000002"))
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

EVMScheduleCustom EVMScheduleContractGasSchedule(true,true,true,true,{10,10,10,10,10,10,10,10},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,5000,24576);
EVMScheduleCustom EVMScheduleContractGasSchedule2(true,true,true,true,{13,10,10,10,10,10,10,10},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,5000,13);
EVMScheduleCustom EVMScheduleContractGasSchedule3(true,true,true,true,{13,13,10,10,10,10,10,10},10,50,30,6,200,20000,5000,15000,
    1,375,8,375,32000,700,2300,9000,25000,24000,3,512,200,21000,53000,4,68,3,700,700,400,13,13);

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

BOOST_FIXTURE_TEST_SUITE(dgp_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(default_state_test1){
    initState();
    contractLoading();
    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(globalState.get(), 100);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(default_state_test2){
    initState();
    contractLoading();
    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(globalState.get(), 0);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(one_paramsInstance_introductory_block_1_test1){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(globalState.get(), 0);
    BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
}

BOOST_AUTO_TEST_CASE(one_paramsInstance_introductory_block_1_test2){
    initState();
    contractLoading();

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    auto result = executeBC(txs);

    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    dev::eth::EVMSchedule schedule = qtumDGP.getGasSchedule(globalState.get(), 17);
    BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule));
}

BOOST_AUTO_TEST_CASE(passage_from_0_to_130_three_paramsInstance_test){
    initState();
    contractLoading();
    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    
    std::function<void(size_t n)> generateBlocks = [&](size_t n){
        dev::h256 oldHashStateRoot = globalState->rootHash();
        dev::h256 oldHashUTXORoot = globalState->rootHashUTXO();
        for(size_t i = 0; i < 50; i++)
            CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
    };

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    auto result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code[3], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[4], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code[5], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[6], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    result = executeBC(txs);

    dev::eth::EVMSchedule schedule;
    for(size_t i = 0; i < 130; i++){
        schedule = qtumDGP.getGasSchedule(globalState.get(), i);
        if(i > 116)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule3));
        if(116 > i > 67)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule2));
        if(66 > i > 16)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule));
        if(16 > i > 0)
            BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
    }
}

BOOST_AUTO_TEST_CASE(passage_from_130_to_0_three_paramsInstance_test){
    initState();
    contractLoading();
    QtumDGP qtumDGP(dev::Address("0000000000000000000000000000000000000080"));
    
    std::function<void(size_t n)> generateBlocks = [&](size_t n){
        dev::h256 oldHashStateRoot = globalState->rootHash();
        dev::h256 oldHashUTXORoot = globalState->rootHashUTXO();
        for(size_t i = 0; i < 50; i++)
            CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
    };

    dev::h256 hashTemp(hash);
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(code[0], 0, dev::u256(500000), dev::u256(1), hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    txs.push_back(createQtumTransaction(code[1], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[2], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    auto result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code[3], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[4], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    result = executeBC(txs);

    generateBlocks(50);
    txs.clear();
    txs.push_back(createQtumTransaction(code[5], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address(), 0));
    txs.push_back(createQtumTransaction(code[6], 0, dev::u256(500000), dev::u256(1), ++hashTemp, dev::Address("0000000000000000000000000000000000000080"), 0));
    result = executeBC(txs);

    dev::eth::EVMSchedule schedule;
    for(size_t i = 130; i > 0; i--){
        schedule = qtumDGP.getGasSchedule(globalState.get(), i);
        if(i > 116)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule3));
        if(116 > i > 67)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule2));
        if(66 > i > 16)
            BOOST_CHECK(compareEVMSchedule(schedule, EVMScheduleContractGasSchedule));
        if(16 > i > 0)
            BOOST_CHECK(compareEVMSchedule(schedule, dev::eth::EIP158Schedule));
    }
}

BOOST_AUTO_TEST_SUITE_END()

}
