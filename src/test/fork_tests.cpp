// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2016 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "uint256.h"
#include "util.h"
#include "arith_uint256.h"
#include "legacy/consensus/merkle.h"
#include "validationinterface.h"
#include "blocksignature.h"
#include "wallet.h"
#include "pos.h" // Lico remove these includes

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(fork_tests)


void LogBlockFound(int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake)
{
    cout << "Block === " << blockNumber << " === " << endl;
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
    cout << "uint256(\"" << pblock->hashMerkleRoot.ToString().c_str() << "\") },";
    cout << " // " << blockNumber << endl;
}

void ScanForWalletTransactions(CWallet* pwallet)
{
    pwallet->nTimeFirstKey = chainActive[0]->nTime;
    // pwallet->fFileBacked = true;
    // CBlockIndex* genesisBlock = chainActive[0];
    // pwallet->ScanForWalletTransactions(genesisBlock, true);
}

void GenerateBlocks(int startBlock, int endBlock, CWallet* pwallet, bool fProofOfStake)
{
    CReserveKey reservekey(pwallet);

    bool fGenerateBitcoins = false;
    bool fMintableCoins = false;
    int nMintableLastCheck = 0;

    // Each thread has its own key and counter
    unsigned int nExtraNonce = 0;
    //ScanForWalletTransactions(pwallet);

    for (int j = startBlock; j < endBlock; j++) {
        if (fProofOfStake) {
            //control the amount of times the client will check for mintable coins
            if ((GetTime() - nMintableLastCheck > Params().ClientMintibleCoinsInterval()))
            {
                nMintableLastCheck = GetTime();
                fMintableCoins = pwallet->MintableCoins();
            }

            while (pwallet->IsLocked() || !fMintableCoins || 
                  (pwallet->GetBalance() > 0 && nReserveBalance >= pwallet->GetBalance()) )
            {
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
                    cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }

            if (mapHashedBlocks.count(chainActive.Tip()->nHeight)) //search our map of hashed blocks, see if bestblock has been hashed yet
            {
                if (GetTime() - mapHashedBlocks[chainActive.Tip()->nHeight] < max(pwallet->nHashInterval, (unsigned int)1)) // wait half of the nHashDrift with max wait of 3 minutes
                {
                    MilliSleep(5000);
                    cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }
        }

        //
        // Create new block
        //
        cout << "KOREMiner: Creating new Block " << endl;
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

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey, pwallet, fProofOfStake));
        if (!pblocktemplate.get())
            continue;

        CBlock* pblock = &pblocktemplate->block;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

        //Stake miner main
        if (fProofOfStake) {
            cout << "CPUMiner : proof-of-stake block found " << pblock->GetHash().ToString() << endl;
            if (!SignBlock(*pblock, *pwallet)) {
                cout << "BitcoinMiner(): Signing new block with UTXO key failed" << endl;
                continue;
            }

            cout << "CPUMiner : proof-of-stake block was signed " << pblock->GetHash().ToString() << endl;
            ProcessBlockFound(pblock, *pwallet, reservekey);
            continue;
        }

        //
        // Search
        //
        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        cout << "target: " << hashTarget.GetHex() << endl;
        while (true) {
            unsigned int nHashesDone = 0;

            uint256 hash;
            
            cout << "nbits : " << pblock->nBits << endl;
            while (true) {
                hash = pblock->GetHash();
                cout << "pblock.nBirthdayA: " << pblock->nBirthdayA << endl;
                cout << "pblock.nBirthdayB: " << pblock->nBirthdayB << endl;
                cout << "hash             : " << hash.ToString() << endl;
                cout << "hashTarget       : " << hashTarget.ToString() << endl;

                if (hash <= hashTarget) {
                    // Found a solution
                    cout << "BitcoinMiner:" << endl;
                    cout << "proof-of-work found  "<< endl;
                    cout << "hash  : " << hash.GetHex() << endl;
                    cout << "target: " << hashTarget.GetHex() << endl;
                    LogBlockFound(j, pblock, nExtraNonce, fProofOfStake);
                    ProcessBlockFound(pblock, *pwallet, reservekey);

                    // In regression test mode, stop mining after a block is found. This
                    // allows developers to controllably generate a block on demand.
                    if (Params().MineBlocksOnDemand())
                        throw boost::thread_interrupted();
                    break;
                }                
                pblock->nNonce += 1;                
                nHashesDone += 1;
                cout << "Looking for a solution with nounce " << pblock->nNonce << " hashesDone : " << nHashesDone << endl;
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
                            cout << "hashmeter %6.0f khash/s " << dHashesPerMin / 1000.0 << endl;
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
            if (Params().AllowMinDifficultyBlocks()) {
                // Changing pblock->nTime can change work required on testnet:
                hashTarget.SetCompact(pblock->nBits);
            }
        }
    }
}

void GeneratePOWLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript & scriptPubKey)
{
    const CChainParams& chainparams = Params();
    unsigned int nExtraNonce = 0;
    //ScanForWalletTransactions(pwallet);

    for (int j = startBlock; j < endBlock; j++) {
        bool foundBlock = false;
        //
        // Create new block
        //
        CBlockIndex* pindexPrev = chainActive.Tip();

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, NULL, false));

        if (!pblocktemplate.get()) {
            cout << "Error in KoreMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread" << endl;
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

        for (;!foundBlock;) {
            unsigned int nHashesDone = 0;
            unsigned int nNonceFound = (unsigned int)-1;

            for (int i = 0; i < 1; i++) {
                pblock->nNonce = pblock->nNonce + 1;
                testHash = pblock->CalculateBestBirthdayHash();
                nHashesDone++;
                cout << "proof-of-work found  "<< endl;
                cout << "testHash  : " << UintToArith256(testHash).ToString() << endl;
                cout << "target    : " << hashTarget.GetHex() << endl;
                if (UintToArith256(testHash) < hashTarget) {
                    // Found a solution
                    nNonceFound = pblock->nNonce;
                    // Found a solution
                    assert(testHash == pblock->GetHash());
                    // We have our data, lets print them
                    LogBlockFound(j, pblock, nExtraNonce, false);
                    foundBlock = true;
                    ProcessBlockFound_Legacy(pblock, chainparams);
                    break;
                }
            }

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev);
        }
    }
}


void GeneratePOSLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();

    for (int j = startBlock; j < endBlock; j++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
        if (!pblocktemplate.get())
            return;        
        CBlock *pblock = &pblocktemplate->block;
        if(SignBlock_Legacy(pwallet, pblock))
        {
            // Lico it is failing here, I believe because the scriptSig is now at the block
            // and it will be checked !
            CValidationState state;
            CheckProofOfStake_Legacy(mapBlockIndex[pblock->hashPrevBlock], pblock->vtx[1], pblock->nBits, state);

//            ProcessBlockFound_Legacy(pblock, chainparams);
            // we dont have extranounce for pos
//            LogBlockFound(j, pblock, 0, true);
        }
    }
}

BOOST_AUTO_TEST_CASE(generate_chain)
{
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(3);
    
    ScanForWalletTransactions(pwalletMain);
    CReserveKey reservekey(pwalletMain);
    CPubKey pubkey;
    reservekey.GetReservedKey(pubkey);

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;

    // generate 1 pow blocks
    GeneratePOWLegacyBlocks(1,2, pwalletMain, scriptPubKey);
    cout << "My Local Balance : " << pwalletMain->GetBalance() << endl;
    // generate 1 pos blocks
    GeneratePOSLegacyBlocks(2,3, pwalletMain, scriptPubKey);
    //ScanForWalletTransactions(pwalletMain);
    cout << "My Local Balance : " << pwalletMain->GetBalance() << endl;

    // here the fork will happen, lets check pos
    GenerateBlocks(3,5, pwalletMain, true);
    cout << "My Local Balance : " << pwalletMain->GetBalance() << endl;

    ModifiableParams()->setHeightToFork(oldHeightToFork);}

