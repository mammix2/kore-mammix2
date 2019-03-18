

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
    hash=0941a130e2c3b331e7b0384f1aa39d0eb1af190a9e5ec24008bbba9763935553 
    ver=1073741825 
    hashPrevBlock=0aab10677b4fe0371a67f99e78a69e7d9fa03a1c7d48747978da405dc5abeb99, 
    hashMerkleRoot=27e49b8169164df0ceb30a43d5b661f7441929413624f0d417b509f893fa0329, 
    nTime=1552489800, 
    nBits=201fffff, 
    nNonce=7, 
    nBirthdayA=25146415, 
    nBirthdayB=61526564, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=27e49b8169, ver=2,  nTime=1552489653, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 510101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    27e49b8169164df0ceb30a43d5b661f7441929413624f0d417b509f893fa0329
CBlock <<<<============================
{0, 1552489800, 1552489653 , 538968063 , 7 , 1 , 25146415 , 61526564 , uint256("0941a130e2c3b331e7b0384f1aa39d0eb1af190a9e5ec24008bbba9763935553") , uint256("27e49b8169164df0ceb30a43d5b661f7441929413624f0d417b509f893fa0329") , 0 }, // Block 1
CBlock ============================>>>>
    hash=0c6b415130711c3abaa2a815a661f6464ac5c1c14b591f15f790ea5f37e89e02 
    ver=1073741825 
    hashPrevBlock=0941a130e2c3b331e7b0384f1aa39d0eb1af190a9e5ec24008bbba9763935553, 
    hashMerkleRoot=db126cc8aef49bb38567954b7ecc7ebcbbda8621bc098f9756a6cf86f9fb49a8, 
    nTime=1552489849, 
    nBits=201fffff, 
    nNonce=2, 
    nBirthdayA=32082578, 
    nBirthdayB=41733278, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=db126cc8ae, ver=2,  nTime=1552489824, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 520101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    db126cc8aef49bb38567954b7ecc7ebcbbda8621bc098f9756a6cf86f9fb49a8
CBlock <<<<============================
{0, 1552489849, 1552489824 , 538968063 , 2 , 1 , 32082578 , 41733278 , uint256("0c6b415130711c3abaa2a815a661f6464ac5c1c14b591f15f790ea5f37e89e02") , uint256("db126cc8aef49bb38567954b7ecc7ebcbbda8621bc098f9756a6cf86f9fb49a8") , 0 }, // Block 2
CBlock ============================>>>>
    hash=002d3b8ba401ed9d3cb81a49da2d175691ceaf060134701c36ecb9a9f661cb40 
    ver=1073741825 
    hashPrevBlock=0c6b415130711c3abaa2a815a661f6464ac5c1c14b591f15f790ea5f37e89e02, 
    hashMerkleRoot=122c51e4eca1241bcd8761f6a5cca12c41f5c893fa7579ce2fa91141fedaf9cc, 
    nTime=1552491316, 
    nBits=2001d41b, 
    nNonce=58, 
    nBirthdayA=16589978, 
    nBirthdayB=41260825, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=122c51e4ec, ver=2,  nTime=1552489872, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 530101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    122c51e4eca1241bcd8761f6a5cca12c41f5c893fa7579ce2fa91141fedaf9cc
CBlock <<<<============================
{0, 1552491316, 1552489872 , 536990747 , 58 , 1 , 16589978 , 41260825 , uint256("002d3b8ba401ed9d3cb81a49da2d175691ceaf060134701c36ecb9a9f661cb40") , uint256("122c51e4eca1241bcd8761f6a5cca12c41f5c893fa7579ce2fa91141fedaf9cc") , 0 }, // Block 3
CBlock ============================>>>>
    hash=000247961c2ed49ee4c7f01f238cfbfe6524abbf6ac8f01330204d5e921c18aa 
    ver=1073741825 
    hashPrevBlock=002d3b8ba401ed9d3cb81a49da2d175691ceaf060134701c36ecb9a9f661cb40, 
    hashMerkleRoot=7fb7c67587ff05250189d4f48e32ee365382c4588d6a41f607af0ba8aa202eec, 
    nTime=1552491568, 
    nBits=200125de, 
    nNonce=10, 
    nBirthdayA=0, 
    nBirthdayB=0, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=7fb7c67587, ver=2,  nTime=1552491340, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 540101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    7fb7c67587ff05250189d4f48e32ee365382c4588d6a41f607af0ba8aa202eec
