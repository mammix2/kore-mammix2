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
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>


BOOST_AUTO_TEST_SUITE(fork_tests)


static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");

CScript GenerateSamePubKeyScript4Wallet(CWallet* pwallet)
{
    
    CBitcoinSecret bsecret;
    bsecret.SetString(strSecret);
    CKey key = bsecret.GetKey();
    CPubKey pubKey = key.GetPubKey();
    CKeyID keyID = pubKey.GetID();
    CScript scriptPubKey = GetScriptForDestination(keyID);

    //pwallet->NewKeyPool();
    LOCK(pwallet->cs_wallet);
    pwallet->AddKeyPubKey(key, pubKey);
    pwallet->SetDefaultKey(pubKey);

    return scriptPubKey;
}

void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake)
{
    //cout << pblock->ToString().c_str();
    /*
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
*/
    //cout << "Current Time : " << GetTime() << endl;
    //cout << "" << endl;
    cout << "Block " << blockNumber << endl;
    cout << " hash             : " << pblock->GetHash().ToString().c_str() << endl;
    cout << " StakeModifier    : " << chainActive.Tip()->nStakeModifier << endl;
    cout << " OldStakeModifier : " << chainActive.Tip()->nStakeModifierOld.ToString() << endl;
    cout << " ---- " << endl;
}

void ScanForWalletTransactions(CWallet* pwallet)
{
    pwallet->nTimeFirstKey = chainActive[0]->nTime;
    // pwallet->fFileBacked = true;
    // CBlockIndex* genesisBlock = chainActive[0];
    // pwallet->ScanForWalletTransactions(genesisBlock, true);
}

void GenerateBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript &scriptPubKey, bool fProofOfStake)
{
    bool fGenerateBitcoins = false;
    bool fMintableCoins = false;
    int nMintableLastCheck = 0;
    CReserveKey reservekey(pwallet); // Lico, once we want to use the same pubkey, we dont need to remove it from key pool

    // Each thread has its own key and counter
    unsigned int nExtraNonce = 0;

    int oldnHeight = chainActive.Tip()->nHeight;

    for (int j = startBlock; j < endBlock; j++) {
        if (!fProofOfStake) MilliSleep(Params().TargetSpacing()*1000);
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
    BOOST_CHECK(oldnHeight + endBlock - startBlock  == chainActive.Tip()->nHeight);
}

void GeneratePOWLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript & scriptPubKey)
{
    const CChainParams& chainparams = Params();
    unsigned int nExtraNonce = 0;
    //ScanForWalletTransactions(pwallet);

    for (int j = startBlock; j < endBlock; j++) {
        int lastBlock = chainActive.Tip()->nHeight;
        CAmount oldBalance = pwallet->GetBalance();
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

        for (;!foundBlock;) {
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
                    LogBlockFound(pwallet, j, pblock, nExtraNonce, false);

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
        BOOST_CHECK(pwallet->GetBalance() == oldBalance + bValue * 0.9 );
        
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
                LogBlockFound(pwallet, j, pblock, 0, true);
                // Let's wait to generate the nextBlock
                MilliSleep(Params().TargetSpacing()*1000);
            } else {
                //cout << "NOT ABLE TO PROCESS BLOCK :" << j << endl;
            }
        } 
    }
}

/*
  This test case is only used if we need to regenerate our blockinfo
*/

BOOST_AUTO_TEST_CASE(generate_chain)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0); 
    ModifiableParams()->setEnableBigRewards(true);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setTargetSpacing(10);
    
    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used      : " << scriptPubKey.ToString() << endl;
    cout << "pub key used (hex): " << HexStr(scriptPubKey) << endl;

    // generate 5 pow blocks
    GeneratePOWLegacyBlocks(1,6, pwalletMain, scriptPubKey);
    // generate 5 pos blocks
    GeneratePOSLegacyBlocks(6,11, pwalletMain, scriptPubKey);

    // here the fork will happen, lets check pow
    GenerateBlocks(11,16, pwalletMain, scriptPubKey, false);

    GenerateBlocks(16,21, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    ModifiableParams()->setTargetTimespan(oldTargetTimespan);
    ModifiableParams()->setTargetSpacing(oldTargetSpacing);
}


