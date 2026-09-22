#include <boost/test/unit_test.hpp>
#include <test/qtumtests/test_utils.h>
#include <qtum/qtumutils.h>
#include <chainparams.h>

namespace PectraTest{

const dev::u256 GASLIMIT = dev::u256(500000);
const dev::h256 HASHTX = dev::h256(ParseHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
int HISTORY_SERVE_WINDOW = 8191;

std::vector<unsigned char> concat(
    const std::vector<unsigned char>& a,
    const std::vector<unsigned char>& b)
{
    std::vector<unsigned char> result;
    result.reserve(a.size() + b.size());
    result.insert(result.end(), a.begin(), a.end());
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Codes used to check that pectra fork
const std::vector<valtype> CODE = {
    // EIP-2537
    /*
    pragma solidity ^0.8.0;

    contract PrecompileCheck {
        function verifyCallSuccess(
            address addr,
            bytes calldata data,
            bytes calldata result
        ) public view returns (bool) {
            (bool ok, bytes memory out) = address(addr).staticcall(data);
            require(ok, "Precompile call failed");
            bool ret = ok && isEqual(result, out);
            return ret;
        }
        
        function verifyCallFail(
            address addr,
            bytes calldata data
        ) public view returns (bool) {
            (bool ok, bytes memory out) = address(addr).staticcall(data);
            require(ok, "Precompile call failed");
            bool ret = ok == false && out.length == 0;
            return ret;
        }

        function isEqual(bytes calldata result, bytes memory out
        ) internal pure returns (bool) {
		    if (result.length != out.length) {
		        return false;
		    }
		    for (uint i = 0; i < result.length; i++) {
		        if (result[i] != out[i]) {
		            return false;
		        }
		    }
		    return true;
	    }
    }
    */
    valtype(ParseHex("6080604052348015600e575f5ffd5b506106178061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610034575f3560e01c8063889f55c914610038578063979c3bb114610068575b5f5ffd5b610052600480360381019061004d91906103c7565b610098565b60405161005f919061043e565b60405180910390f35b610082600480360381019061007d9190610457565b610167565b60405161008f919061043e565b60405180910390f35b5f5f5f8573ffffffffffffffffffffffffffffffffffffffff1685856040516100c2929190610524565b5f60405180830381855afa9150503d805f81146100fa576040519150601f19603f3d011682016040523d82523d5f602084013e6100ff565b606091505b509150915081610144576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161013b90610596565b60405180910390fd5b5f5f151583151514801561015857505f8251145b90508093505050509392505050565b5f5f5f8773ffffffffffffffffffffffffffffffffffffffff168787604051610191929190610524565b5f60405180830381855afa9150503d805f81146101c9576040519150601f19603f3d011682016040523d82523d5f602084013e6101ce565b606091505b509150915081610213576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161020a90610596565b60405180910390fd5b5f8280156102285750610227868684610239565b5b905080935050505095945050505050565b5f8151848490501461024d575f90506102fd565b5f5f90505b848490508110156102f7578281815181106102705761026f6105b4565b5b602001015160f81c60f81b7effffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff19168585838181106102b0576102af6105b4565b5b9050013560f81c60f81b7effffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff1916146102ea575f9150506102fd565b8080600101915050610252565b50600190505b9392505050565b5f5ffd5b5f5ffd5b5f73ffffffffffffffffffffffffffffffffffffffff82169050919050565b5f6103358261030c565b9050919050565b6103458161032b565b811461034f575f5ffd5b50565b5f813590506103608161033c565b92915050565b5f5ffd5b5f5ffd5b5f5ffd5b5f5f83601f84011261038757610386610366565b5b8235905067ffffffffffffffff8111156103a4576103a361036a565b5b6020830191508360018202830111156103c0576103bf61036e565b5b9250929050565b5f5f5f604084860312156103de576103dd610304565b5b5f6103eb86828701610352565b935050602084013567ffffffffffffffff81111561040c5761040b610308565b5b61041886828701610372565b92509250509250925092565b5f8115159050919050565b61043881610424565b82525050565b5f6020820190506104515f83018461042f565b92915050565b5f5f5f5f5f606086880312156104705761046f610304565b5b5f61047d88828901610352565b955050602086013567ffffffffffffffff81111561049e5761049d610308565b5b6104aa88828901610372565b9450945050604086013567ffffffffffffffff8111156104cd576104cc610308565b5b6104d988828901610372565b92509250509295509295909350565b5f81905092915050565b828183375f83830152505050565b5f61050b83856104e8565b93506105188385846104f2565b82840190509392505050565b5f610530828486610500565b91508190509392505050565b5f82825260208201905092915050565b7f507265636f6d70696c652063616c6c206661696c6564000000000000000000005f82015250565b5f61058060168361053c565b915061058b8261054c565b602082019050919050565b5f6020820190508181035f8301526105ad81610574565b9050919050565b7f4e487b71000000000000000000000000000000000000000000000000000000005f52603260045260245ffdfea2646970667358221220491270ba3ff352458ecf7585d3dcaa046d83b116e3f6938b746b048a6c967cf464736f6c634300081e0033")),
    //verifyCallSuccess add_G1
    valtype(ParseHex("979c3bb1000000000000000000000000000000000000000000000000000000000000000b0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000018000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e100000000000000000000000000000000112b98340eee2777cc3c14163dea3ec97977ac3dc5c70da32e6e87578f44912e902ccef9efe28d4a78b8999dfbca942600000000000000000000000000000000186b28d92356c4dfec4b5201ad099dbdede3781f8998ddf929b4cd7756192185ca7b8f4ef7088f813270ac3d48868a210000000000000000000000000000000000000000000000000000000000000080000000000000000000000000000000000a40300ce2dec9888b60690e9a41d3004fda4886854573974fab73b046d3147ba5b7a5bde85279ffede1b45b3918d82d0000000000000000000000000000000006d3d887e9f53b9ec4eb6cedf5607226754b07c01ace7834f57f3e7315faefb739e59018e22c492006190fba4a870025")),
    //verifyCallFail add_G1
    valtype(ParseHex("889f55c9000000000000000000000000000000000000000000000000000000000000000b000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000ff00000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e100000000000000000000000000000000112b98340eee2777cc3c14163dea3ec97977ac3dc5c70da32e6e87578f44912e902ccef9efe28d4a78b8999dfbca942600000000000000000000000000000000186b28d92356c4dfec4b5201ad099dbdede3781f8998ddf929b4cd7756192185ca7b8f4ef7088f813270ac3d48868a2100")),
    //verifyCallSuccess msm_G1
    valtype(ParseHex("979c3bb1000000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000012000000000000000000000000000000000000000000000000000000000000000a00000000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e100000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000080000000000000000000000000000000000572cbea904d67468808c8eb50a9450c9721db309128012543902d0ac358a62ae28f75bb8f1c7c42c39a8c5529bf0f4e00000000000000000000000000000000166a9d8cabc673a322fda673779d8e3822ba3ecb8670e461f73bb9021d5fd76a4c56d9d4cd16bd1bba86881979749d28")),
    //verifyCallFail msm_G1
    valtype(ParseHex("889f55c9000000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000009f00000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e1000000000000000000000000000000000000000000000000000000000000000200")),
    //verifyCallSuccess add_G2
    valtype(ParseHex("979c3bb1000000000000000000000000000000000000000000000000000000000000000d00000000000000000000000000000000000000000000000000000000000000600000000000000000000000000000000000000000000000000000000000000280000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801000000000000000000000000000000000606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be00000000000000000000000000000000103121a2ceaae586d240843a398967325f8eb5a93e8fea99b62b9f88d8556c80dd726a4b30e84a36eeabaf3592937f2700000000000000000000000000000000086b990f3da2aeac0a36143b7d7c824428215140db1bb859338764cb58458f081d92664f9053b50b3fbd2e4723121b68000000000000000000000000000000000f9e7ba9a86a8f7624aa2b42dcc8772e1af4ae115685e60abc2c9b90242167acef3d0be4050bf935eed7c3b6fc7ba77e000000000000000000000000000000000d22c3652d0dc6f0fc9316e14268477c2049ef772e852108d269d9c38dba1d4802e8dae479818184c08f9a569d8784510000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000b54a8a7b08bd6827ed9a797de216b8c9057b3a9ca93e2f88e7f04f19accc42da90d883632b9ca4dc38d013f71ede4db00000000000000000000000000000000077eba4eecf0bd764dce8ed5f45040dd8f3b3427cb35230509482c14651713282946306247866dfe39a8e33016fcbe520000000000000000000000000000000014e60a76a29ef85cbd69f251b9f29147b67cfe3ed2823d3f9776b3a0efd2731941d47436dc6d2b58d9e65f8438bad073000000000000000000000000000000001586c3c910d95754fef7a732df78e279c3d37431c6a2b77e67a00c7c130a8fcd4d19f159cbeb997a178108fffffcbd20")),
    //verifyCallFail add_G2
    valtype(ParseHex("889f55c9000000000000000000000000000000000000000000000000000000000000000d000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000001ff000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801000000000000000000000000000000000606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be00000000000000000000000000000000103121a2ceaae586d240843a398967325f8eb5a93e8fea99b62b9f88d8556c80dd726a4b30e84a36eeabaf3592937f2700000000000000000000000000000000086b990f3da2aeac0a36143b7d7c824428215140db1bb859338764cb58458f081d92664f9053b50b3fbd2e4723121b68000000000000000000000000000000000f9e7ba9a86a8f7624aa2b42dcc8772e1af4ae115685e60abc2c9b90242167acef3d0be4050bf935eed7c3b6fc7ba77e000000000000000000000000000000000d22c3652d0dc6f0fc9316e14268477c2049ef772e852108d269d9c38dba1d4802e8dae479818184c08f9a569d87845100")),
    //verifyCallSuccess msm_G2
    valtype(ParseHex("979c3bb1000000000000000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000001a0000000000000000000000000000000000000000000000000000000000000012000000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801000000000000000000000000000000000606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be00000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000001638533957d540a9d2370f17cc7ed5863bc0b995b8825e0ee1ea1e1e4d00dbae81f14b0bf3611b78c952aacab827a053000000000000000000000000000000000a4edef9c1ed7f729f520e47730a124fd70662a904ba1074728114d1031e1572c6c886f6b57ec72a6178288c47c33577000000000000000000000000000000000468fb440d82b0630aeb8dca2b5256789a66da69bf91009cbfe6bd221e47aa8ae88dece9764bf3bd999d95d71e4c9899000000000000000000000000000000000f6d4552fa65dd2638b361543f887136a43253d9c66c411697003f7a13c308f5422e1aa0a59c8967acdefd8b6e36ccf3")),
    //verifyCallFail msm_G2
    valtype(ParseHex("889f55c9000000000000000000000000000000000000000000000000000000000000000e0000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000011f000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801000000000000000000000000000000000606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be000000000000000000000000000000000000000000000000000000000000000200")),
    //verifyCallSuccess pairing_check
    valtype(ParseHex("979c3bb1000000000000000000000000000000000000000000000000000000000000000f0000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001800000000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000001")),
    //verifyCallFail pairing_check
    valtype(ParseHex("889f55c9000000000000000000000000000000000000000000000000000000000000000f000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000002ff00000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e100000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000ce5d527727d6e118cc9cdc6da2e351aadfd9baa8cbdd3a76d429a695160d12c923ac9cc3baca289e193548608b82801000000000000000000000000000000000606c4a02ea734cc32acd2b02bc28b99cb3e287e85a763af267492ab572e99ab3f370d275cec1da1aaa9075ff05f79be0000000000000000000000000000000017f1d3a73197d7942695638c4fa9ac0fc3688c4f9774b905a14e3a3f171bac586c55e83ff97a1aeffb3af00adb22c6bb0000000000000000000000000000000008b3f481e3aaa0f1a09e30ed741d8ae4fcf5e095d5d00af600db18cb2c04b3edd03cc744a2888ae40caa232946c5e7e100000000000000000000000000000000024aa2b2f08f0a91260805272dc51051c6e47ad4fa403b02b4510b647ae3d1770bac0326a805bbefd48056c8c121bdb80000000000000000000000000000000013e02b6052719f607dacd3a088274f65596bd0d09920b61ab5da61bbdc7f5049334cf11213945d57e5ac7d055d042b7e000000000000000000000000000000000d1b3cc2c7027888be51d9ef691d77bcb679afda66c73f17f9ee3837a55024f78c71363275a75d75d86bab79f74782aa0000000000000000000000000000000013fa4d4a0ad8b1ce186ed5061789213d993923066dddaf1040bc3ff59f825c78df74f2d75467e25e0f55f8a00fa030ed00")),
    //verifyCallSuccess map_fp_to_G1
    valtype(ParseHex("979c3bb10000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000156c8a6a2c184569d69a76be144b5cdc5141d2d2ca4fe341f011e25e3969c55ad9e9b9ce2eb833c81a908e5fa4ac5f03000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000184bb665c37ff561a89ec2122dd343f20e0f4cbcaec84e3c3052ea81d1834e192c426074b02ed3dca4e7676ce4ce48ba0000000000000000000000000000000004407b8d35af4dacc809927071fc0405218f1401a6d15af775810e4e460064bcc9468beeba82fdc751be70476c888bf3")),
    //verifyCallFail map_fp_to_G1
    valtype(ParseHex("889f55c900000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000003f00000000000000000000000000000000156c8a6a2c184569d69a76be144b5cdc5141d2d2ca4fe341f011e25e3969c55ad9e9b9ce2eb833c81a908e5fa4ac5f00")),
   //verifyCallSuccess map_fp2_to_G2
    valtype(ParseHex("979c3bb100000000000000000000000000000000000000000000000000000000000000110000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000010000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000000007355d25caf6e7f2f0cb2812ca0e513bd026ed09dda65b177500fa31714e09ea0ded3a078b526bed3307f804d4b93b040000000000000000000000000000000002829ce3c021339ccb5caf3e187f6370e1e2a311dec9b75363117063ab2015603ff52c3d3b98f19c2f65575e99e8b78c00000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000e7f4568a82b4b7dc1f14c6aaa055edf51502319c723c4dc2688c7fe5944c213f510328082396515734b6612c4e7bb700000000000000000000000000000000126b855e9e69b1f691f816e48ac6977664d24d99f8724868a184186469ddfd4617367e94527d4b74fc86413483afb35b000000000000000000000000000000000caead0fd7b6176c01436833c79d305c78be307da5f6af6c133c47311def6ff1e0babf57a0fb5539fce7ee12407b0a42000000000000000000000000000000001498aadcf7ae2b345243e281ae076df6de84455d766ab6fcdaad71fab60abb2e8b980a440043cd305db09d283c895e3d")),
    //verifyCallFail map_fp2_to_G2
    valtype(ParseHex("889f55c900000000000000000000000000000000000000000000000000000000000000110000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000007f0000000000000000000000000000000007355d25caf6e7f2f0cb2812ca0e513bd026ed09dda65b177500fa31714e09ea0ded3a078b526bed3307f804d4b93b040000000000000000000000000000000002829ce3c021339ccb5caf3e187f6370e1e2a311dec9b75363117063ab2015603ff52c3d3b98f19c2f65575e99e8b700")),
    // EIP-2935
    /*
    pragma solidity ^0.8.0;

    contract BlockHashChecks {
        address constant HISTORY_STORAGE_ADDRESS = 0x0000F90827F1C53a10cb7A02335B175320002935;
		function getBlockHash(uint256 blockNumber) external view (bytes32 hash) {
		    return blockhash(blockNumber);
		}

		function getHistoricalBlockHash(uint256 blockNumber) external returns (bytes32 hash) {
			bytes memory data = abi.encode(blockNumber);
			(bool success, bytes memory result) = HISTORY_STORAGE_ADDRESS.call(data);
			require(success && result.length == 32, "Fallback call failed or invalid response");
			assembly {
				hash := mload(add(result, 32))
			}
		}
    }
    */
valtype(ParseHex("6080604052348015600e575f5ffd5b506103958061001c5f395ff3fe608060405234801561000f575f5ffd5b5060043610610034575f3560e01c8063ee82ac5e14610038578063f2e8410c14610068575b5f5ffd5b610052600480360381019061004d91906101d5565b610098565b60405161005f9190610218565b60405180910390f35b610082600480360381019061007d91906101d5565b6100a2565b60405161008f9190610218565b60405180910390f35b5f81409050919050565b5f5f826040516020016100b59190610240565b60405160208183030381529060405290505f5f71f90827f1c53a10cb7a02335b17532000293573ffffffffffffffffffffffffffffffffffffffff16836040516100ff91906102ab565b5f604051808303815f865af19150503d805f8114610138576040519150601f19603f3d011682016040523d82523d5f602084013e61013d565b606091505b5091509150818015610150575060208151145b61018f576040517f08c379a000000000000000000000000000000000000000000000000000000000815260040161018690610341565b60405180910390fd5b60208101519350505050919050565b5f5ffd5b5f819050919050565b6101b4816101a2565b81146101be575f5ffd5b50565b5f813590506101cf816101ab565b92915050565b5f602082840312156101ea576101e961019e565b5b5f6101f7848285016101c1565b91505092915050565b5f819050919050565b61021281610200565b82525050565b5f60208201905061022b5f830184610209565b92915050565b61023a816101a2565b82525050565b5f6020820190506102535f830184610231565b92915050565b5f81519050919050565b5f81905092915050565b8281835e5f83830152505050565b5f61028582610259565b61028f8185610263565b935061029f81856020860161026d565b80840191505092915050565b5f6102b6828461027b565b915081905092915050565b5f82825260208201905092915050565b7f46616c6c6261636b2063616c6c206661696c6564206f7220696e76616c6964205f8201527f726573706f6e7365000000000000000000000000000000000000000000000000602082015250565b5f61032b6028836102c1565b9150610336826102d1565b604082019050919050565b5f6020820190508181035f8301526103588161031f565b905091905056fea2646970667358221220b971cd37d9ac7018cf9560dc047cbeb980b2b57244e740b7b2974f72f006b7bd64736f6c634300081e0033")),
    //getBlockHash
valtype(ParseHex("ee82ac5e")),
    //getHistoricalBlockHash
valtype(ParseHex("f2e8410c")),
};

// Codes IDs used to check that pectra fork is present
enum class CodeID
{
    precompileCheckContract = 0,
    verifyCallSuccessAddG1,
    verifyCallFailAddG1,
    verifyCallSuccessMsmG1,
    verifyCallFailMsmG1,
    verifyCallSuccessAddG2,
    verifyCallFailAddG2,
    verifyCallSuccessMsmG2,
    verifyCallFailMsmG2,
    verifyCallSuccessPairingCheck,
    verifyCallFailPairingCheck,
    verifyCallSuccessMapFpToG1,
    verifyCallFailMapFpToG1,
    verifyCallSuccessMapFp2ToG2,
    verifyCallFailMapFp2ToG2,
    blockHashContract,
    getBlockHash,
    getHistoricalBlockHash,
};

// Get the code identified by the ID
valtype getCode(CodeID id)
{
    return CODE[(int)id];
}

// Get the code identified by the ID
valtype getCode(CodeID id, dev::h256 nHeight)
{
    valtype vCode = CODE[(int)id];
    valtype vHeight = nHeight.asBytes();
    return concat(vCode, vHeight);
}

void genesisLoading(){
    const CChainParams& chainparams = Params();
    int coinbaseMaturity = Params().GetConsensus().CoinbaseMaturity(0);
    int forkHeight = coinbaseMaturity + 499;
    dev::eth::EVMConsensus evmConsensus;
    evmConsensus.QIP6Height = coinbaseMaturity;
    evmConsensus.QIP7Height = coinbaseMaturity;
    evmConsensus.nMuirGlacierHeight = coinbaseMaturity;
    evmConsensus.nLondonHeight = coinbaseMaturity;
    evmConsensus.nShanghaiHeight = coinbaseMaturity;
    evmConsensus.nCancunHeight = coinbaseMaturity;
    evmConsensus.nPectraHeight = forkHeight;
    UpdatePectraHeight(forkHeight);
    dev::eth::ChainParams cp(chainparams.EVMGenesisInfo(evmConsensus));
    globalState->populateFrom(cp.genesisState);
    globalSealEngine = std::unique_ptr<dev::eth::SealEngineFace>(cp.createSealEngine());
    globalState->db().commit();
}

void createNewBlocks(TestChain100Setup* testChain100Setup, size_t n){
    std::function<void(size_t n)> generateBlocks = [&](size_t n){
        dev::h256 oldHashStateRoot = globalState->rootHash();
        dev::h256 oldHashUTXORoot = globalState->rootHashUTXO();
        for(size_t i = 0; i < n; i++){
            testChain100Setup->CreateAndProcessBlock({}, GetScriptForRawPubKey(testChain100Setup->coinbaseKey.GetPubKey()));
        }
        globalState->setRoot(oldHashStateRoot);
        globalState->setRootUTXO(oldHashUTXORoot);
    };

    generateBlocks(n);
}
BOOST_FIXTURE_TEST_SUITE(pectrafork_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(checking_add_G1_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that add G1 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessAddG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailAddG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check add G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 55871);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check add G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492943);
}

BOOST_AUTO_TEST_CASE(checking_add_G1_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that add G1 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessAddG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailAddG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check add G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 31001);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check add G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 28723);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_msm_G1_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that msm G1 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMsmG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMsmG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check msm G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 65978);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check msm G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492918);
}