CBlock <<<<============================
{0, 1552491568, 1552491340 , 536946142 , 10 , 1 , 0 , 0 , uint256("000247961c2ed49ee4c7f01f238cfbfe6524abbf6ac8f01330204d5e921c18aa") , uint256("7fb7c67587ff05250189d4f48e32ee365382c4588d6a41f607af0ba8aa202eec") , 900000000000 }, // Block 4
CBlock ============================>>>>
    hash=018c501c6d813b5d9c469bb43fecbc5e29dfd818e34752ed208f475afae55253 
    ver=1073741825 
    hashPrevBlock=000247961c2ed49ee4c7f01f238cfbfe6524abbf6ac8f01330204d5e921c18aa, 
    hashMerkleRoot=d7567e02b8519a0161d54697234d4d5a3385fc53d1d80fab743a773161547cfb, 
    nTime=1552492940, 
    nBits=2001c54a, 
    nNonce=55, 
    nBirthdayA=49500033, 
    nBirthdayB=60637363, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=d7567e02b8, ver=2,  nTime=1552491593, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 550101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    d7567e02b8519a0161d54697234d4d5a3385fc53d1d80fab743a773161547cfb
CBlock <<<<============================
{0, 1552492940, 1552491593 , 536986954 , 55 , 1 , 49500033 , 60637363 , uint256("018c501c6d813b5d9c469bb43fecbc5e29dfd818e34752ed208f475afae55253") , uint256("d7567e02b8519a0161d54697234d4d5a3385fc53d1d80fab743a773161547cfb") , 1800000000000 }, // Block 5
CBlock ============================>>>>
    hash=0107785abedd816659ad86a5f23fa46d9fc34c500e3b5d4706ee7ecd405225c0 
    ver=1073741825 
    hashPrevBlock=018c501c6d813b5d9c469bb43fecbc5e29dfd818e34752ed208f475afae55253, 
    hashMerkleRoot=757af5d9cfe1719293b8759902941512026af6787bba34e2648c29d46277132f, 
    nTime=1552493733, 
    nBits=2001305d, 
    nNonce=32, 
    nBirthdayA=40603150, 
    nBirthdayB=54710580, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=757af5d9cf, ver=2,  nTime=1552492965, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 560101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    757af5d9cfe1719293b8759902941512026af6787bba34e2648c29d46277132f
CBlock <<<<============================
{0, 1552493733, 1552492965 , 536948829 , 32 , 1 , 40603150 , 54710580 , uint256("0107785abedd816659ad86a5f23fa46d9fc34c500e3b5d4706ee7ecd405225c0") , uint256("757af5d9cfe1719293b8759902941512026af6787bba34e2648c29d46277132f") , 2700000000000 }, // Block 6
CBlock ============================>>>>
    hash=01a65b85d5e24daed1da1dfd87d2cbb6a2f12677e051c71a0f4158cbd2fe7fb6 
    ver=1073741825 
    hashPrevBlock=0107785abedd816659ad86a5f23fa46d9fc34c500e3b5d4706ee7ecd405225c0, 
    hashMerkleRoot=9f7b3b169a05fe04ac5d7de0115cc8a5633121f369976ce23bbed98dd26bb0d5, 
    nTime=1552493758, 
    nBits=20022fce, 
    nNonce=1, 
    nBirthdayA=0, 
    nBirthdayB=0, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=9f7b3b169a, ver=2,  nTime=1552493758, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 570101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    9f7b3b169a05fe04ac5d7de0115cc8a5633121f369976ce23bbed98dd26bb0d5
