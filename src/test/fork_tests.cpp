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

 
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
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
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(pwalletMain);
    cout << "pub key used      : " << scriptPubKey.ToString() << endl;
    cout << "pub key used (hex): " << HexStr(scriptPubKey) << endl;
/* Doing like this we will not have the balance
    CReserveKey reservekey(pwalletMain);
    CPubKey pubkey;
    reservekey.GetReservedKey(pubkey);
    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
    cout << "Importante !!! use the same pubkey when creating from blockinfo" << endl;
    cout << "pub key used: " << scriptPubKey.ToString() << endl;
    */

    // generate 5 pow blocks
    //GeneratePOWLegacyBlocks(1,6, pwalletMain, scriptPubKey);
    GeneratePOWLegacyBlocks(1,2, pwalletMain, scriptPubKey);
    // generate 5 pos blocks
    //GeneratePOSLegacyBlocks(6,11, pwalletMain, scriptPubKey);
    GeneratePOSLegacyBlocks(2,3, pwalletMain, scriptPubKey);
    //ScanForWalletTransactions(pwalletMain);

    // here the fork will happen, lets check pow
    //GenerateBlocks(11,16, pwalletMain, scriptPubKey, false);

    //GenerateBlocks(16,21, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    
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
{0, 1548363373, 1548363257 , 538968063 , 7 , 1 , 0 , 0 , uint256("135cef224e04488b3b14d770cfa00af6b35fbb7bfaf667eeedd6608290e221f2") , uint256("1765ecf1026e437be5d8eb63a057def5ef7f6dfe16a8900d2ed2e3985828727c") , 900000000000 }, // Block 1
{0, 1548363450, 1548363392 , 538968063 , 4 , 1 , 0 , 0 , uint256("02ed7443886c0fb8f1140221dd3d2ad7a26e21164fa95ca2180b755d1e0a4045") , uint256("5327a94d58a421244f6f88f0dd942e96b59b5bf4047f7292cc884ed13ab8c98f") , 1800000000000 }, // Block 2
{0, 1548363508, 1548363469 , 538968063 , 3 , 1 , 0 , 0 , uint256("0f6ffa109f5a38ad98690bacfdcf92c86fdbdf38a1a4eb508f73195a650efdcf") , uint256("c1175d828e840edfe229166b67093698eea9d4b6f7687ea922597ce5ade2ad02") , 2700000000000 }, // Block 3
{0, 1548363565, 1548363527 , 538968063 , 3 , 1 , 0 , 0 , uint256("193133dd8ec02382b3c961469365f7450813c1611dd10a4addeedbc8ebf51b11") , uint256("1d0884e243ca1f99074fda53775edf72b5e19b499895d87b59e5028d0effd917") , 3600000000000 }, // Block 4
{0, 1548363648, 1548363589 , 538968063 , 4 , 1 , 0 , 0 , uint256("0a7ae7de02e0c0ada6bdb8431ac666e500f118869baf1c170d064f5d078d0518") , uint256("7294029ddc84e5f6be93fe3477d1abdc2a4ca467799633b4c625c70e831253a2") , 4500000000000 }, // Block 5
{1, 1548363698, 1548363698 , 520159231 , 0 , 0 , 0 , 0 , uint256("465e0fc6f85e12075c438e959ce4a81ff242968bfd0374c77363b2b6811949a7") , uint256("18d2715c4b840f0aacd2649036c8f185ef9b57302e62ecef4d608a622c5470f2") , 4500194400000 }, // Block 6
{1, 1548363758, 1548363758 , 520148308 , 0 , 0 , 0 , 0 , uint256("72630e42205fad5b739eefb0798ff872acf945289879747dc652e73f42034894") , uint256("abe8330ab7e30d9f4463d3ffeaa55046a0b5f3e16909139e880fb2bbfb57be7a") , 4500388800000 }, // Block 7
{1, 1548363818, 1548363818 , 520148308 , 0 , 0 , 0 , 0 , uint256("68e84c56c44f1bedfbd90f691946932fa8f546f0e1f85d74d8703bca7e24538c") , uint256("3e36cfd6e1aad046fef8357bf3a50c50c0e616f776864aaf27347262d720765b") , 4500583200000 }, // Block 8
{1, 1548363878, 1548363878 , 520148308 , 0 , 0 , 0 , 0 , uint256("788c67b82a6d2219c3e3f6b2782b638fbce63e3071a88be0b365a145f4ed1f01") , uint256("59284e0503d53a555eacac1394c0b0bb7b9b8b9aa1fe00f3d2acfba0b212572f") , 4500777600000 }, // Block 9
{1, 1548363938, 1548363938 , 520148308 , 0 , 0 , 0 , 0 , uint256("69946e7efe06a32bc3d0e15db5bb69dac482ad3f6912308596518e6c0a9360c5") , uint256("81a1547664ba19eff71327f58765e2ec6e1b6bace64cc6fb4ccc9b05e333c7dd") , 4500972000000 }, // Block 10
{0, 1548363998, 0 , 538968063 , 4 , 1 , 0 , 0 , uint256("1700dfd0131b1ab0d6d588375cfd3a27e54f84bc91a61ba56d0a19d3d2d8c3f3") , uint256("486444324677b9f7d06b940a9602e3ad4f33d8850f4b8e461a7a9e3cd8a25361") , 5500972000000 }, // Block 11
{0, 1548363998, 0 , 538968063 , 12 , 1 , 0 , 0 , uint256("1d00511875433c88b96cc5288d1f11adfac80e374566a4785451b51fae8e5b74") , uint256("abc79f95e4bffa35945f9d0d47dd07281d584deef4be3b43c9d075112aed8e4f") , 6500972000000 }, // Block 12
{0, 1548363998, 0 , 538968063 , 27 , 1 , 0 , 0 , uint256("07f7b5e76c84625acd2f1c6fbf64f86dcd21c0cdc2e8ac4050605349932da41e") , uint256("a7257772333553ba04490171747871c7033cb3c38318e630730c5fdf4b2b0e87") , 7500972000000 }, // Block 13
{0, 1548363998, 0 , 538968063 , 5 , 1 , 0 , 0 , uint256("1525f88c832b66af47ecc6d5b140e624dc902de1ca0cdfab2f2f8c57e417bd79") , uint256("7b89c7afc49c2ff3874f3348611cd0987dd35bc4b18a76f000094758b23e36f2") , 8500972000000 }, // Block 14
{0, 1548363998, 0 , 538968063 , 6 , 1 , 0 , 0 , uint256("1d7d3c9834f0db455e16b55801bb94b3e3260da496ac6826e6241db84a1a94ae") , uint256("217e2b5cda1281474eff8b9cdfc0c75185cd9e477b8f0ca7799423e8c1c76ebf") , 9500972000000 }, // Block 15
{1, 1548364053, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("b506aaa2b0456e1f7b41574444d4aa3ca9af3394a1076cb4462c28e97679c14b") , uint256("59e29d95211ebb9093f64274f3719a0b6c63f35031a695636333faa6c5f7a08b") , 10400972000000 }, // Block 16
{1, 1548364063, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("4f67563095be63d2268b32dd2ed0691982b6afbc9f38921aac5011d39974c8ba") , uint256("5707b11bf1382ad83876dc9f2d86889e799cab47e7f88c82501236380ab69141") , 11300972000000 }, // Block 17
{1, 1548364073, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("1db9ba42d9b9d14839f9369da384834ddad13097bf3d8c5f6d360d057712c858") , uint256("56adb227fa5b1fd57c3d76b2e33cb5d813d8cef6fda16910adba56dcc7229251") , 12200972000000 }, // Block 18
{1, 1548364083, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("3eb9dc986d45ebd14c2f3abcab1c05f4425aed75de80d0bbe0242278d9f0686f") , uint256("4d15e698e2978551a4ab67c7b0260da040f3fbf738cf3fc79f7942bf035a6959") , 13100972000000 }, // Block 19
{1, 1548364093, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("0b6fda9d1523b73ff85c0cf17eb22f1a0fa54814b746b21d4528143646091f3d") , uint256("6ac7a606c10f536d2493316ca47909a0785999b3b6de5619e4234ff4f2c0fb3f") , 14000972000000 }, // Block 20
};