BOOST_AUTO_TEST_CASE(fork_from_pos)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0); 
    ModifiableParams()->setEnableBigRewards(true);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setTargetSpacing(10);
    
    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used      : " << scriptPubKey.ToString() << endl;
    cout << "pub key used (hex): " << HexStr(scriptPubKey) << endl;

    // generate 1 pow blocks
    GeneratePOWLegacyBlocks(1,2, pwalletMain, scriptPubKey);
    // generate 8 pos blocks
    GeneratePOSLegacyBlocks(2,11, pwalletMain, scriptPubKey);
    //ScanForWalletTransactions(pwalletMain);

    // here the fork will happen, lets check pow
    GenerateBlocks(11,21, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    ModifiableParams()->setTargetTimespan(oldTargetTimespan);
    ModifiableParams()->setTargetSpacing(oldTargetSpacing);    
}


BOOST_AUTO_TEST_CASE(pow_pos_pos)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0);
    ModifiableParams()->setEnableBigRewards(true);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setTargetSpacing(15);

    
    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used      : " << scriptPubKey.ToString() << endl;
    cout << "pub key used (hex): " << HexStr(scriptPubKey) << endl;

    // generate 5 pow blocks
    GeneratePOWLegacyBlocks(1,2, pwalletMain, scriptPubKey);
    // generate 5 pos blocks
    GeneratePOSLegacyBlocks(2,11, pwalletMain, scriptPubKey);

    GenerateBlocks(11,100, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    ModifiableParams()->setTargetTimespan(oldTargetTimespan);
    ModifiableParams()->setTargetSpacing(oldTargetSpacing);
}

