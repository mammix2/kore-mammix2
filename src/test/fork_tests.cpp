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
#include "utiltime.h"

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


void InitializeLastCoinStakeSearchTime(CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();

    // this is just to initialize nLastCoinStakeSearchTime
    unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
    if (!pblocktemplate.get())
      return;        
    CBlock *pblock = &pblocktemplate->block;
    SignBlock_Legacy(pwallet, pblock);
    MilliSleep(30000);
}

void GeneratePOSLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();
        
    InitializeLastCoinStakeSearchTime(pwallet, scriptPubKey);

    for (int j = startBlock; j < endBlock; j++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
        if (!pblocktemplate.get())
            return;        
        CBlock *pblock = &pblocktemplate->block;
        if(SignBlock_Legacy(pwallet, pblock))
        {
            if (ProcessBlockFound_Legacy(pblock, chainparams)) {
                // we dont have extranounce for pos
                LogBlockFound(j, pblock, 0, true);
                // Let's wait to generate the nextBlock
                MilliSleep(Params().TargetSpacing()*1000);
            } else {
                cout << "NOT ABLE TO PROCESS BLOCK :" << j << endl;
            }
        } 
    }
}

BOOST_AUTO_TEST_CASE(generate_chain)
{
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(3);
    // we dont need any blocks to confirm.
    ModifiableParams()->setStakeMinConfirmations(0); 
    
    ScanForWalletTransactions(pwalletMain);
    CReserveKey reservekey(pwalletMain);
    CPubKey pubkey;
    reservekey.GetReservedKey(pubkey);

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;

    // generate 1 pow blocks
    GeneratePOWLegacyBlocks(1,2, pwalletMain, scriptPubKey);
    cout << "My Local Balance : " << pwalletMain->GetBalance() << endl;
    // generate 1 pos blocks
    GeneratePOSLegacyBlocks(2,4, pwalletMain, scriptPubKey);
    //ScanForWalletTransactions(pwalletMain);
    cout << "My Local Balance : " << pwalletMain->GetBalance() << endl;

    // here the fork will happen, lets check pos
    //GenerateBlocks(3,5, pwalletMain, true);
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

