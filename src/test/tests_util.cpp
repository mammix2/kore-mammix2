

#include "arith_uint256.h"
#include "blocksignature.h"
#include "legacy/consensus/merkle.h"
#include "main.h"
#include "miner.h"
#include "primitives/block.h"
#include "pubkey.h"
#include "tests_util.h"
#include "txdb.h"
#include "uint256.h"
#include "util.h"
#include "utiltime.h"
#include "validationinterface.h"
#include "wallet.h"


#include <boost/test/unit_test.hpp>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>


CWallet* pwalletMain;

static CCoinsViewDB* pcoinsdbview = NULL;

CCoinsViewCache* SaveDatabaseState()
{
    // This method is based on the PrepareShutdown
    
    if (pwalletMain)
        bitdb.Flush(false);        

    {
        LOCK(cs_main);
        CCoinsViewCache* savedPcoinsTip = pcoinsTip;
        if (pcoinsTip != NULL) {
            FlushStateToDisk();

            //record that client took the proper shutdown procedure
            pblocktree->WriteFlag("shutdown", true);
        }

        delete pcoinsTip;
        pcoinsTip = NULL;
    }

    //if (pwalletMain)
    //    bitdb.Flush(true);

    //delete pwalletMain;
    //pwalletMain = NULL;
        
}

bool ReadDatabaseState()
{
    string strBlockIndexError = "";
    std::string strLoadError;
    UnloadBlockIndex();



/*
  Gives segmentation fault ! 
#ifdef ENABLE_WALLET
    bitdb.MakeMock();
#endif
*/
    pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pblocktree = new CBlockTreeDB(1 << 20, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);

    if (!LoadBlockIndex()) {
        strLoadError = _("Error loading block database");
        strLoadError = strprintf("%s : %s", strLoadError, strBlockIndexError);
        LogPrintf("ReadDatabaseState %s \n", strLoadError);
        return false;
    }

    // Initialize the block index (no-op if non-empty database was already loaded)
    if (!InitBlockIndex()) {
        strLoadError = _("Error initializing block database");
        LogPrintf("ReadDatabaseState %s \n", strLoadError);
        return false;
    }
    if (!CVerifyDB().VerifyDB(pcoinsdbview, GetArg("-checklevel", DEFAULT_CHECKLEVEL),
            GetArg("-checkblocks", DEFAULT_CHECKBLOCKS))) {
        strLoadError = _("Corrupted block database detected");
        LogPrintf("ReadDatabaseState %s \n", strLoadError);
        return false;
    }

    //bool fFirstRun;
    //pwalletMain = new CWallet("wallet.dat");
    //pwalletMain->LoadWallet(fFirstRun);
    

    return true;
}

void CheckDatabaseState(CWallet* pwalletMain)
{
    SaveDatabaseState();

    BOOST_CHECK(ReadDatabaseState());
}