CBlock <<<<============================
{0, 1552493758, 1552493758 , 537014222 , 1 , 1 , 0 , 0 , uint256("01a65b85d5e24daed1da1dfd87d2cbb6a2f12677e051c71a0f4158cbd2fe7fb6") , uint256("9f7b3b169a05fe04ac5d7de0115cc8a5633121f369976ce23bbed98dd26bb0d5") , 3600000000000 }, // Block 7
CBlock ============================>>>>
    hash=00267e535bb79ecdc3ad3c42f76a1ef75fac360be9a04139ba129f96ab5e6a9a 
    ver=1073741825 
    hashPrevBlock=01a65b85d5e24daed1da1dfd87d2cbb6a2f12677e051c71a0f4158cbd2fe7fb6, 
    hashMerkleRoot=772012355a5ca5571aef69dd58b08bdf01c5e3635c195dd8f744dfa5e74de7ca, 
    nTime=1552494204, 
    nBits=20031fb8, 
    nNonce=18, 
    nBirthdayA=685289, 
    nBirthdayB=38917086, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=772012355a, ver=2,  nTime=1552493783, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 580101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    772012355a5ca5571aef69dd58b08bdf01c5e3635c195dd8f744dfa5e74de7ca
CBlock <<<<============================
{0, 1552494204, 1552493783 , 537075640 , 18 , 1 , 685289 , 38917086 , uint256("00267e535bb79ecdc3ad3c42f76a1ef75fac360be9a04139ba129f96ab5e6a9a") , uint256("772012355a5ca5571aef69dd58b08bdf01c5e3635c195dd8f744dfa5e74de7ca") , 4500000000000 }, // Block 8
CBlock ============================>>>>
    hash=0064814b06c584bae5a599bd1c161033cc929baffb2f89a19b530b25edc84269 
    ver=1073741825 
    hashPrevBlock=00267e535bb79ecdc3ad3c42f76a1ef75fac360be9a04139ba129f96ab5e6a9a, 
    hashMerkleRoot=2b017b4ee0aa8ed0df1fa19823acf20289b63527642a95ca6c23bc948cfa4dd1, 
    nTime=1552495296, 
    nBits=2001cf6a, 
    nNonce=41, 
    nBirthdayA=0, 
    nBirthdayB=0, 
    vchBlockSig=, 
    Vtx : size 1 
    CTransaction(hash=2b017b4ee0, ver=2,  nTime=1552494229, vin.size=1, vout.size=2, nLockTime=0)
    CTxIn(COutPoint(0000000000, 4294967295), coinbase 590101)
    CTxOut(nValue=9000.00000000, scriptPubKey=76a914ff197b14e502ab41f3bc8ccb48c4abac9eab35bc88ac)
    CTxOut(nValue=1000.00000000, scriptPubKey=4104d410c4a7fec6dbf6fedc9721104ada1571d5e3e4791085efc083a9f3f4c007d240a6a647dda0ca1466641b0739a86a67b97ac48484fc7ca88257804b7ce52ed2ac)

  vMerkleTree:     vMerkleTree : size 1 
    2b017b4ee0aa8ed0df1fa19823acf20289b63527642a95ca6c23bc948cfa4dd1
CBlock <<<<============================
{0, 1552495296, 1552494229 , 536989546 , 41 , 1 , 0 , 0 , uint256("0064814b06c584bae5a599bd1c161033cc929baffb2f89a19b530b25edc84269") , uint256("2b017b4ee0aa8ed0df1fa19823acf20289b63527642a95ca6c23bc948cfa4dd1") , 5400000000000 }, // Block 9

