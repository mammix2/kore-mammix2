// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The KORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINPARAMS_H
#define BITCOIN_CHAINPARAMS_H

#include "chainparamsbase.h"
#include "checkpoints.h"
#include "primitives/block.h"
#include "protocol.h"
#include "uint256.h"

#include <vector>

typedef unsigned char MessageStartChars[MESSAGE_START_SIZE];

struct CDNSSeedData {
    std::string name, host;
    CDNSSeedData(const std::string& strName, const std::string& strHost) : name(strName), host(strHost) {}
};

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * KORE system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,     // BIP16
        EXT_PUBLIC_KEY, // BIP32
        EXT_SECRET_KEY, // BIP32
        EXT_COIN_TYPE,  // BIP44

        MAX_BASE58_TYPES
    };

    enum DeploymentPos {
        DEPLOYMENT_TESTDUMMY,
        DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
        // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
        MAX_VERSION_BITS_DEPLOYMENTS
    };

    struct BIP9Deployment {
        /** Bit position to select the particular bit in nVersion. */
        int bit;
        /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
        int64_t nStartTime;
        /** Timeout/expiry MedianTime for the deployment attempt. */
        int64_t nTimeout;
    };

    typedef BIP9Deployment vDeployments_type[MAX_VERSION_BITS_DEPLOYMENTS];

    // mark the Fork Block here
    const int HeightToFork() const { return heightToFork; };

    const uint256& HashGenesisBlock() const { return hashGenesisBlock; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const std::vector<unsigned char>& AlertKey() const { return vAlertPubKey; }
    int GetDefaultPort() const { return nDefaultPort; }
    const uint256& ProofOfWorkLimit() const { return bnProofOfWorkLimit; }
    const uint256& ProofOfStakeLimit() const { return bnProofOfStakeLimit; }
    int SubsidyHalvingInterval() const { return nSubsidyHalvingInterval; }
    /** Used to check majorities for block version upgrade */
    int RejectBlockOutdatedMajority() const { return nRejectBlockOutdatedMajority; }
    int GetMajorityBlockUpgradeToCheck() const { return nToCheckBlockUpgradeMajority; }
    int GetMaxReorganizationDepth() const { return nMaxReorganizationDepth; }

    /** Used if GenerateBitcoins is called with a negative number of threads */
    int GetDefaultMinerThreads() const { return nMinerThreads; }
    const CBlock& GenesisBlock() const { return genesis; }
    /** Make miner wait to have peers to avoid wasting work */
    bool DoesMiningRequiresPeers() const { return fMiningRequiresPeers; }
    /** Headers first syncing is disabled */
    bool IsHeadersFirstSyncingActive() const { return fHeadersFirstSyncingActive; };
    /** Default value for -checkmempool and -checkblockindex argument */
    bool IsConsistencyChecksDefault() const { return fDefaultConsistencyChecks; }
    /** Allow mining of a min-difficulty block */
    bool AllowMinDifficultyBlocks() const { return fAllowMinDifficultyBlocks; }
    /** Skip proof-of-work check: allow mining of any difficulty block */
    bool SkipProofOfWorkCheck() const { return fSkipProofOfWorkCheck; }
    /** Make standard checks */
    bool RequireStandard() const { return fRequireStandard; }
    uint GetTargetTimespan() const { return nTargetTimespan; }
    uint GetTargetSpacing() const { return nTargetSpacing; }
    // minimum spacing is maturity - 1
    int64_t GetTargetSpacingForStake() const {return nStakeTargetSpacing;}
    int GetMaxStakeModifierInterval() const { return std::min(nStakeMinConfirmations, 64U); }
    int64_t GetDifficultyAdjustmentInterval() const { return nTargetTimespan / nTargetSpacing; }
    int64_t GetPastBlocksMin() const { return nPastBlocksMin; }
    int64_t GetPastBlocksMax() const { return nPastBlocksMax; }
    unsigned int GetStakeMinAge() const {return nStakeMinAge;}
    // minimum Stake confirmations is 2 !!!
    unsigned int GetStakeMinConfirmations() const {return nStakeMinConfirmations;}
    unsigned int GetModifierInterval() const {return nModifierInterval;}
    int64_t GetClientMintableCoinsInterval() const { return nClientMintibleCoinsInterval; }
    int64_t GetEnsureMintableCoinsInterval() const { return nEnsureMintibleCoinsInterval; }
    
    int64_t Interval() const { return nTargetTimespan / nTargetSpacing; }
    int GetCoinbaseMaturity() const { return nMaturity; }
    CAmount GetMaxMoneyOut() const { return nMaxMoneyOut; }
    /** The masternode count that we will allow the see-saw reward payments to be off by */
    int GetMasternodeCountDrift() const { return nMasternodeCountDrift; }
    int64_t GetMaxTipAge() const { return nMaxTipAge; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }    
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool ShouldMineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string GetNetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }

    uint32_t GetRuleChangeActivationThreshold() const { return nRuleChangeActivationThreshold;}
    uint32_t GetMinerConfirmationWindow() const { return nMinerConfirmationWindow;}

    int64_t  GetStakeLockInterval() const             { return nStakeLockInterval; }
    int64_t  GetStakeLockSequenceNumber() const       { return (nStakeLockInterval >> CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) | CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG; }

    const CChainParams::vDeployments_type & GetVDeployments() const { return vDeployments;}
    const std::vector<CAddress>& FixedSeeds() const { return vFixedSeeds; }
    virtual const Checkpoints::CCheckpointData& GetCheckpoints() const = 0;
    int GetPoolMaxTransactions() const { return nPoolMaxTransactions; }

    /** Spork key and Masternode Handling **/
    std::string GetDevFundPubKey() const { return strDevFundPubKey; }
    std::string GetSporkKey() const { return strSporkKey; }
    int64_t GetSporkKeyEnforceNew() const { return nEnforceNewSporkKey; }
    int64_t RejectOldSporkKey() const { return nRejectOldSporkKey; }
    std::string GetObfuscationPoolDummyAddress() const { return strObfuscationPoolDummyAddress; }
    int64_t GetStartMasternodePayments() const { return nStartMasternodePayments; }
    int64_t GetBudgetVoteUpdate() const { return nBudgetVoteUpdate; }
    int64_t GetBudgetFeeConfirmations() const { return nBudgetFeeConfirmations; }

    int64_t GetMasternodeMinConfirmations() const { return nMasternodeMinConfirmations; }
    int64_t GetMasternodeMinMNPSeconds() const { return nMasternodeMinMNPSeconds; }
    int64_t GetMasternodeMinMNBSeconds() const { return nMasternodeMinMNBSeconds; }
    int64_t GetMasternodePingSeconds() const { return nMasternodePingSeconds; }
    int64_t GetMasternodeExpirationSeconds() const { return nMasternodeExpirationSeconds; }    
    int64_t GetMasternodeRemovalSeconds() const { return nMasternodeRemovalSeconds; }
    int64_t GetMasternodeCheckSeconds() const { return nMasternodeCheckSeconds; }
    int64_t GetMasternodeCoinScore() const { return nMasternodeCoinScore; }
    int64_t GetMasternodeBudgetPaymentCycle() const { return nMasternodeBudgetPaymentCycle; }
    int64_t GetMasternodeFinalizationWindow() const { return nMasternodeFinalizationWindow; }
        
    CBaseChainParams::Network GetNetworkID() const { return networkID; }
    int GetLastPoWBlock() const { return nLastPOWBlock; }
    int GetBlockEnforceInvalid() const { return nBlockEnforceInvalidUTXO; }
    bool EnableBigRewards() const { return fEnableBigReward;}

