// Copyright (c) 2018 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blocksignature.h"
#include "init.h"
#include "main.h"
#include "miner.h"
#include "pubkey.h"
#include "script/sign.h"
#include "timedata.h"
#include "uint256.h"
#include "util.h"
#include "utilmoneystr.h"
#include "utiltime.h"
#include "wallet.h"

#include <boost/test/unit_test.hpp>
#include <cmath>
#include <random>

BOOST_AUTO_TEST_SUITE(pos_tests)

// #define RUN_INTEGRATION_TEST

static const string strSecret("5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj");
static const int WALLETS_AVAILABLE = 100;
static const int MASTERNODES_AVAILABLE = 20;

static CWallet* wallets = new CWallet[WALLETS_AVAILABLE];
static CAmount _supply = 2000000 * COIN;
static int walletCount = 1;
static CAmount currentSuply;
static std::default_random_engine generator;
static uint genesisTime;
int blockCount = 200;
int lastPoSWallet = -1;
int nextToLastPoSWallet = -1;

static CAmount* masternodes = new CAmount[MASTERNODES_AVAILABLE];
static int currMasternode = 0;

static std::map<int, int> lastBlockStaked;

static struct {
    unsigned int extraNonce;
    unsigned int nonce;
    unsigned int nTime;
} blockinfo[] = {
    {0, 0, 0}, {1, 4, 1547574894}, {2, 3, 1547574895}, {2, 1, 1547574895}, {1, 1, 1547574896}, {2, 4, 1547574896}, {2, 1, 1547574896},
    {1, 0, 1547574896}, {1, 3, 1547574897}, {1, 16, 1547574897}, {1, 0, 1547574897}, {2, 3, 1547574897}, {2, 16, 1547574897}, {1, 15, 1547574897},
    {1, 3, 1547574898}, {1, 4, 1547574898}, {1, 5, 1547574898}, {1, 8, 1547574898}, {1, 0, 1547574898}, {1, 0, 1547574898}, {1, 1, 1547574899},
    {1, 5, 1547574899}, {1, 12, 1547574899}, {1, 6, 1547574899}, {1, 0, 1547574899}, {1, 0, 1547574899}, {1, 21, 1547574900}, {1, 6, 1547574900},
    {1, 2, 1547574900}, {1, 8, 1547574900}, {1, 5, 1547574900}, {1, 3, 1547574900}, {1, 0, 1547574901}, {1, 19, 1547574901}, {1, 125, 1547574901},
    {1, 41, 1547574901}, {1, 19, 1547574901}, {1, 111, 1547574901}, {1, 28, 1547574902}, {1, 14, 1547574902}, {1, 230, 1547574902}, {1, 151, 1547574902},
    {1, 68, 1547574902}, {1, 102, 1547574902}, {1, 30, 1547574903}, {1, 145, 1547574903}, {1, 189, 1547574903}, {1, 108, 1547574903}, {1, 186, 1547574903},
    {1, 42, 1547574903}, {1, 124, 1547574904}, {1, 25, 1547574904}, {1, 32, 1547574904}, {1, 217, 1547574904}, {1, 127, 1547574904}, {1, 185, 1547574904},
    {1, 197, 1547574905}, {1, 84, 1547574905}, {1, 90, 1547574905}, {1, 56, 1547574905}, {1, 136, 1547574905}, {1, 220, 1547574905}, {1, 38, 1547574906},
    {1, 243, 1547574906}, {1, 24, 1547574906}, {1, 36, 1547574906}, {1, 99, 1547574906}, {1, 115, 1547574906}, {1, 24, 1547574907}, {1, 194, 1547574907},
    {1, 48, 1547574907}, {1, 127, 1547574907}, {1, 151, 1547574907}, {1, 36, 1547574907}, {1, 124, 1547574908}, {1, 232, 1547574908}, {1, 81, 1547574908},
    {1, 183, 1547574908}, {1, 224, 1547574908}, {1, 216, 1547574908}, {1, 76, 1547574909}, {1, 41, 1547574909}, {1, 79, 1547574909}, {1, 107, 1547574909},
    {1, 185, 1547574909}, {1, 16, 1547574909}, {1, 41, 1547574910}, {1, 3, 1547574910}, {1, 13, 1547574910}, {1, 16, 1547574910}, {1, 210, 1547574910},
    {1, 4, 1547574910}, {1, 48, 1547574911}, {1, 245, 1547574911}, {1, 51, 1547574911}, {1, 118, 1547574911}, {1, 223, 1547574911}, {1, 40, 1547574911},
    {1, 11, 1547574912}, {1, 199, 1547574912}, {1, 182, 1547574912}, {1, 25, 1547574912}, {1, 41, 1547574912}, {1, 129, 1547574912}, {1, 61, 1547574913},
    {1, 116, 1547574913}, {1, 196, 1547574913}, {1, 183, 1547574913}, {1, 247, 1547574913}, {1, 223, 1547574913}, {1, 83, 1547574914}, {1, 161, 1547574914},
    {1, 8, 1547574914}, {1, 249, 1547574914}, {1, 17, 1547574914}, {1, 24, 1547574914}, {1, 160, 1547574915}, {1, 146, 1547574915}, {1, 34, 1547574915},
    {1, 205, 1547574915}, {1, 122, 1547574915}, {1, 254, 1547574915}, {1, 191, 1547574916}, {1, 231, 1547574916}, {1, 224, 1547574916}, {1, 113, 1547574916},
    {1, 121, 1547574916}, {1, 147, 1547574916}, {1, 203, 1547574917}, {1, 194, 1547574917}, {1, 152, 1547574917}, {1, 48, 1547574917}, {1, 39, 1547574917},
    {1, 231, 1547574917}, {1, 155, 1547574918}, {1, 204, 1547574918}, {1, 50, 1547574918}, {1, 102, 1547574918}, {1, 182, 1547574918}, {1, 235, 1547574918},
    {1, 219, 1547574919}, {1, 230, 1547574919}, {1, 97, 1547574919}, {1, 194, 1547574919}, {1, 75, 1547574919}, {1, 224, 1547574920}, {1, 228, 1547574922},
    {1, 99, 1547574923}, {1, 38, 1547574924}, {1, 158, 1547574929}, {1, 104, 1547574929}, {1, 162, 1547574935}, {1, 117, 1547574935}, {1, 47, 1547574935},
    {1, 120, 1547574936}, {1, 124, 1547574937}, {1, 229, 1547574944}, {1, 52, 1547574945}, {1, 0, 1547574946}, {1, 181, 1547574946}, {1, 233, 1547574949},
    {1, 220, 1547574950}, {1, 223, 1547574952}, {1, 36, 1547574954}, {1, 176, 1547574955}, {1, 225, 1547574956}, {1, 234, 1547574959}, {1, 15, 1547574959},
    {1, 31, 1547574962}, {1, 67, 1547574968}, {1, 200, 1547574973}, {1, 34, 1547574974}, {1, 254, 1547574974}, {1, 251, 1547574975}, {1, 0, 1547574977},
    {1, 10, 1547574985}, {1, 85, 1547574991}, {1, 220, 1547574994}, {1, 65, 1547574994}, {1, 53, 1547574994}, {1, 25, 1547575007}, {1, 233, 1547575014},
    {1, 205, 1547575023}, {1, 82, 1547575024}, {1, 143, 1547575029}, {1, 46, 1547575030}, {1, 104, 1547575033}, {1, 174, 1547575035}, {1, 15, 1547575037},
    {1, 223, 1547575037}, {1, 26, 1547575038}, {1, 25, 1547575039}, {1, 106, 1547575039}, {1, 70, 1547575046}, {1, 121, 1547575046}, {1, 255, 1547575047},
    {1, 254, 1547575049}, {1, 214, 1547575050}, {1, 90, 1547575055}, {1, 185, 1547575055}, {2, 251, 1547575065}};

