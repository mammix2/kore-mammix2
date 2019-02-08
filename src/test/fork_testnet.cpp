#include "chainparams.h"
#include "checkpoints.h"
#include "init.h"
#include "tests_util.h"


#include <boost/test/unit_test.hpp>

static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");

/* This testcase will take long time to run */
BOOST_AUTO_TEST_SUITE(fork_testnet)

BOOST_AUTO_TEST_CASE(testnet_parameters)
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
    int minConfirmations = 25;
    ModifiableParams()->setHeightToFork(minConfirmations + 2);
    ModifiableParams()->setStakeMinConfirmations(minConfirmations);
    ModifiableParams()->setCoinbaseMaturity(minConfirmations);
    ModifiableParams()->setTargetSpacing(minConfirmations - 1);
    ModifiableParams()->setStakeModifierInterval(minConfirmations - 1);
    ModifiableParams()->setStakeMinAge(0);
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setEnableBigRewards(true);

    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(strSecret, pwalletMain);

    // recreate old pow blocks
    CreateOldBlocksFromBlockInfo(1, minConfirmations + 2, blockinfo[0], pwalletMain, scriptPubKey, false);

    // lets generate pow blocks
    int pow = minConfirmations + 2 + Params().StakeMinConfirmations() + 10;
    GenerateBlocks(minConfirmations + 2, pow, pwalletMain, scriptPubKey, false);

    // lets generate enought block and have 10 POS blocks confirmed
    GenerateBlocks(pow, pow + Params().StakeMinConfirmations() + 10, pwalletMain, scriptPubKey, true);

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