// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2016 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "checkpoints.h"
#include "init.h"
#include "tests_util.h"
#include "util.h"

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(fork_kore_from_blockinfo)


static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");


BOOST_AUTO_TEST_CASE(quick_fork)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    int oldCoinBaseMaturity = Params().COINBASE_MATURITY();
    int oldStakeMinAge = Params().StakeMinAge();
    int oldModifier = Params().GetModifierInterval();
    // confirmations    : 3
    // remember that the miminum spacing is 10 !!!
    // spacing          : [confirmations-1, max(confirmations-1, value)]
    // modifierInterval : [spacing, spacing)]
    // pow blocks       : [confirmations + 1, max(confirmations+1, value)], this way we will have 2 modifiers
    int minConfirmations = 3;
    ModifiableParams()->setHeightToFork(9);
    ModifiableParams()->setStakeMinConfirmations(minConfirmations);
    ModifiableParams()->setTargetSpacing(minConfirmations - 1);
    ModifiableParams()->setStakeModifierInterval(minConfirmations - 1);
    ModifiableParams()->setCoinbaseMaturity(minConfirmations); 
    ModifiableParams()->setStakeMinAge(0);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setEnableBigRewards(true);

    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(strSecret, pwalletMain);

    // generate 4 pow blocks
    CreateOldBlocksFromBlockInfo(1, minConfirmations + 2, blockinfo[0], pwalletMain, scriptPubKey, false);

    // generate 4 pos blocks
    GeneratePOSLegacyBlocks(minConfirmations + 2, 9, pwalletMain, scriptPubKey);

    GenerateBlocks(9, 100, pwalletMain, scriptPubKey, true);

    // Leaving old values
    Checkpoints::fEnabled = true;
    ModifiableParams()->setHeightToFork(oldHeightToFork);
    ModifiableParams()->setEnableBigRewards(false);
    ModifiableParams()->setCoinbaseMaturity(oldCoinBaseMaturity);
    ModifiableParams()->setStakeMinAge(oldStakeMinAge);
    ModifiableParams()->setStakeModifierInterval(oldModifier);
    ModifiableParams()->setStakeMinConfirmations(oldStakeMinConfirmations);
    ModifiableParams()->setTargetTimespan(oldTargetTimespan);
    ModifiableParams()->setTargetSpacing(oldTargetSpacing);
}

BOOST_AUTO_TEST_SUITE_END()
