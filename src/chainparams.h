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
    const int HeigthToFork() const { return heightToFork; };

    const uint256& HashGenesisBlock() const { return hashGenesisBlock; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const std::vector<unsigned char>& AlertKey() const { return vAlertPubKey; }
    int GetDefaultPort() const { return nDefaultPort; }
    const uint256& ProofOfWorkLimit() const { return bnProofOfWorkLimit; }
    const uint256& ProofOfStakeLimit() const { return bnProofOfStakeLimit; }
    int SubsidyHalvingInterval() const { return nSubsidyHalvingInterval; }
    /** Used to check majorities for block version upgrade */
    int EnforceBlockUpgradeMajority() const { return nEnforceBlockUpgradeMajority; }
    int RejectBlockOutdatedMajority() const { return nRejectBlockOutdatedMajority; }
    int ToCheckBlockUpgradeMajority() const { return nToCheckBlockUpgradeMajority; }
    int MaxReorganizationDepth() const { return nMaxReorganizationDepth; }

    /** Used if GenerateBitcoins is called with a negative number of threads */
    int DefaultMinerThreads() const { return nMinerThreads; }
    const CBlock& GenesisBlock() const { return genesis; }
    /** Make miner wait to have peers to avoid wasting work */
    bool MiningRequiresPeers() const { return fMiningRequiresPeers; }
    /** Headers first syncing is disabled */
    bool HeadersFirstSyncingActive() const { return fHeadersFirstSyncingActive; };
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Allow mining of a min-difficulty block */
    bool AllowMinDifficultyBlocks() const { return fAllowMinDifficultyBlocks; }
    /** Skip proof-of-work check: allow mining of any difficulty block */
    bool SkipProofOfWorkCheck() const { return fSkipProofOfWorkCheck; }
    /** Make standard checks */
    bool RequireStandard() const { return fRequireStandard; }
    int64_t TargetTimespan() const { return nTargetTimespan; }
    int64_t TargetSpacing() const { return nTargetSpacing; }
    int64_t StakeTargetSpacing() const {return nStakeTargetSpacing;}
    int64_t DifficultyAdjustmentInterval() const { return nTargetTimespan / nTargetSpacing; }
    int64_t PastBlocksMin() const { return nPastBlocksMin; }
    int64_t PastBlocksMax() const { return nPastBlocksMax; }
    unsigned int StakeMinAge() const {return nStakeMinAge;}
    unsigned int GetModifier() const {return nModifier;}
    int64_t ClientMintibleCoinsInterval() const { return nClientMintibleCoinsInterval; }
    int64_t EnsureMintibleCoinsInterval() const { return nEnsureMintibleCoinsInterval; }
    
    int64_t Interval() const { return nTargetTimespan / nTargetSpacing; }
    int COINBASE_MATURITY() const { return nMaturity; }
    CAmount MaxMoneyOut() const { return nMaxMoneyOut; }
    /** The masternode count that we will allow the see-saw reward payments to be off by */
    int MasternodeCountDrift() const { return nMasternodeCountDrift; }
    int64_t MaxTipAge() const { return nMaxTipAge; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }    
    /** Make miner stop after a block is found. In RPC, don't return until nGenProcLimit blocks are generated */
    bool MineBlocksOnDemand() const { return fMineBlocksOnDemand; }
    /** Return the BIP70 network string (main, test or regtest) */
    std::string NetworkIDString() const { return strNetworkID; }
    const std::vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }

    uint32_t RuleChangeActivationThreshold() const { return nRuleChangeActivationThreshold;}
    uint32_t MinerConfirmationWindow() const { return nMinerConfirmationWindow;}

    const CChainParams::vDeployments_type & GetVDeployments() const { return vDeployments;}
    const std::vector<CAddress>& FixedSeeds() const { return vFixedSeeds; }
    virtual const Checkpoints::CCheckpointData& Checkpoints() const = 0;
    int PoolMaxTransactions() const { return nPoolMaxTransactions; }

    /** Spork key and Masternode Handling **/
    std::string DevFundPubKey() const { return strDevFundPubKey; }
    std::string SporkKey() const { return strSporkKey; }
    int64_t NewSporkStart() const { return nEnforceNewSporkKey; }
    int64_t RejectOldSporkKey() const { return nRejectOldSporkKey; }
    std::string ObfuscationPoolDummyAddress() const { return strObfuscationPoolDummyAddress; }
    int64_t StartMasternodePayments() const { return nStartMasternodePayments; }
    int64_t BudgetVoteUpdate() const { return nBudgetVoteUpdate; }
    int64_t BudgetFeeConfirmations() const { return nBudgetFeeConfirmations; }

    int64_t MasternodeMinConfirmations() const { return nMasternodeMinConfirmations; }
    int64_t MasternodeMinMNPSeconds() const { return nMasternodeMinMNPSeconds; }
    int64_t MasternodeMinMNBSeconds() const { return nMasternodeMinMNBSeconds; }
    int64_t MasternodePingSeconds() const { return nMasternodePingSeconds; }
    int64_t MasternodeExpirationSeconds() const { return nMasternodeExpirationSeconds; }    
    int64_t MasternodeRemovalSeconds() const { return nMasternodeRemovalSeconds; }
    int64_t MasternodeCheckSeconds() const { return nMasternodeCheckSeconds; }
    int64_t MasternodeCoinScore() const { return nMasternodeCoinScore; }
    int64_t MasternodeBudgetPaymentCycle() const { return nMasternodeBudgetPaymentCycle; }
    int64_t MasternodeFinalizationWindow() const { return nMasternodeFinalizationWindow; }
        
    CBaseChainParams::Network NetworkID() const { return networkID; }
    int LAST_POW_BLOCK() const { return nLastPOWBlock; }
    int Block_Enforce_Invalid() const { return nBlockEnforceInvalidUTXO; }
    int ModifierUpgradeBlock() const { return nModifierUpdateBlock; }

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
    int nEnforceBlockUpgradeMajority;
    int nRejectBlockOutdatedMajority;
    int nToCheckBlockUpgradeMajority;
    int64_t nTargetTimespan;
    int64_t nTargetSpacing;
    int64_t nStakeTargetSpacing;
    int64_t nPastBlocksMin; // used when calculating the NextWorkRequired 
    int64_t nPastBlocksMax;
    unsigned int nStakeMinAge;
    unsigned int nModifier;
    int64_t nClientMintibleCoinsInterval; // PoS mining
    int64_t nEnsureMintibleCoinsInterval;
    int nLastPOWBlock;
    int nMasternodeCountDrift;
    int nMaturity;
    int nModifierUpdateBlock;
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
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority) = 0;
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority) = 0;
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority) = 0;
    virtual void setDefaultConsistencyChecks(bool aDefaultConsistencyChecks) = 0;
    virtual void setAllowMinDifficultyBlocks(bool aAllowMinDifficultyBlocks) = 0;
    virtual void setSkipProofOfWorkCheck(bool aSkipProofOfWorkCheck) = 0;
    virtual void setHeightToFork(int aHeightToFork) = 0;
    virtual void setLastPOW(int aLastPOW) = 0;
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
