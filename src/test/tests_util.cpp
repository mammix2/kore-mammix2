

#include "arith_uint256.h"
#include "blocksignature.h"
#include "legacy/consensus/merkle.h"
#include "main.h"
#include "miner.h"
#include "primitives/block.h"
#include "pubkey.h"
#include "tests_util.h"
#include "txdb.h"
#include "uint256.h"
#include "util.h"
#include "utiltime.h"
#include "validationinterface.h"
#include "wallet.h"


#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>


CWallet* pwalletMain;

static CCoinsViewDB* pcoinsdbview = NULL;

void InitializeDBTest()
{
    
#ifdef ENABLE_WALLET
    bitdb.MakeMock();
#endif
    pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pblocktree = new CBlockTreeDB(1 << 20, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    InitBlockIndex();
#ifdef ENABLE_WALLET
    bool fFirstRun;
    pwalletMain = new CWallet("wallet.dat");
    pwalletMain->LoadWallet(fFirstRun);
    RegisterValidationInterface(pwalletMain);
#endif
}

void FinalizeDBTest(bool shutdown) 
{
#ifdef ENABLE_WALLET
    bitdb.Flush(shutdown);
    //bitdb.Close();
#endif
    delete pcoinsTip;
    delete pcoinsdbview;
    delete pblocktree;
#ifdef ENABLE_WALLET
    UnregisterValidationInterface(pwalletMain);
    delete pwalletMain;
    pwalletMain = NULL;
#endif
}

blockinfo_t blockinfo[] =
    {
        //created with maturity=3
        {0, 1549485673, 1549485569, 538968063, 6, 1, 18753111, 52202898, uint256("03340b7345872e71782044ff3ad56e470d1720d777f3cc7c74ef446dd5bdbc53"), uint256("fb9450c8f3c4aecf1379b59a3044a849dce0c4706f69c7daade65435ff68cb02"), 0},               // Block 1
        {0, 1549485777, 1549485694, 538968063, 5, 1, 0, 0, uint256("142ca08a61ef456dbd44ab666bf7f0f114995a49eb4f10395dae9966d52be916"), uint256("6cad8859a352a13f0901d9ddc21113545de277430bba50271183965c6ab670f3"), 0},                             // Block 2
        {0, 1549485909, 1549485799, 538968063, 6, 1, 21299433, 43770174, uint256("098955c4bf86a1d0f5617f54ca2dc1aac5b487128b92a9776f29c1610cd17f8e"), uint256("206b5d370f20f67c8b095df5267be8177f548e34713574a274df715d29489c26"), 0},               // Block 3
        {0, 1549485930, 1549485930, 538968063, 1, 1, 0, 0, uint256("0a5002dc6f1ef79d1bd9e11ba1ee2277ada665c9bb6e744b96214d07efbdaf03"), uint256("58db1303f7d1d5087d98837cc316c1254ac3353325028d48145b79a9ad4efc9f"), 900000000000},                  // Block 4
        {0, 1549486012, 1549485950, 538968063, 4, 1, 12786563, 63121880, uint256("052f4c4622ed060dc6ff9382981c3ac565b3ebd65a6e16e846cf811fee1ba1ac"), uint256("1e4da7d8eadddec3f6e067f91057eef7030adb6bb7ffee82a96cdc7088de7cca"), 1800000000000},   // Block 5
        {0, 1549486074, 1549486033, 538968063, 3, 1, 0, 0, uint256("00c284d73b43692dc68e09a5c646ac4a7d1f1b231ccdcfe995e1b7fabf0b7a69"), uint256("098b56db6c2d7252d3e96646eab171cab433301d10ec8b5901f04697ed2b4360"), 2700000000000},                 // Block 6
        {0, 1549486177, 1549486095, 538968063, 5, 1, 26268915, 61302955, uint256("00617e350df5048f8d46b84fd5954caea946e44e8259bc5714c86dfe8b51a4f8"), uint256("0a595754279e2403283eee3b29869be051ab1c3a61d36b233afce65f86634703"), 3600000000000},   // Block 7
        {0, 1549486300, 1549486197, 538968063, 6, 1, 0, 0, uint256("18d0eb04b7ce4634d5b5e3352482676f3b06e942fefda835ced9687af701bde1"), uint256("a1c9bf367f80d2b2abaea6e4c2875ec0fce37c71657c418f04b0a2820483ea9e"), 4500000000000},                 // Block 8
        {0, 1549486381, 1549486320, 538968063, 4, 1, 1065493, 39083610, uint256("0a3a9fb3465f3aa3d57d14fcadfb32287b914937783a2e334a15344020fe0d8f"), uint256("6a2096e8ab7cec9e0a91004132592a1eca81d7f854ec127c5c9150321b014c73"), 5400000000000},    // Block 9
        {0, 1549486523, 1549486401, 538968063, 7, 1, 0, 0, uint256("0d13269b9a7a3076bef98cbc8f14f815589fd5e6b7d9de2ea4d8d692ec75a31e"), uint256("3bc6de3fc156e62a8e51dad1fc516b207bda9104ed7c5ee1b751ad1504f0c4dc"), 6300000000000},                 // Block 10
        {0, 1549486544, 1549486544, 538968063, 1, 1, 26982162, 56431979, uint256("00385b297d3be51f0bdc17ac0c7db82729810bbf96469619882355a2d3276f3e"), uint256("84a4a4eba4254e1eb6b301d6f1bfea3b52d437c88650333de37745de6b6fd444"), 7200000000000},   // Block 11
        {0, 1549486565, 1549486565, 538968063, 1, 1, 24278518, 41822296, uint256("053b1cf5cfeeb6567ac8c01a91042c2aaef7bbddee36337bb845fb26e47a8000"), uint256("edd694fa73d89a5a3ceafdc67dccd69ffd8c82d2543d3603cf38b563df8ad74b"), 8100000000000},   // Block 12
        {0, 1549486692, 1549486588, 538968063, 6, 1, 28527126, 51565814, uint256("0217bd3092abb74193d9cea3c5456dbc6c0e4f325fa2cf40fe692144950fcb52"), uint256("059fcc86b91f65e7bec4691238d0f05ba4cca86b2eac23c0910f255d79be68c9"), 9000000000000},   // Block 13
        {0, 1549486959, 1549486712, 538968063, 13, 1, 9125063, 25600419, uint256("04f1a8ee92164f7b6d8cee4711d7c084a4394481306e750624be98b00a4c3fdd"), uint256("a251ef9c105cf439603454c993b913d5a8c97b365bd05fc0cd3cc3d68fa9147a"), 9900000000000},   // Block 14
        {0, 1549487084, 1549486980, 538968063, 6, 1, 49903213, 65999374, uint256("04b9bea8380ada8d630c4e19abcfa7676f6993764ade221cec97c7728bb1ded3"), uint256("daf3109ee4e8187cddd42e4c798701fff64c4d31f463a285c603970d24a807bc"), 10800000000000},  // Block 15
        {0, 1549487146, 1549487104, 538968063, 3, 1, 0, 0, uint256("1ae14f2175ea7ff458647e18269c5242cdd803bda552f7534bd3922d7d827af4"), uint256("c672e0987bd6397c00a569e2dcc5281ab5c9bad736c839a5fdfc960d72fa34e3"), 11700000000000},                // Block 16
        {0, 1549487167, 1549487167, 538968063, 1, 1, 0, 0, uint256("0c160483605abeecffc9fec1ecddc274fc054c2d66119539050f13f0e5c94d03"), uint256("6ad3f9b8132bb6c6db867ad66a4deb0332122ffa14d997fce1577ba198920203"), 12600000000000},                // Block 17
        {0, 1549487187, 1549487187, 538968063, 1, 1, 0, 0, uint256("056f47e34a9b95ee40a314e32c1a71fb26a80bea28c91248d9d67d5f84bee729"), uint256("a2ca337375dd8022b91b4b914c143fb3568b9fa51e1f8d5037d7e532c614da13"), 13500000000000},                // Block 18
        {0, 1549487291, 1549487208, 538968063, 5, 1, 0, 0, uint256("0cd833c00deb87d23a0e26a87147f62d90e549955d1afb36778431506d992203"), uint256("467ecc216af0b6b751523e2195db6f046480cbce0444587a7fd182f966946c61"), 14400000000000},                // Block 19
        {0, 1549487312, 1549487312, 538968063, 1, 1, 0, 0, uint256("0aec1e7569118fc72d2a9bcad567cff471593deccc5e69ea4e1b68d8d3407bfc"), uint256("f2f169a53c9faf514855cb0d70991e5a738d93837c67ea07ba55b87bff20ee78"), 15300000000000},                // Block 20
        {0, 1549487514, 1549487332, 538968063, 9, 1, 16916289, 61719329, uint256("0dbeed7fd63148ec8311602128db4924c9523b0d1b3cae0e9713bf7925e322f8"), uint256("16118c09898d1c6c0e83163963b1a61660621aa264c21d04b4d6f55fe5fc50cd"), 16200000000000},  // Block 21
        {0, 1549487547, 1549487547, 538968063, 1, 1, 24005170, 51938242, uint256("02990dc936e6b7707aa8f239a654858577f1e09d96a4b587cb51489368d9a2b5"), uint256("4e4b71453d6cadc6c20e778520326a288015d72cfb5b72900a9c7a7f52bae386"), 17100000000000},  // Block 22
        {0, 1549487943, 1549487580, 538968063, 12, 1, 0, 0, uint256("116fe4c92ee2beced0bd89a8f5fe795b4875c37a1c2ae4f87812bbbb3084802d"), uint256("84f26024c1b9f543a5617c5eff30b4933599775a76a669068a1ff0d551feb200"), 18000000000000},               // Block 23
        {0, 1549488141, 1549487976, 538968063, 6, 1, 47572768, 51222577, uint256("05eb9fb09cc15f3420f5c0e5b9b3921b9be9b0668648c3ee510d457c2a30b996"), uint256("3b339f87e62af649e2ffaae1bade1a003ad9b380b5333e0aa046c466184643a8"), 18900000000000},  // Block 24
        {0, 1549488405, 1549488174, 538968063, 8, 1, 38861503, 45325726, uint256("0d161f1b1d6b089ac510437e17d787709bb3994670bae911d8141b886d0032f0"), uint256("963515b40e835a0ec4a5eb67df3f049d8800cd42e8f082dee101aa8dc748623f"), 19800000000000},  // Block 25
        {0, 1549488571, 1549488438, 538968063, 5, 1, 0, 0, uint256("0a24369e635269c3562e1c8b2ff91f4b54dc7e363fb61c9c37541d30a5ab532b"), uint256("3bf4fa847a2273679d21b4b7cecbfe52c8929e899ce9569957c8fdd68c3ceeec"), 20700000000000},                // Block 26
        {0, 1549488768, 1549488604, 538968063, 6, 1, 13848634, 49798752, uint256("046ea00819d0a3d593b3bc00c1ba86e629796e7f6a5b79e911732594db8b2358"), uint256("c97880c91ae4af93a417c0dba893a3ba0c1a26423454eee33e340f5452b853dc"), 21600000000000},  // Block 27
        {0, 1549488867, 1549488801, 538968063, 3, 1, 0, 0, uint256("1ae02250e9d47a9dfaa90d41a2f6cdb60ac7616d8345aa655ac6bf00c92ac0e5"), uint256("db772b89e29e364c57ecad805e8a5457166bcebe371efd566ae7bb1ca668f5f5"), 22500000000000},                // Block 28
        {0, 1549488900, 1549488900, 538968063, 1, 1, 6878010, 32281252, uint256("05731a4fd3f1bb6829fe00d7ca61abefa014cb400c67255b38b274b7811075ab"), uint256("d082e15fa9b4dd203f0e345bef8603d16190007dbf9f87ff3ce9ef1bbda70924"), 23400000000000},   // Block 29
        {0, 1549488932, 1549488932, 538968063, 1, 1, 0, 0, uint256("12f33f05c9e16aa55a6da120bd5ae5719cef22db309dc37afad24f4036ad7ffe"), uint256("77a7eeaa7cf17daea6eca9c4e81100127054f767b2dfc36c36da4968bf2e8191"), 24300000000000},                // Block 30
        {0, 1549488965, 1549488965, 538968063, 1, 1, 40135435, 66903443, uint256("0191a6087c6c86e58abc1c9f8685e1cdc538654e00c5af66006a7134fe59300d"), uint256("9e8b3c1ccdff9f8f891f8890645c8ac39900c5178876b7cde5e4f259f1d77cfb"), 25200000000000},  // Block 31
        {0, 1549488998, 1549488998, 538968063, 1, 1, 0, 0, uint256("1101383699ae3b95c86fc07c05cd2faa8d07d752e6044384ca53d09f5e5d5d82"), uint256("6eb00e081a720b81989d833552656fc218f1d1430a26648aac972419c049511a"), 26100000000000},                // Block 32
        {0, 1549489095, 1549489031, 538968063, 3, 1, 16095418, 18701770, uint256("0b94c3204181078de0370dd4e6fae7c040fd3e4cb42f9ad4d040d11f3ee69b37"), uint256("3dae70491a33734b4430a03338e213b00935a83b75c4b221ec4ff1043520b820"), 27000000000000},  // Block 33
        {0, 1549489126, 1549489126, 538968063, 1, 1, 7247295, 17657865, uint256("02d5d8ff7fa0bed0f37d22055a79b20c8bdcc2e5958234e7a3a7d0d51b344a0d"), uint256("f402efc763d3153776a3847ee33222a8dbe76c68d598f31b333775b9727050e7"), 27900000000000},   // Block 34
        {0, 1549489187, 1549489157, 538968063, 2, 1, 0, 0, uint256("17176b3725d5e4883aa3544438b1edaa5ae7eecc688dea92232268b34a9fe57e"), uint256("4ca16590f786511a00d1a09fb8836cd7090ae28f460ec0ce4e533afcb6bf6ad0"), 28800000000000},                // Block 35
        {0, 1549489218, 1549489218, 538968063, 1, 1, 0, 0, uint256("11a537c6cc8ad5b7c68de30545bc7b84aff094a3e664d688c82e991b2b29dea0"), uint256("1123a7bc7d2971ee6705e6f898c4e83a1cebc5a6c0ebdec78b4768b2e1b5c475"), 29700000000000},                // Block 36
        {0, 1549489249, 1549489249, 538968063, 1, 1, 0, 0, uint256("04169a557ff27daff325bb94d9758fa981d1a42dad84d5ca882f3b640dbb0ab6"), uint256("a74a87853a36b49ebabe06dae8b24642d753787036e82f921015953876b05c88"), 30600000000000},                // Block 37
        {0, 1549489343, 1549489280, 538968063, 3, 1, 39476970, 47522080, uint256("0491a582132c2baf4fe7d63fb933fdc364a5fce249467fae48ea9796beafd4d4"), uint256("3251609cd0bf4872f65638df1f07c4e5ba96c41fd8a2f7eff7232217b8b07309"), 31500000000000},  // Block 38
        {0, 1549489405, 1549489374, 538968063, 2, 1, 0, 0, uint256("1c14e6bc6fc696a4d1f73264e615c27b14b1303c7739b6baae420daa1dd3c39c"), uint256("fb1d3fe6530d429a7ec3c5ce035774038bbc551877489cadd218d954221728d8"), 32400000000000},                // Block 39
        {0, 1549489715, 1549489436, 538968063, 10, 1, 16556408, 56370049, uint256("02775effbeb32220ad958a7915fdcdf53446b8404c55cc059c75353880497742"), uint256("7bc692fac8beda5b790978438017dbc2b5af1eb4494a3e3b840b3ebe747d587d"), 33300000000000}, // Block 40
        {0, 1549489746, 1549489746, 538968063, 1, 1, 0, 0, uint256("167c1f964b70c3fe4d888a2a0d3905ba033baffb386bd52132fd376b34a59467"), uint256("8a40e60de5351045bbcbde507c3561575d82fd4e7d18eb7bb74d812c6d8c1acc"), 34200000000000},                // Block 41
        {0, 1549489963, 1549489777, 538968063, 7, 1, 0, 0, uint256("1890f4ce015c352634a26ca8c28fa110f978e2ddca6617fa66534927698b8918"), uint256("8a5ea74f6e0b49d3b91d175a83f64fdc99f690939abe855b5ddf3c70fba68099"), 35100000000000},                // Block 42
        {0, 1549489994, 1549489994, 538968063, 1, 1, 26779084, 39320367, uint256("04ef9a2c7c0cd514c1865e90613a72093044ee8f4f0a2440f100ff125be37d8c"), uint256("d8838da5c405e2c39e7e7e064c56f17b8ae87d5b1a697d565fa2d60af092b767"), 36000000000000},  // Block 43
        {0, 1549490304, 1549490025, 538968063, 10, 1, 0, 0, uint256("165b3f59d0af5bdcecf32c0cbd1c6b07eb921713f0964843953927d23ab0c8cb"), uint256("da420711034098424d32d7d97c16b6401edfce016a441b5d9c37181b4ceac430"), 36900000000000},               // Block 44
        {0, 1549490428, 1549490335, 538968063, 4, 1, 46947395, 63616209, uint256("021349e3305943c4709e33474e83f98685ca52e6ee5ee3048840df1097b0f0f2"), uint256("0c7119e91247ba04024ae13f09dbbe52493f6b2b26f80a5edba90b1a0b90f1b5"), 37800000000000},  // Block 45
        {0, 1549490645, 1549490459, 538968063, 7, 1, 16710948, 61596783, uint256("05289dbdfe614bae05ec9d1f8a74e8144b5a39d32a8bfbf29718ba0db910990e"), uint256("c8e253cdccdc1e70c9817fdd05b72762be234bcd3c8ad6d976926e512d7d3878"), 38700000000000},  // Block 46
        {0, 1549491017, 1549490676, 538968063, 12, 1, 0, 0, uint256("082fea971f1bfc3dbc1729b4d9123625f0f9295d746591b750deb8eb2d94b0cc"), uint256("364cf326f487d9fb8520d3adb12f46c26d3a1a86bb435b888505fb61418f9ae6"), 39600000000000},               // Block 47
        {0, 1549491048, 1549491048, 538968063, 1, 1, 0, 0, uint256("02dc45fcc71fda6ef4dad7bb57c935578a5283daa55f198dc351731f00ea814d"), uint256("c58ff89e7a702d56ed87bd74c3c46d95fd329fc756ed0fb4b23b008c069381f8"), 40500000000000},                // Block 48
        {0, 1549491110, 1549491079, 538968063, 2, 1, 9468699, 51388320, uint256("06985b068ee54c58c989e3d72e85695d1271fe26075b8744b07abebdaf5c324a"), uint256("1f6e853edec93cae3eedf2497ae6defc45c5d27df83f126b2658e41ea7ad51c5"), 41400000000000},   // Block 49
        {0, 1549491173, 1549491142, 538968063, 2, 1, 6772547, 28619537, uint256("0abd9e01668f5472ae12dc346f486192984b1a59fafce2d95160d476575f0172"), uint256("2414e6eab66d26b95faa4b5511967d099e33088682d4f285fc229325af476178"), 42300000000000},   // Block 50
        {0, 1549491235, 1549491204, 538968063, 2, 1, 0, 0, uint256("0aec7126d4eed1988e9e4849085700f1fa3903db5126a15e3197a3ad05c0c6dd"), uint256("08f4b76c917aa647789fc2040bcb7eb6c89a3b00c4b3dd21ed00e9fd37f9e451"), 43200000000000},                // Block 51
        {0, 1549491544, 1549491266, 538968063, 10, 1, 12789365, 44677085, uint256("0b6173edef6a09f1dec2584bc406af664d2b3fa73480579fea2f204ee39345f1"), uint256("6a44496200784c774b1dc3a488d58ef07afdfae35be02bd327c401c7e7ee9da7"), 44100000000000}, // Block 52
        {0, 1549491854, 1549491575, 538968063, 10, 1, 11310309, 58186993, uint256("0d1a34802c3f6458678a69c9c1a0e86b5c61490c13536ffd41f23ba19b5c80b6"), uint256("9b618e324e7461c1e82fe877f93bcefdac3e787fc476c2bd4e131753c2d805c5"), 45000000000000}, // Block 53
        {0, 1549492040, 1549491885, 538968063, 6, 1, 2536149, 23400225, uint256("03e01639b6b1e2c87b2e3600992cad2366c376a13dbe52abccb22deb57d20b9a"), uint256("dca2237dac2b2906a3284aefd9c29d9f3e9ba251481e411365095acc3c722a04"), 45900000000000},   // Block 54
        {0, 1549492071, 1549492071, 538968063, 1, 1, 6716855, 19144947, uint256("0b0367ad9b43005250a159933f50262843c2166562580084bf3074928a68934d"), uint256("51aa3f7bf06ba6e83adea61d21aa82538d94cbafa571af2c12f58fd05fcf0df6"), 46800000000000},   // Block 55
        {0, 1549492289, 1549492102, 538968063, 7, 1, 14600058, 34808102, uint256("0863f3bade00c072acdf6863c93a6d07334d7e920280f90077177c24612cd713"), uint256("124a17311f41384765464293da758b7e281d489390b594aa26636d20de4724bc"), 47700000000000},  // Block 56
        {0, 1549492505, 1549492319, 538968063, 7, 1, 0, 0, uint256("00ce6da10efc465e801d28780526fcd3627727b5b31d7ddb49f8e8d6f5d1520f"), uint256("9c426666a2df166027c3bfe9ea6f93783419d708c199c3ae317ebab5a1a735bd"), 48600000000000},                // Block 57
        {0, 1549492536, 1549492536, 538968063, 1, 1, 0, 0, uint256("07bf12e55dd17fec563ff004cc98de859f8d3baa0191431b5084ac3bfe10af8a"), uint256("8ac0ca702ef8b83eab25c9e25284165202314e7f2c3197c1b50663c1b99a3c7d"), 49500000000000},                // Block 58
        {0, 1549492567, 1549492567, 538968063, 1, 1, 0, 0, uint256("0d4416502f865c2f162cc76e2c20dec06a1c68f23c4482af8b6b2ca810232753"), uint256("0e35d2f82413255bae5128dc71cb09cbb7424effd0bffb5c7d9e96bcf4200f37"), 50400000000000},                // Block 59
        {0, 1549492598, 1549492598, 538968063, 1, 1, 49013517, 55273964, uint256("0b3107887faeb17dd2eeda57c14821fe4244e21cd29659e40ddc8ad812e3d119"), uint256("9782fe8be1b55dffa3560673f82f6748d2c3f14942875b93fe21d4ebb84d873c"), 51300000000000},  // Block 60
        {0, 1549492939, 1549492629, 538968063, 11, 1, 0, 0, uint256("0b0fb13a2ee39e3f67ebeb5b2e0beb1a91299b3b1f56dcd45c2425d6540ea00f"), uint256("57016a6be969efc55d2ff935fda56357836e4f6dc2ca0fbcee8b6068117a7560"), 52200000000000},               // Block 61
        {0, 1549493526, 1549492970, 538968063, 19, 1, 0, 0, uint256("11fc5f73c4b35815f03c5095bb83c4e806fca048c6eee824e0e634ddc60213dd"), uint256("a4122beb1c719bc4c2cdb7f37a7187307529a043028334e1185528c30fa67044"), 53100000000000},               // Block 62
        {0, 1549493557, 1549493557, 538968063, 1, 1, 0, 0, uint256("07a13a18e9236a9e4882dfc626c0253fae1f3c5c2ac66a66ffd311b34f273fa7"), uint256("67f09886d01d0e92d7ed9bc57308ae8562d67ee768a96ea0b4e3faab842f21e4"), 54000000000000},                // Block 63
        {0, 1549494032, 1549493588, 538968063, 15, 1, 2467757, 66314735, uint256("0fb94c1cb87d9c9fdf72feaa26cb67d4bd32bfeceffb79039fb0507b3c567075"), uint256("fd095e6b67e2206bb8bc4cc40bf8167cfb6c2d6c2a06c3b5fb412163072684ec"), 54900000000000},  // Block 64
        {0, 1549494156, 1549494063, 538968063, 4, 1, 0, 0, uint256("1c305c0b1bf0c9dbcd9d0eb5a2c3d997fa9da21bd9de557eb07dad8c684af256"), uint256("9083bd8b48ed44b6ba1f9055be5ede53db2c55ac7c859b46fc0a549244711fcd"), 55800000000000},                // Block 65
        {0, 1549494217, 1549494187, 538968063, 2, 1, 0, 0, uint256("035d69d7b0eb1cb66c3d283a5cfcc708761be2e8fafa3e347100b27eb0bfb982"), uint256("8cb73ceddee63d85f508e70ba3c556cbde707ede92d4e473b54f53e7b6a20bb6"), 56700000000000},                // Block 66
        {0, 1549494682, 1549494249, 538968063, 15, 1, 50187470, 50823772, uint256("0a1c2a1424c30cec40246d45872420ed9fb8df72987547cf161b76b2d1fee615"), uint256("41c094ed8b52bf73d37ffcd9eca6e19a2094dc50c297d5018709e70e68a149d0"), 57600000000000}, // Block 67
        {0, 1549494745, 1549494714, 538968063, 2, 1, 10728160, 32208428, uint256("04f0a7915d1e0b042acbd14294c6f2588ef65693b460ecf0c0122a7bee9a9def"), uint256("2dfca54a6c1dfa438691582cfcc9fbee2139c95e41c0d2c69ac6e8f3bf4881e7"), 58500000000000},  // Block 68
        {0, 1549495024, 1549494776, 538968063, 9, 1, 19194104, 35698617, uint256("0efe041dcd5c9a75dbfe01b8333940a6a48b2f71874068ed7d9c077b42bf6a9c"), uint256("1346a99d805e8bf0915156acdd1db984c5753cd0bcb80a3ad4c0904d1b962acf"), 59400000000000},  // Block 69
        {0, 1549495147, 1549495055, 538968063, 4, 1, 0, 0, uint256("1811667cbd7468acd88e2e6755e7a0a27045bbea781ce37b18da0da521f4a1b0"), uint256("518c98d71630999792efc472defc5766b7403539f790a6d12e3df4f28b01a3c9"), 60300000000000},                // Block 70
        {0, 1549495240, 1549495178, 538968063, 3, 1, 0, 0, uint256("1a80188cc9a83215f319ffb2fdf5015c0c12cd676c5dbe05b0e16c96c1cdddda"), uint256("bfd3e3c86a7170f85b2391e874b33784d41231f4b30bfed211285a0ed53523b9"), 61200000000000},                // Block 71
        {0, 1549495364, 1549495271, 538968063, 4, 1, 0, 0, uint256("129c0c7769c5d0bc8c136a80660dc60e8a2603eb51277af5b0fab0d4e5285428"), uint256("9e38f7d521bec966f25a3cf0e7a0e544cb486f9be0d32ec9247aac71adb03a68"), 62100000000000},                // Block 72
        {0, 1549495549, 1549495394, 538968063, 6, 1, 18859795, 35887903, uint256("0cd2bbd4dc894da7aa0aa8d4e77a161a598082ec0d36eef26eade99334698feb"), uint256("87634c0a733610d0683c5f6967023f1183988939fe95ad5501bb65e3e5c30a4a"), 63000000000000},  // Block 73
        {0, 1549495579, 1549495579, 538968063, 1, 1, 14521443, 24702817, uint256("026ed67e91645dacd0427cf33a89e624f30b527de3f44ef086b2a83a13d85f3d"), uint256("4640e938cafba7dd97b4c8b47ecc897b28004a3b625404cc68d01e11776d0ec5"), 63900000000000},  // Block 74
        {0, 1549495672, 1549495610, 538968063, 3, 1, 19372443, 60658554, uint256("08e7febbc16b0de2ca2f2f41132d34ea1e1afbc454d0a71ee80b01b7a79141a5"), uint256("a05adb3d9f1249c1abaf708b0716dc657f1dd562c67fc6563f349f625e981691"), 64800000000000},  // Block 75
        {0, 1549495765, 1549495703, 538968063, 3, 1, 0, 0, uint256("050e2208480d754aca3390942468b97f55f33f1e08fa4ca2c50b9002658096b5"), uint256("edd0c98f8da78612d619428b2d76403aae7953dd9df7ef38eaf750dc26c87999"), 65700000000000},                // Block 76
        {0, 1549495826, 1549495796, 538968063, 2, 1, 9550932, 12332872, uint256("07254ed5041126ce17828b507afd72406485186c20e5355168ccbc23c79c3775"), uint256("1e592b3d80d0d86c8ad58e48491f2fdb6c22e7d84b22fac380e747d981dd996e"), 66600000000000},   // Block 77
        {0, 1549495857, 1549495857, 538968063, 1, 1, 0, 0, uint256("02aa9603b8cffdc753939729ed137a27430ea270c0c12ca94d02476ba6e65392"), uint256("7ab3a6fc0b1f4dfa102a4dbf6e0b5a12bdc827579f9ab90f67610b859914a27b"), 67500000000000},                // Block 78
        {0, 1549496042, 1549495888, 538968063, 6, 1, 0, 0, uint256("032883646ca4cf7ed8ae2283a4ac74a4ef79f8a489cda0c4c5f14c4e52e2a3b2"), uint256("ca6c8c988c1deb8b6cdb0a991cb509e905778c98423cfe2b4eddfb2f4469605e"), 68400000000000},                // Block 79
        {0, 1549496197, 1549496073, 538968063, 5, 1, 0, 0, uint256("0865bedeae456bff1e5bcf5c1def68f073a1b21606809b384399bc9e411bc643"), uint256("4ef027eea5b7acd43acec0c69dad2f9c11c7b8bdbeeabd922839e62c971e6bba"), 69300000000000},                // Block 80
        {0, 1549496351, 1549496228, 538968063, 5, 1, 10863278, 61146728, uint256("0f515232acb913fa7030ae5feb4e50b88f33a30e5b0092157ef98f5868b863f7"), uint256("df8145f8025bedcafcad86c09e7af41c48679a2f559f5ecf6161da38350293ff"), 70200000000000},  // Block 81
        {0, 1549496443, 1549496382, 538968063, 3, 1, 0, 0, uint256("0cd2becc6da9ae2b9d4ab87974a0547e9c30bb19dcb50d59d65a13d6d8285a8a"), uint256("176113d6e69706ea3f0cfd3eaf098f411823367b71672514d682377881198a33"), 71100000000000},                // Block 82
        {0, 1549496659, 1549496474, 538968063, 7, 1, 9641991, 31226241, uint256("05c5537f46125986c812b48db9ab21eb8718e4574db899a27e7eff5c6d1005f2"), uint256("5307188a684799e793e3271ff3ee7a996467f868592de6c6d6c2b2557bdbfe20"), 72000000000000},   // Block 83
        {0, 1549496874, 1549496689, 538968063, 7, 1, 0, 0, uint256("05b6e4bb5fbea2aeaa737de54b3beb9715af3e6c044ddcfff08f2c404eb30e25"), uint256("cc9924a2140cf1a6507d95e7a61d6528248604fc1c36a114524a9480c04a1339"), 72900000000000},                // Block 84
        {0, 1549496936, 1549496905, 538968063, 2, 1, 17389235, 31742876, uint256("0437529b83d270666cb94ed0753d779aaf9427dd2392f7ee8c6e10ca2ed248c8"), uint256("72aa015e51bde945a9703882c4819164833a53cc69051a46ce747e5a9ff731a0"), 73800000000000},  // Block 85
        {0, 1549497029, 1549496967, 538968063, 3, 1, 0, 0, uint256("0482b139ea180915d9c322d79075d34b5d99d3e4d20f4144036ae854e0b63569"), uint256("f7ad86e3629f5a34931c9329c3752bc8d0481c6ddd8ebfc668b0a0f20db4d531"), 74700000000000},                // Block 86
        {0, 1549497122, 1549497060, 538968063, 3, 1, 50056659, 54685352, uint256("0f50fc14637cdbd13d70475cebcde4479e3a4f568ab59becbad3113ee9099c4d"), uint256("36ad53db38cb32222b7fcdb3489d7faec6edb088bc3d32e88df7bb91e90fb69d"), 75600000000000},  // Block 87
        {0, 1549497184, 1549497153, 538968063, 2, 1, 0, 0, uint256("14f268f22416ea7b1aad3385c8ec9283fbb9abd16cf8d30ab6a9ae6f62ca1e73"), uint256("da5b3e0ff96b186e378b9e6637aa3b713c20163f890a39842402743e03c360af"), 76500000000000},                // Block 88
        {0, 1549497307, 1549497214, 538968063, 4, 1, 22809766, 38872937, uint256("0ebafb69284ff507b9cc66eebd3af698c396c58d936bd0afa037b2892856663a"), uint256("aef50c7fc41c53cc3bf3fba697cd6a4eb9d82f6fc882abe2afdcd8c90387ff95"), 77400000000000},  // Block 89
        {0, 1549497338, 1549497338, 538968063, 1, 1, 30707680, 48376836, uint256("0de32168a4ef45d5c51a242bb88f5e55ed3d0ecabf232665ea6b36c7f526cb52"), uint256("c57987877114e2b2b4ed2a277faf3c106a03c0bfe53d8d7d1e1ce67c065e640d"), 78300000000000},  // Block 90
        {0, 1549497493, 1549497369, 538968063, 5, 1, 0, 0, uint256("0ba3945d32450d06334e99800675293ffcea88fd92e1e21d5b6e1d9e9f543d3f"), uint256("c69f791598ba75af616393448cdcce240bab240fe8ab0093d3f81cb7141ff065"), 79200000000000},                // Block 91
        {0, 1549497523, 1549497523, 538968063, 1, 1, 9592434, 61611458, uint256("0e08d8ff3a3708cb7b57feb8c2e5d494c83474bd011e18a772b4fd7478a70427"), uint256("38e72a8ba3c8a18ca28180d4a09301a1518a18ed70b2ee95d3cd289aa13c52ab"), 80100000000000},   // Block 92
        {0, 1549497709, 1549497554, 538968063, 6, 1, 0, 0, uint256("0527c6cb46297f60db08497f44bdb71f4503ce22c67bc5351585397e1cb59044"), uint256("4a45b600904176a4567661c6ceee5cde223d2f0190e6c99eaeec59fb2bc294ee"), 81000000000000},                // Block 93
        {0, 1549497770, 1549497740, 538968063, 2, 1, 0, 0, uint256("12fe230ecbe1160539d4564787df1b0159e9439a03dc43c46a70af44b495dd7a"), uint256("5d6d4cec64790f37a8b4dd10cf48ce9969082814406278cc1653852bfe723bb8"), 81900000000000},                // Block 94
        {0, 1549498326, 1549497802, 538968063, 18, 1, 0, 0, uint256("12d59cc64eb6878ab2c16220baa82c19443b6c0cc5cb6328da600ef533a5e345"), uint256("bced7de4c091a1441a2f3f3770a1b0d82bf0e9a4c137b43667f17dc465bf225b"), 82800000000000},               // Block 95
        {0, 1549498388, 1549498358, 538968063, 2, 1, 0, 0, uint256("00440f746c6963d23602a63931505359507902c62e8e56eebd443e39f7a84511"), uint256("d5bef8c5b396bbc8cd5115c546264c39e317520d18d8fec0bb7119aef469864e"), 83700000000000},                // Block 96
        {0, 1549498450, 1549498419, 538968063, 2, 1, 0, 0, uint256("13da1da1fbc83bc55a39558eba7a5fb258f535e9c740d054d9dec46a37a8a442"), uint256("92488b46f6ae70caaa0ebd97138cb53893c7457c38e3ae20264c53ab1c3a6aa6"), 84600000000000},                // Block 97
        {0, 1549498481, 1549498481, 538968063, 1, 1, 0, 0, uint256("0b60c231cea876d763cae9d4ce99be8b1f156095a91e3f967fc91550066f4ae9"), uint256("061824c074b3263f9249399fa3ab38e21b04b5bce86eb62b33fa9e334e129898"), 85500000000000},                // Block 98
        {0, 1549498666, 1549498512, 538968063, 6, 1, 0, 0, uint256("10c749e21e22e5b1d43fa5eae295c3c6e484e496e0a2731fba0a4b94046e2615"), uint256("d78a7940de6d5aa0ff0ffaf232c4d8b409f02dc1afce40e94bac55438116359a"), 86400000000000},                // Block 99
        {0, 1549498850, 1549498697, 538968063, 6, 1, 0, 0, uint256("01a8b5dc5a8d26fa9aa0572e5f1c3132ffd2ed39e1625730228d46a22b003756"), uint256("049c70cf5867b1d2c96ebabc0bf3e4980808b60678d379466b416266949c4e53"), 87300000000000}                // Block 100
};


void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake, bool logToStdout)
{
    if (logToStdout) {
        //cout << pblock->ToString().c_str();
        cout << "{" << fProofOfStake << ", ";
        cout << pblock->nTime << ", ";
        cout << pblock->vtx[0].nTime << " , ";
        cout << pblock->nBits << " , ";
        cout << pblock->nNonce << " , ";
        cout << nExtraNonce << " , ";
        cout << pblock->nBirthdayA << " , ";
        cout << pblock->nBirthdayB << " , ";
        cout << "uint256(\"" << pblock->GetHash().ToString().c_str() << "\") , ";
        cout << "uint256(\"" << pblock->hashMerkleRoot.ToString().c_str() << "\") , ";
        cout << pwallet->GetBalance() << " },";
        cout << " // " << "Block " << blockNumber << endl;
    }

    if (fDebug) {
        LogPrintf("Block %d %s \n",blockNumber, (pblock->IsProofOfStake() ? " (PoS) " : " (PoW) "));
        LogPrintf(" nTime               : %u \n", pblock->nTime);
        LogPrintf(" hash                : %s \n", pblock->GetHash().ToString().c_str());
        LogPrintf(" StakeModifier       : %u \n", chainActive.Tip()->nStakeModifier);
        LogPrintf(" OldStakeModifier    : %s \n", chainActive.Tip()->nStakeModifierOld.ToString());
        LogPrintf(" Modifier Generated? : %s \n", (chainActive.Tip()->GeneratedStakeModifier() ? "True" : "False"));
        LogPrintf(" Balance             : %d \n", pwallet->GetBalance() );
        LogPrintf(" Unconfirmed Balance : %d \n", pwallet->GetUnconfirmedBalance());
        LogPrintf(" Immature  Balance   : %d \n",pwallet->GetImmatureBalance());
        LogPrintf(" ---- \n");
    }
}