/*
pub key used      : OP_DUP OP_HASH160 ff197b14e502ab41f3bc8ccb48c4abac9eab35bc OP_EQUALVERIFY OP_CHECKSIG
pub key used (hex): 76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac
CBlock ============================>>>>
    hash=135cef224e04488b3b14d770cfa00af6b35fbb7bfaf667eeedd6608290e221f2
    ver=1
    hashPrevBlock=0a9ab95126cdf38d00973715c656e5f27e35ed17bc13e3ac061afda26e3a7cb9,
    hashMerkleRoot=1765ecf1026e437be5d8eb63a057def5ef7f6dfe16a8900d2ed2e3985828727c,
    nTime=1548363373,
    nBits=201fffff,
    nNonce=7,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=1765ecf102, ver=1,  nTime=1548363257, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 510101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    1765ecf1026e437be5d8eb63a057def5ef7f6dfe16a8900d2ed2e3985828727cCBlock <<<<============================
{0, 1548363373, 1548363257 , 538968063 , 7 , 1 , 0 , 0 , uint256("135cef224e04488b3b14d770cfa00af6b35fbb7bfaf667eeedd6608290e221f2") , uint256("1765ecf1026e437be5d8eb63a057def5ef7f6dfe16a8900d2ed2e3985828727c") , 900000000000 }, // Block 1
CBlock ============================>>>>
    hash=02ed7443886c0fb8f1140221dd3d2ad7a26e21164fa95ca2180b755d1e0a4045
    ver=1
    hashPrevBlock=135cef224e04488b3b14d770cfa00af6b35fbb7bfaf667eeedd6608290e221f2,
    hashMerkleRoot=5327a94d58a421244f6f88f0dd942e96b59b5bf4047f7292cc884ed13ab8c98f,
    nTime=1548363450,
    nBits=201fffff,
    nNonce=4,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=5327a94d58, ver=1,  nTime=1548363392, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 520101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    5327a94d58a421244f6f88f0dd942e96b59b5bf4047f7292cc884ed13ab8c98fCBlock <<<<============================
{0, 1548363450, 1548363392 , 538968063 , 4 , 1 , 0 , 0 , uint256("02ed7443886c0fb8f1140221dd3d2ad7a26e21164fa95ca2180b755d1e0a4045") , uint256("5327a94d58a421244f6f88f0dd942e96b59b5bf4047f7292cc884ed13ab8c98f") , 1800000000000 }, // Block 2
CBlock ============================>>>>
    hash=0f6ffa109f5a38ad98690bacfdcf92c86fdbdf38a1a4eb508f73195a650efdcf
    ver=1
    hashPrevBlock=02ed7443886c0fb8f1140221dd3d2ad7a26e21164fa95ca2180b755d1e0a4045,
    hashMerkleRoot=c1175d828e840edfe229166b67093698eea9d4b6f7687ea922597ce5ade2ad02,
    nTime=1548363508,
    nBits=201fffff,
    nNonce=3,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=c1175d828e, ver=1,  nTime=1548363469, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 530101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    c1175d828e840edfe229166b67093698eea9d4b6f7687ea922597ce5ade2ad02CBlock <<<<============================
{0, 1548363508, 1548363469 , 538968063 , 3 , 1 , 0 , 0 , uint256("0f6ffa109f5a38ad98690bacfdcf92c86fdbdf38a1a4eb508f73195a650efdcf") , uint256("c1175d828e840edfe229166b67093698eea9d4b6f7687ea922597ce5ade2ad02") , 2700000000000 }, // Block 3
CBlock ============================>>>>
    hash=193133dd8ec02382b3c961469365f7450813c1611dd10a4addeedbc8ebf51b11
    ver=1
    hashPrevBlock=0f6ffa109f5a38ad98690bacfdcf92c86fdbdf38a1a4eb508f73195a650efdcf,
    hashMerkleRoot=1d0884e243ca1f99074fda53775edf72b5e19b499895d87b59e5028d0effd917,
    nTime=1548363565,
    nBits=201fffff,
    nNonce=3,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=1d0884e243, ver=1,  nTime=1548363527, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 540101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    1d0884e243ca1f99074fda53775edf72b5e19b499895d87b59e5028d0effd917CBlock <<<<============================
{0, 1548363565, 1548363527 , 538968063 , 3 , 1 , 0 , 0 , uint256("193133dd8ec02382b3c961469365f7450813c1611dd10a4addeedbc8ebf51b11") , uint256("1d0884e243ca1f99074fda53775edf72b5e19b499895d87b59e5028d0effd917") , 3600000000000 }, // Block 4
CBlock ============================>>>>
    hash=0a7ae7de02e0c0ada6bdb8431ac666e500f118869baf1c170d064f5d078d0518
    ver=1
    hashPrevBlock=193133dd8ec02382b3c961469365f7450813c1611dd10a4addeedbc8ebf51b11,
    hashMerkleRoot=7294029ddc84e5f6be93fe3477d1abdc2a4ca467799633b4c625c70e831253a2,
    nTime=1548363648,
    nBits=201fffff,
    nNonce=4,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=7294029ddc, ver=1,  nTime=1548363589, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 550101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1
    7294029ddc84e5f6be93fe3477d1abdc2a4ca467799633b4c625c70e831253a2CBlock <<<<============================
{0, 1548363648, 1548363589 , 538968063 , 4 , 1 , 0 , 0 , uint256("0a7ae7de02e0c0ada6bdb8431ac666e500f118869baf1c170d064f5d078d0518") , uint256("7294029ddc84e5f6be93fe3477d1abdc2a4ca467799633b4c625c70e831253a2") , 4500000000000 }, // Block 5
CBlock ============================>>>>
    hash=465e0fc6f85e12075c438e959ce4a81ff242968bfd0374c77363b2b6811949a7
    ver=1
    hashPrevBlock=0a7ae7de02e0c0ada6bdb8431ac666e500f118869baf1c170d064f5d078d0518,
    hashMerkleRoot=18d2715c4b840f0aacd2649036c8f185ef9b57302e62ecef4d608a622c5470f2,
    nTime=1548363698,
    nBits=1f00ffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100c6d5d1ccda47715bd71ff57e4b866fc67231c1ba2b97e16bbe3b127957006409022050935c8f694a2db20f70c63b601829520aa73974d3d558ab59f598dc3e7e2da6,
    Vtx : size 2
    CTransaction(hash=4da3ec6e0a, ver=1,  nTime=1548363698, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5600)
    CTxOut(empty)

    CTransaction(hash=a539ea0b19, ver=1,  nTime=1548363698, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(1765ecf102, 0), scriptSig=483045022100ad9d0cdf9723)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    4da3ec6e0a1a58fd01a75f3a96922e0ad67d400ff95f3837f2f9d2820d7e7aa0    a539ea0b19297704ce570fb9eedadaf4b5195a94381dc8aa4b531b9317655114    18d2715c4b840f0aacd2649036c8f185ef9b57302e62ecef4d608a622c5470f2CBlock <<<<============================
{1, 1548363698, 1548363698 , 520159231 , 0 , 0 , 0 , 0 , uint256("465e0fc6f85e12075c438e959ce4a81ff242968bfd0374c77363b2b6811949a7") , uint256("18d2715c4b840f0aacd2649036c8f185ef9b57302e62ecef4d608a622c5470f2") , 4500194400000 }, // Block 6
CBlock ============================>>>>
    hash=72630e42205fad5b739eefb0798ff872acf945289879747dc652e73f42034894
    ver=1
    hashPrevBlock=465e0fc6f85e12075c438e959ce4a81ff242968bfd0374c77363b2b6811949a7,
    hashMerkleRoot=abe8330ab7e30d9f4463d3ffeaa55046a0b5f3e16909139e880fb2bbfb57be7a,
    nTime=1548363758,
    nBits=1f00d554,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100d1e7c3a28b98561ae9f1aa536c449e6c47e85c85ed0621ad5ca00946c9dd68ec0220215594a38f5847a4261b8ebc176909e0cd3d1796ff58066deb38bbd3b52aff5a,
    Vtx : size 2
    CTransaction(hash=55552b7af5, ver=1,  nTime=1548363758, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5700)
    CTxOut(empty)

    CTransaction(hash=a7b2be03c3, ver=1,  nTime=1548363758, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(5327a94d58, 0), scriptSig=47304402207c458bcc2134e0)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    55552b7af5a03e1903a5723c4142e4fd8b865474982e84d3ae7d73f9888abdcf    a7b2be03c374ab969bfcbf8dd90b4213584ce144d07657586018248eaab3cde0    abe8330ab7e30d9f4463d3ffeaa55046a0b5f3e16909139e880fb2bbfb57be7aCBlock <<<<============================
{1, 1548363758, 1548363758 , 520148308 , 0 , 0 , 0 , 0 , uint256("72630e42205fad5b739eefb0798ff872acf945289879747dc652e73f42034894") , uint256("abe8330ab7e30d9f4463d3ffeaa55046a0b5f3e16909139e880fb2bbfb57be7a") , 4500388800000 }, // Block 7
CBlock ============================>>>>
    hash=68e84c56c44f1bedfbd90f691946932fa8f546f0e1f85d74d8703bca7e24538c
    ver=1
    hashPrevBlock=72630e42205fad5b739eefb0798ff872acf945289879747dc652e73f42034894,
    hashMerkleRoot=3e36cfd6e1aad046fef8357bf3a50c50c0e616f776864aaf27347262d720765b,
    nTime=1548363818,
    nBits=1f00d554,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304402204b29685572099b3d7f80e79d4b0945bcaca8d5ebe2a7bc71171b5b005d210eae022070e92b25612aed85067d9630f2e488a086fd555f3c90a57a2e2b7adf1ba17569,
    Vtx : size 2
    CTransaction(hash=de23fa5231, ver=1,  nTime=1548363818, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5800)
    CTxOut(empty)

    CTransaction(hash=0418c92bb8, ver=1,  nTime=1548363818, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(c1175d828e, 0), scriptSig=4730440220497b3d5bb373c2)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    de23fa523153dabfe4ced0606fb29468452ae63f85c7d6181875a658743a1cba    0418c92bb83f4c74dc3845744e4971dd201332b9b19add65e86f41a911aee0b8    3e36cfd6e1aad046fef8357bf3a50c50c0e616f776864aaf27347262d720765bCBlock <<<<============================
{1, 1548363818, 1548363818 , 520148308 , 0 , 0 , 0 , 0 , uint256("68e84c56c44f1bedfbd90f691946932fa8f546f0e1f85d74d8703bca7e24538c") , uint256("3e36cfd6e1aad046fef8357bf3a50c50c0e616f776864aaf27347262d720765b") , 4500583200000 }, // Block 8
CBlock ============================>>>>
    hash=788c67b82a6d2219c3e3f6b2782b638fbce63e3071a88be0b365a145f4ed1f01
    ver=1
    hashPrevBlock=68e84c56c44f1bedfbd90f691946932fa8f546f0e1f85d74d8703bca7e24538c,
    hashMerkleRoot=59284e0503d53a555eacac1394c0b0bb7b9b8b9aa1fe00f3d2acfba0b212572f,
    nTime=1548363878,
    nBits=1f00d554,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100f8c8ff1d01f8c0bf5c7fe55315956d77ef279338ac21c957e678c04993b3bef30220281eda6a6328bbad0f3ebb9e7c1b697d2041d1e0e685ea444ceb2d3c74f9a762,
    Vtx : size 2
    CTransaction(hash=212c22dbb9, ver=1,  nTime=1548363878, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5900)
    CTxOut(empty)

    CTransaction(hash=9cc9e7a04b, ver=1,  nTime=1548363878, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(1d0884e243, 0), scriptSig=47304402200dac0c84890579)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    212c22dbb9a4de92d964db66881a68969030a13cc21190154643720c4ded4768    9cc9e7a04b3a2f7005f4a77bd2a5fff2bbd709ef5d71dacda79467ac31cad8aa    59284e0503d53a555eacac1394c0b0bb7b9b8b9aa1fe00f3d2acfba0b212572fCBlock <<<<============================
{1, 1548363878, 1548363878 , 520148308 , 0 , 0 , 0 , 0 , uint256("788c67b82a6d2219c3e3f6b2782b638fbce63e3071a88be0b365a145f4ed1f01") , uint256("59284e0503d53a555eacac1394c0b0bb7b9b8b9aa1fe00f3d2acfba0b212572f") , 4500777600000 }, // Block 9
CBlock ============================>>>>
    hash=69946e7efe06a32bc3d0e15db5bb69dac482ad3f6912308596518e6c0a9360c5
    ver=1
    hashPrevBlock=788c67b82a6d2219c3e3f6b2782b638fbce63e3071a88be0b365a145f4ed1f01,
    hashMerkleRoot=81a1547664ba19eff71327f58765e2ec6e1b6bace64cc6fb4ccc9b05e333c7dd,
    nTime=1548363938,
    nBits=1f00d554,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=304502210093a2e25abed80e244ba1ca72540e96599ff01b38028904cb84f93d94a18b6ebc0220554d466e7a84aafbbeb8e7f1b4482c085738c59ed87c40efb6ac895aa05bad6c,
    Vtx : size 2
    CTransaction(hash=0dd8e7a65d, ver=1,  nTime=1548363938, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5a00)
    CTxOut(empty)

    CTransaction(hash=c1fa6a44af, ver=1,  nTime=1548363938, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(7294029ddc, 0), scriptSig=483045022100bdb0c9fb3134)
    CTxOut(empty)
    CTxOut(nValue=4500.97000000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=4500.97400000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=0.21600000, scriptPubKey=2102f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058bac)

  vMerkleTree:     vMerkleTree : size 3
    0dd8e7a65debee2e1a091aff0e23745e6f92e8ca545d515d7d0e2e8e6a924147    c1fa6a44af56cd2b86905d2d3a544d43c5fd212aa8f017643a795e98dd96d743    81a1547664ba19eff71327f58765e2ec6e1b6bace64cc6fb4ccc9b05e333c7ddCBlock <<<<============================
{1, 1548363938, 1548363938 , 520148308 , 0 , 0 , 0 , 0 , uint256("69946e7efe06a32bc3d0e15db5bb69dac482ad3f6912308596518e6c0a9360c5") , uint256("81a1547664ba19eff71327f58765e2ec6e1b6bace64cc6fb4ccc9b05e333c7dd") , 4500972000000 }, // Block 10
CBlock ============================>>>>
    hash=1700dfd0131b1ab0d6d588375cfd3a27e54f84bc91a61ba56d0a19d3d2d8c3f3
    ver=1
    hashPrevBlock=69946e7efe06a32bc3d0e15db5bb69dac482ad3f6912308596518e6c0a9360c5,
    hashMerkleRoot=486444324677b9f7d06b940a9602e3ad4f33d8850f4b8e461a7a9e3cd8a25361,
    nTime=1548363998,
    nBits=201fffff,
    nNonce=4,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=4864443246, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5b0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)

  vMerkleTree:     vMerkleTree : size 1
    486444324677b9f7d06b940a9602e3ad4f33d8850f4b8e461a7a9e3cd8a25361CBlock <<<<============================
{0, 1548363998, 0 , 538968063 , 4 , 1 , 0 , 0 , uint256("1700dfd0131b1ab0d6d588375cfd3a27e54f84bc91a61ba56d0a19d3d2d8c3f3") , uint256("486444324677b9f7d06b940a9602e3ad4f33d8850f4b8e461a7a9e3cd8a25361") , 5500972000000 }, // Block 11
CBlock ============================>>>>
    hash=1d00511875433c88b96cc5288d1f11adfac80e374566a4785451b51fae8e5b74
    ver=1
    hashPrevBlock=1700dfd0131b1ab0d6d588375cfd3a27e54f84bc91a61ba56d0a19d3d2d8c3f3,
    hashMerkleRoot=abc79f95e4bffa35945f9d0d47dd07281d584deef4be3b43c9d075112aed8e4f,
    nTime=1548363998,
    nBits=201fffff,
    nNonce=12,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=abc79f95e4, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5c0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)

  vMerkleTree:     vMerkleTree : size 1
    abc79f95e4bffa35945f9d0d47dd07281d584deef4be3b43c9d075112aed8e4fCBlock <<<<============================
{0, 1548363998, 0 , 538968063 , 12 , 1 , 0 , 0 , uint256("1d00511875433c88b96cc5288d1f11adfac80e374566a4785451b51fae8e5b74") , uint256("abc79f95e4bffa35945f9d0d47dd07281d584deef4be3b43c9d075112aed8e4f") , 6500972000000 }, // Block 12
CBlock ============================>>>>
    hash=07f7b5e76c84625acd2f1c6fbf64f86dcd21c0cdc2e8ac4050605349932da41e
    ver=1
    hashPrevBlock=1d00511875433c88b96cc5288d1f11adfac80e374566a4785451b51fae8e5b74,
    hashMerkleRoot=a7257772333553ba04490171747871c7033cb3c38318e630730c5fdf4b2b0e87,
    nTime=1548363998,
    nBits=201fffff,
    nNonce=27,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=a725777233, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5d0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)

  vMerkleTree:     vMerkleTree : size 1
    a7257772333553ba04490171747871c7033cb3c38318e630730c5fdf4b2b0e87CBlock <<<<============================
{0, 1548363998, 0 , 538968063 , 27 , 1 , 0 , 0 , uint256("07f7b5e76c84625acd2f1c6fbf64f86dcd21c0cdc2e8ac4050605349932da41e") , uint256("a7257772333553ba04490171747871c7033cb3c38318e630730c5fdf4b2b0e87") , 7500972000000 }, // Block 13
CBlock ============================>>>>
    hash=1525f88c832b66af47ecc6d5b140e624dc902de1ca0cdfab2f2f8c57e417bd79
    ver=1
    hashPrevBlock=07f7b5e76c84625acd2f1c6fbf64f86dcd21c0cdc2e8ac4050605349932da41e,
    hashMerkleRoot=7b89c7afc49c2ff3874f3348611cd0987dd35bc4b18a76f000094758b23e36f2,
    nTime=1548363998,
    nBits=201fffff,
    nNonce=5,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=7b89c7afc4, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5e0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)

  vMerkleTree:     vMerkleTree : size 1
    7b89c7afc49c2ff3874f3348611cd0987dd35bc4b18a76f000094758b23e36f2CBlock <<<<============================
{0, 1548363998, 0 , 538968063 , 5 , 1 , 0 , 0 , uint256("1525f88c832b66af47ecc6d5b140e624dc902de1ca0cdfab2f2f8c57e417bd79") , uint256("7b89c7afc49c2ff3874f3348611cd0987dd35bc4b18a76f000094758b23e36f2") , 8500972000000 }, // Block 14
CBlock ============================>>>>
    hash=1d7d3c9834f0db455e16b55801bb94b3e3260da496ac6826e6241db84a1a94ae
    ver=1
    hashPrevBlock=1525f88c832b66af47ecc6d5b140e624dc902de1ca0cdfab2f2f8c57e417bd79,
    hashMerkleRoot=217e2b5cda1281474eff8b9cdfc0c75185cd9e477b8f0ca7799423e8c1c76ebf,
    nTime=1548363998,
    nBits=201fffff,
    nNonce=6,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=217e2b5cda, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 5f0101)
    CTxOut(nValue=10000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)

  vMerkleTree:     vMerkleTree : size 1
    217e2b5cda1281474eff8b9cdfc0c75185cd9e477b8f0ca7799423e8c1c76ebfCBlock <<<<============================
{0, 1548363998, 0 , 538968063 , 6 , 1 , 0 , 0 , uint256("1d7d3c9834f0db455e16b55801bb94b3e3260da496ac6826e6241db84a1a94ae") , uint256("217e2b5cda1281474eff8b9cdfc0c75185cd9e477b8f0ca7799423e8c1c76ebf") , 9500972000000 }, // Block 15
CBlock ============================>>>>
    hash=b506aaa2b0456e1f7b41574444d4aa3ca9af3394a1076cb4462c28e97679c14b
    ver=1
    hashPrevBlock=1d7d3c9834f0db455e16b55801bb94b3e3260da496ac6826e6241db84a1a94ae,
    hashMerkleRoot=59e29d95211ebb9093f64274f3719a0b6c63f35031a695636333faa6c5f7a08b,
    nTime=1548364053,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100f225e77482ad8c6b13b50f902c3edb92bc23d83feeffd88590f1f53987f1d02f022075ad36a89242c66a8569cfe21f97cfb6b831a3469f6c27aea977a5368e7073a1,
    Vtx : size 2
    CTransaction(hash=4e25d9e71b, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 600101)
    CTxOut(empty)

    CTransaction(hash=de95fc4f42, ver=1,  nTime=1548364053, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(0418c92bb8, 1), scriptSig=47304402206bf2bca7310e2d)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    4e25d9e71b6b8171d06475e6f8f0785e4973d0677912eab7cc07fadd2d94ab13    de95fc4f42fd2469fa5ded7f7b9e3adf11bc0d1e2467cae99108e79e47ea7357    59e29d95211ebb9093f64274f3719a0b6c63f35031a695636333faa6c5f7a08bCBlock <<<<============================
{1, 1548364053, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("b506aaa2b0456e1f7b41574444d4aa3ca9af3394a1076cb4462c28e97679c14b") , uint256("59e29d95211ebb9093f64274f3719a0b6c63f35031a695636333faa6c5f7a08b") , 10400972000000 }, // Block 16
CBlock ============================>>>>
    hash=4f67563095be63d2268b32dd2ed0691982b6afbc9f38921aac5011d39974c8ba
    ver=1
    hashPrevBlock=b506aaa2b0456e1f7b41574444d4aa3ca9af3394a1076cb4462c28e97679c14b,
    hashMerkleRoot=5707b11bf1382ad83876dc9f2d86889e799cab47e7f88c82501236380ab69141,
    nTime=1548364063,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=30440220675cc4ddf746b0ebc0f49ce99974ebd18f5a1b308dbc85f66488848c5e06dbe002204cef9ed51dc96e05ef841c514884db338212199a1d72eb4b763716c1a55c4ea7,
    Vtx : size 2
    CTransaction(hash=67d63851ea, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01110101)
    CTxOut(empty)

    CTransaction(hash=50cbd87c68, ver=1,  nTime=1548364063, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(0418c92bb8, 2), scriptSig=483045022100ccacca946c05)
    CTxOut(empty)
    CTxOut(nValue=6750.48700000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=6750.48700000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    67d63851ea56d591d1d818fb924877a55b3e4731cb8628f2b6dea1835c499762    50cbd87c68199a1a81c6a03b69df09201ea0bcb632f4ba1cb12f1e3102b45fb3    5707b11bf1382ad83876dc9f2d86889e799cab47e7f88c82501236380ab69141CBlock <<<<============================
{1, 1548364063, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("4f67563095be63d2268b32dd2ed0691982b6afbc9f38921aac5011d39974c8ba") , uint256("5707b11bf1382ad83876dc9f2d86889e799cab47e7f88c82501236380ab69141") , 11300972000000 }, // Block 17
CBlock ============================>>>>
    hash=1db9ba42d9b9d14839f9369da384834ddad13097bf3d8c5f6d360d057712c858
    ver=1
    hashPrevBlock=4f67563095be63d2268b32dd2ed0691982b6afbc9f38921aac5011d39974c8ba,
    hashMerkleRoot=56adb227fa5b1fd57c3d76b2e33cb5d813d8cef6fda16910adba56dcc7229251,
    nTime=1548364073,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100b5abdc3d105483c4e4502f42ef4358b6c0e785b29211834db1993c0a6d08a34502204600f54f3e86e25f33c4cba52e84ac9f60652d0840ce9590b817035c5c5eb5c4,
    Vtx : size 2
    CTransaction(hash=dba97e03d1, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01120101)
    CTxOut(empty)

    CTransaction(hash=caded95c86, ver=1,  nTime=1548364073, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(9cc9e7a04b, 1), scriptSig=483045022100a832bd40e11a)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    dba97e03d1e31fcfbb2b72ed39d07e6dae86ca0fe15ee6d4f9fe3e1b052dae8e    caded95c8637f8855311d3605c0522eb47be28fa5c7cd92a175db41f06842ee0    56adb227fa5b1fd57c3d76b2e33cb5d813d8cef6fda16910adba56dcc7229251CBlock <<<<============================
{1, 1548364073, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("1db9ba42d9b9d14839f9369da384834ddad13097bf3d8c5f6d360d057712c858") , uint256("56adb227fa5b1fd57c3d76b2e33cb5d813d8cef6fda16910adba56dcc7229251") , 12200972000000 }, // Block 18
CBlock ============================>>>>
    hash=3eb9dc986d45ebd14c2f3abcab1c05f4425aed75de80d0bbe0242278d9f0686f
    ver=1
    hashPrevBlock=1db9ba42d9b9d14839f9369da384834ddad13097bf3d8c5f6d360d057712c858,
    hashMerkleRoot=4d15e698e2978551a4ab67c7b0260da040f3fbf738cf3fc79f7942bf035a6959,
    nTime=1548364083,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3044022006e31b6126acae168ddd4741165aa75f415055306d1fa87ad779ef13bd9fce0402205728f788d000fd0a04abfa2850313dead2e33bb1ff45308275943beb7be9cda9,
    Vtx : size 2
    CTransaction(hash=834e034a96, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01130101)
    CTxOut(empty)

    CTransaction(hash=6e464fa848, ver=1,  nTime=1548364083, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(9cc9e7a04b, 2), scriptSig=483045022100a1c2f3b0d5d2)
    CTxOut(empty)
    CTxOut(nValue=6750.48700000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=6750.48700000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    834e034a965b061ee819fa86191f4d20e4b283f6758e9cc87b2ebc91bbbb8dbe    6e464fa848f6074e8d66ad1c1e5c9d33fa1f8456149671a40edd036239306bc3    4d15e698e2978551a4ab67c7b0260da040f3fbf738cf3fc79f7942bf035a6959CBlock <<<<============================
{1, 1548364083, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("3eb9dc986d45ebd14c2f3abcab1c05f4425aed75de80d0bbe0242278d9f0686f") , uint256("4d15e698e2978551a4ab67c7b0260da040f3fbf738cf3fc79f7942bf035a6959") , 13100972000000 }, // Block 19
CBlock ============================>>>>
    hash=0b6fda9d1523b73ff85c0cf17eb22f1a0fa54814b746b21d4528143646091f3d
    ver=1
    hashPrevBlock=3eb9dc986d45ebd14c2f3abcab1c05f4425aed75de80d0bbe0242278d9f0686f,
    hashMerkleRoot=6ac7a606c10f536d2493316ca47909a0785999b3b6de5619e4234ff4f2c0fb3f,
    nTime=1548364093,
    nBits=201fffff,
    nNonce=0,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=3045022100e4f0354b5346d7b0537528b03cd3ff4b48b867985e36b7f400412cdcf6ee56820220582a760ae4f831c9af794a1f8e1f90ac3ebcfee37dd92540f5de53fc552f1013,
    Vtx : size 2
    CTransaction(hash=92ba0f4f06, ver=1,  nTime=0, vin.size=1, vout.size=1, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 01140101)
    CTxOut(empty)

    CTransaction(hash=ada13f5d13, ver=1,  nTime=1548364093, vin.size=1, vout.size=4, nLockTime=0)
    CTxIn(COutPoint(a539ea0b19, 1), scriptSig=483045022100ca7f6f87b943)
    CTxOut(empty)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=6750.48500000, scriptPubKey=41040b4c866585dd868a9d62348a9cd008d6a312937048fff31670e7e920cfc7a7447b5f0bba9e01e6fe4735c8383e6e7a3347a0fd72381b8f797a19f694054e5a69ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 3
    92ba0f4f0669af156bb4ae4c2522540613f590032f353da1b17fc71533116119    ada13f5d13897436d4cd7c2ce72185c3b6f09bd9b9709ae33d312fc0f4192a40    6ac7a606c10f536d2493316ca47909a0785999b3b6de5619e4234ff4f2c0fb3fCBlock <<<<============================
{1, 1548364093, 0 , 538968063 , 0 , 1 , 0 , 0 , uint256("0b6fda9d1523b73ff85c0cf17eb22f1a0fa54814b746b21d4528143646091f3d") , uint256("6ac7a606c10f536d2493316ca47909a0785999b3b6de5619e4234ff4f2c0fb3f") , 14000972000000 }, // Block 20
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

void CreateOldBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t & blockInfo, CWallet* pwallet, CScript & scriptPubKey, bool fProofOfStake)
{
    CBlockTemplate *pblocktemplate;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, fProofOfStake));

    std::vector<CTransaction*>txFirst;
    for (int i = startBlock-1; i < endBlock-1; i++)
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
            cout << pblock->ToString() << endl;
            //cout << "scriptPubKey: " << HexStr(scriptPubKey) << endl;
            BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        } else {
            cout << pblock->ToString() << endl;
            BOOST_CHECK(scriptPubKey == pblock->vtx[0].vout[0].scriptPubKey);
            BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        }

        BOOST_CHECK(state.IsValid());
        // we should get the same balance
        cout << "Balance: " << pwallet->GetBalance() << endl;
        BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance());
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


    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
}


BOOST_AUTO_TEST_SUITE_END()