static std::vector<CRecipient> PopulateWalletByWealth(double numberOfWallets, double toDistribute)
{
    std::uniform_int_distribution<int> distribution(1, 25);
    std::vector<CRecipient> vecSend;

    printf("Distributing %.0f for %.0f wallets.\n", toDistribute, numberOfWallets);
    printf(" Creating transactions: 0%%");

    for (int i = 0; i < numberOfWallets; i++) {
        CAmount val = 0;

        if (i == numberOfWallets - 1) {
            val = toDistribute;
        } else {
            do {
                double mul = distribution(generator) / (double)100;
                val = mul * toDistribute;
            } while (toDistribute - val <= 0 && toDistribute - 10000 > 0);
        }

        if (val < 10000 && toDistribute - 10000 >= 0)
            val = 10000;


        // create walletDB name
        stringstream ss;
        ss << "wallet_" << walletCount << ".dat";
        wallets[walletCount].strWalletFile = ss.str();

        CWalletDB walletDB(wallets[walletCount].strWalletFile, "crw");

        if (val >= 10000) {
            CPubKey key;
            wallets[walletCount].GetKeyFromPool(key);
            CScript scriptPubKey = GetScriptForDestination(key.GetID());

            CRecipient recipient = {scriptPubKey, val, false};
            vecSend.push_back(recipient);
        }

        wallets[walletCount].nTimeFirstKey = genesisTime;
        wallets[walletCount].nStakeSetUpdateTime = 0;
        wallets[walletCount].nStakeSplitThreshold = 5000 * COIN;

        printf("\xd Creating transactions: %.2f%%", (i * 100) / numberOfWallets);

        toDistribute -= val;
        walletCount++;
    }

    printf("\xd Creating transactions: 100%% \t \n");
    return vecSend;
}

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet)
{
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("KOREMiner: generated block is stale");
    }

    // Process this block the same as if we had receiveMinerd it from another node
    CValidationState state;
    if (!ProcessNewBlock(state, NULL, pblock)) {
        return error("KOREMiner: ProcessNewBlock, block not accepted");
    }

    return true;
}

