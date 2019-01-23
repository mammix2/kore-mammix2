// Copyright (c) 2018 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pob.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "primitives/block.h"
#include "uint256.h"

#include <cmath>

CAmount GetBlockReward1(CBlockIndex* pindexPrev)
{
    if (pindexPrev->nHeight >= Params().LAST_POW_BLOCK()) {
        static double k = 135711131719231;
        static double max_money_double = MAX_MONEY;
        static double oneThird = (double)1 / 3;
        static double half = (double)1 / 2;

        CAmount reward = ceil(pow(pow(max_money_double, 3) - pow((float)pindexPrev->nMoneySupply, 3), oneThird) / pow(k, half));
        if (reward + pindexPrev->nMoneySupply < 12000000 * COIN)
            return reward;
        else
            return (12000000 * COIN) - pindexPrev->nMoneySupply;
    } else {
        if ((Params().NetworkID() == CBaseChainParams::TESTNET || Params().NetworkID() == CBaseChainParams::UNITTEST) 
            && pindexPrev->nHeight >= 0 && pindexPrev->nHeight < 500) {
            
            return 10000 * COIN;
        }
        
        return 5 * COIN;        
    }
}

// BlackCoin kernel protocol v3
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula
//     hash(nStakeModifier + txPrev.nTime + txPrev.vout.hash + txPrev.vout.n + nTime) < bnTarget * nWeight
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coins one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier: scrambles computation to make it very difficult to precompute
//                   future proof-of-stake
//   txPrev.nTime: slightly scrambles computation
//   txPrev.vout.hash: hash of txPrev, to reduce the chance of nodes
//                     generating coinstake at the same time
//   txPrev.vout.n: output number of txPrev, to reduce the chance of nodes
//                  generating coinstake at the same time
//   nTime: current timestamp
//   block/tx hash should not be used here as they can be generated in vast
//   quantities so as to generate blocks faster, degrading the system back into
//   a proof-of-work situation.
bool CheckStakeKernelHash1(const CBlockIndex* pindexPrev, unsigned int nBits, const CCoins* txPrev, const COutPoint& prevout, unsigned int nTimeTx)
{
    // Weight
    int64_t nValueIn = txPrev->vout[prevout.n].nValue;
    if (nValueIn == 0)
        return false;

    // Base target
    arith_uint256 bnTarget = arith_uint256().SetCompact(nBits);

    if (fDebug) {
        LogPrintf("CheckStakeKernelHash: pindexPrev->nStakeModifier:%u txPrev->nTime:%u \n", pindexPrev->nStakeModifier, txPrev->nTime);
        LogPrintf("CheckStakeKernelHash: prevout.hash: %s prevout.n:%u nTimeTx:%u \n", prevout.hash.GetHex(), prevout.n, nTimeTx);
    }

    // Calculate hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << pindexPrev->nStakeModifier << txPrev->nTime << prevout.hash << prevout.n << nTimeTx;
    uint256 hashProofOfStake = ss.GetHash();

    if (fDebug) LogPrintf("CheckStakeKernelHash: hashProofOfStake:%s nValueIn:%d target: %x \n", hashProofOfStake.GetHex(), nValueIn, bnTarget.GetCompact());

    // Now check if proof-of-stake hash meets target protocol
    if (UintToArith256(hashProofOfStake) / nValueIn > bnTarget) {
        if (fDebug) LogPrintf("CheckStakeKernelHash() : hash does not meet protocol target   previous = %x  target = %x\n", nBits, bnTarget.GetCompact());
        return false;
    }

    return true;
}

bool CheckKernel1(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime)
{
    uint256 targetProofOfStake;

    CTransaction prevtx;
    uint256 hashBlock;
    if (!GetTransaction(prevout.hash, prevtx, hashBlock, true))
        return false;

    CBlockIndex* pIndex = NULL;
    BlockMap::iterator iter = mapBlockIndex.find(hashBlock);
    if (iter != mapBlockIndex.end())
        pIndex = iter->second;

    // Read block header
    CBlock block;
    if (!ReadBlockFromDisk(block, pIndex))
        return false;

    // Maturity requirement
    if (pindexPrev->nHeight - pIndex->nHeight < Params().StakeMinAge())
        return false;

    if (pBlockTime)
        *pBlockTime = block.GetBlockTime();

    // Min age requirement
    if (prevtx.nTime + Params().StakeMinAge() > nTime) // Min age requirement
        return false;

    return CheckStakeKernelHash1(pindexPrev, nBits, new CCoins(prevtx, pindexPrev->nHeight), prevout, nTime);
}

unsigned int GetNextTarget1(const CBlockIndex* pindexLast, const CBlockHeader* pblock)
{
    /* current difficulty formula, pivx - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = 24;
    int64_t PastBlocksMax = 24;
    int64_t CountBlocks = 0;
    uint256 PastDifficultyAverage;
    uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfStakeLimit().GetCompact();
    }

    if (pindexLast->nHeight + 1 > Params().LAST_POW_BLOCK()) {
        uint256 bnTargetLimit = (~uint256(0) >> 24);
        int64_t nTargetSpacing = 60;
        int64_t nTargetTimespan = 60 * 40;

        int64_t nActualSpacing = 0;
        if (pindexLast->nHeight != 0)
            nActualSpacing = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

        if (nActualSpacing < 0)
            nActualSpacing = 1;

        int64_t nNewSpacing = pblock->GetBlockTime() - pindexLast->GetBlockTime();

        // ppcoin: target change every block
        // ppcoin: retarget with exponential moving toward target spacing
        uint256 bnNew;
        bnNew.SetCompact(pindexLast->nBits);

        int64_t nInterval = nTargetTimespan / nTargetSpacing;
        bnNew *= ((nInterval - 1) * nTargetSpacing + (nActualSpacing * nNewSpacing));
        bnNew /= ((nInterval + 1) * nTargetSpacing);

        if (bnNew <= 0 || bnNew > bnTargetLimit)
            bnNew = bnTargetLimit;

        return bnNew.GetCompact();
    }

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) {
            break;
        }
        CountBlocks++;

        if (CountBlocks <= PastBlocksMin) {
            if (CountBlocks == 1) {
                PastDifficultyAverage.SetCompact(BlockReading->nBits);
            } else {
                PastDifficultyAverage = ((PastDifficultyAveragePrev * CountBlocks) + (uint256().SetCompact(BlockReading->nBits))) / (CountBlocks + 1);
            }
            PastDifficultyAveragePrev = PastDifficultyAverage;
        }

        if (LastBlockTime > 0) {
            int64_t Diff = (LastBlockTime - BlockReading->GetBlockTime());
            nActualTimespan += Diff;
        }
        LastBlockTime = BlockReading->GetBlockTime();

        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    uint256 bnNew(PastDifficultyAverage);

    int64_t _nTargetTimespan = CountBlocks * Params().TargetSpacing();

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;

    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfStakeLimit()) {
        bnNew = Params().ProofOfStakeLimit();
    }

    return bnNew.GetCompact();
}