BOOST_AUTO_TEST_CASE(checking_msm_G1_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that msm G1 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMsmG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMsmG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check msm G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 29483);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check msm G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 27181);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_add_G2_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that add G2 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessAddG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailAddG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check add G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 88064);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check add G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492996);
}

BOOST_AUTO_TEST_CASE(checking_add_G2_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that add G2 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessAddG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailAddG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check add G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 36077);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check add G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 32123);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_msm_G2_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that msm G2 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMsmG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMsmG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check msm G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 106710);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check msm G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492945);
}

BOOST_AUTO_TEST_CASE(checking_msm_G2_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that msm G2 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMsmG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMsmG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check msm G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 32823);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check msm G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 28893);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_pairing_check_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that pairing check bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessPairingCheck), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailPairingCheck), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check pairing check bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 103511);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check pairing check bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 493048);
}

BOOST_AUTO_TEST_CASE(checking_pairing_check_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that pairing check bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessPairingCheck), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailPairingCheck), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check pairing check bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 28885);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check pairing check bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 35476);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_map_fp_to_G1_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that map fp to G1 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMapFpToG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMapFpToG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check map fp to G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 58485);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check map fp to G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492906);
}

BOOST_AUTO_TEST_CASE(checking_map_fp_to_G1_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that map fp to G1 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMapFpToG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMapFpToG1), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check map fp to G1 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 28481);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check map fp to G1 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 26191);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_map_fp2_to_G2_bls_after_fork){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that map fp2 to G2 bls precompiled contract exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMapFp2ToG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMapFp2ToG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check map fp2 to G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 106167);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(1));

    // Check map fp2 to G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 492916);
}