*/
blockinfo_t blockinfo[] = {
{0, 1552489800, 1552489653 , 538968063 , 7 , 1 , 25146415 , 61526564 , uint256("0941a130e2c3b331e7b0384f1aa39d0eb1af190a9e5ec24008bbba9763935553") , uint256("27e49b8169164df0ceb30a43d5b661f7441929413624f0d417b509f893fa0329") , 0 }, // Block 1
{0, 1552489849, 1552489824 , 538968063 , 2 , 1 , 32082578 , 41733278 , uint256("0c6b415130711c3abaa2a815a661f6464ac5c1c14b591f15f790ea5f37e89e02") , uint256("db126cc8aef49bb38567954b7ecc7ebcbbda8621bc098f9756a6cf86f9fb49a8") , 0 }, // Block 2
{0, 1552491316, 1552489872 , 536990747 , 58 , 1 , 16589978 , 41260825 , uint256("002d3b8ba401ed9d3cb81a49da2d175691ceaf060134701c36ecb9a9f661cb40") , uint256("122c51e4eca1241bcd8761f6a5cca12c41f5c893fa7579ce2fa91141fedaf9cc") , 0 }, // Block 3
{0, 1552491568, 1552491340 , 536946142 , 10 , 1 , 0 , 0 , uint256("000247961c2ed49ee4c7f01f238cfbfe6524abbf6ac8f01330204d5e921c18aa") , uint256("7fb7c67587ff05250189d4f48e32ee365382c4588d6a41f607af0ba8aa202eec") , 900000000000 }, // Block 4
{0, 1552492940, 1552491593 , 536986954 , 55 , 1 , 49500033 , 60637363 , uint256("018c501c6d813b5d9c469bb43fecbc5e29dfd818e34752ed208f475afae55253") , uint256("d7567e02b8519a0161d54697234d4d5a3385fc53d1d80fab743a773161547cfb") , 1800000000000 }, // Block 5
{0, 1552493733, 1552492965 , 536948829 , 32 , 1 , 40603150 , 54710580 , uint256("0107785abedd816659ad86a5f23fa46d9fc34c500e3b5d4706ee7ecd405225c0") , uint256("757af5d9cfe1719293b8759902941512026af6787bba34e2648c29d46277132f") , 2700000000000 }, // Block 6
{0, 1552493758, 1552493758 , 537014222 , 1 , 1 , 0 , 0 , uint256("01a65b85d5e24daed1da1dfd87d2cbb6a2f12677e051c71a0f4158cbd2fe7fb6") , uint256("9f7b3b169a05fe04ac5d7de0115cc8a5633121f369976ce23bbed98dd26bb0d5") , 3600000000000 }, // Block 7
{0, 1552494204, 1552493783 , 537075640 , 18 , 1 , 685289 , 38917086 , uint256("00267e535bb79ecdc3ad3c42f76a1ef75fac360be9a04139ba129f96ab5e6a9a") , uint256("772012355a5ca5571aef69dd58b08bdf01c5e3635c195dd8f744dfa5e74de7ca") , 4500000000000 }, // Block 8
{0, 1552495296, 1552494229 , 536989546 , 41 , 1 , 0 , 0 , uint256("0064814b06c584bae5a599bd1c161033cc929baffb2f89a19b530b25edc84269") , uint256("2b017b4ee0aa8ed0df1fa19823acf20289b63527642a95ca6c23bc948cfa4dd1") , 5400000000000 } // Block 9
};


void LogBlockFound(CWallet* pwallet, int blockNumber, CBlock* pblock, unsigned int nExtraNonce, bool fProofOfStake, bool logToStdout)
{
    if (logToStdout) {
        cout << pblock->ToString().c_str();
        cout << "{" << fProofOfStake << ", ";
        cout << pblock->nTime << ", ";
        cout << pblock->vtx[0].nTime << " , ";
        cout << pblock->nBits << " , ";
        cout << pblock->nNonce << " , ";
        cout << nExtraNonce << " , ";
        cout << pblock->nBirthdayA << " , ";
        cout << pblock->nBirthdayB << " , ";
        cout << "uint256(\"" << pblock->GetHash().ToString().c_str() << "\") , ";
        cout << "uint256(\"" << pblock->hashMerkleRoot.ToString().c_str() << "\") , ";
        cout << pwallet->GetBalance() << " },";
        cout << " // " << "Block " << blockNumber << endl;
    }

    if (fDebug) {
        LogPrintf("Block %d %s \n",blockNumber, (pblock->IsProofOfStake() ? " (PoS) " : " (PoW) "));
        LogPrintf(" nTime               : %u \n", pblock->nTime);
        LogPrintf(" hash                : %s \n", pblock->GetHash().ToString().c_str());
        LogPrintf(" StakeModifier       : %u \n", chainActive.Tip()->nStakeModifier);
        LogPrintf(" OldStakeModifier    : %s \n", chainActive.Tip()->nStakeModifierOld.ToString());
        LogPrintf(" Modifier Generated? : %s \n", (chainActive.Tip()->GeneratedStakeModifier() ? "True" : "False"));
        LogPrintf(" Balance             : %d \n", pwallet->GetBalance() );
        LogPrintf(" Unconfirmed Balance : %d \n", pwallet->GetUnconfirmedBalance());
        LogPrintf(" Immature  Balance   : %d \n",pwallet->GetImmatureBalance());
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