void UpdateMasternodeBalance(CAmount amount)
{
    if (currMasternode > MASTERNODES_AVAILABLE - 1)
        currMasternode = 0;

    masternodes[currMasternode] += amount;
    currMasternode++;
}

void StartPreMineAndWalletAllocation()
{
    CBlockTemplate* pblocktemplate;
    CBlockIndex* actualBlock = chainActive.Genesis();

    CBitcoinSecret bsecret;
    bsecret.SetString(strSecret);
    CKey key = bsecret.GetKey();
    CPubKey pubKey = key.GetPubKey();
    CKeyID keyID = pubKey.GetID();
    CScript script = GetScriptForDestination(keyID);

    wallets[0].strWalletFile = "wallet_0.dat";
    CWalletDB walletDB(wallets[0].strWalletFile, "crw");

    {
        LOCK(wallets[0].cs_wallet);
        wallets[0].AddKeyPubKey(key, pubKey);
    }
    wallets[0].SetDefaultKey(pubKey);
    wallets[0].nTimeFirstKey = actualBlock->nTime;
    wallets[0].fFileBacked = true;
    wallets[0].SetBroadcastTransactions(true);
    wallets[0].nStakeSplitThreshold = 5000 * COIN;

    int i = 1;
    int populate = 0;
    while (i <= blockCount) {
        pblocktemplate = CreateNewBlock(script, &wallets[0], false);

        CBlockIndex* pindexPrev = chainActive.Tip();
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience

        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        unsigned int nHashesDone = 0;

        pblock->nNonce = blockinfo[i].nonce;
        pblock->nTime = blockinfo[i].nTime;

        CMutableTransaction txCoinbase(pblock->vtx[0]);
        txCoinbase.vin[0].scriptSig = (CScript() << i << CScriptNum(blockinfo[i].extraNonce)) + COINBASE_FLAGS;
        pblock->vtx[0] = CTransaction(txCoinbase);
        pblock->hashMerkleRoot = pblock->BuildMerkleTree();

        CValidationState state;
        BOOST_CHECK(ProcessBlockFound(pblock, wallets[0]));

        if (i == 5 || i == 37 || i == 95 || i == 198) {
            wallets[0].ScanForWalletTransactions(actualBlock, true);
            actualBlock = chainActive[i];

            CWalletTx txNew;
            CReserveKey reserveKey(&wallets[0]);
            CAmount feeRate;
            string failReason;

            std::vector<CRecipient> vecSend;
            switch (populate) {
            case 0:
                vecSend = PopulateWalletByWealth(69, 39800 * COIN); //1388, 2, true);
                break;
            case 1:
                vecSend = PopulateWalletByWealth(20, 318400 * COIN); //900, 16);
                break;
            case 2:
                vecSend = PopulateWalletByWealth(9, 577100 * COIN); //90, 29);
                break;
            case 3:
                vecSend = PopulateWalletByWealth(1, 1054700 * COIN); //10, 53);
                break;
            default:
                printf("No transactions done for case %d in block %d.\n", populate, i);
                break;
            }

            if (wallets[0].CreateTransaction(vecSend, txNew, reserveKey, feeRate, failReason, (const CCoinControl*)__null, ALL_COINS, false, 0L)) {
                if (wallets[0].CommitTransaction(txNew, reserveKey)) {
                    printf("Transactions done for case %d in block %d.\n", populate, i);
                    populate++;
                }
            }
        }

        i++;
        delete pblocktemplate;
    }
}