void InitializeLastCoinStakeSearchTime(CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();

    // this is just to initialize nLastCoinStakeSearchTime
    unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
    if (!pblocktemplate.get())
        return;
    CBlock* pblock = &pblocktemplate->block;
    SignBlock_Legacy(pwallet, pblock);
    MilliSleep(30000);
}


CScript GenerateSamePubKeyScript4Wallet( const string & secret, CWallet* pwallet )
{
    CBitcoinSecret bsecret;
    bsecret.SetString(secret);
    CKey key = bsecret.GetKey();
    CPubKey pubKey = key.GetPubKey();
    CKeyID keyID = pubKey.GetID();
    CScript scriptPubKey = GetScriptForDestination(keyID);

    //pwallet->NewKeyPool();
    LOCK(pwallet->cs_wallet);
    pwallet->AddKeyPubKey(key, pubKey);
    pwallet->SetDefaultKey(pubKey);

    if(fDebug) { 
        LogPrintf("pub key used      : %s \n", scriptPubKey.ToString()); 
        LogPrintf("pub key used (hex): %x \n", HexStr(scriptPubKey)); 
    }

    return scriptPubKey;
}


void ScanForWalletTransactions(CWallet* pwallet)
{
    pwallet->nTimeFirstKey = chainActive[0]->nTime;
    // pwallet->fFileBacked = true;
    // CBlockIndex* genesisBlock = chainActive[0];
    // pwallet->ScanForWalletTransactions(genesisBlock, true);
}

void GenerateBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake)
{
    bool fGenerateBitcoins = false;
    bool fMintableCoins = false;
    int nMintableLastCheck = 0;
    CReserveKey reservekey(pwallet); // Lico, once we want to use the same pubkey, we dont need to remove it from key pool

    // Each thread has its own key and counter
    unsigned int nExtraNonce = 0;

    int oldnHeight = chainActive.Tip()->nHeight;

    for (int j = startBlock; j < endBlock; j++) {
        MilliSleep(Params().TargetSpacing() * 1000);
        if (fProofOfStake) {
            //control the amount of times the client will check for mintable coins
            if ((GetTime() - nMintableLastCheck > Params().ClientMintibleCoinsInterval())) {
                nMintableLastCheck = GetTime();
                fMintableCoins = pwallet->MintableCoins();
            }

            while (pwallet->IsLocked() || !fMintableCoins ||
                   (pwallet->GetBalance() > 0 && nReserveBalance >= pwallet->GetBalance())) {
                nLastCoinStakeSearchInterval = 0;
                // Do a separate 1 minute check here to ensure fMintableCoins is updated
                if (!fMintableCoins) {
                    if (GetTime() - nMintableLastCheck > Params().EnsureMintibleCoinsInterval()) // 1 minute check time
                    {
                        nMintableLastCheck = GetTime();
                        fMintableCoins = pwallet->MintableCoins();
                    }
                }

                MilliSleep(5000);
                boost::this_thread::interruption_point();

                if (!fGenerateBitcoins && !fProofOfStake) {
                    //cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }

            if (mapHashedBlocks.count(chainActive.Tip()->nHeight)) //search our map of hashed blocks, see if bestblock has been hashed yet
            {
                if (GetTime() - mapHashedBlocks[chainActive.Tip()->nHeight] < max(pwallet->nHashInterval, (unsigned int)1)) // wait half of the nHashDrift with max wait of 3 minutes
                {
                    MilliSleep(5000);
                    //cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }
        }

        //
        // Create new block
        //
        //cout << "KOREMiner: Creating new Block " << endl;
        if (fDebug) {
            LogPrintf("vNodes Empty  ? %s \n", vNodes.empty() ? "true" : "false");
            LogPrintf("Wallet Locked ? %s \n", pwallet->IsLocked() ? "true" : "false");
            LogPrintf("Is there Mintable Coins ? %s \n", fMintableCoins ? "true" : "false");
            LogPrintf("Do we have Balance ? %s \n", pwallet->GetBalance() > 0 ? "true" : "false");
            LogPrintf("Balance is Greater than reserved one ? %s \n", nReserveBalance >= pwallet->GetBalance() ? "true" : "false");
        }
        unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrev = chainActive.Tip();
        if (!pindexPrev) {
            continue;
        }


        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
        // need to create a new block
        BOOST_CHECK(pblocktemplate.get());
        if (!pblocktemplate.get())
            continue;
        CBlock* pblock = &pblocktemplate->block;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
        //Stake miner main
        if (fProofOfStake) {
            //cout << "CPUMiner : proof-of-stake block found " << pblock->GetHash().ToString() << endl;
            if (!SignBlock(*pblock, *pwallet)) {
                //cout << "BitcoinMiner(): Signing new block with UTXO key failed" << endl;
                continue;
            }
            //cout << "CPUMiner : proof-of-stake block was signed " << pblock->GetHash().ToString() << endl;
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
            LogBlockFound(pwallet, j, pblock, nExtraNonce, fProofOfStake);

            continue;
        }

        //
        // Search
        //
        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        //cout << "target: " << hashTarget.GetHex() << endl;
        while (true) {
            unsigned int nHashesDone = 0;

            uint256 hash;

            //cout << "nbits : " << pblock->nBits << endl;
            while (true) {
                hash = pblock->GetHash();
                //cout << "pblock.nBirthdayA: " << pblock->nBirthdayA << endl;
                //cout << "pblock.nBirthdayB: " << pblock->nBirthdayB << endl;
                //cout << "hash             : " << hash.ToString() << endl;
                //cout << "hashTarget       : " << hashTarget.ToString() << endl;

                if (hash <= hashTarget) {
                    // Found a solution
                    //cout << "BitcoinMiner:" << endl;
                    //cout << "proof-of-work found  "<< endl;
                    //cout << "hash  : " << hash.GetHex() << endl;
                    //cout << "target: " << hashTarget.GetHex() << endl;
                    BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
                    LogBlockFound(pwallet, j, pblock, nExtraNonce, fProofOfStake);

                    // In regression test mode, stop mining after a block is found. This
                    // allows developers to controllably generate a block on demand.
                    // if (Params().MineBlocksOnDemand())
                    //    throw boost::thread_interrupted();
                    break;
                }
                pblock->nNonce += 1;
                nHashesDone += 1;
                //cout << "Looking for a solution with nounce " << pblock->nNonce << " hashesDone : " << nHashesDone << endl;
                if ((pblock->nNonce & 0xFF) == 0)
                    break;
            }

            // Meter hashes/sec
            static int64_t nHashCounter;
            if (nHPSTimerStart == 0) {
                nHPSTimerStart = GetTimeMillis();
                nHashCounter = 0;
            } else
                nHashCounter += nHashesDone;
            if (GetTimeMillis() - nHPSTimerStart > 4000) {
                static CCriticalSection cs;
                {
                    LOCK(cs);
                    if (GetTimeMillis() - nHPSTimerStart > 4000) {
                        dHashesPerMin = 1000.0 * nHashCounter / (GetTimeMillis() - nHPSTimerStart);
                        nHPSTimerStart = GetTimeMillis();
                        nHashCounter = 0;
                        static int64_t nLogTime;
                        if (GetTime() - nLogTime > 30 * 60) {
                            nLogTime = GetTime();
                            //cout << "hashmeter %6.0f khash/s " << dHashesPerMin / 1000.0 << endl;
                        }
                    }
                }
            }

            // Check for stop or if block needs to be rebuilt
            boost::this_thread::interruption_point();
            // Regtest mode doesn't require peers
            if (vNodes.empty() && Params().MiningRequiresPeers())
                break;
            if (pblock->nNonce >= 0xffff0000)
                break;
            if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                break;
            if (pindexPrev != chainActive.Tip())
                break;

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev, fProofOfStake);
            // Changing pblock->nTime can change work required on testnet:
            hashTarget.SetCompact(pblock->nBits);
        }
    }

    // lets check if we have generated the munber of blocks requested
    BOOST_CHECK(oldnHeight + endBlock - startBlock == chainActive.Tip()->nHeight);
}

void GeneratePOWLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey, bool logToStdout)
{
    const CChainParams& chainparams = Params();
    unsigned int nExtraNonce = 0;

    for (int j = startBlock; j < endBlock; j++) {
        int lastBlock = chainActive.Tip()->nHeight;
        CAmount oldBalance = pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance();
        // Let-s make sure we have the correct spacing
        //MilliSleep(Params().TargetSpacing()*1000);
        bool foundBlock = false;
        //
        // Create new block
        //
        CBlockIndex* pindexPrev = chainActive.Tip();

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, NULL, false));

        if (!pblocktemplate.get()) {
            //cout << "Error in KoreMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread" << endl;
            return;
        }
        CBlock* pblock = &pblocktemplate->block;
        IncrementExtraNonce_Legacy(pblock, pindexPrev, nExtraNonce);

        LogPrintf("Running KoreMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
            ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

        //
        // Search
        //
        int64_t nStart = GetTime();
        arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
        uint256 testHash;

        for (; !foundBlock;) {
            unsigned int nHashesDone = 0;
            unsigned int nNonceFound = (unsigned int)-1;

            for (int i = 0; i < 1; i++) {
                pblock->nNonce = pblock->nNonce + 1;
                testHash = pblock->CalculateBestBirthdayHash();
                nHashesDone++;
                //cout << "proof-of-work found  "<< endl;
                //cout << "testHash  : " << UintToArith256(testHash).ToString() << endl;
                //cout << "target    : " << hashTarget.GetHex() << endl;
                if (UintToArith256(testHash) < hashTarget) {
                    // Found a solution
                    nNonceFound = pblock->nNonce;
                    // Found a solution
                    assert(testHash == pblock->GetHash());
                    foundBlock = true;
                    ProcessBlockFound_Legacy(pblock, chainparams);
                    // We have our data, lets print them
                    LogBlockFound(pwallet, j, pblock, nExtraNonce, false, logToStdout);

                    break;
                }
            }

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev);
        }

        // a new block was created
        BOOST_CHECK(chainActive.Tip()->nHeight == lastBlock + 1);
        // lets check if the block was created and if the balance is correct
        CAmount bValue = GetBlockValue(chainActive.Tip()->nHeight);
        // 10% to dev fund, we don't have masternode
        if(fDebug) {
            LogPrintf("Checking balance is %s \n", (pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance() == oldBalance + bValue * 0.9 ? "OK" : "NOK"));
        }
        BOOST_CHECK(pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance() == oldBalance + bValue * 0.9);
    }
}

void GeneratePOSLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();

    InitializeLastCoinStakeSearchTime(pwallet, scriptPubKey);

    for (int j = startBlock; j < endBlock; j++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
        if (!pblocktemplate.get())
            return;
        CBlock* pblock = &pblocktemplate->block;
        if (SignBlock_Legacy(pwallet, pblock)) {
            if (ProcessBlockFound_Legacy(pblock, chainparams)) {
                // we dont have extranounce for pos
                LogBlockFound(pwallet, j, pblock, 0, true);
                // Let's wait to generate the nextBlock
                MilliSleep(Params().TargetSpacing() * 1000);
            } else {
                //cout << "NOT ABLE TO PROCESS BLOCK :" << j << endl;
            }
        }
    }
}

void Create_Transaction(CBlock* pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

void Create_NewTransaction(CBlock* pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    //txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

void CreateOldBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t& blockInfo, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake)
{
    CBlockTemplate* pblocktemplate;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();


    std::vector<CTransaction*> txFirst;
    for (int i = startBlock - 1; i < endBlock - 1; i++) {
        // Simple block creation, nothing special yet:
        BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, fProofOfStake));
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = CBlockHeader::CURRENT_VERSION;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        if (!fProofOfStake) {
            pblock->nTime = blockinfo[i].nTime;
            Create_Transaction(pblock, pindexPrev, blockinfo, i);
        }
        pblock->nNonce = blockinfo[i].nonce;
        pblock->nBirthdayA = blockinfo[i].nBirthdayA;
        pblock->nBirthdayB = blockinfo[i].nBirthdayB;
        CValidationState state;
        //cout << "Found Block === " << i+1 << " === " << endl;
        //cout << "nTime         : " << pblock->nTime << endl;
        //cout << "nNonce        : " << pblock->nNonce << endl;
        //cout << "extranonce    : " << blockinfo[i].extranonce << endl;
        //cout << "nBirthdayA    : " << pblock->nBirthdayA << endl;
        //cout << "nBirthdayB    : " << pblock->nBirthdayB << endl;
        //cout << "nBits         : " << pblock->nBits << endl;
        //cout << "Hash          : " << pblock->GetHash().ToString().c_str() << endl;
        //cout << "hashMerkleRoot: " << pblock->hashMerkleRoot.ToString().c_str()  << endl;
        //cout << "New Block values" << endl;

        if (fProofOfStake) {
            BOOST_CHECK(SignBlock_Legacy(pwallet, pblock));
            //cout << pblock->ToString() << endl;
            //cout << "scriptPubKey: " << HexStr(scriptPubKey) << endl;
            // the coin selected can be a different one, so the hash will be different
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        } else {
            //cout << pblock->ToString() << endl;
            BOOST_CHECK(scriptPubKey == pblock->vtx[0].vout[0].scriptPubKey);
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
            LogBlockFound(pwallet, i + 1, pblock, blockinfo[i].extranonce, fProofOfStake);
        }

        BOOST_CHECK(state.IsValid());
        // we should get the same balance, depends the maturity
        // cout << "Block: " << i+1 << " time ("<< pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        // BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
        if (pblocktemplate)
            delete pblocktemplate;
    }
}

void createNewBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t& blockInfo, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake)
{
    CBlockTemplate* pblocktemplate;
    const CChainParams& chainparams = Params();
    CReserveKey reservekey(pwallet); // only for consistency !!!


    CBlockIndex* pindexPrev = chainActive.Tip();

    std::vector<CTransaction*> txFirst;
    for (int i = startBlock - 1; i < endBlock - 1; i++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
        assert(pblocktemplate.get() != NULL);
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = CBlockHeader::CURRENT_VERSION;
        //pblock->nTime = blockinfo[i].nTime;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        if (!fProofOfStake)
            Create_NewTransaction(pblock, pindexPrev, blockinfo, i);
        pblock->nNonce = blockinfo[i].nonce;
        pblock->nBirthdayA = blockinfo[i].nBirthdayA;
        pblock->nBirthdayB = blockinfo[i].nBirthdayB;
        CValidationState state;
        //cout << "Found Block === " << i+1 << " === " << endl;
        //cout << "nTime         : " << pblock->nTime << endl;
        //cout << "nNonce        : " << pblock->nNonce << endl;
        //cout << "extranonce    : " << blockinfo[i].extranonce << endl;
        //cout << "nBirthdayA    : " << pblock->nBirthdayA << endl;
        //cout << "nBirthdayB    : " << pblock->nBirthdayB << endl;
        //cout << "nBits         : " << pblock->nBits << endl;
        //cout << "Hash          : " << pblock->GetHash().ToString().c_str() << endl;
        //cout << "hashMerkleRoot: " << pblock->hashMerkleRoot.ToString().c_str()  << endl;
        //cout << "New Block values" << endl;

        if (fProofOfStake) {
            BOOST_CHECK(SignBlock(*pblock, *pwallet));
            //cout << pblock->ToString() << endl;
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
        } else {
            //cout << pblock->ToString() << endl;
            //BOOST_CHECK(scriptPubKey == pblock->vtx[0].vout[0].scriptPubKey);
            // previous block hash is not the same
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
        }

        BOOST_CHECK(state.IsValid());
        // we should get the same balance
        cout << "Block: " << i + 1 << " time (" << pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
    }
}