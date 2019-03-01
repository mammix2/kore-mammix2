#include "chainparams.h"
#include "checkpoints.h"
#include "init.h"
#include "tests_util.h"
#include "util.h"
#include "utiltime.h"


#include <boost/test/unit_test.hpp>

static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");

BOOST_AUTO_TEST_SUITE(fork_afterwords)

BOOST_AUTO_TEST_CASE(after_fork)
{
    if (fDebug) {
        LogPrintf("****************************************** \n");
        LogPrintf("**  Starting fork_afterwords/after_fork ** \n");
        LogPrintf("****************************************** \n");
    }

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
    ModifiableParams()->setHeightToFork(0);
    ModifiableParams()->setStakeMinConfirmations(minConfirmations - 1);
    ModifiableParams()->setCoinbaseMaturity(minConfirmations - 1);
    ModifiableParams()->setTargetSpacing(minConfirmations - 1);
    ModifiableParams()->setStakeModifierInterval(minConfirmations - 1);
    //ModifiableParams()->setTargetSpacing(10);
    //ModifiableParams()->setStakeModifierInterval(10);
    ModifiableParams()->setStakeMinAge(0);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setEnableBigRewards(true);
    //ModifiableParams()->setMineBlocksOnDemand(false);
    SetMockTime(0);

    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(strSecret, pwalletMain);

    // generate pow blocks, so we can stake
    GenerateBlocks(1, minConfirmations + 2, pwalletMain, scriptPubKey, false);

    // we are just checking if we are able to generate PoS blocks after fork
    // lets exercise more than 64 blocks, this way we will see if the max
    // modifierinterval is working, it gets max(64 blocks)
    GenerateBlocks(minConfirmations + 2, minConfirmations + 2 + 100, pwalletMain, scriptPubKey, true);

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