protected:
    CChainParams() {}

    int heightToFork;

    uint256 hashGenesisBlock;
    MessageStartChars pchMessageStart;
    //! Raw pub key bytes for the broadcast alert signing key.
    std::vector<unsigned char> vAlertPubKey;
    int nDefaultPort;
    long nMaxTipAge;    
    uint64_t nPruneAfterHeight; // Legacy    
    uint256 bnProofOfWorkLimit;
    uint256 bnProofOfStakeLimit;
    int nMaxReorganizationDepth;
    int nSubsidyHalvingInterval;
    int nRejectBlockOutdatedMajority;
    int nToCheckBlockUpgradeMajority;
    uint nTargetTimespan;
    uint nTargetSpacing;
    int64_t nStakeTargetSpacing;
    int64_t nPastBlocksMin; // used when calculating the NextWorkRequired 
    int64_t nPastBlocksMax;
    unsigned int nStakeMinAge;
    unsigned int nStakeMinConfirmations;
    unsigned int nModifierInterval;
    int64_t nClientMintibleCoinsInterval; // PoS mining
    int64_t nEnsureMintibleCoinsInterval;
    int nLastPOWBlock;
    int nMasternodeCountDrift;
    int nMaturity;
    int64_t                    nStakeLockInterval;
    CAmount nMaxMoneyOut;
    int nMinerThreads;
    std::vector<CDNSSeedData> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    vDeployments_type vDeployments;
    CBaseChainParams::Network networkID;
    std::string strNetworkID;
    std::string strDevFundPubKey;
    CBlock genesis;
    std::vector<CAddress> vFixedSeeds;
    bool fMiningRequiresPeers;
    bool fAllowMinDifficultyBlocks;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool fMineBlocksOnDemand;
    bool fSkipProofOfWorkCheck;
    bool fHeadersFirstSyncingActive;
    bool fEnableBigReward;
    int nPoolMaxTransactions;
    std::string strSporkKey;
    int64_t nEnforceNewSporkKey;
    int64_t nRejectOldSporkKey;
    std::string strObfuscationPoolDummyAddress;
    int64_t nStartMasternodePayments;
    int64_t nBudgetVoteUpdate;
    void  MineNewGenesisBlock_Legacy();
    int64_t nBudgetFeeConfirmations;
    int64_t nMasternodeMinConfirmations;
    int64_t nMasternodeMinMNPSeconds;
    int64_t nMasternodeMinMNBSeconds;
    int64_t nMasternodePingSeconds;
    int64_t nMasternodeExpirationSeconds;
    int64_t nMasternodeRemovalSeconds;
    int64_t nMasternodeCheckSeconds;
    int64_t nMasternodeCoinScore;   
    int64_t nMasternodeBudgetPaymentCycle;
    int64_t nMasternodeFinalizationWindow;

    int nBlockEnforceSerialRange;
    int nBlockRecalculateAccumulators;
    int nBlockFirstFraudulent;
    int nBlockLastGoodCheckpoint;
    int nBlockEnforceInvalidUTXO;
};