#ifdef RUN_INTEGRATION_TEST

BOOST_AUTO_TEST_CASE(pos_integration)
{
    {
        LOCK(cs_main);
        Checkpoints::fEnabled = false;
    }

    CBlockIndex* genesisBlock = chainActive.Genesis();

    SelectParams(CBaseChainParams::UNITTEST);
    ModifiableParams()->setHeightToFork(0);
    ModifiableParams()->setLastPowBlock(200);

    // Mine 200 blocks and alocate funds to all wallets
    StartPreMineAndWalletAllocation();

    currentSuply = chainActive.Tip()->nMoneySupply;

    for (int i = 0; i < WALLETS_AVAILABLE; i++) {
        wallets[i].ScanForWalletTransactions(genesisBlock, true);
        printf("Balance for wallet %d is %s.\n", i, FormatMoney(wallets[i].GetBalance()).c_str());
    }

    std::uniform_int_distribution<int> distribution(0, WALLETS_AVAILABLE - 1);

    while (_supply < MAX_MONEY) {
        uint nExtraNonce = 0;
        int walletID = distribution(generator);

        CWallet* wallet = &wallets[walletID];
        CReserveKey reservekey(wallet);

        // BitcoinMiner(wallet, true);

        if (wallet->GetBalance() > 0 && wallet->MintableCoins()) {
            CBlockIndex* pindexPrev = chainActive.Tip();

            unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey, wallet, true));
            if (!pblocktemplate.get())
                continue;

            CBlock* pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            if (!SignBlock(*pblock, *wallet)) {
                printf("BitcoinMiner(): Signing new block with UTXO key failed \n");
                continue;
            }

            if (!ProcessBlockFound(pblock, *wallet))
                continue;

            blockCount++;

            if (nextToLastPoSWallet >= 0) {
                int last = lastBlockStaked[nextToLastPoSWallet];
                CAmount prevBalance = wallets[nextToLastPoSWallet].GetBalance();
                wallets[nextToLastPoSWallet].ScanForWalletTransactions(chainActive[last]);
                CAmount balance = wallets[nextToLastPoSWallet].GetBalance();
                printf("Wallet %d generated %s and has a balance of %s after mint on block %d.\n", nextToLastPoSWallet, FormatMoney(balance - prevBalance).c_str(), FormatMoney(balance).c_str(), last);
            }

            lastBlockStaked[walletID] = blockCount;
            nextToLastPoSWallet = lastPoSWallet;
            lastPoSWallet = walletID;

            _supply = chainActive.Tip()->nMoneySupply;
        }

        if (blockCount == 525600) break;
    }

    for (int i = 0; i < WALLETS_AVAILABLE; i++) {
        wallets[i].ScanForWalletTransactions(genesisBlock, true);
        printf("Final balance for wallet %d is %s.\n", i, FormatMoney(wallets[i].GetBalance()).c_str());
    }
}

#endif

static long nTime = GetTime();
static int nHeight = 1;