void InitializeDBTest()
{
    
#ifdef ENABLE_WALLET
    bitdb.MakeMock();
#endif
    pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pblocktree = new CBlockTreeDB(1 << 20, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    InitBlockIndex();
#ifdef ENABLE_WALLET
    bool fFirstRun;
    pwalletMain = new CWallet("wallet.dat");
    pwalletMain->LoadWallet(fFirstRun);
    RegisterValidationInterface(pwalletMain);
#endif
}

void FinalizeDBTest(bool shutdown) 
{
#ifdef ENABLE_WALLET
    bitdb.Flush(shutdown);
    //bitdb.Close();
#endif
    delete pcoinsTip;
    delete pcoinsdbview;
    delete pblocktree;
#ifdef ENABLE_WALLET
    UnregisterValidationInterface(pwalletMain);
    delete pwalletMain;
    pwalletMain = NULL;
#endif
}

/*
CBlock ============================>>>>
    hash=0c4b7b9d1ee2b602889210f846c5aad1136d804b2c8b9123bf0fc3d09b20dc59
    ver=1
    hashPrevBlock=0aab10677b4fe0371a67f99e78a69e7d9fa03a1c7d48747978da405dc5abeb99,
    hashMerkleRoot=93fa82e59aec389489e594a24c0765f51a82409b246a4002742c0e976f9b23f3,
    nTime=1553349553,
    nBits=201fffff,
    nNonce=3,
    nBirthdayA=103360,
    nBirthdayB=47071114,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=93fa82e59a, ver=2,  nTime=1553349553, vin.size=1, vout.size=3, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 510101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)
    CTxOut(nValue=0.00000000, scriptPubKey=1e43726561746564206f6e2076657273696f6e203133207072652d666f726b6a)

  vMerkleTree:     vMerkleTree : size 1
    93fa82e59aec389489e594a24c0765f51a82409b246a4002742c0e976f9b23f3
CBlock <<<<============================
{0, 1553349553, 1553349553 , 538968063 , 3 , 1 , 103360 , 47071114 , uint256("0c4b7b9d1ee2b602889210f846c5aad1136d804b2c8b9123bf0fc3d09b20dc59") , uint256("93fa82e59aec389489e594a24c0765f51a82409b246a4002742c0e976f9b23f3") , 0 }, // Block 1
CBlock ============================>>>>
    hash=028b879ab893ad10346f917269df71e6859454048dde5e175ae4a4545ab83699
    ver=1
    hashPrevBlock=0c4b7b9d1ee2b602889210f846c5aad1136d804b2c8b9123bf0fc3d09b20dc59,
    hashMerkleRoot=9063abcde8e0eb929aa10d4a99255ab9fee6c327a223c6606c3902fc58fa62cf,
    nTime=1553349573,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=18592727,
    nBirthdayB=66015663,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=9063abcde8, ver=2,  nTime=1553349573, vin.size=1, vout.size=3, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 520101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)
    CTxOut(nValue=0.00000000, scriptPubKey=1e43726561746564206f6e2076657273696f6e203133207072652d666f726b6a)

  vMerkleTree:     vMerkleTree : size 1
    9063abcde8e0eb929aa10d4a99255ab9fee6c327a223c6606c3902fc58fa62cf
CBlock <<<<============================
{0, 1553349573, 1553349573 , 538968063 , 1 , 1 , 18592727 , 66015663 , uint256("028b879ab893ad10346f917269df71e6859454048dde5e175ae4a4545ab83699") , uint256("9063abcde8e0eb929aa10d4a99255ab9fee6c327a223c6606c3902fc58fa62cf") , 0 }, // Block 2
CBlock ============================>>>>
    hash=05ac7e4e70c54ad567d8bd6493abdd0bddd24c4a15d7ebc766ff5b2a02068351
    ver=1
    hashPrevBlock=028b879ab893ad10346f917269df71e6859454048dde5e175ae4a4545ab83699,
    hashMerkleRoot=6d34d59c695e735ec39ff84c2bf785874e4254c2efb69b8c3314ddb96affb77f,
    nTime=1553349593,
    nBits=201fffff,
    nNonce=14,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=6d34d59c69, ver=2,  nTime=1553349593, vin.size=1, vout.size=3, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 530101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)
    CTxOut(nValue=0.00000000, scriptPubKey=1e43726561746564206f6e2076657273696f6e203133207072652d666f726b6a)

  vMerkleTree:     vMerkleTree : size 1
    6d34d59c695e735ec39ff84c2bf785874e4254c2efb69b8c3314ddb96affb77f
CBlock <<<<============================
{0, 1553349593, 1553349593 , 538968063 , 14 , 1 , 0 , 0 , uint256("05ac7e4e70c54ad567d8bd6493abdd0bddd24c4a15d7ebc766ff5b2a02068351") , uint256("6d34d59c695e735ec39ff84c2bf785874e4254c2efb69b8c3314ddb96affb77f") , 0 }, // Block 3
CBlock ============================>>>>
    hash=09bfeea176cd27daef27ac873ac786a7ff5439c7ce5c0449e441ed58dca85ebf
    ver=1
    hashPrevBlock=05ac7e4e70c54ad567d8bd6493abdd0bddd24c4a15d7ebc766ff5b2a02068351,
    hashMerkleRoot=561dbcbee44b70c94823ef417714ec6fa8dcb01564e978e0683d7e85228f3a74,
    nTime=1553349613,
    nBits=201fffff,
    nNonce=1,
    nBirthdayA=0,
    nBirthdayB=0,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=561dbcbee4, ver=2,  nTime=1553349613, vin.size=1, vout.size=3, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 540101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)
    CTxOut(nValue=0.00000000, scriptPubKey=1e43726561746564206f6e2076657273696f6e203133207072652d666f726b6a)

  vMerkleTree:     vMerkleTree : size 1
    561dbcbee44b70c94823ef417714ec6fa8dcb01564e978e0683d7e85228f3a74
CBlock <<<<============================
{0, 1553349613, 1553349613 , 538968063 , 1 , 1 , 0 , 0 , uint256("09bfeea176cd27daef27ac873ac786a7ff5439c7ce5c0449e441ed58dca85ebf") , uint256("561dbcbee44b70c94823ef417714ec6fa8dcb01564e978e0683d7e85228f3a74") , 900000000000 }, // Block 4
CBlock ============================>>>>
    hash=0bf516d19099a85a641e22c441000ced92dc33287d2000e4c20f5e33cacc110b
    ver=1
    hashPrevBlock=09bfeea176cd27daef27ac873ac786a7ff5439c7ce5c0449e441ed58dca85ebf,
    hashMerkleRoot=53c7f9eb1fa3ef94cb2f0de80022c56e8e6550cb8dd56ac7f159acda7083c2b0,
    nTime=1553349633,
    nBits=201fffff,
    nNonce=4,
    nBirthdayA=14417357,
    nBirthdayB=64909763,
    vchBlockSig=,
    Vtx : size 1
    CTransaction(hash=53c7f9eb1f, ver=2,  nTime=1553349633, vin.size=1, vout.size=3, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 550101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)
    CTxOut(nValue=0.00000000, scriptPubKey=1e43726561746564206f6e2076657273696f6e203133207072652d666f726b6a)

  vMerkleTree:     vMerkleTree : size 1
    53c7f9eb1fa3ef94cb2f0de80022c56e8e6550cb8dd56ac7f159acda7083c2b0
CBlock <<<<============================
{0, 1553349633, 1553349633 , 538968063 , 4 , 1 , 14417357 , 64909763 , uint256("0bf516d19099a85a641e22c441000ced92dc33287d2000e4c20f5e33cacc110b") , uint256("53c7f9eb1fa3ef94cb2f0de80022c56e8e6550cb8dd56ac7f159acda7083c2b0") , 1800000000000 }, // Block 5
*/

blockinfo_t blockinfo[] = {
{0, 1553349553, 1553349553 , 538968063 , 3 , 1 , 103360 , 47071114 , uint256("0c4b7b9d1ee2b602889210f846c5aad1136d804b2c8b9123bf0fc3d09b20dc59") , uint256("93fa82e59aec389489e594a24c0765f51a82409b246a4002742c0e976f9b23f3") , 0 }, // Block 1
{0, 1553349573, 1553349573 , 538968063 , 1 , 1 , 18592727 , 66015663 , uint256("028b879ab893ad10346f917269df71e6859454048dde5e175ae4a4545ab83699") , uint256("9063abcde8e0eb929aa10d4a99255ab9fee6c327a223c6606c3902fc58fa62cf") , 0 }, // Block 2
{0, 1553349593, 1553349593 , 538968063 , 14 , 1 , 0 , 0 , uint256("05ac7e4e70c54ad567d8bd6493abdd0bddd24c4a15d7ebc766ff5b2a02068351") , uint256("6d34d59c695e735ec39ff84c2bf785874e4254c2efb69b8c3314ddb96affb77f") , 0 }, // Block 3
{0, 1553349613, 1553349613 , 538968063 , 1 , 1 , 0 , 0 , uint256("09bfeea176cd27daef27ac873ac786a7ff5439c7ce5c0449e441ed58dca85ebf") , uint256("561dbcbee44b70c94823ef417714ec6fa8dcb01564e978e0683d7e85228f3a74") , 900000000000 }, // Block 4
{0, 1553349633, 1553349633 , 538968063 , 4 , 1 , 14417357 , 64909763 , uint256("0bf516d19099a85a641e22c441000ced92dc33287d2000e4c20f5e33cacc110b") , uint256("53c7f9eb1fa3ef94cb2f0de80022c56e8e6550cb8dd56ac7f159acda7083c2b0") , 1800000000000 }, // Block 5
};


void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake, bool logToStdout)
{
    std::stringstream str;
    str << "{" << fProofOfStake << ", ";
    str << pblock->nTime << ", ";
    str << pblock->vtx[0].nTime << " , ";
    str << pblock->nBits << " , ";
    str << pblock->nNonce << " , ";
    str << nExtraNonce << " , ";
    str << pblock->nBirthdayA << " , ";
    str << pblock->nBirthdayB << " , ";
    str << "uint256(\"" << pblock->GetHash().ToString().c_str() << "\") , ";
    str << "uint256(\"" << pblock->hashMerkleRoot.ToString().c_str() << "\") , ";
    str << pwallet->GetBalance() << " },";
    str << " // Block " << blockNumber << endl;

    if (logToStdout) {
        cout << pblock->ToString().c_str() << endl;
        cout << str.str();
    }

    if (fDebug) {
        LogPrintf("%s \n", str.str());
        LogPrintf("Block %d %s \n", blockNumber, (pblock->IsProofOfStake() ? " (PoS) " : " (PoW) "));
        LogPrintf(" nTime               : %u \n", pblock->nTime);
        LogPrintf(" hash                : %s \n", pblock->GetHash().ToString().c_str());
        LogPrintf(" StakeModifier       : %u \n", chainActive.Tip()->nStakeModifier);
        LogPrintf(" OldStakeModifier    : %s \n", chainActive.Tip()->nStakeModifierOld.ToString());
        LogPrintf(" Modifier Generated? : %s \n", (chainActive.Tip()->GeneratedStakeModifier() ? "True" : "False"));
        LogPrintf(" Balance             : %d \n", pwallet->GetBalance());
        LogPrintf(" Unconfirmed Balance : %d \n", pwallet->GetUnconfirmedBalance());
        LogPrintf(" Immature  Balance   : %d \n", pwallet->GetImmatureBalance());
        LogPrintf(" ---- \n");
    }
}

