// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    CAmount nMoneySupplyPoWEnd = 5 * Params().LAST_POW_BLOCK() * COIN;
    CAmount nSum = 0;    

    for (int nHeight = 0; nHeight < 1000; nHeight++) {
        /* PoW */
        CAmount nSubsidy = GetBlockValue(nHeight);
        BOOST_CHECK(nSubsidy == 5 * COIN);
        BOOST_CHECK(MoneyRange(nSubsidy));
        nSum += nSubsidy;
        BOOST_CHECK(nSum <= nMoneySupplyPoWEnd);
    }

    for (int nHeight = 1001; nHeight <= 2100000; nHeight++) {
        /* PoS */
        CAmount nSubsidy = GetBlockValue(nHeight);
        BOOST_CHECK(nSubsidy == 5 * COIN);
        BOOST_CHECK(MoneyRange(nSubsidy));
        nSum += nSubsidy;
        BOOST_CHECK(nSum <= MAX_MONEY);
    }
    BOOST_CHECK(nSum == MAX_MONEY);
}

BOOST_AUTO_TEST_SUITE_END()