/**
 * Modifiable parameters interface is used by test cases to adapt the parameters in order
 * to test specific features more easily. Test cases should always restore the previous
 * values after finalization.
 */

class CModifiableParams
{
public:
    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval) = 0;
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) = 0;
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) = 0;
    virtual void setDefaultConsistencyChecks(bool aDefaultConsistencyChecks) = 0;
    virtual void setAllowMinDifficultyBlocks(bool aAllowMinDifficultyBlocks) = 0;
    virtual void setSkipProofOfWorkCheck(bool aSkipProofOfWorkCheck) = 0;
    virtual void setStakeLockInterval(int aStakeLockInterval) = 0;
    virtual void setHeightToFork(int aHeightToFork) = 0;
    virtual void setLastPowBlock(int aLastPOWBlock) = 0;
    virtual void setStakeMinConfirmations(int aStakeMinConfirmations) = 0;
    virtual void setStakeMinAge(int aStakeMinAge) = 0;
    virtual void setStakeModifierInterval(int aStakeModifier) = 0;
    virtual void setCoinbaseMaturity(int aCoinbaseMaturity) = 0;
    virtual void setLastPOW(int aLastPOW) = 0;
    virtual void setEnableBigRewards(bool bigRewards) = 0;
    virtual void setTargetTimespan(uint aTargetTimespan) = 0;
    virtual void setTargetSpacing(uint aTargetSpacing) = 0;
    virtual void setMineBlocksOnDemand(bool mineBlocks) = 0;

};


/**
 * Return the currently selected parameters. This won't change after app startup
 * outside of the unit tests.
 */
const CChainParams& Params();

/** Return parameters for the given network. */
CChainParams& Params(CBaseChainParams::Network network);

/** Get modifiable network parameters (UNITTEST only) */
CModifiableParams* ModifiableParams();

/** Sets the params returned by Params() to those for the given network. */
void SelectParams(CBaseChainParams::Network network);

/**
 * Looks for -regtest or -testnet and then calls SelectParams as appropriate.
 * Returns false if an invalid combination is given.
 */
bool SelectParamsFromCommandLine();

#endif // BITCOIN_CHAINPARAMS_H