void InitializeLastCoinStakeSearchTime(CWallet* pwallet, CScript& scriptPubKey)
{
    const CChainParams& chainparams = Params();

    // this is just to initialize nLastCoinStakeSearchTime
    unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
    if (!pblocktemplate.get())
        return;
    CBlock* pblock = &pblocktemplate->block;
    SignBlock_Legacy(pwallet, pblock);
    SetMockTime(GetTime() + 30);
}


CScript GenerateSamePubKeyScript4Wallet( const string & secret, CWallet* pwallet )
{
    CBitcoinSecret bsecret;
    bsecret.SetString(secret);
    CKey key = bsecret.GetKey();
    CPubKey pubKey = key.GetPubKey();
    CKeyID keyID = pubKey.GetID();
    CScript scriptPubKey = GetScriptForDestination(keyID);

    //pwallet->NewKeyPool();
    LOCK(pwallet->cs_wallet);
    pwallet->AddKeyPubKey(key, pubKey);
    pwallet->SetDefaultKey(pubKey);

    if(fDebug) { 
        LogPrintf("pub key used      : %s \n", scriptPubKey.ToString()); 
        LogPrintf("pub key used (hex): %x \n", HexStr(scriptPubKey)); 
    }

    return scriptPubKey;
}


void ScanForWalletTransactions(CWallet* pwallet)
{
    pwallet->nTimeFirstKey = chainActive[0]->nTime;
    // pwallet->fFileBacked = true;
    // CBlockIndex* genesisBlock = chainActive[0];
    // pwallet->ScanForWalletTransactions(genesisBlock, true);
}

void GenerateBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake, bool logToStdout)
{
    bool fGenerateBitcoins = false;
    bool fMintableCoins = false;
    int nMintableLastCheck = 0;
    CReserveKey reservekey(pwallet); // Lico, once we want to use the same pubkey, we dont need to remove it from key pool

    // Each thread has its own key and counter
    unsigned int nExtraNonce = 0;

    int oldnHeight = chainActive.Tip()->nHeight;

    for (int j = startBlock; j < endBlock;) {
        SetMockTime(GetTime() + Params().GetTargetSpacing());
        if (fProofOfStake) {
            //control the amount of times the client will check for mintable coins
            if ((GetTime() - nMintableLastCheck > Params().GetClientMintableCoinsInterval())) {
                nMintableLastCheck = GetTime();
                fMintableCoins = pwallet->MintableCoins();
            }

            while (pwallet->IsLocked() || !fMintableCoins ||
                   (pwallet->GetBalance() > 0 && nReserveBalance >= pwallet->GetBalance())) {
                nLastCoinStakeSearchInterval = 0;
                // Do a separate 1 minute check here to ensure fMintableCoins is updated
                if (!fMintableCoins) {
                    if (GetTime() - nMintableLastCheck > Params().GetEnsureMintableCoinsInterval()) // 1 minute check time
                    {
                        nMintableLastCheck = GetTime();
                        fMintableCoins = pwallet->MintableCoins();
                    }
                }

                MilliSleep(5000);
                boost::this_thread::interruption_point();

                if (!fGenerateBitcoins && !fProofOfStake) {
                    //cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }

            if (mapHashedBlocks.count(chainActive.Tip()->nHeight)) //search our map of hashed blocks, see if bestblock has been hashed yet
            {
                if (GetTime() - mapHashedBlocks[chainActive.Tip()->nHeight] < max(pwallet->nHashInterval, (unsigned int)1)) // wait half of the nHashDrift with max wait of 3 minutes
                {
                    SetMockTime(GetTime() + 5);
                    //cout << "BitcoinMiner Going out of Loop !!!" << endl;
                    continue;
                }
            }
        }

        //
        // Create new block
        //
        //cout << "KOREMiner: Creating new Block " << endl;
        if (fDebug) {
            LogPrintf("vNodes Empty  ? %s \n", vNodes.empty() ? "true" : "false");
            LogPrintf("Wallet Locked ? %s \n", pwallet->IsLocked() ? "true" : "false");
            LogPrintf("Is there Mintable Coins ? %s \n", fMintableCoins ? "true" : "false");
            LogPrintf("Do we have Balance ? %s \n", pwallet->GetBalance() > 0 ? "true" : "false");
            LogPrintf("Balance is Greater than reserved one ? %s \n", nReserveBalance >= pwallet->GetBalance() ? "true" : "false");
        }
        unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrev = chainActive.Tip();
        if (!pindexPrev) {
            continue;
        }


        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
        // need to create a new block
        //BOOST_CHECK(pblocktemplate.get());
        if (!pblocktemplate.get())
        {
            SetMockTime(GetTime() + (Params().GetTargetSpacing() * 2));
            continue;
        }
        CBlock* pblock = &pblocktemplate->block;
        IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);
        //Stake miner main
        if (fProofOfStake) {
            //cout << "CPUMiner : proof-of-stake block found " << pblock->GetHash().ToString() << endl;
            if (!SignBlock(*pblock, *pwallet)) {
                //cout << "BitcoinMiner(): Signing new block with UTXO key failed" << endl;
                continue;
            }
            //cout << "CPUMiner : proof-of-stake block was signed " << pblock->GetHash().ToString() << endl;
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
            LogBlockFound(pwallet, j, pblock, nExtraNonce, fProofOfStake, logToStdout);
            j++;
            continue;
        }

        //
        // Search
        //
        int64_t nStart = GetTime();
        uint256 hashTarget = uint256().SetCompact(pblock->nBits);
        //cout << "target: " << hashTarget.GetHex() << endl;
        while (true) {
            unsigned int nHashesDone = 0;

            uint256 hash;

            //cout << "nbits : " << pblock->nBits << endl;
            while (true) {
                hash = pblock->GetHash();
                //cout << "pblock.nBirthdayA: " << pblock->nBirthdayA << endl;
                //cout << "pblock.nBirthdayB: " << pblock->nBirthdayB << endl;
                //cout << "hash             : " << hash.ToString() << endl;
                //cout << "hashTarget       : " << hashTarget.ToString() << endl;

                if (hash <= hashTarget) {
                    // Found a solution
                    //cout << "BitcoinMiner:" << endl;
                    //cout << "proof-of-work found  "<< endl;
                    //cout << "hash  : " << hash.GetHex() << endl;
                    //cout << "target: " << hashTarget.GetHex() << endl;
                    BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
                    LogBlockFound(pwallet, j, pblock, nExtraNonce, fProofOfStake, logToStdout);

                    // In regression test mode, stop mining after a block is found. This
                    // allows developers to controllably generate a block on demand.
                    // if (Params().ShouldMineBlocksOnDemand())
                    //    throw boost::thread_interrupted();
                    break;
                }
                pblock->nNonce += 1;
                nHashesDone += 1;
                //cout << "Looking for a solution with nounce " << pblock->nNonce << " hashesDone : " << nHashesDone << endl;
                if ((pblock->nNonce & 0xFF) == 0)
                    break;
            }

            // Check for stop or if block needs to be rebuilt
            boost::this_thread::interruption_point();
            // Regtest mode doesn't require peers
            if (vNodes.empty() && Params().DoesMiningRequiresPeers())
                break;
            if (pblock->nNonce >= 0xffff0000)
                break;
            if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                break;
            if (pindexPrev != chainActive.Tip())
                break;

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev, fProofOfStake);
            // Changing pblock->nTime can change work required on testnet:
            hashTarget.SetCompact(pblock->nBits);
        }
        j++;
    }

    // lets check if we have generated the munber of blocks requested
    BOOST_CHECK(oldnHeight + endBlock - startBlock == chainActive.Tip()->nHeight);
}

void GeneratePOWLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey, bool logToStdout)
{
    const CChainParams& chainparams = Params();
    unsigned int nExtraNonce = 0;

    for (int j = startBlock; j < endBlock; j++) {
        SetMockTime(GetTime() + (Params().GetTargetSpacing() * 2));
        int lastBlock = chainActive.Tip()->nHeight;
        CAmount oldBalance = pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance();
        // Let-s make sure we have the correct spacing
        //MilliSleep(Params().GetTargetSpacing()*1000);
        bool foundBlock = false;
        //
        // Create new block
        //
        CBlockIndex* pindexPrev = chainActive.Tip();

        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, NULL, false));

        if (!pblocktemplate.get()) {
            //cout << "Error in KoreMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread" << endl;
            return;
        }
        CBlock* pblock = &pblocktemplate->block;
        IncrementExtraNonce_Legacy(pblock, pindexPrev, nExtraNonce);

        LogPrintf("Running KoreMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
            ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

        //
        // Search
        //
        int64_t nStart = GetTime();
        arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
        uint256 testHash;

        for (; !foundBlock;) {
            unsigned int nHashesDone = 0;
            unsigned int nNonceFound = (unsigned int)-1;

            for (int i = 0; i < 1; i++) {
                pblock->nNonce = pblock->nNonce + 1;
                testHash = pblock->CalculateBestBirthdayHash();
                nHashesDone++;
                //cout << "proof-of-work found  "<< endl;
                //cout << "testHash  : " << UintToArith256(testHash).ToString() << endl;
                //cout << "target    : " << hashTarget.GetHex() << endl;
                if (fDebug) {
                    LogPrintf("testHash : %s \n", UintToArith256(testHash).ToString());
                    LogPrintf("target   : %s \n", hashTarget.ToString());
                }

                if (UintToArith256(testHash) < hashTarget) {
                    // Found a solution
                    nNonceFound = pblock->nNonce;
                    // Found a solution
                    assert(testHash == pblock->GetHash());
                    foundBlock = true;
                    ProcessBlockFound_Legacy(pblock, chainparams);
                    // We have our data, lets print them
                    LogBlockFound(pwallet, j, pblock, nExtraNonce, false, logToStdout);

                    break;
                }
            }

            // Update nTime every few seconds
            UpdateTime(pblock, pindexPrev);
        }

        // a new block was created
        BOOST_CHECK(chainActive.Tip()->nHeight == lastBlock + 1);
        // lets check if the block was created and if the balance is correct
        CAmount bValue = GetBlockValue(chainActive.Tip()->nHeight);
        // 10% to dev fund, we don't have masternode
        if(fDebug) {
            LogPrintf("Checking balance is %s \n", (pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance() == oldBalance + bValue * 0.9 ? "OK" : "NOK"));
        }
        BOOST_CHECK(pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance() == oldBalance + bValue * 0.9);
    }
}