CMutableTransaction GetNewTransaction(CScript script, CAmount nvalue, bool fisMined = true, bool fisPoS = false)
{
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nTime = ++nTime;
    CTxIn txIn;
    if (fisMined) {
        txIn.prevout.SetNull();
        txIn.scriptSig = CScript() << nHeight << OP_0;
    }
    tx.vin.emplace_back(txIn);
    CTxOut txOut;
    txOut.scriptPubKey = script;
    txOut.nValue = nvalue;

    if (fisPoS) {
        CTxOut txOut2;
        txOut2.nValue = 0.1 * nvalue;
        txOut2.scriptPubKey = CScript() << OP_0;
        tx.vout.emplace_back(txOut2);

        txOut.nValue -= txOut2.nValue;
    }

    tx.vout.emplace_back(txOut);

    return tx;
}

CBlock GetNewPoWBlock(uint256 nprevBlockHash, CTransaction tx)
{
    CBlock block;
    block.nVersion = 1;
    block.nTime = ++nTime + 20;
    block.payee = tx.vout[0].scriptPubKey;
    block.fChecked = true;
    block.hashPrevBlock = nprevBlockHash;
    block.vtx.emplace_back(tx);
    block.hashMerkleRoot = block.BuildMerkleTree();

    CBlockIndex* idx = InsertBlockIndex(block.GetHash());
    idx->nVersion = block.nVersion;
    idx->hashMerkleRoot = block.hashMerkleRoot;
    idx->nTime = block.nTime;
    idx->nHeight = nHeight++;

    chainActive.SetTip(idx);

    return block;
}

CBlock GetNewPoSBlock(uint256 nprevBlockHash, CTransaction tx, CTransaction tx_stake, CScript script)
{
    CBlock block;
    block.nVersion = 1;
    block.nTime = ++nTime + 30;
    block.payee = script;
    block.fChecked = true;
    block.hashPrevBlock = nprevBlockHash;
    block.vtx.emplace_back(tx);
    block.vtx.emplace_back(tx_stake);
    block.hashMerkleRoot = block.BuildMerkleTree();

    CBlockIndex* idx = InsertBlockIndex(block.GetHash());
    idx->nVersion = block.nVersion;
    idx->hashMerkleRoot = block.hashMerkleRoot;
    idx->nTime = block.nTime;
    idx->nHeight = nHeight++;

    chainActive.SetTip(idx);

    return block;
}

CWalletTx AddToWallet(CWallet* wallet, CTransaction tx, CBlock block)
{
    CWalletTx wtx(wallet, tx);
    {
        LOCK(cs_main);
        wtx.SetMerkleBranch(block);
        wtx.fMerkleVerified = true;
    }

    CWalletDB walletDB(wallet->strWalletFile, "crw");

    wallet->AddToWallet(wtx, false, &walletDB);

    mempool.addUnchecked(tx.GetHash(), CTxMemPoolEntry(tx, 0, block.nTime, 100.0, nHeight));

    return wtx;
}

/*
** This test takes a long time to finish (49 seconds of sleep) because of the nature of a time locking transaction.
** We're creating PoW and PoS blocks here to test if the GetBalance method works as intended.
** The UnitTest ChainParams configuration states that any coinbase should be available after 1 block (nMaturity = 1)
** and we set the locking interval of a PoS transaction to 60 seconds (aStakeLockInterval = 60).
** Because of the SEQUENCE_LOCKTIME_GRANULARITY chosen (5 bits), we can only set the locking interval in increments of
** 32 seconds. The minimal locking time is 32 seconds and the maximum locking time is 65504 seconds or 1091 minutes or
** 18 hours.
** The balance is tested against the last coinbase sum up to that point.
** To test the addition of the locked coin to the balance, we must wait at least 32 seconds.
 */