BOOST_AUTO_TEST_CASE(checking_map_fp2_to_G2_bls_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::precompileCheckContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Check that map fp2 to G2 bls precompiled contract doesn't exist
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txPectra;
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallSuccessMapFp2ToG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txPectra.push_back(createQtumTransaction(getCode(CodeID::verifyCallFailMapFp2ToG2), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txPectra, *m_node.chainman);

    // Check map fp2 to G2 bls with valid data
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 30965);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));

    // Check map fp2 to G2 bls with not valid data
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 27023);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_opcode_blockhash_and_history_blockhash_value){
    genesisLoading();
    createNewBlocks(this, 499);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::blockHashContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Prepare variables
    dev::h256 expectedResult;
    dev::h256 nHeight;
    dev::h256 nHeightBeforePectra;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(pindex != 0);
        expectedResult = uintToh256(*pindex->phashBlock);
        nHeight = (dev::h256) dev::u256(pindex->nHeight);
        nHeightBeforePectra = (dev::h256) dev::u256(pindex->nHeight - 1);
    }

    // Check that both opcode block hash and historical block hash has the same value when the index is found
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBlockHash;
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getBlockHash, nHeight), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeight), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightBeforePectra), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txBlockHash, *m_node.chainman);

    // Check opcode block hash value is the expected
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == expectedResult);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 21807);

    // Check historical block hash value is the expected
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == expectedResult);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 27379);

    // Check historical block hash value is not present before Pectra
    BOOST_CHECK(result.first[2].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(dev::h256(result.first[2].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_historical_precompile_contract_before_fork){
    genesisLoading();
    createNewBlocks(this, 498);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::blockHashContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Prepare variables
    dev::h256 nHeight;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(pindex != 0);
        nHeight = (dev::h256) dev::u256(pindex->nHeight);
    }

    // Get the historical block hash before the Pactra fork
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBlockHash;
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeight), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txBlockHash, *m_node.chainman);

    // Check historical block hash value is not present
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_historical_precompile_contract_edges){
    genesisLoading();
    createNewBlocks(this, 10000);
    dev::h256 hashTx(HASHTX);

    // Create contract
    std::vector<QtumTransaction> txs;
    txs.push_back(createQtumTransaction(getCode(CodeID::blockHashContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txs, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Prepare variables
    dev::h256 nHeightTip;
    dev::h256 expectedTip;
    dev::h256 nHeightLast;
    dev::h256 expectedLast;
    dev::h256 nHeightAfterTip;
    dev::h256 nHeightBeforeLast;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(pindex != 0);
        nHeightTip = (dev::h256) dev::u256(pindex->nHeight);
        expectedTip = uintToh256(*pindex->phashBlock);

        int nHeight = pindex->nHeight;
        pindex = m_node.chainman->ActiveChain()[nHeight - HISTORY_SERVE_WINDOW + 1];
        BOOST_CHECK(pindex != 0);
        nHeightLast = (dev::h256) dev::u256(pindex->nHeight);
        expectedLast = uintToh256(*pindex->phashBlock);

        nHeightAfterTip = (dev::h256) dev::u256(nHeight + 1);
        nHeightBeforeLast = (dev::h256) dev::u256(nHeight - HISTORY_SERVE_WINDOW);
    }

    // Get the historical block hash edges
    dev::Address proxy = createQtumAddress(txs[0].getHashWith(), txs[0].getNVout());
    std::vector<QtumTransaction> txBlockHash;
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightTip), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightLast), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightAfterTip), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightBeforeLast), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txBlockHash, *m_node.chainman);

    // Check tip block hash value is present
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == expectedTip);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 27379);

    // Check last block hash value is present
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[1].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == expectedLast);
    BOOST_CHECK(result.first[1].execRes.gasUsed == 27379);

    // Check after tip block hash value is not present
    BOOST_CHECK(result.first[2].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(dev::h256(result.first[2].execRes.output) == dev::h256(0));

    // Check before last block hash value is not present
    BOOST_CHECK(result.first[3].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(dev::h256(result.first[3].execRes.output) == dev::h256(0));
}

BOOST_AUTO_TEST_CASE(checking_historical_precompile_reorganize_blocks){

    /* -------------------------------
     * 1. Mine 9000 blocks
     * -------------------------------*/

    genesisLoading();
    createNewBlocks(this, 9000);
    dev::h256 hashTx(HASHTX);

    /* -------------------------------
     * 2. Check historical contract
     * -------------------------------*/

    // Create a contract that use the history precompile contract
    std::vector<QtumTransaction> txCreateBlockHash;
    txCreateBlockHash.push_back(createQtumTransaction(getCode(CodeID::blockHashContract), 0, GASLIMIT, dev::u256(1), ++hashTx, dev::Address()));
    auto result = executeBC(txCreateBlockHash, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Get tip height and expected value
    dev::h256 nHeightTip;
    dev::h256 oldExpectedValue;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain().Tip();
        BOOST_CHECK(pindex != 0);
        nHeightTip = (dev::h256) dev::u256(pindex->nHeight);
        oldExpectedValue = uintToh256(*pindex->phashBlock);
    }

    // Get the history block hash for the tip
    dev::Address proxy = createQtumAddress(txCreateBlockHash[0].getHashWith(), txCreateBlockHash[0].getNVout());
    std::vector<QtumTransaction> txGetBlockHash;
    txGetBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightTip), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txGetBlockHash, *m_node.chainman);

    // Check tip block hash value is present and expected
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == oldExpectedValue);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 27379);

    /* -------------------------------
     * 3. Remove 10 blocks
     * -------------------------------*/

    const CChain& active{*WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return &Assert(m_node.chainman)->ActiveChain())};
    auto* orig_tip = active.Tip();
    for (int i = 0; i < 10; ++i) {
        BlockValidationState state;
        m_node.chainman->ActiveChainstate().InvalidateBlock(state, active.Tip());
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight - 10);

    /* -------------------------------
     * 4. Check historical contract
     * -------------------------------*/

    // Call the contract that use the history storage after invalidate
    std::vector<QtumTransaction> txAfterInvalidate;
    txAfterInvalidate.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightTip), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txGetBlockHash, *m_node.chainman);

    // Check that the contract does not exist
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::Unknown);

    // Re-create the contract that use the history precompile contract
    result = executeBC(txCreateBlockHash, *m_node.chainman);
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);

    // Get the height of the last stored block hash and the height of the not stored
    dev::h256 expectedLast;
    dev::h256 nHeightLast;
    dev::h256 nHeightBeforeLast;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain().Tip();
        int nHeight = pindex->nHeight;
        pindex = m_node.chainman->ActiveChain()[nHeight - HISTORY_SERVE_WINDOW + 1];
        BOOST_CHECK(pindex != 0);
        nHeightLast = (dev::h256) dev::u256(pindex->nHeight);
        expectedLast = uintToh256(*pindex->phashBlock);
        nHeightBeforeLast = (dev::h256) dev::u256(nHeight - HISTORY_SERVE_WINDOW);
    }

    // Get the last history block hash
    std::vector<QtumTransaction> txLastBlockHash;
    txLastBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightLast), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    txLastBlockHash.push_back(createQtumTransaction(getCode(CodeID::getHistoricalBlockHash, nHeightBeforeLast), 0, GASLIMIT, dev::u256(1), ++hashTx, proxy));
    result = executeBC(txLastBlockHash, *m_node.chainman);

    // Check last block hash value is present and expected
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == expectedLast);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 27379);

    // Check the block hash is not stored due to buffer max size
    BOOST_CHECK(result.first[1].execRes.excepted == dev::eth::TransactionException::RevertInstruction);
    BOOST_CHECK(dev::h256(result.first[1].execRes.output) == dev::h256(0));

    /* -------------------------------
     * 5. Mine 20 blocks
     * -------------------------------*/

    SetMockTime(orig_tip->nTime + 1);
    createNewBlocks(this, 20);

    /* -------------------------------
     * 6. Check historical contract
     * -------------------------------*/

    // Get the new tip value
    dev::h256 newExpectedValue;
    {
        LOCK(::cs_main);
        CBlockIndex* pindex = m_node.chainman->ActiveChain()[orig_tip->nHeight];
        BOOST_CHECK(pindex != 0);
        newExpectedValue = uintToh256(*pindex->phashBlock);
    }

    // Execute the get block hash transaction
    result = executeBC(txGetBlockHash, *m_node.chainman);

    // Check block hash value is present and expected
    BOOST_CHECK(result.first[0].execRes.excepted == dev::eth::TransactionException::None);
    BOOST_CHECK(result.first[0].execRes.output.size() == 32);
    BOOST_CHECK(dev::h256(result.first[0].execRes.output) == newExpectedValue);
    BOOST_CHECK(result.first[0].execRes.gasUsed == 27379);

    // Check that the old value is different then the new value
    BOOST_CHECK(oldExpectedValue != newExpectedValue);
}

BOOST_AUTO_TEST_SUITE_END()

}
