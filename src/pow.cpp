// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "main.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

static arith_uint256 GetTargetLimit_Legacy(int64_t nTime, bool fProofOfStake)
{
    uint256 nLimit = fProofOfStake ? Params().ProofOfStakeLimit() : Params().ProofOfWorkLimit();

    return UintToArith256(nLimit);
}


unsigned int CalculateNextWorkRequired_Legacy(const CBlockIndex* pindexLast, int64_t nFirstBlockTime)
{
    int64_t nActualSpacing = pindexLast->GetBlockTime() - nFirstBlockTime;
    int64_t nTargetSpacing = Params().TargetSpacing();

    // Limit adjustment step

    if (nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;

    // Retarget
    const arith_uint256 bnPowLimit = GetTargetLimit_Legacy(pindexLast->GetBlockTime(), pindexLast->IsProofOfStake());
    arith_uint256 bnNew, bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= ((Params().DifficultyAdjustmentInterval() - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((Params().DifficultyAdjustmentInterval() + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    if (fDebug) {
        LogPrintf("RETARGET\n");
        LogPrintf("params.nTargetSpacing = %d    nActualSpacing = %d\n", Params().TargetSpacing(), nActualSpacing);
        LogPrintf("Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
        LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());
    }

    return bnNew.GetCompact();
}


unsigned int GetNextWorkRequired_Legacy(const CBlockIndex* pindexLast, const CBlockHeader* pblock, bool fProofOfStake)
{
    unsigned int nTargetLimit = UintToArith256(Params().ProofOfWorkLimit()).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nTargetLimit;

    const CBlockIndex* pindexPrev = GetLastBlockIndex_Legacy(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL)
        return nTargetLimit; // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex_Legacy(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return nTargetLimit; // second block

    return CalculateNextWorkRequired_Legacy(pindexPrev, pindexPrevPrev->GetBlockTime());
}


unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock, bool fProofOfStake)
{
    // Lico
    // if FORK_condition
    //return GetNextWorkRequired_Legacy(pindexLast, pblock, fProofOfStake);

    /* current difficulty formula, kore - DarkGravity v3, written by Evan Duffield - evan@dashpay.io */
    const CBlockIndex* BlockLastSolved = pindexLast;
    const CBlockIndex* BlockReading = pindexLast;
    int64_t nActualTimespan = 0;
    int64_t LastBlockTime = 0;
    int64_t PastBlocksMin = Params().PastBlocksMin();
    int64_t PastBlocksMax = Params().PastBlocksMax();
    int64_t CountBlocks = 0;
    uint256 PastDifficultyAverage;
    uint256 PastDifficultyAveragePrev;

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 || BlockLastSolved->nHeight < PastBlocksMin) {
        return Params().ProofOfWorkLimit().GetCompact();
    }

    if (pindexLast->nHeight > Params().LAST_POW_BLOCK() || !fProofOfStake) {
        //uint256 bnTargetLimit = (~uint256(0) >> 24);
        uint256 bnTargetLimit = fProofOfStake ? Params().ProofOfStakeLimit() : Params().ProofOfWorkLimit();
        int64_t nTargetSpacing = Params().TargetSpacing();
        int64_t nTargetTimespan = Params().TargetTimespan();
        //int64_t nTargetSpacing = 60;
        //int64_t nTargetTimespan = 60 * 40;

        int64_t nActualSpacing = 0;
        if (pindexLast->nHeight != 0)
            nActualSpacing = pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();

        if (nActualSpacing < 0)
            nActualSpacing = 1;

        // ppcoin: target change every block
        // ppcoin: retarget with exponential moving toward target spacing
        uint256 bnNew;
        bnNew.SetCompact(pindexLast->nBits);

        int64_t nInterval = nTargetTimespan / nTargetSpacing;
        bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
        bnNew /= ((nInterval + 1) * nTargetSpacing);

        if (bnNew <= 0 || bnNew > bnTargetLimit)
            bnNew = bnTargetLimit;

        if (fDebug) LogPrintf("GetNextWorkRequired: %s \n", bnNew.ToString().c_str());
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
    if (fDebug) {
        LogPrintf("nActualTimespan: %d \n", nActualTimespan);
        LogPrintf("PastDifficultyAverage: %s \n", PastDifficultyAverage.ToString().c_str());
        LogPrintf("_nTargetTimespan : %d \n", _nTargetTimespan);
    }

    if (nActualTimespan < _nTargetTimespan / 3)
        nActualTimespan = _nTargetTimespan / 3;
    if (nActualTimespan > _nTargetTimespan * 3)
        nActualTimespan = _nTargetTimespan * 3;


    // Retarget
    bnNew *= nActualTimespan;
    bnNew /= _nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit()) {
        bnNew = Params().ProofOfWorkLimit();
    }

    if (fDebug) {
        LogPrintf("nActualTimespan: %d \n", nActualTimespan);
        LogPrintf("GetNextWorkRequired: %s \n", bnNew.ToString().c_str());
    }

    return bnNew.GetCompact();
}


bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    // Lico FORK
    // return CheckProofOfWork_Legacy(hash, nBits);
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;

    if (Params().SkipProofOfWorkCheck())
        return true;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Params().ProofOfWorkLimit()) {
        if (fDebug) LogPrintf("CheckProofOfWork() : nBits below minimum work");
        return false;
    }


    // Check proof of work matches claimed amount
    if (fDebug) {
        //LogPrintf("CheckProofOfWork \n");
        //LogPrintf("hash    : %s \n", hash.ToString().c_str());
        //LogPrintf("bnTarget: %s \n", bnTarget.ToString().c_str());
    }
    if (hash > bnTarget) {
        if (fDebug) LogPrintf("CheckProofOfWork() : hash doesn't match nBits");
        return false;
    }

    return true;
}

bool CheckProofOfWork_Legacy(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(Params().ProofOfWorkLimit()))
        return false;

    if (fDebug) {
        LogPrintf("CheckProofOfWork \n");
        LogPrintf("nBits    : %x \n", nBits);
        LogPrintf("hash    : %s \n", UintToArith256(hash).ToString().c_str());
        LogPrintf("bnTarget: %s \n", bnTarget.ToString().c_str());
    }
    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

uint256 GetBlockProof(const CBlockIndex& block)
{
    uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}