BOOST_AUTO_TEST_CASE(pos_GetBalance)
{
    // Set ChainParams for the test
    SelectParams(CBaseChainParams::UNITTEST);
    ModifiableParams()->setHeightToFork(0);
    ModifiableParams()->setStakeLockInterval(60);

    CBitcoinSecret bsecret;
    bsecret.SetString(strSecret);
    CKey key = bsecret.GetKey();
    CPubKey pubKey = key.GetPubKey();
    CKeyID keyID = pubKey.GetID();
    CScript script = GetScriptForDestination(keyID);

    CWallet wallet;
    wallet.strWalletFile = "pos_GetBalance.dat";
    {
        LOCK(wallet.cs_wallet);
        wallet.AddKeyPubKey(key, pubKey);
    }
    CWalletDB walletDB(wallet.strWalletFile, "crw");

    // Add a PoW 5 KORE tx to the wallet
    CMutableTransaction tx1 = GetNewTransaction(script, 5 * COIN);
    CBlock block1 = GetNewPoWBlock(chainActive.Genesis()->GetBlockHash(), tx1);
    CWalletTx wtx1 = AddToWallet(&wallet, tx1, block1);
    // This coin will be available only on the next block
    BOOST_CHECK(wallet.GetBalance() == 0);

    sleep(2);

    // Add a PoW 5 KORE tx to the wallet
    CMutableTransaction tx2 = GetNewTransaction(script, 5 * COIN);
    CBlock block2 = GetNewPoWBlock(block1.GetHash(), tx2);
    CWalletTx wtx2 = AddToWallet(&wallet, tx2, block2);
    // We're checking the balance against first coin
    BOOST_CHECK(wallet.GetBalance() == 5 * COIN);

    sleep(2);

    // Spend the first coin
    CMutableTransaction tx3 = GetNewTransaction(CScript() << OP_0, 5 * COIN, false);
    tx3.vin[0].prevout = COutPoint(tx1.GetHash(), 0);
    SignSignature(wallet, wtx1, tx3, 0);
    CBlock block3 = GetNewPoWBlock(block2.GetHash(), tx3);
    CWalletTx wtx3 = AddToWallet(&wallet, tx3, block3);
    // We're checking the balance against the second coin
    BOOST_CHECK(wallet.GetBalance() == 5 * COIN);

    sleep(2);

    // Lock the second coin as stake
    CMutableTransaction tx4 = GetNewTransaction(script, 10 * COIN, true, true);
    CMutableTransaction tx4_stake = GetNewTransaction(CScript() << Params().StakeLockInterval() << OP_CHECKSEQUENCEVERIFY << OP_DROP << pubKey << OP_CHECKSIG, 5 * COIN, false);
    tx4_stake.vin[0].prevout = COutPoint(tx2.GetHash(), 0);
    SignSignature(wallet, wtx2, tx4_stake, 0);
    CBlock block4 = GetNewPoSBlock(block3.GetHash(), tx4, tx4_stake, script);
    CWalletTx wtx4 = AddToWallet(&wallet, tx4, block4);
    CWalletTx wtx4_stake = AddToWallet(&wallet, tx4_stake, block4);
    // The balance should be 0 until the next block
    BOOST_CHECK(wallet.GetBalance() == 0);

    sleep(13);

    // Add a PoW 5 KORE tx to the wallet
    CMutableTransaction tx5 = GetNewTransaction(script, 5 * COIN);
    CBlock block5 = GetNewPoWBlock(block4.GetHash(), tx5);
    CWalletTx wtx5 = AddToWallet(&wallet, tx5, block5);
    // We're checking the balance against the PoS coin
    BOOST_CHECK(wallet.GetBalance() == 9 * COIN);

    sleep(13);

    // Add a PoW 5 KORE tx to the wallet
    CMutableTransaction tx6 = GetNewTransaction(script, 5 * COIN);
    CBlock block6 = GetNewPoWBlock(block5.GetHash(), tx6);
    CWalletTx wtx6 = AddToWallet(&wallet, tx6, block6);
    // We're checking the balance against the last PoW coin
    BOOST_CHECK(wallet.GetBalance() == 14 * COIN);

    sleep(17);

    // Add a PoW 5 KORE tx to the wallet
    CMutableTransaction tx7 = GetNewTransaction(script, 5 * COIN);
    CBlock block7 = GetNewPoWBlock(block6.GetHash(), tx7);
    CWalletTx wtx7 = AddToWallet(&wallet, tx7, block7);
    // We're checking the balance against the expired lock coin
    BOOST_CHECK(wallet.GetBalance() == 24 * COIN);
}

BOOST_AUTO_TEST_SUITE_END()