typedef struct {
    uint32_t nTime;
    uint32_t transactionTime;
    uint32_t nBits;
    unsigned int nonce;
    unsigned int extranonce;
    uint32_t nBirthdayA;
    uint32_t nBirthdayB;
    uint256 hash;
    uint256 hashMerkleRoot;
} blockinfo_t;
static blockinfo_t blockinfo[] = 
{
    {1547752107, 1547752061 , 538968063 , 3 , 1 , 13515173 , 46200744 , uint256("06dcd72b5b7fa5dc76dfef3d1eb753f58fc2536be68398bb812f5ca04ee5932d") , uint256("e5d190ca67264337967d3adca2dc07fa0e7905b739e8823a33ebbe60bfb0dfdf") }, // 1
    {1547752152, 1547752129 , 538968063 , 2 , 1 , 33389340 , 40260916 , uint256("091ad906bdafc871cd147722af3313a0a15472f7e51b6f4ac4a73daa364e8d81") , uint256("5145866200779930b8dd49a1392973e0dea417466fc2cc0086f69fda43f5d528") }, // 2
};
/*
{
    {1547646047,538968063, 1, 1, 0, 0, uint256("04ebab8f214e8d513484082f2ed45e200d1afca8d151a76c6b4d5cce8a4c1bbe"), uint256("ad92624f705b94d24d96297649b1b914b5eaae7087b1f999d5b9096a696587e7")}, // 1
    {1547646190,538968063, 2, 7, 13152152, 62639306, uint256("022233dddaa2969b168511becedfb7f3f7966488796fcdd06d44e88f3b409f74"), uint256("3ae2e6a06efd375e345ef77bdd594b3c04851e29f93d2eceb7a6822a0317be24")}, // 2
    {1547646332,538968063, 3, 7, 14758381, 34298591, uint256("0dd1953ac4dab1a15490bbc499cc4c554c766cc9bebecf636388f5b7b71650ae"), uint256("3b51ee61b673f867532a59a741d4bc2ec446e6dfcddea505ead7b74cac9344bf")}, // 3
    {1547646352,538968063, 4, 1, 20802262, 38093666, uint256("09eda99b7d7a7ec8d08808320cd9894e99c16abd65b4e9ddfa7f25586e2a3b2e"), uint256("0239ce261590c070ca54f9583c96d9731326a1374e10fff4f3d371be38b956ab")}, // 4
    {1547646460,538968063, 5, 5, 43998604, 51531886, uint256("0027fa0c8be02d439910f4253a95c9cf5d2b4302bb639cf7eb1cff663aab6f60"), uint256("8344a51d783d0b8627e4cf46bf393a4aaf24ea47b52abc341136f18b45e71243")}, // 5
    {1547646563,538968063, 6, 5, 0, 0, uint256("15737b540cf7dc2409af6b259095fef6b4a1a274bcd3555cf7e13a98b1e1e095"), uint256("3dc14788d0d91d165bcb8bf01792b74117e900636d3c51f072d6a93f438731cc")}, // 6
    {1547646583,538968063, 7, 1, 24376095, 34997369, uint256("0988f98068970832a3c5a9f3bfc22840fd471d05e6fe7b491ed6b271e31a1238"), uint256("41fa0be1ad94c227394a9a573819f4863588e83c1535ab156c8206222a9de978")}, // 7
    {1547646687,538968063, 8, 5, 0, 0, uint256("03a99f32061e45a97911a76dd9ce5ce93fa0c77847aa7b2190497ab10797cc4b"), uint256("059f1858317e180f041f323e01737505f6e9f597ca5d0ef96a486b33998d574b")}, // 8
    {1547646853,538968063, 9, 8, 4229335, 17397782, uint256("0af6b2789b0794c1e5283e09633829c3fb40418c0008fa87d789b8caf2fce04d"), uint256("1581ac41dd69b4a43ab18e6cddf23e2c5d3e5dea311f9ce0acedd19d7c55d911")}, // 9
    {1547646915,538968063, 10, 3, 6218908, 32475497, uint256("060865272ed246a3e3f31881155409f30a52d50296e4e51fda3e0db0aa18e7e8"), uint256("2d3b478aa298da6894ed8f2854f95045af1053f5df10cce7fca9073ec48cdb5a")}  // 10
};
*/

void create_transaction(CBlock *pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(basic_fork)
{
    SelectParams(CBaseChainParams::UNITTEST);
    int oldHeightToFork = Params().HeigthToFork();
    int oldLastPOW = Params().LAST_POW_BLOCK();
    ModifiableParams()->setHeightToFork(10);
    ModifiableParams()->setLastPOW(5);
    
    // Lets create 5 pow blocks than 5 pos than we fork

    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    CBlockTemplate *pblocktemplate;

    LOCK(cs_main);
    Checkpoints::fEnabled = false;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwalletMain, false));

    // lets create 5 pow blocks
    std::vector<CTransaction*>txFirst;
    for (unsigned int i = 0; i < 5; ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
        pblock->nTime = blockinfo[i].nTime;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        create_transaction(pblock, pindexPrev, blockinfo, i);
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
        cout << pblock->ToString() << endl;
        BOOST_CHECK(pblock->GetHash()==blockinfo[i].hash);
        BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
        BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        BOOST_CHECK(state.IsValid());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
    }
    delete pblocktemplate;

    Checkpoints::fEnabled = true;
    // Leaving old values
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setLastPOW(oldLastPOW);
}

