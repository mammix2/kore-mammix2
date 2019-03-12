

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

blockinfo_t blockinfo[] = {
    {0, 1552326080, 1552326005 , 538968063 , 5 , 1 , 3006047 , 59734105 , uint256("0a4d78a0eee2a4414361d3a47a9c56d5613b9deea70c97aee5da5d4b8b66eae9") , uint256("ec6a962b227824b0bb405f00d74c46f38f8b8e945001484e7a5fbc525e30fbf6") , 0 },       // Block 1    
    {0, 1552326458, 1552326099 , 538968063 , 19 , 1 , 0 , 0 , uint256("15868e9f0e2893b0cbc71da336c833687191cc154caec8a994832280cb3be6c3") , uint256("f687437b883b2c0689297a26b07e2e7c0d40b4f0faae8345ff64e944aa446b10") , 0 },                   // Block 2
    {0, 1552327987, 1552326482 , 537050656 , 71 , 1 , 2064757 , 13884784 , uint256("02a622f1e2a2c167156dd882448524e785756b92494ae46b46c64a2b25162cc2") , uint256("183eb70efbbc832a456cb94d9d3798df706501b0ab52f4daaadb05a0410ace13") , 0 }, // Block 3    
    {0, 1552328394, 1552328013 , 536942286 , 17 , 1 , 7892929 , 58159534 , uint256("00bfca079fd6a03b4d38048f70746257322c715fcb0abe4ce6e7399fd65efe3f") , uint256("220650be41d0259edc22a3c0ddc1e8aced3c229346352201953a7cf260fe9489") , 900000000000 }, // Block 4    
    {0, 1551475584, 1551474450, 527027404, 63, 1, 21241524, 33555986, uint256("003044cffa95e38e9af9cee7251ca79227ba300395c67e82c45cd80167baab45"), uint256("254a60aa559088a997d0d0b59022d899a7dbbadc9d512905896d20b80f7560eb"), 1800000000000},   // Block 5
    {0, 1551476799, 1551475602, 537061937, 61, 1, 53340836, 58025984, uint256("0041034b9203fe319f48f9de026a20ea69dfe0e9347c71b7e4ac4b178a49da9d"), uint256("fb77c45bcfd87b9a4a0e4456c6b22f858cc79e186f6fa0a2d5e277101bd82318"), 2700000000000},   // Block 6
    {0, 1551483623, 1551476828, 536927768, 236, 1, 16990015, 43555895, uint256("0075f7051456d5a2a6f7e0dfcb992235fb422eff2c9e68b6e9c8e3b1358f978e"), uint256("531f7ed02da106f52ebf3ee37f8a16d5080a9a28015ed8e480ab440e956f61a2"), 3600000000000},  // Block 7
    {0, 1551489611, 1551483652, 536971791, 209, 1, 15667256, 45726627, uint256("01320bfd93cc9c93d039736d1f7cc646f78f8d3a4efb5381442559a8af21596a"), uint256("1859960daf24babcfdeadf772a034596936166c4575a2f51088d6ea292f09a6c"), 4500000000000},  // Block 8
    {0, 1551494608, 1551489639, 536945359, 176, 1, 29412413, 44441601, uint256("00e8bdce06f4ed3ad862675a7f7f8994236a0c22a28ffd063d278245a0883063"), uint256("e27cddc4d7121ce8a7e7c814d2e7cd23fb7991ef47aa52b4d3ba8475179c9ecf"), 5400000000000},  // Block 9
    {0, 1551495264, 1551494637, 537007314, 23, 1, 0, 0, uint256("0181067961449d035a63df9e78badb1d0de020ba5732b486dc7cbeb68e6b6447"), uint256("aafb92ffa1a21e3cd29b208032a07a9c0d44cdfd8c5be2a7893c4a5d20b5752c"), 6300000000000},                 // Block 10
    {0, 1551497736, 1551495293, 524592522, 87, 1, 48047704, 51726335, uint256("003c23b7ec76178e6cf3c3c47684b64a284b0603245e0cfbbd0d2b06daf12e67"), uint256("963960b09ec8f37444c9b0c10a32970f1114891cb8695570f34a0bd386d6c7eb"), 7200000000000},   // Block 11
    {0, 1551498729, 1551497764, 536926285, 35, 1, 52630118, 52808990, uint256("005233057cad0b766e46335c835302c087cb22f92fcadb8d1353b5612815c683"), uint256("fa7eec8e5d8fc19611137549df9074f159efa85129e36e6adfce8aab290bce3a"), 8100000000000},   // Block 12
    {0, 1551500913, 1551498758, 537043428, 77, 1, 0, 0, uint256("000b8b5767174c161c8e08ecdfa9c43a35e2c55449bb9c9147b1497c24e4e3b4"), uint256("a60761fc6040329f6560bbce819b908a19ceb2be6100d334073708ca0636ea6c"), 9000000000000},                 // Block 13
    {0, 1551501482, 1551500942, 536973782, 20, 1, 1583178, 49097368, uint256("01725abcb51ffed04dda0e7a18c069513fe4a58679c3a2eea62d9f88c8b9c2e0"), uint256("c780f1bd40f0acad8e6b5448ef56c2cc8e51ab6e4ddd3bc42cee52952cb1f39d"), 9900000000000},    // Block 14
    {0, 1551501822, 1551501511, 536939041, 12, 1, 0, 0, uint256("0029f678b5eb25e97f1f84ce7f7a82c538ccd5bf84f8e185f80c56b7ef9d4173"), uint256("3622b665dab85a1d20d010b18d4adfae9eecc64c14e6ab9e243218274624089f"), 10800000000000},                // Block 15
    {0, 1551502448, 1551501851, 537102051, 22, 1, 19348442, 53522700, uint256("02ab43e3c0d88ec50271c69f618e45f76954b7a6267133bba73de4224ccf6e54"), uint256("68cfa0629befb9e55f56662297d8a5f092f1746c6ee975ce992e58ad1ea6754e"), 11700000000000},  // Block 16
    {0, 1551502476, 1551502476, 537095630, 1, 1, 17323987, 61197640, uint256("02c86e3adfad214b080c3453d822d28b148eb926a4201dcfbccdec3d7548934d"), uint256("7d398d02e1c3d90206946b3c38a5df0fbf8bbf725e16727bbded80a42e228961"), 12600000000000},   // Block 17
    {0, 1551503327, 1551502505, 536971524, 30, 1, 37836578, 66709231, uint256("0162ec5ef5a4d83b53ca4e0f2356572fc631d6eb84b179a5230ccd28dd22f57d"), uint256("9d72ef4945df73f51c4fa17b156dd7fc5dc17dd17c1d730c6a7e9aa0e44e4ab5"), 13500000000000},  // Block 18
    {0, 1551504351, 1551503356, 536992341, 36, 1, 3987627, 56434268, uint256("015bd277203a8da0abde3a12b7bc226bc58311e7ee85224ca0c75feddbbbccd7"), uint256("835bf778772fe674813ef70e226d30cdeb8463d41bf7b71e36773c07dca7c8c3"), 14400000000000},   // Block 19
    {0, 1551508166, 1551504379, 536915194, 134, 1, 8633900, 23229541, uint256("009cb2d47d0644f10e5fd34f483c61b08c9e3526e9d34ff9ecc122ac4257d0dd"), uint256("2901660624b1aebc9a80491b5f2d142aff44765bd775ac410a6105e27a4ef884"), 15300000000000},  // Block 20
    {0, 1551508450, 1551508195, 536935789, 10, 1, 41155409, 63486626, uint256("009ce8b647f5516117ef0ca4290f2da04ef92fb23f14ef7d2f0258b75e6b5399"), uint256("b86ce3cd8440e2a7c881f3871990e01922db824cd7bf138d0006db0c50438ac1"), 16200000000000},  // Block 21
    {0, 1551510210, 1551508478, 536964334, 62, 1, 0, 0, uint256("0048e8a639bc7019a162050bc4d9d9df6bf9c70d34570e0fc3d2c03199bc36ad"), uint256("4ec9d8f246e90437e52dc35f48db92d8026a18302c8e95f13239042136d1dcc4"), 17100000000000},                // Block 22
    {0, 1551510608, 1551510238, 537081618, 14, 1, 0, 0, uint256("0086a9f3739a2d372215e13cef19c5182e4fd119851826cef3938630020d3025"), uint256("a70379c58ba8b417bcb94a960849370bbae8f28273c87f3953dfbe744d368b32"), 18000000000000},                // Block 23
    {0, 1551511488, 1551510636, 537020699, 31, 1, 45711880, 59054890, uint256("002da7f26d66c8a85c77db19aedb5e0447f982fa1741e8c3e1592f6c0dc904e6"), uint256("2cc725aa243f0d6638d8288922869247c247c7d88fda88713e887d01fe81b35c"), 18900000000000},  // Block 24
    {0, 1551514104, 1551511516, 536909183, 92, 1, 0, 0, uint256("0061d09fd70e0b8da5db5bd5643122e4b7ef5d50e74d8556a43ea33048e1eb77"), uint256("d1ee53ebfef66b6b05b7b911252173f18104033fcf688e6d328ace59efec8ba4"), 19800000000000},                // Block 25
    {0, 1551529284, 1551514132, 523095588, 564, 1, 31748974, 41739780, uint256("0015ef37b9815e919483bb0645ed4a0de2689ce3496b391cc08ee989d9499b72"), uint256("c53852379083cb14cae2624d46b500dacd73ff7b072a952985b0f0400cd0227a"), 20700000000000}, // Block 26
    {0, 1551529586, 1551529303, 536931917, 17, 1, 0, 0, uint256("000d9a76d66f5547ea1baa411460a1f13c03f7a51e63df069334038f3c153736"), uint256("1db42fbdb2b512b602e693158ac5e01720ae40dbe31fef4687f0e22e2fb5cc64"), 21600000000000},                // Block 27
    {0, 1551530095, 1551529602, 536961523, 28, 1, 29977910, 36381927, uint256("01543ded937c05f056542fdf62562527c4979891127d1bccb8a2aa6a3e96c86c"), uint256("43dc09c34bf726308322bb118c791181c2837e4be03910d5027148ea248b1f4b"), 22500000000000},  // Block 28
    {0, 1551530241, 1551530113, 537055004, 8, 1, 16723414, 50019166, uint256("015b6516ec2155a7ddf40985b4d24fa4128217d55870e4f2455e964d2e663981"), uint256("7dacce7171bd5cde6c9da2365e990afc870380ec3849656293a9d738975944b6"), 23400000000000},   // Block 29
    {0, 1551531075, 1551530258, 537051309, 46, 1, 3343940, 33374859, uint256("009017e737fef7490fe9d565dc702a2988ea5113f382dac1f782b5c74df587a0"), uint256("fc7872c4d7fb4502eec00d8ffa63548b0e18bb8499a11cfa67aee1afceff4bba"), 24300000000000}    // Block 30
};


