// Copyright (c) 2018 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KORE_POB_H
#define KORE_POB_H

#include "amount.h"
#include "main.h"

#include <stdint.h>

class arith_uint256;
class CBlockHeader;
class CBlockIndex;
class CCoins;
class COutPoint;
class CTransaction;
class uint256;

CAmount GetBlockReward1(CBlockIndex* pindexPrev);

CAmount GetMinterReward(CAmount blockValue, CAmount stakedBalance, CBlockIndex* pindexPrev);

CAmount GetMasternodePayment1(CAmount blockValue, CAmount stakedBalance, CBlockIndex* pindexPrev);

bool CheckStakeKernelHash1(const CBlockIndex* pindexPrev, unsigned int nBits, const CCoins* txPrev, const COutPoint& prevout, unsigned int nTimeTx);

bool CheckKernel1(CBlockIndex* pindexPrev, unsigned int nBits, int64_t nTime, const COutPoint& prevout, int64_t* pBlockTime);

unsigned int GetNextTarget1(const CBlockIndex* pindexLast, const CBlockHeader* pblock);

#endif