BOOST_AUTO_TEST_CASE(after_fork)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(0);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(1);
    ModifiableParams()->setEnableBigRewards(true);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setTargetSpacing(15);
    
    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used      : " << scriptPubKey.ToString() << endl;
    cout << "pub key used (hex): " << HexStr(scriptPubKey) << endl;

    // generate 1 pow blocks, so we can stake
    GenerateBlocks(1,2, pwalletMain, scriptPubKey,false);

    // we are just checking if we are able to generate PoS blocks after fork
    GenerateBlocks(2,100, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    ModifiableParams()->setTargetTimespan(oldTargetTimespan);
    ModifiableParams()->setTargetSpacing(oldTargetSpacing);
    
}
typedef struct {
    bool fProofOfStake;
    uint32_t nTime;
    uint32_t transactionTime;
    uint32_t nBits;
    unsigned int nonce;
    unsigned int extranonce;
    uint32_t nBirthdayA;
    uint32_t nBirthdayB;
    uint256 hash;
    uint256 hashMerkleRoot;
    CAmount balance;
} blockinfo_t;
static blockinfo_t blockinfo[] = 
{
{0, 1548446433, 1548446391 , 538968063 , 3 , 1 , 1686962 , 9902691 , uint256("06f086e12b5105ec9eb0efb6a50d5c27e6a6a8c2b2c8990a6aa61f863509f24e") , uint256("321da69fb932d441003dfc410150391e6376ab2c76a851172206700f6209c0c4") , 900000000000 }, // Block 1
{0, 1548446575, 1548446513 , 538968063 , 4 , 1 , 27275463 , 35044953 , uint256("0ce844679bfb9a54fa4b8e221368de0830956251c6a528e8a4580f358caf5f00") , uint256("d8bc7443ad1625630d9bec7869f6b0bcae0a3f2746bcce03f7e4f3340e48b9c6") , 1800000000000 }, // Block 2
{0, 1548446781, 1548446656 , 538968063 , 7 , 1 , 0 , 0 , uint256("1d1d931414c05e9d1adf32f1e23d758b66516500dca576a626f9afb4aec48289") , uint256("7493d4a4e7e5843a3fd1866c01b3d8e124b31eb201ad3c5c33f45851885c0b97") , 2700000000000 }, // Block 3
{0, 1548446862, 1548446862 , 538968063 , 1 , 1 , 0 , 0 , uint256("1b6fa7ec751fe8d2a5e51d21e8d3ed27c128d32160a2ca27165e6664363cef6a") , uint256("5a065809f4c246adc9d9d291a781369f3e39711b3cd2f8968027efdc8f98c755") , 3600000000000 }, // Block 4
{0, 1548447005, 1548446943 , 538968063 , 4 , 1 , 0 , 0 , uint256("0d25e2b765502ca3a3c4db820f47dbc7d88f5b3e15c57a511a447e27d809d05e") , uint256("7eab63c99d1bb3773496156b56fd7be42cf8d0df1842645a47409c1b21062cde") , 4500000000000 }, // Block 5
{1, 1548447055, 1548447055 , 520159231 , 0 , 0 , 0 , 0 , uint256("8a8937844f132c12db664842b2d3c31a64d26e87adf60d8fea03cd310b76d5b6") , uint256("a63ac6334cf4220af3e70693a9794559dcf924c11557663e5fb093f12a7f09b8") , 4500194400000 }, // Block 6
{1, 1548447694, 1548447694 , 520148308 , 0 , 0 , 0 , 0 , uint256("ea5677af4993aec20e170da08b0fa0a848fb213f8bf9b8f3aec34d4d2ee68d2a") , uint256("c4f2b67b08d1a7170b3587990ff822945adcaa40c87d82d4d32728e689f8e04b") , 4500388800000 }, // Block 7
{1, 1548447754, 1548447754 , 520159231 , 0 , 0 , 0 , 0 , uint256("31f60a3d52c4576750aab1dc717c1f4881ca69fc08efb92067fd706edd384eb7") , uint256("88747c489a870c5f4c2b8a301604ab754e7326e22e0b38145c4161a9a9f745ef") , 4500583200000 }, // Block 8
{1, 1548447814, 1548447814 , 520159231 , 0 , 0 , 0 , 0 , uint256("a8fbf7889dc06c2a25eed3cc92fb7e0d12b96c3e001868e3d7c9dfa4d2b0c9a2") , uint256("294ea2e4f63b6611ac13a2c05175dc1fcd9b97871c93203c2c68b1933f1b7a63") , 4500777600000 }, // Block 9
{1, 1548447874, 1548447874 , 520159231 , 0 , 0 , 0 , 0 , uint256("2f83bb69c2f012688f130f2a061914750ee9761dfcf75ad51e010caf14748dd4") , uint256("969bd203b89b641b83f4e4b91a9bab76120bc15641d90fa4562b5d6015308dfe") , 4500972000000 }, // Block 10
{0, 1548448022, 0 , 538968063 , 14 , 1 , 0 , 0 , uint256("1b6410ab625a11d08a9458657212f79cd6c87822712d8dd246082ff2ee6d791b") , uint256("486444324677b9f7d06b940a9602e3ad4f33d8850f4b8e461a7a9e3cd8a25361") , 5500972000000 }, // Block 11
{0, 1548448372, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("0a2973010cf67752b6bf035fd5519ee105011562a010465341a6108ab29e4a96") , uint256("abc79f95e4bffa35945f9d0d47dd07281d584deef4be3b43c9d075112aed8e4f") , 6500972000000 }, // Block 12
{0, 1548448801, 0 , 538968063 , 4 , 1 , 0 , 0 , uint256("04c03f0378ac2cc32536eaa30f40a9c5c68039f4e923737d2fa7fdca0f777a23") , uint256("a7257772333553ba04490171747871c7033cb3c38318e630730c5fdf4b2b0e87") , 7500972000000 }, // Block 13
{0, 1548448923, 0 , 538968063 , 17 , 1 , 0 , 0 , uint256("1ba465981dbd879ac14d677a50f3d6e0408cad71544e0e2a4cd457d3556d2ff0") , uint256("7b89c7afc49c2ff3874f3348611cd0987dd35bc4b18a76f000094758b23e36f2") , 8500972000000 }, // Block 14
{0, 1548448983, 0 , 538968063 , 2 , 1 , 0 , 0 , uint256("04a0732c26d1f81244893d198d6bf02572673c2e185e67f73461e0d9d960e56a") , uint256("217e2b5cda1281474eff8b9cdfc0c75185cd9e477b8f0ca7799423e8c1c76ebf") , 9500972000000 }, // Block 15
{1, 1548449085, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("33e75308b22e8d4fbd49bceabf28888d136e46600e01118ea1cd24af3ed57bec") , uint256("21b034995a51db7053766136e9d4de291fd32edece40c8c7bb11d6432496ae90") , 10400972000000 }, // Block 16
{1, 1548449158, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("3d235d0f3cd04900568ed9a25b7b26b67d493bdd0922e7aa6a9632b7c1c8d30f") , uint256("789ecf7ea219f774813aaad3fea11c0637dfb77ed77c563871eca26490c9ae10") , 11300972000000 }, // Block 17
{1, 1548449249, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("55f8af5215dc0393b5206863e4b6305d080d689324fe75b3ead667b44e82ef5e") , uint256("af1f23039cdb439569689eb68f16fd9227bb5608e4a9dd83a990aa4546032a09") , 12200972000000 }, // Block 18
{1, 1548449319, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("8c6cd703facc397ca8efc3cbd26e673a4ce5a48f81b93e64d8c82e658a4d8a6a") , uint256("03866c652c40e028fa9a08ac3e415e283d0ccb4f1a2b78a2897985f06d92eecb") , 13100972000000 }, // Block 19
{1, 1548449388, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("c8d0491cdb0e92b1fa066388cb79d190680fad2443742872e70031a586f11031") , uint256("9f6f00498e76f09421451f6678b052106772f4d34cec9e28106eeaf9ae7ebf44") , 14000972000000 }, // Block 20
};


