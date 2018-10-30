// Copyright (c) 2011-2013 The Bitcoin Core developers
// Copyright (c) 2017 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p120000 = uint256("0x70edc85193638b8adadb71ea766786d207f78a173dd13f965952eb76932f5729");
    uint256 p209536 = uint256("0x8a718dbb44b57a5693ac70c951f2f81a01b39933e3e19e841637f757598f571a");
    BOOST_CHECK(Checkpoints::CheckBlock(120000, p120000));
    BOOST_CHECK(Checkpoints::CheckBlock(209536, p209536));


    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(120000, p209536));
    BOOST_CHECK(!Checkpoints::CheckBlock(209536, p120000));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(120000+1, p209536));
    BOOST_CHECK(Checkpoints::CheckBlock(209536+1, p120000));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 209536);
}

BOOST_AUTO_TEST_SUITE_END()