void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake, bool logToStdout)
{
    if (logToStdout) {
        cout << pblock->ToString().c_str();
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
    SetMockTime(GetTime() + 30);
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

    for (int j = startBlock; j < endBlock;) {
        SetMockTime(GetTime() + Params().GetTargetSpacing());
        if (fProofOfStake) {
            //control the amount of times the client will check for mintable coins
            if ((GetTime() - nMintableLastCheck > Params().GetClientMintableCoinsInterval())) {
                nMintableLastCheck = GetTime();
                fMintableCoins = pwallet->MintableCoins();
            }

            while (pwallet->IsLocked() || !fMintableCoins ||
                   (pwallet->GetBalance() > 0 && nReserveBalance >= pwallet->GetBalance())) {
                nLastCoinStakeSearchInterval = 0;
                // Do a separate 1 minute check here to ensure fMintableCoins is updated
                if (!fMintableCoins) {
                    if (GetTime() - nMintableLastCheck > Params().GetEnsureMintableCoinsInterval()) // 1 minute check time
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
                    SetMockTime(GetTime() + 5);
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
        //BOOST_CHECK(pblocktemplate.get());
        if (!pblocktemplate.get())
        {
            SetMockTime(GetTime() + (Params().GetTargetSpacing() * 2));
            continue;
        }
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
            j++;
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
                    // if (Params().ShouldMineBlocksOnDemand())
                    //    throw boost::thread_interrupted();
                    break;
                }
                pblock->nNonce += 1;
                nHashesDone += 1;
                //cout << "Looking for a solution with nounce " << pblock->nNonce << " hashesDone : " << nHashesDone << endl;
                if ((pblock->nNonce & 0xFF) == 0)
                    break;
            }

            // Check for stop or if block needs to be rebuilt
            boost::this_thread::interruption_point();
            // Regtest mode doesn't require peers
            if (vNodes.empty() && Params().DoesMiningRequiresPeers())
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
        j++;
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
        //MilliSleep(Params().GetTargetSpacing()*1000);
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
                SetMockTime(GetTime() + Params().GetTargetSpacing());
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
        cout << "Found Block === " << i+1 << " === " << endl;
        cout << "nTime         : " << pblock->nTime << endl;
        cout << "nNonce        : " << pblock->nNonce << endl;
        cout << "extranonce    : " << blockinfo[i].extranonce << endl;
        cout << "nBirthdayA    : " << pblock->nBirthdayA << endl;
        cout << "nBirthdayB    : " << pblock->nBirthdayB << endl;
        cout << "nBits         : " << pblock->nBits << endl;
        cout << "Hash          : " << pblock->GetHash().ToString().c_str() << endl;
        cout << "hashMerkleRoot: " << pblock->hashMerkleRoot.ToString().c_str()  << endl;
        cout << "New Block values" << endl;

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
