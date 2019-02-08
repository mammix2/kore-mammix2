
#include "chainparams.h"
#include "checkpoints.h"
#include "init.h"
#include "tests_util.h"


#include <boost/test/unit_test.hpp>

static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");

BOOST_AUTO_TEST_SUITE(generate_block_info)

/*
  This TEST CASE SHOULD BE USED ONLY WHEN YOU WANT TO CREATE DATA TO BLOCK INFO
  
BOOST_AUTO_TEST_CASE(generate_old_pow)
{
    Checkpoints::fEnabled = false;
    int64_t oldTargetTimespan = Params().TargetTimespan();
    int64_t oldTargetSpacing = Params().TargetSpacing();
    int oldHeightToFork = Params().HeigthToFork();
    int oldStakeMinConfirmations = Params().StakeMinConfirmations();
    int oldCoinBaseMaturity = Params().COINBASE_MATURITY();
    int oldStakeMinAge = Params().StakeMinAge();
    int oldModifier = Params().GetModifierInterval();
    int minConfirmations = 11;
    ModifiableParams()->setHeightToFork(999);
    ModifiableParams()->setStakeMinConfirmations(minConfirmations);
    ModifiableParams()->setCoinbaseMaturity(minConfirmations);
    ModifiableParams()->setTargetSpacing(minConfirmations-1);
    ModifiableParams()->setStakeModifierInterval(minConfirmations-1);
    ModifiableParams()->setStakeMinAge(0);      
    ModifiableParams()->setTargetTimespan(1);
    ModifiableParams()->setEnableBigRewards(true);

    
    ScanForWalletTransactions(pwalletMain);
    CScript scriptPubKey = GenerateSamePubKeyScript4Wallet(strSecret, pwalletMain);

    int totalOldPow = 2;
    // generate old pow blocks
    GeneratePOWLegacyBlocks(1,totalOldPow+1, pwalletMain, scriptPubKey);
 
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
*/

BOOST_AUTO_TEST_SUITE_END()