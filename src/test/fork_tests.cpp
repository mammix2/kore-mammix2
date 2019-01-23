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


void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake)
{
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
    cout << pwalletMain->GetBalance() << " },";
    cout << " // " << "Block === " << blockNumber << " === " << endl;
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

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey, pwallet, fProofOfStake));
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
            ProcessBlockFound(pblock, *pwallet, reservekey);
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
                    ProcessBlockFound(pblock, *pwallet, reservekey);
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
/*
BOOST_AUTO_TEST_CASE(generate_chain)
{
    Checkpoints::fEnabled = false;
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0); 
    ModifiableParams()->setEnableBigRewards(true);
    
    ScanForWalletTransactions(pwalletMain);
    CReserveKey reservekey(pwalletMain);
    CPubKey pubkey;
    reservekey.GetReservedKey(pubkey);

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;

    // generate 5 pow blocks
    GeneratePOWLegacyBlocks(1,6, pwalletMain, scriptPubKey);
    // generate 5 pos blocks
    GeneratePOSLegacyBlocks(6,11, pwalletMain, scriptPubKey);
    //ScanForWalletTransactions(pwalletMain);

    // here the fork will happen, lets check pow
    GenerateBlocks(11,16, pwalletMain, false);

    GenerateBlocks(16,21, pwalletMain, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    
}

*/

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
   {0, 1548273712, 1548273587 , 538968063 , 7 , 1 , 0 , 0 , uint256("016e8c8f8f7488f6c9c760ebec7d6fe411f48ee29cd2b7392fe164e186111b02") , uint256("a1b1077e981aa0b70b661fcf0714057ed17058ebb461ce30b18ce7527a246978") , 900000000000 }, // Block === 1 ===
   {0, 1548273755, 1548273755 , 538968063 , 1 , 1 , 0 , 0 , uint256("0fab188e54284f9980cfba801eb4b36f3fc71b0b2654a46e3397096c7e48fb00") , uint256("22f929b9ef63d50a0458a8d2108bd2458834820eb3308d004e95cd32083ffce2") , 1800000000000 }, // Block === 2 ===
   {0, 1548273808, 1548273786 , 538968063 , 2 , 1 , 0 , 0 , uint256("0e3bac772a0c97d0f68ac1e85495d8fb0184bcfe01f74a90c47bb4833339edd7") , uint256("e3d26b8e5539e9a3a69ca05a8b8d563eaff2d32f32e0e5f28550097457170c98") , 2700000000000 }, // Block === 3 ===
   {0, 1548273889, 1548273829 , 538968063 , 4 , 1 , 10375524 , 44917300 , uint256("059364ee412784316e6f7a30c0cc7c08dec6828725267f1079d6b03d6198e9b1") , uint256("95e69ac0ce7d832dabe79f6f540204edc2b61bcd81b3bf954b29e88923f73bf5") , 3600000000000 }, // Block === 4 ===
   {0, 1548273910, 1548273910 , 538968063 , 1 , 1 , 33647248 , 37508789 , uint256("010369eda2912ee7274580691d2309b05bf1e5ac3618de65f8d113bfd3e40224") , uint256("056dbf8aac3ceac1eeb5f01dfe27540a9e0bc310a08b87fe1016b5dd1e98abfc") , 4500000000000 }, // Block === 5 ===
   {1, 1548273970, 1548273970 , 520159231 , 0 , 0 , 0 , 0 , uint256("559fdff28a47671e37570f17af717e4ba7ea26d940d556bdad200fa9f33d8fe9") , uint256("2c2e078698e5bd07a64e05de57e799c193b04f4ef1e4e69c538ae45218a740ce") , 4500194400000 }, // Block === 6 ===
   {1, 1548274042, 1548274042 , 520159231 , 0 , 0 , 0 , 0 , uint256("a3e8376e149131efdb342477804935859b3f2b0f4eb7ea07edce3d3fd1401072") , uint256("b5bec46432682de43b7adc9fab7c8a3243f8d8d017f1dcc2c898e63048e5a144") , 4500388800000 }, // Block === 7 ===
   {1, 1548274102, 1548274102 , 520159231 , 0 , 0 , 0 , 0 , uint256("d1ce36bbe63bba944e9db290b0b5602c4c05c381f23aa13ab2a561b222b244e5") , uint256("e1e8f40bad43e5860ef4214d0d422a04c3fab6fdd4dda7141f7df85c7b23e811") , 4500583200000 }, // Block === 8 ===
   {1, 1548274162, 1548274162 , 520159231 , 0 , 0 , 0 , 0 , uint256("adc22bc38c3bacb61af7d9059b0e6c2683ab0e4f0b6647e96f9057f96f6a3de8") , uint256("7573787871c7f00cca5e3d66bc6642c0bb8558d286a09749261606ed57a060fa") , 4500777600000 }, // Block === 9 ===
   {1, 1548274222, 1548274222 , 520159231 , 0 , 0 , 0 , 0 , uint256("9d8f4bc656517af35f1aeda9cfd15c755a35ac02f0583a47f886341559ec8881") , uint256("b537d5956e2e09fb39f81c1205375b0d3a337de76b2efe6bb0d1f43b36b71d34") , 4500972000000 }, // Block === 10 ===
   {0, 1548274320, 0 , 538968063 , 5 , 1 , 0 , 0 , uint256("088c3f55f76602963617acc4eb96fce9cc3472891f81033b4172b1ec02fbce0e") , uint256("b028a394647c934107d4bc4dabe363db8f08ffaa950877be5456908b3dc454ab") , 5500972000000 }, // Block === 11 ===
   {0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("0e1302a813836de83f148e502e5db8489f5681ded7d59d64d03e1016da27beae") , uint256("b8fb9a0f093c08b6441af567684f580eabb23c728a603767be226bbc15c45237") , 6500972000000 }, // Block === 12 ===
   {0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("06eade7da72b390307313f8f5ded0fe2e6aeb8d84096768c888cd43733222cce") , uint256("9766f04735723d8b3b07762bc78af8ac038eb67eeb77aaf0e384b6e462629998") , 7500972000000 }, // Block === 13 ===
   {0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("01cfdb06fe0c79ba094672d4c3d74a874a673ef20afaa71d57d588b1f2b7eb5f") , uint256("21eea3fdc6f11af0b7f4400e0d7416c4631324ad1a2e143e0120a18322cbfd30") , 8500972000000 }, // Block === 14 ===
   {0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("0cd56778479396ac8eadbdad7855eb40d2502682ebc7fcafcdba79bf31ad5315") , uint256("3947643fc4883aac5500f4464f76a7549c34218b4aefab2f2a74138974284315") , 9500972000000 }, // Block === 15 ===
   {1, 1548274416, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("884b890bb9e9b18fff035382de6fb5224d6a6281fd4c8a23ce1dc1d1d00a2773") , uint256("82476d6fb65fb8dbeb86315b151731a67b9f7202c12fe46e77e6b8344e5f77cf") , 10400972000000 }, // Block === 16 ===
   {1, 1548274426, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("fcade7e05e851d8dbd3c9570a5b7e34156366c88aec829ba3c09d1c93bf9b98f") , uint256("0b6c29eeaebdc09b251a39d9576f002255e9bbc1a3bd59c266822f6cdb064c28") , 11300972000000 }, // Block === 17 ===
   {1, 1548274434, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("84d4e09d67f174210011edb23ba671cbc942d4f841296faf7dc9b80ae141b8f3") , uint256("64d5e74f10cd583fcd8eda0bac03d77353aafc0a5ec04551aa5dc5e97a671130") , 12200972000000 }, // Block === 18 ===
   {1, 1548274446, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("546a3b1c2ed3c0cd3daf0fd239187cdf6db9b3987625d7b6dd8a74360628e645") , uint256("d2653b72c019bb8d2147280ba8e84aa5d111df3f48b7e199e10fcd79126ea2f2") , 13100972000000 }, // Block === 19 ===
   {1, 1548274456, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("60d2911c7a53eb7c5075a607ad5d2d79dcca9a88ac63dbdb6209fc296b7a0a8d") , uint256("3bb81b414e546c526eb3b8d68d208631ac474172e97c040137992cd34909d191") , 14000972000000 }, // Block === 20 ===
};

