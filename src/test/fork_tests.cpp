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
#include "../legacy/consensus/merkle.h"
#include "../miner.h"

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(fork_tests)

static
struct {
    unsigned char extranonce;
    unsigned int nonce;
    uint32_t nBirthdayA;
    uint32_t nBirthdayB;
} blockinfo[] = {
    {4, 1, 28322513, 40002432}
};


void GenerateTestBlocks()
{
    const CChainParams& chainparams = Params();
    unsigned int nExtraNonce = 0;

    boost::shared_ptr<CReserveScript> coinbaseScript;

    //
    // Create new block
    //
    unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
    CBlockIndex* pindexPrev = chainActive.Tip();

    unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, coinbaseScript->reserveScript, NULL, true));

    if (!pblocktemplate.get()) {
        LogPrintf("Error in KoreMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
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

    for (;;) {
        unsigned int nHashesDone = 0;
        unsigned int nNonceFound = (unsigned int)-1;

        for (int i = 0; i < 1; i++) {
            pblock->nNonce = pblock->nNonce + 1;
            testHash = pblock->CalculateBestBirthdayHash();
            nHashesDone++;
            if (fDebug) LogPrintf("testHash %s\n", testHash.ToString().c_str());
            if (fDebug) LogPrintf("Hash Target %s\n", hashTarget.ToString().c_str());

            if (UintToArith256(testHash) < hashTarget) {
                // Found a solution
                nNonceFound = pblock->nNonce;
                LogPrintf("Found Hash %s\n", testHash.ToString().c_str());
                LogPrintf("hash2 %s\n", pblock->GetHash().ToString().c_str());
                // Found a solution
                assert(testHash == pblock->GetHash());
                // We have our data, lets print them
                LogPrintf("Found our Soluction");
                break;
            }
        }

        // Update nTime every few seconds
        UpdateTime(pblock, pindexPrev);
    }
}


BOOST_AUTO_TEST_CASE(generate_chain)
{
    GenerateTestBlocks();
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
    CMutableTransaction tx,tx2;
    CScript script;
    uint256 hash;

    LOCK(cs_main);
    Checkpoints::fEnabled = false;
    const CChainParams& chainparams = Params();

    // Simple block creation, nothing special yet:
    BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwalletMain, false));

    // We can't make transactions until we have inputs
    // Therefore, load 100 blocks :)
    std::vector<CTransaction*>txFirst;
    for (unsigned int i = 0; i < 5; ++i)
    {
        CBlock *pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = 1;
        pblock->nTime = chainActive.Tip()->GetMedianTimePast()+1;
        CMutableTransaction txCoinbase(pblock->vtx[0]);
        txCoinbase.vin[0].scriptSig = CScript();
        txCoinbase.vin[0].scriptSig.push_back(blockinfo[i].extranonce);
        txCoinbase.vin[0].scriptSig.push_back(chainActive.Height());
        txCoinbase.vout[0].scriptPubKey = CScript();
        pblock->vtx[0] = CTransaction(txCoinbase);
        if (txFirst.size() < 2)
            txFirst.push_back(new CTransaction(pblock->vtx[0]));
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();
        pblock->nNonce = blockinfo[i].nonce;
        pblock->nBirthdayA = blockinfo[i].nBirthdayA;
        pblock->nBirthdayB = blockinfo[i].nBirthdayB;
        CValidationState state;
        BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        BOOST_CHECK(state.IsValid());
        pblock->hashPrevBlock = pblock->GetHash();
    }
    delete pblocktemplate;

    Checkpoints::fEnabled = true;
    // Leaving old values
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setLastPOW(oldLastPOW);
}

BOOST_AUTO_TEST_SUITE_END()
