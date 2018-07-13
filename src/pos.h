// Copyright (c) 2015-2016 The Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef POS_H
#define POS_H

#include "main.h"
#include "stakeinput.h"

class CBlockIndex;
class CCoins;
class COutPoint;
class uint256;
class CTransaction;

static const int STAKE_MIN_CONFIRMATIONS = 25;
static const int STAKE_MIN_AGE = 4 * 60 * 60;

uint256 ComputeStakeModifierOld(const CBlockIndex* pindexPrev, const uint256& kernel);

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CCoins* txPrev, 
                          const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake);
bool CheckProofOfStake_Old(CBlockIndex* pindexPrev, const CTransaction& tx, unsigned int nBits,
                           uint256& hashProofOfStake, std::unique_ptr<CStakeInput>& stake);
bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);
#endif // POS_H
