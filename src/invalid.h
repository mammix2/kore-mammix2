// Copyright (c) 2018 The KORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KORE_INVALID_H
#define KORE_INVALID_H

#endif //KORE_INVALID_H

#ifdef ZEROCOIN
#include <libzerocoin/bignum.h>
#endif

#include <univalue/include/univalue.h>
#include <primitives/transaction.h>

namespace invalid_out
{
#ifdef ZEROCOIN    
    extern std::set<CBigNum> setInvalidSerials;
#endif    
    extern std::set<COutPoint> setInvalidOutPoints;

    UniValue read_json(const std::string& jsondata);

    bool ContainsOutPoint(const COutPoint& out);
#ifdef ZEROCOIN        
    bool ContainsSerial(const CBigNum& bnSerial);
    bool LoadSerials();
#endif
    bool LoadOutpoints();
   
}