void GeneratePOSLegacyBlocks(int startBlock, int endBlock, CWallet* pwallet, CScript& scriptPubKey, bool logToStdout)
{
    const CChainParams& chainparams = Params();

    InitializeLastCoinStakeSearchTime(pwallet, scriptPubKey);

    for (int j = startBlock; j < endBlock; j++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, true));
        if (!pblocktemplate.get())
            return;
        CBlock* pblock = &pblocktemplate->block;
        if (SignBlock_Legacy(pwallet, pblock)) {
            if (ProcessBlockFound_Legacy(pblock, chainparams)) {
                // we dont have extranounce for pos
                LogBlockFound(pwallet, j, pblock, 0, true, logToStdout);
                // Let's wait to generate the nextBlock
                SetMockTime(GetTime() + Params().GetTargetSpacing());
            } else {
                //cout << "NOT ABLE TO PROCESS BLOCK :" << j << endl;
            }
        }
    }
}

void Create_Transaction(CBlock* pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

void Create_NewTransaction(CBlock* pblock, const CBlockIndex* pindexPrev, const blockinfo_t blockinfo[], int i)
{
    // This method simulates the transaction creation, similar to IncrementExtraNonce_Legacy
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(blockinfo[i].extranonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);
    // lets update the time in order to simulate the creation
    //txCoinbase.nTime = blockinfo[i].transactionTime;
    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

void CreateOldBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t& blockInfo, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake, bool logToStdout)
{
    CBlockTemplate* pblocktemplate;
    const CChainParams& chainparams = Params();

    CBlockIndex* pindexPrev = chainActive.Tip();


    std::vector<CTransaction*> txFirst;
    for (int i = startBlock - 1; i < endBlock - 1; i++) {
        // Simple block creation, nothing special yet:
        BOOST_CHECK(pblocktemplate = CreateNewBlock_Legacy(chainparams, scriptPubKey, pwallet, fProofOfStake));
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = CBlockHeader::CURRENT_VERSION;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        if (!fProofOfStake) {
            pblock->nTime = blockinfo[i].nTime;
            Create_Transaction(pblock, pindexPrev, blockinfo, i);
        }
        pblock->nNonce = blockinfo[i].nonce;
        pblock->nBirthdayA = blockinfo[i].nBirthdayA;
        pblock->nBirthdayB = blockinfo[i].nBirthdayB;
        CValidationState state;
        /*
        cout << "Found Block === " << i+1 << " === " << endl;
        cout << "nTime         : " << pblock->nTime << endl;
        cout << "nNonce        : " << pblock->nNonce << endl;
        cout << "extranonce    : " << blockinfo[i].extranonce << endl;
        cout << "nBirthdayA    : " << pblock->nBirthdayA << endl;
        cout << "nBirthdayB    : " << pblock->nBirthdayB << endl;
        cout << "nBits         : " << pblock->nBits << endl;
        cout << "Hash          : " << pblock->GetHash().ToString().c_str() << endl;
        cout << "hashMerkleRoot: " << pblock->hashMerkleRoot.ToString().c_str()  << endl;
        cout << "New Block values" << endl;
        */

        if (fProofOfStake) {
            BOOST_CHECK(SignBlock_Legacy(pwallet, pblock));
            //cout << pblock->ToString() << endl;
            //cout << "scriptPubKey: " << HexStr(scriptPubKey) << endl;
            // the coin selected can be a different one, so the hash will be different
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        } else {
            //cout << pblock->ToString() << endl;
            BOOST_CHECK(scriptPubKey == pblock->vtx[0].vout[0].scriptPubKey);
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessNewBlock_Legacy(state, chainparams, NULL, pblock, true, NULL));
        }
        LogBlockFound(pwallet, i + 1, pblock, blockinfo[i].extranonce, fProofOfStake, logToStdout);
        BOOST_CHECK(state.IsValid());
        // we should get the same balance, depends the maturity
        // cout << "Block: " << i+1 << " time ("<< pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        // BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance() + pwallet->GetImmatureBalance() + pwallet->GetUnconfirmedBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
        if (pblocktemplate)
            delete pblocktemplate;
    }
}