void Create_Transaction(CBlock *pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    //txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

void Create_NewTransaction(CBlock *pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    //txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}


void CreateOldBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t & blockInfo, CWallet* pwallet, CScript & scriptPubKey, bool fProofOfStake)
{
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();


    std::vector<CTransaction*>txFirst;
    for (int i = startBlock-1; i < endBlock-1; i++)
    {
        // Simple block creation, nothing special yet:
        BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, fProofOfStake));
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
        //pblock->nTime = blockinfo[i].nTime;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        if (!fProofOfStake)
           Create_Transaction(pblock, pindexPrev, blockinfo, i);
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
        }

        BOOST_CHECK(state.IsValid());
        // we should get the same balance
        cout << "Block: " << i+1 << " time ("<< pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
        if (pblocktemplate)
         delete pblocktemplate;
    }
   
}


void createNewBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t & blockInfo, CWallet* pwallet, CScript & scriptPubKey, bool fProofOfStake)
{
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params();
    CReserveKey reservekey(pwallet); // only for consistency !!!


    CBlockIndex* pindexPrev = chainActive.Tip();

    std::vector<CTransaction*>txFirst;
    for (int i = startBlock-1; i < endBlock-1; i++)
    {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
        assert(pblocktemplate.get() != NULL);
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
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
        cout << "Block: " << i+1 << " time ("<< pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();        
    }
}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(basic_fork)
{
    SelectParams(CBaseChainParams::UNITTEST);
    ScanForWalletTransactions(pwalletMain);
    Checkpoints::fEnabled = false;
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0); 
    ModifiableParams()->setEnableBigRewards(true);

    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used: " << scriptPubKey.ToString() << endl;

    
    // Lets create 5 pow blocks than 5 pos than we fork

    // when running the generate testcase, dont forget to update this pubkey with
    // the correct pubkey.
    // in order to get the correct pubkey, get the pub and remove the first and last bytes
    //CScript scriptPubKey = CScript() << ParseHex("042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395") << OP_CHECKSIG;
    //cout << "PubKey : " << scriptPubKey.ToString() << endl;

    LOCK(cs_main);

    // lets create 5 old pow blocks from blockinfo
    CreateOldBlocksFromBlockInfo(1,6, blockinfo[0], pwalletMain, scriptPubKey, false);

    InitializeLastCoinStakeSearchTime(pwalletMain, scriptPubKey);
    CreateOldBlocksFromBlockInfo(6,11, blockinfo[0], pwalletMain, scriptPubKey, true);

    createNewBlocksFromBlockInfo(11,16, blockinfo[0], pwalletMain, scriptPubKey, false);
    createNewBlocksFromBlockInfo(16,21, blockinfo[0], pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
}


BOOST_AUTO_TEST_SUITE_END()