BOOST_AUTO_TEST_SUITE_END()

/*
Found Block === 1 ===
nTime         : 1547648900
nNonce        : 1
nExtraNonce   : 1
nBirthdayA    : 1368286
nBirthdayB    : 22233386
nBits         : 538968063
Hash          : 09630d6236c75cf87c343cf79b0cc976d3fd3ae0e6933987b70ca2348f3aa1f7
hashMerkleRoot: 6674b0e18d781542ecd95c7e05cf6f0b2f51d7401124e46edbf7ef4f2a2a61fc
Found Block === 2 ===
nTime         : 1547649081
nNonce        : 8
nExtraNonce   : 2
nBirthdayA    : 27496484
nBirthdayB    : 60410717
nBits         : 538968063
Hash          : 099371a45c9f7c8538c7be3b73a0497999db36c76f78a31ebdbb5f233a924779
hashMerkleRoot: 3a7858f5ac2d63e8633903e16d2c65d9574773b881b2b71c2cf74e3ce6566f2f
Found Block === 3 ===
nTime         : 1547649158
nNonce        : 2
nExtraNonce   : 3
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 1344b5c95f057c49c2306b1e1699bd783cb6c887631c7e69b15d6cefd2c44ab0
hashMerkleRoot: 17d2f12ffdf32bfc82cc06615182d297b53fb6f919438aac3f133d314e4922f4
Found Block === 4 ===
nTime         : 1547649224
nNonce        : 3
nExtraNonce   : 4
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 049139d1364a001660e4f47881f2d200e2e62f648b46821a88354c23a1279399
hashMerkleRoot: ff301100f57b1cef79a064a93a6b406b6322d75da291fd57be24bd7dbb9935da
Found Block === 5 ===
nTime         : 1547649289
nNonce        : 3
nExtraNonce   : 5
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 12b780a43dd2d65ffad840596cbdd4827f27ed775281e001f3dde578d8918f50
hashMerkleRoot: 30d028184bb8500c0a533c97d3db26151dfb1b777768f979fc98520c12ca6dc4
Found Block === 6 ===
nTime         : 1547649311
nNonce        : 1
nExtraNonce   : 6
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 124d01a68e72bffccca3e5591f89f19d5531bb30f7315183780b54dd41aa2e06
hashMerkleRoot: 9ad4d77cfa40c428d3bfb516062081976d540024a04d4e5a759db4610c6956b2
Found Block === 7 ===
nTime         : 1547649378
nNonce        : 3
nExtraNonce   : 7
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 015c15ee52c89d6037086fc60ed2037a12f67f8821f6ed435744a45bc9f1b498
hashMerkleRoot: f6b1f19b1c174da521e4011195a34324dd662f64e6eb9111e8c5622fc5b2205b
Found Block === 8 ===
nTime         : 1547649422
nNonce        : 2
nExtraNonce   : 8
nBirthdayA    : 2791632
nBirthdayB    : 35301644
nBits         : 538968063
Hash          : 0429c8d05895ccee3821f527d31a4e36d3544e89b6737317e065fb42f6a83bff
hashMerkleRoot: 794c006641c9b8aa01adaa63789df56f58fb6c902275925b47ace733c2d59cce
Found Block === 9 ===
nTime         : 1547649645
nNonce        : 10
nExtraNonce   : 9
nBirthdayA    : 6529734
nBirthdayB    : 24972700
nBits         : 538968063
Hash          : 03320946798e145ec7f09ab977c0c619b6240b85e4de50cf5ce6b01ce2e5c0d6
hashMerkleRoot: 88b75b08a97379abc6f2e60d870d0e3413998a5636d86265c3153bd2f6d96282
Found Block === 10 ===
nTime         : 1547649793
nNonce        : 7
nExtraNonce   : 10
nBirthdayA    : 0
nBirthdayB    : 0
nBits         : 538968063
Hash          : 01cdbfe5e18a93ce72745981bd7e31daaebcc556ba8736f1241e9a9ad979faf4
hashMerkleRoot: 2785b5ffa82ac5d94c15a48ecb0df8254ed30109f155dac105ab914cce1346dd
*/