void createNewBlocksFromBlockInfo(int startBlock, int endBlock, blockinfo_t& blockInfo, CWallet* pwallet, CScript& scriptPubKey, bool fProofOfStake, bool logToStdout)
{
    CBlockTemplate* pblocktemplate;
    const CChainParams& chainparams = Params();
    CReserveKey reservekey(pwallet); // only for consistency !!!


    CBlockIndex* pindexPrev = chainActive.Tip();

    std::vector<CTransaction*> txFirst;
    for (int i = startBlock - 1; i < endBlock - 1; i++) {
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(scriptPubKey, pwallet, fProofOfStake));
        assert(pblocktemplate.get() != NULL);
        CBlock* pblock = &pblocktemplate->block; // pointer for convenience
        pblock->nVersion = CBlockHeader::CURRENT_VERSION;
        //pblock->nTime = blockinfo[i].nTime;
        pblock->nBits = blockinfo[i].nBits;
        // Lets create the transaction
        if (!fProofOfStake)
            Create_NewTransaction(pblock, pindexPrev, blockinfo, i);
        pblock->nNonce = blockinfo[i].nonce;
        pblock->nBirthdayA = blockinfo[i].nBirthdayA;
        pblock->nBirthdayB = blockinfo[i].nBirthdayB;
        CValidationState state;
        //cout << "Found Block === " << i+1 << " === " << endl;
        //cout << "nTime         : " << pblock->nTime << endl;
        //cout << "nNonce        : " << pblock->nNonce << endl;
        //cout << "extranonce    : " << blockinfo[i].extranonce << endl;
        //cout << "nBirthdayA    : " << pblock->nBirthdayA << endl;
        //cout << "nBirthdayB    : " << pblock->nBirthdayB << endl;
        //cout << "nBits         : " << pblock->nBits << endl;
        //cout << "Hash          : " << pblock->GetHash().ToString().c_str() << endl;
        //cout << "hashMerkleRoot: " << pblock->hashMerkleRoot.ToString().c_str()  << endl;
        //cout << "New Block values" << endl;

        if (fProofOfStake) {
            BOOST_CHECK(SignBlock(*pblock, *pwallet));
            //cout << pblock->ToString() << endl;
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
        } else {
            //cout << pblock->ToString() << endl;
            //BOOST_CHECK(scriptPubKey == pblock->vtx[0].vout[0].scriptPubKey);
            // previous block hash is not the same
            //BOOST_CHECK(pblock->GetHash() == blockinfo[i].hash);
            //BOOST_CHECK(pblock->hashMerkleRoot == blockinfo[i].hashMerkleRoot);
            BOOST_CHECK(ProcessBlockFound(pblock, *pwallet, reservekey));
        }

        BOOST_CHECK(state.IsValid());
        // we should get the same balance
        cout << "Block: " << i + 1 << " time (" << pblock->GetBlockTime() << ") Should have balance: " << blockinfo[i].balance << " Actual Balance: " << pwallet->GetBalance() << endl;
        BOOST_CHECK(blockinfo[i].balance == pwallet->GetBalance());
        // if we have added a new block the chainActive should be correct
        BOOST_CHECK(pindexPrev != chainActive.Tip());
        pblock->hashPrevBlock = pblock->GetHash();
        pindexPrev = chainActive.Tip();
    }
}