/*
CBlock ============================>>>>
    hash=016e8c8f8f7488f6c9c760ebec7d6fe411f48ee29cd2b7392fe164e186111b02
    ver=1
    hashPrevBlock=0a9ab95126cdf38d00973715c656e5f27e35ed17bc13e3ac061afda26e3a7cb9,
    hashMerkleRoot=a1b1077e981aa0b70b661fcf0714057ed17058ebb461ce30b18ce7527a246978,
    nTime=1548273712,
    nBits=201fffff,
    nNonce=7,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=a1b1077e98, ver=1,  nTime=1548273587, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 510101)
    CTxOut(nValue=9000.00000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    a1b1077e981aa0b70b661fcf0714057ed17058ebb461ce30b18ce7527a246978CBlock <<<<============================
{0, 1548273712, 1548273587 , 538968063 , 7 , 1 , 0 , 0 , uint256("016e8c8f8f7488f6c9c760ebec7d6fe411f48ee29cd2b7392fe164e186111b02") , uint256("a1b1077e981aa0b70b661fcf0714057ed17058ebb461ce30b18ce7527a246978") , 900000000000") }, // Block === 1 ===
CBlock ============================>>>>
    hash=0fab188e54284f9980cfba801eb4b36f3fc71b0b2654a46e3397096c7e48fb00
    ver=1
    hashPrevBlock=016e8c8f8f7488f6c9c760ebec7d6fe411f48ee29cd2b7392fe164e186111b02,
    hashMerkleRoot=22f929b9ef63d50a0458a8d2108bd2458834820eb3308d004e95cd32083ffce2,
    nTime=1548273755,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=22f929b9ef, ver=1,  nTime=1548273755, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 520101)
    CTxOut(nValue=9000.00000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    22f929b9ef63d50a0458a8d2108bd2458834820eb3308d004e95cd32083ffce2CBlock <<<<============================
{0, 1548273755, 1548273755 , 538968063 , 1 , 1 , 0 , 0 , uint256("0fab188e54284f9980cfba801eb4b36f3fc71b0b2654a46e3397096c7e48fb00") , uint256("22f929b9ef63d50a0458a8d2108bd2458834820eb3308d004e95cd32083ffce2") , 1800000000000") }, // Block === 2 ===
CBlock ============================>>>>
    hash=0e3bac772a0c97d0f68ac1e85495d8fb0184bcfe01f74a90c47bb4833339edd7
    ver=1
    hashPrevBlock=0fab188e54284f9980cfba801eb4b36f3fc71b0b2654a46e3397096c7e48fb00,
    hashMerkleRoot=e3d26b8e5539e9a3a69ca05a8b8d563eaff2d32f32e0e5f28550097457170c98,
    nTime=1548273808,
    nBits=201fffff,
    nNonce=2,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=e3d26b8e55, ver=1,  nTime=1548273786, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 530101)
    CTxOut(nValue=9000.00000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    e3d26b8e5539e9a3a69ca05a8b8d563eaff2d32f32e0e5f28550097457170c98CBlock <<<<============================
{0, 1548273808, 1548273786 , 538968063 , 2 , 1 , 0 , 0 , uint256("0e3bac772a0c97d0f68ac1e85495d8fb0184bcfe01f74a90c47bb4833339edd7") , uint256("e3d26b8e5539e9a3a69ca05a8b8d563eaff2d32f32e0e5f28550097457170c98") , 2700000000000") }, // Block === 3 ===
CBlock ============================>>>>
    hash=059364ee412784316e6f7a30c0cc7c08dec6828725267f1079d6b03d6198e9b1
    ver=1
    hashPrevBlock=0e3bac772a0c97d0f68ac1e85495d8fb0184bcfe01f74a90c47bb4833339edd7,
    hashMerkleRoot=95e69ac0ce7d832dabe79f6f540204edc2b61bcd81b3bf954b29e88923f73bf5,
    nTime=1548273889,
    nBits=201fffff,
    nNonce=4,
    nBirthdayA=10375524,
    nBirthdayB=44917300,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=95e69ac0ce, ver=1,  nTime=1548273829, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 540101)
    CTxOut(nValue=9000.00000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    95e69ac0ce7d832dabe79f6f540204edc2b61bcd81b3bf954b29e88923f73bf5CBlock <<<<============================
{0, 1548273889, 1548273829 , 538968063 , 4 , 1 , 10375524 , 44917300 , uint256("059364ee412784316e6f7a30c0cc7c08dec6828725267f1079d6b03d6198e9b1") , uint256("95e69ac0ce7d832dabe79f6f540204edc2b61bcd81b3bf954b29e88923f73bf5") , 3600000000000") }, // Block === 4 ===
CBlock ============================>>>>
    hash=010369eda2912ee7274580691d2309b05bf1e5ac3618de65f8d113bfd3e40224
    ver=1
    hashPrevBlock=059364ee412784316e6f7a30c0cc7c08dec6828725267f1079d6b03d6198e9b1,
    hashMerkleRoot=056dbf8aac3ceac1eeb5f01dfe27540a9e0bc310a08b87fe1016b5dd1e98abfc,
    nTime=1548273910,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=33647248,
    nBirthdayB=37508789,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=056dbf8aac, ver=1,  nTime=1548273910, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 550101)
    CTxOut(nValue=9000.00000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    056dbf8aac3ceac1eeb5f01dfe27540a9e0bc310a08b87fe1016b5dd1e98abfcCBlock <<<<============================
{0, 1548273910, 1548273910 , 538968063 , 1 , 1 , 33647248 , 37508789 , uint256("010369eda2912ee7274580691d2309b05bf1e5ac3618de65f8d113bfd3e40224") , uint256("056dbf8aac3ceac1eeb5f01dfe27540a9e0bc310a08b87fe1016b5dd1e98abfc") , 4500000000000") }, // Block === 5 ===
CBlock ============================>>>>
    hash=559fdff28a47671e37570f17af717e4ba7ea26d940d556bdad200fa9f33d8fe9
    ver=1
    hashPrevBlock=010369eda2912ee7274580691d2309b05bf1e5ac3618de65f8d113bfd3e40224,
    hashMerkleRoot=2c2e078698e5bd07a64e05de57e799c193b04f4ef1e4e69c538ae45218a740ce,
    nTime=1548273970,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100b990d0c302d486b4062c12f8746043c1f94a2aa69e916613fc7ee95f31efe64602202df7703ddb2522879a0de96ad59bdcb657d99717de5fcd5aa87e90254c019b15,
    Vtx : size 2
    CTransaction(hash=f2e40134cb, ver=1,  nTime=1548273970, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5600)
    CTxOut(empty)

    CTransaction(hash=54d3e2f375, ver=1,  nTime=1548273970, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(a1b1077e98, 0), scriptSig=47304402203aa8b2b8d2e2a7)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    f2e40134cbbcc81a8b506623490da24b4e3185fed4b1ecb749166aa6b0ce10d6    54d3e2f37584023db6d7eb437d727bec766a67309de39cc87b64256c53401748    2c2e078698e5bd07a64e05de57e799c193b04f4ef1e4e69c538ae45218a740ceCBlock <<<<============================
{1, 1548273970, 1548273970 , 520159231 , 0 , 0 , 0 , 0 , uint256("559fdff28a47671e37570f17af717e4ba7ea26d940d556bdad200fa9f33d8fe9") , uint256("2c2e078698e5bd07a64e05de57e799c193b04f4ef1e4e69c538ae45218a740ce") , 4500194400000") }, // Block === 6 ===
CBlock ============================>>>>
    hash=a3e8376e149131efdb342477804935859b3f2b0f4eb7ea07edce3d3fd1401072
    ver=1
    hashPrevBlock=559fdff28a47671e37570f17af717e4ba7ea26d940d556bdad200fa9f33d8fe9,
    hashMerkleRoot=b5bec46432682de43b7adc9fab7c8a3243f8d8d017f1dcc2c898e63048e5a144,
    nTime=1548274042,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100d1cfaf571c10138bcaa3afefd9ba958a4bee18401996eaede0e6ef308c171d1a02207d79082ffb4a4f7f7f9736b9ca2d2e35b43cea5ae5b8464adbd458acd30e45d6,
    Vtx : size 2
    CTransaction(hash=3dd66796cb, ver=1,  nTime=1548274042, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5700)
    CTxOut(empty)

    CTransaction(hash=0a9d59d560, ver=1,  nTime=1548274042, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(22f929b9ef, 0), scriptSig=4730440220796d3e56d8d50a)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    3dd66796cbe1d87e69a6dbd0aa530df79b94b338dad63ea92fba6a5034bd8203    0a9d59d5600606caa14306f4476c1335dbb7a57aa198a535d214e20ae1845c0d    b5bec46432682de43b7adc9fab7c8a3243f8d8d017f1dcc2c898e63048e5a144CBlock <<<<============================
{1, 1548274042, 1548274042 , 520159231 , 0 , 0 , 0 , 0 , uint256("a3e8376e149131efdb342477804935859b3f2b0f4eb7ea07edce3d3fd1401072") , uint256("b5bec46432682de43b7adc9fab7c8a3243f8d8d017f1dcc2c898e63048e5a144") , 4500388800000") }, // Block === 7 ===
CBlock ============================>>>>
    hash=d1ce36bbe63bba944e9db290b0b5602c4c05c381f23aa13ab2a561b222b244e5
    ver=1
    hashPrevBlock=a3e8376e149131efdb342477804935859b3f2b0f4eb7ea07edce3d3fd1401072,
    hashMerkleRoot=e1e8f40bad43e5860ef4214d0d422a04c3fab6fdd4dda7141f7df85c7b23e811,
    nTime=1548274102,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100a12ac8b3412dded3d6f9cefbdfb655f439a543ddcda3de8cb2fc3d2bef89ed020220628a989ba1719138f2ca7e7e10de4288780f03f5864457e82213e164f20dc11d,
    Vtx : size 2
    CTransaction(hash=b313f28d48, ver=1,  nTime=1548274102, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5800)
    CTxOut(empty)

    CTransaction(hash=73aa224aa9, ver=1,  nTime=1548274102, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(e3d26b8e55, 0), scriptSig=48304502210083561db48dbf)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    b313f28d48684304f4e69cd065b4e8f29f9eea816e2df856c8d532d33862f4f4    73aa224aa946dc47512bb296958a9923bee91cfbcbceec2c5ecc530eab30e928    e1e8f40bad43e5860ef4214d0d422a04c3fab6fdd4dda7141f7df85c7b23e811CBlock <<<<============================
{1, 1548274102, 1548274102 , 520159231 , 0 , 0 , 0 , 0 , uint256("d1ce36bbe63bba944e9db290b0b5602c4c05c381f23aa13ab2a561b222b244e5") , uint256("e1e8f40bad43e5860ef4214d0d422a04c3fab6fdd4dda7141f7df85c7b23e811") , 4500583200000") }, // Block === 8 ===
CBlock ============================>>>>
    hash=adc22bc38c3bacb61af7d9059b0e6c2683ab0e4f0b6647e96f9057f96f6a3de8
    ver=1
    hashPrevBlock=d1ce36bbe63bba944e9db290b0b5602c4c05c381f23aa13ab2a561b222b244e5,
    hashMerkleRoot=7573787871c7f00cca5e3d66bc6642c0bb8558d286a09749261606ed57a060fa,
    nTime=1548274162,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100baa9289fa6f4d9f8cba8c9b4d3b82be5d40f487280d082c2f4657581196cf711022040ba992544c2a7081d1fe9a4d371b3ad47d1d164d596fe2005ea5e7c08a2af04,
    Vtx : size 2
    CTransaction(hash=185c124534, ver=1,  nTime=1548274162, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5900)
    CTxOut(empty)

    CTransaction(hash=e2f69334cd, ver=1,  nTime=1548274162, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(95e69ac0ce, 0), scriptSig=483045022100e5872616813c)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    185c124534a1568647bf96bbae503e9c9cc5e629fd71bc715d09de30bc4c515b    e2f69334cd6a7ce449ae2d0fc87c80ee5037b9c1bd7de09e48cee25082a24bfd    7573787871c7f00cca5e3d66bc6642c0bb8558d286a09749261606ed57a060faCBlock <<<<============================
{1, 1548274162, 1548274162 , 520159231 , 0 , 0 , 0 , 0 , uint256("adc22bc38c3bacb61af7d9059b0e6c2683ab0e4f0b6647e96f9057f96f6a3de8") , uint256("7573787871c7f00cca5e3d66bc6642c0bb8558d286a09749261606ed57a060fa") , 4500777600000") }, // Block === 9 ===
CBlock ============================>>>>
    hash=9d8f4bc656517af35f1aeda9cfd15c755a35ac02f0583a47f886341559ec8881
    ver=1
    hashPrevBlock=adc22bc38c3bacb61af7d9059b0e6c2683ab0e4f0b6647e96f9057f96f6a3de8,
    hashMerkleRoot=b537d5956e2e09fb39f81c1205375b0d3a337de76b2efe6bb0d1f43b36b71d34,
    nTime=1548274222,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402200c0e90bc58820f0305afc4b84ec0a477fba90de8911ccf4c6f939256133401f20220486092ef12eb5d6b1a5e2ec24e9646d8dc598edcc6faf5043a16933b648bd186,
    Vtx : size 2
    CTransaction(hash=4208dde48e, ver=1,  nTime=1548274222, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5a00)
    CTxOut(empty)

    CTransaction(hash=2b3ddfbf37, ver=1,  nTime=1548274222, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(056dbf8aac, 0), scriptSig=4730440220658165e98a91f0)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    4208dde48ef2ec95bbd06aeab3a6bf4c91bf5c7623eee0f73bd419c9f8ff2c20    2b3ddfbf37938db0972c257b9088374d3f8ee771b89c6ec487341edc8008df5c    b537d5956e2e09fb39f81c1205375b0d3a337de76b2efe6bb0d1f43b36b71d34CBlock <<<<============================
{1, 1548274222, 1548274222 , 520159231 , 0 , 0 , 0 , 0 , uint256("9d8f4bc656517af35f1aeda9cfd15c755a35ac02f0583a47f886341559ec8881") , uint256("b537d5956e2e09fb39f81c1205375b0d3a337de76b2efe6bb0d1f43b36b71d34") , 4500972000000") }, // Block === 10 ===
CBlock ============================>>>>
    hash=088c3f55f76602963617acc4eb96fce9cc3472891f81033b4172b1ec02fbce0e
    ver=1
    hashPrevBlock=9d8f4bc656517af35f1aeda9cfd15c755a35ac02f0583a47f886341559ec8881,
    hashMerkleRoot=b028a394647c934107d4bc4dabe363db8f08ffaa950877be5456908b3dc454ab,
    nTime=1548274320,
    nBits=201fffff,
    nNonce=5,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=b028a39464, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5b0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=4104137b8f0653c4b8bbf2e02071605a47bfa3366b6363556ec64de8deed282c7514d96178e76c66e5766c6d2e81715e356f0c8ae962bf57c1eb6706295514b4a034ac)

  vMerkleTree:     vMerkleTree : size 1
    b028a394647c934107d4bc4dabe363db8f08ffaa950877be5456908b3dc454abCBlock <<<<============================
{0, 1548274320, 0 , 538968063 , 5 , 1 , 0 , 0 , uint256("088c3f55f76602963617acc4eb96fce9cc3472891f81033b4172b1ec02fbce0e") , uint256("b028a394647c934107d4bc4dabe363db8f08ffaa950877be5456908b3dc454ab") , 5500972000000") }, // Block === 11 ===
CBlock ============================>>>>
    hash=0e1302a813836de83f148e502e5db8489f5681ded7d59d64d03e1016da27beae
    ver=1
    hashPrevBlock=088c3f55f76602963617acc4eb96fce9cc3472891f81033b4172b1ec02fbce0e,
    hashMerkleRoot=b8fb9a0f093c08b6441af567684f580eabb23c728a603767be226bbc15c45237,
    nTime=1548274337,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=b8fb9a0f09, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5c0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=41045837d16bf8646de9d4908390ac6af596357317b463e943ea9652366c994a0dec8732a304e4c9369d5efd15c6345afedcb20c14a308ea3ea667d43ecd48336eb5ac)

  vMerkleTree:     vMerkleTree : size 1
    b8fb9a0f093c08b6441af567684f580eabb23c728a603767be226bbc15c45237CBlock <<<<============================
{0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("0e1302a813836de83f148e502e5db8489f5681ded7d59d64d03e1016da27beae") , uint256("b8fb9a0f093c08b6441af567684f580eabb23c728a603767be226bbc15c45237") , 6500972000000") }, // Block === 12 ===
CBlock ============================>>>>
    hash=06eade7da72b390307313f8f5ded0fe2e6aeb8d84096768c888cd43733222cce
    ver=1
    hashPrevBlock=0e1302a813836de83f148e502e5db8489f5681ded7d59d64d03e1016da27beae,
    hashMerkleRoot=9766f04735723d8b3b07762bc78af8ac038eb67eeb77aaf0e384b6e462629998,
    nTime=1548274337,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=9766f04735, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5d0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=4104fbc0eba12fd2f4162b6f4edada6d949c2cdd11a1c3eca2b507fed5412aefde2e6ec10435bdcddd22e706d9749700f6cd997d30502f4a70eb4301d3e32827d836ac)

  vMerkleTree:     vMerkleTree : size 1
    9766f04735723d8b3b07762bc78af8ac038eb67eeb77aaf0e384b6e462629998CBlock <<<<============================
{0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("06eade7da72b390307313f8f5ded0fe2e6aeb8d84096768c888cd43733222cce") , uint256("9766f04735723d8b3b07762bc78af8ac038eb67eeb77aaf0e384b6e462629998") , 7500972000000") }, // Block === 13 ===
CBlock ============================>>>>
    hash=01cfdb06fe0c79ba094672d4c3d74a874a673ef20afaa71d57d588b1f2b7eb5f
    ver=1
    hashPrevBlock=06eade7da72b390307313f8f5ded0fe2e6aeb8d84096768c888cd43733222cce,
    hashMerkleRoot=21eea3fdc6f11af0b7f4400e0d7416c4631324ad1a2e143e0120a18322cbfd30,
    nTime=1548274337,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=21eea3fdc6, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5e0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=410435c97de5124f82293f9297233b21aa31ceb52191538e1282fecd786b9f5dedfad946218d2007c02b4df29f1addd1a887d83c4e4316ffe08d41e29d6aba974df4ac)

  vMerkleTree:     vMerkleTree : size 1
    21eea3fdc6f11af0b7f4400e0d7416c4631324ad1a2e143e0120a18322cbfd30CBlock <<<<============================
{0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("01cfdb06fe0c79ba094672d4c3d74a874a673ef20afaa71d57d588b1f2b7eb5f") , uint256("21eea3fdc6f11af0b7f4400e0d7416c4631324ad1a2e143e0120a18322cbfd30") , 8500972000000") }, // Block === 14 ===
CBlock ============================>>>>
    hash=0cd56778479396ac8eadbdad7855eb40d2502682ebc7fcafcdba79bf31ad5315
    ver=1
    hashPrevBlock=01cfdb06fe0c79ba094672d4c3d74a874a673ef20afaa71d57d588b1f2b7eb5f,
    hashMerkleRoot=3947643fc4883aac5500f4464f76a7549c34218b4aefab2f2a74138974284315,
    nTime=1548274337,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=3947643fc4, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5f0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=41049275c6eb5630456e65165e3d00ceb1ec5b91bacc19a342731f1db5fecb6e071b3f36ae00beb607ec9c10c6b15742b9d1b344e9d7bbd43348908e7554c50e6aaeac)

  vMerkleTree:     vMerkleTree : size 1
    3947643fc4883aac5500f4464f76a7549c34218b4aefab2f2a74138974284315CBlock <<<<============================
{0, 1548274337, 0 , 538968063 , 1 , 1 , 0 , 0 , uint256("0cd56778479396ac8eadbdad7855eb40d2502682ebc7fcafcdba79bf31ad5315") , uint256("3947643fc4883aac5500f4464f76a7549c34218b4aefab2f2a74138974284315") , 9500972000000") }, // Block === 15 ===
CBlock ============================>>>>
    hash=884b890bb9e9b18fff035382de6fb5224d6a6281fd4c8a23ce1dc1d1d00a2773
    ver=1
    hashPrevBlock=0cd56778479396ac8eadbdad7855eb40d2502682ebc7fcafcdba79bf31ad5315,
    hashMerkleRoot=82476d6fb65fb8dbeb86315b151731a67b9f7202c12fe46e77e6b8344e5f77cf,
    nTime=1548274416,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402205832e4473d194303b1eb4a6807fbd274fa62808b98b15377cd21478fd151ab3502205674331b086ffc05980f443f07a2243e66be1d5d7a855fe5f5e4f25327bc2ddf,
    Vtx : size 2
    CTransaction(hash=4e25d9e71b, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 600101)
    CTxOut(empty)

    CTransaction(hash=ac086b6409, ver=1,  nTime=1548274416, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(0a9d59d560, 1), scriptSig=4830450221008347a959a1a3)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    4e25d9e71b6b8171d06475e6f8f0785e4973d0677912eab7cc07fadd2d94ab13    ac086b6409b6342209a3ca6e4255e4dc792ff6017d64da1777304901ac9cb1aa    82476d6fb65fb8dbeb86315b151731a67b9f7202c12fe46e77e6b8344e5f77cfCBlock <<<<============================
{1, 1548274416, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("884b890bb9e9b18fff035382de6fb5224d6a6281fd4c8a23ce1dc1d1d00a2773") , uint256("82476d6fb65fb8dbeb86315b151731a67b9f7202c12fe46e77e6b8344e5f77cf") , 10400972000000") }, // Block === 16 ===
CBlock ============================>>>>
    hash=fcade7e05e851d8dbd3c9570a5b7e34156366c88aec829ba3c09d1c93bf9b98f
    ver=1
    hashPrevBlock=884b890bb9e9b18fff035382de6fb5224d6a6281fd4c8a23ce1dc1d1d00a2773,
    hashMerkleRoot=0b6c29eeaebdc09b251a39d9576f002255e9bbc1a3bd59c266822f6cdb064c28,
    nTime=1548274426,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402206529b97dfdfd0787c66e9ed8a6e7cd286da87f7fc1078655c4ffc679806bf31a022002b1caebbc9544ec19ffa927b2df94bda03057bf092d171172270c5657b9a710,
    Vtx : size 2
    CTransaction(hash=67d63851ea, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01110101)
    CTxOut(empty)

    CTransaction(hash=979c371fcd, ver=1,  nTime=1548274426, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(0a9d59d560, 2), scriptSig=483045022100eceacd30029a)
    CTxOut(empty)
    CTxOut(nValue=6750.48700000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=6750.48700000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    67d63851ea56d591d1d818fb924877a55b3e4731cb8628f2b6dea1835c499762    979c371fcda762422d9bc4c194f810abca9c58393be531b5552823c9b8a8d9d3    0b6c29eeaebdc09b251a39d9576f002255e9bbc1a3bd59c266822f6cdb064c28CBlock <<<<============================
{1, 1548274426, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("fcade7e05e851d8dbd3c9570a5b7e34156366c88aec829ba3c09d1c93bf9b98f") , uint256("0b6c29eeaebdc09b251a39d9576f002255e9bbc1a3bd59c266822f6cdb064c28") , 11300972000000") }, // Block === 17 ===
CBlock ============================>>>>
    hash=84d4e09d67f174210011edb23ba671cbc942d4f841296faf7dc9b80ae141b8f3
    ver=1
    hashPrevBlock=fcade7e05e851d8dbd3c9570a5b7e34156366c88aec829ba3c09d1c93bf9b98f,
    hashMerkleRoot=64d5e74f10cd583fcd8eda0bac03d77353aafc0a5ec04551aa5dc5e97a671130,
    nTime=1548274434,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402200abd46d48f70e0e10b6deff96b3c151e05e1166f62ef1a5b559bdcac1a46223a02206575f0dce30b1e2e6842ab7eee6555ef11289b37fd4e77c133e565133390c42f,
    Vtx : size 2
    CTransaction(hash=dba97e03d1, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01120101)
    CTxOut(empty)

    CTransaction(hash=49a181cc1e, ver=1,  nTime=1548274434, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(2b3ddfbf37, 1), scriptSig=483045022100ebb37ab6a5dd)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    dba97e03d1e31fcfbb2b72ed39d07e6dae86ca0fe15ee6d4f9fe3e1b052dae8e    49a181cc1e34631cf764e3b26968c7bb699822d179e4cd7ac681e1d3475fafa1    64d5e74f10cd583fcd8eda0bac03d77353aafc0a5ec04551aa5dc5e97a671130CBlock <<<<============================
{1, 1548274434, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("84d4e09d67f174210011edb23ba671cbc942d4f841296faf7dc9b80ae141b8f3") , uint256("64d5e74f10cd583fcd8eda0bac03d77353aafc0a5ec04551aa5dc5e97a671130") , 12200972000000") }, // Block === 18 ===
CBlock ============================>>>>
    hash=546a3b1c2ed3c0cd3daf0fd239187cdf6db9b3987625d7b6dd8a74360628e645
    ver=1
    hashPrevBlock=84d4e09d67f174210011edb23ba671cbc942d4f841296faf7dc9b80ae141b8f3,
    hashMerkleRoot=d2653b72c019bb8d2147280ba8e84aa5d111df3f48b7e199e10fcd79126ea2f2,
    nTime=1548274446,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402206b1a100d515f244b1db65eb0b9d8634020b934d9cd0c301a5f96885c2cca05a1022062ed587aeefbd6aa4c5f85a4dc1423c1205de6ec0daca5f3894823e2902bcef0,
    Vtx : size 2
    CTransaction(hash=834e034a96, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01130101)
    CTxOut(empty)

    CTransaction(hash=54a4f60db4, ver=1,  nTime=1548274446, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(2b3ddfbf37, 2), scriptSig=483045022100fc77d13104e1)
    CTxOut(empty)
    CTxOut(nValue=6750.48700000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=6750.48700000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    834e034a965b061ee819fa86191f4d20e4b283f6758e9cc87b2ebc91bbbb8dbe    54a4f60db4cec5cd73f488e6f56b7a3fe66eca5c653cf2343bb4f60597138db6    d2653b72c019bb8d2147280ba8e84aa5d111df3f48b7e199e10fcd79126ea2f2CBlock <<<<============================
{1, 1548274446, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("546a3b1c2ed3c0cd3daf0fd239187cdf6db9b3987625d7b6dd8a74360628e645") , uint256("d2653b72c019bb8d2147280ba8e84aa5d111df3f48b7e199e10fcd79126ea2f2") , 13100972000000") }, // Block === 19 ===
CBlock ============================>>>>
    hash=60d2911c7a53eb7c5075a607ad5d2d79dcca9a88ac63dbdb6209fc296b7a0a8d
    ver=1
    hashPrevBlock=546a3b1c2ed3c0cd3daf0fd239187cdf6db9b3987625d7b6dd8a74360628e645,
    hashMerkleRoot=3bb81b414e546c526eb3b8d68d208631ac474172e97c040137992cd34909d191,
    nTime=1548274456,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100827c67d01e109064be336e44465930760b28bf958b290179f0ccbc5ef1caa57e02204cf9ab4b86abcbeb4ccdd9ccb8e315e8838cbe2a5f86fcb0dc69eeeac5f42163,
    Vtx : size 2
    CTransaction(hash=92ba0f4f06, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01140101)
    CTxOut(empty)

    CTransaction(hash=6dd355c57d, ver=1,  nTime=1548274456, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(54d3e2f375, 1), scriptSig=4730440220516dbfeb4cd310)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41042127dbde8ca144d3ade8ed19baec87a20b6f64c1f1dfda9ff48373c447e924931e0322eb92f0293bc2540b8e869a2b0587ab3e5de14346dc330abb27641b7395ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    92ba0f4f0669af156bb4ae4c2522540613f590032f353da1b17fc71533116119    6dd355c57d505f54e5390ffb4611b1fb11040a948fdd2062b88b6a375f444476    3bb81b414e546c526eb3b8d68d208631ac474172e97c040137992cd34909d191CBlock <<<<============================
{1, 1548274456, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("60d2911c7a53eb7c5075a607ad5d2d79dcca9a88ac63dbdb6209fc296b7a0a8d") , uint256("3bb81b414e546c526eb3b8d68d208631ac474172e97c040137992cd34909d191") , 14000972000000") }, // Block === 20 ===

*/
void Create_Transaction(CBlock *pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
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


void CreateOldPoWFromBlockInfo(blockinfo_t blockInfo, CWallet* pwallet, CScript & scriptPubKey)
{
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, false));

    // lets create 5 pow blocks
    std::vector<CTransaction*>txFirst;
    for (unsigned int i = 0; i < 5; ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
        pblock->nTime = blockinfo[i].nTime;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        Create_Transaction(pblock, pindexPrev, blockinfo, i);
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
}

void createOldPoSFromBlockInfo()
{

}

void CreateNewPoWFromBlockInfo()
{

}

void createNewPoSFromBlockInfo()
{

}

// NOTE: These tests rely on CreateNewBlock doing its own self-validation!
BOOST_AUTO_TEST_CASE(basic_fork)
{
    SelectParams(CBaseChainParams::UNITTEST);
    Checkpoints::fEnabled = false;
    int oldHeightToFork = Params().HeigthToFork();
    ModifiableParams()->setHeightToFork(11);
    // we dont need any blocks to confirm.
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    ModifiableParams()->setStakeMinConfirmations(0); 
    ModifiableParams()->setEnableBigRewards(true);
    
    // Lets create 5 pow blocks than 5 pos than we fork

    CScript scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

    LOCK(cs_main);

    CreateOldPoWFromBlockInfo(blockinfo, pwalletMain, scriptPubKey);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
}


BOOST_AUTO_TEST_SUITE_END()

