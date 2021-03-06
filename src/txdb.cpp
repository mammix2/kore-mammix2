// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2018 The KORE developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "arith_uint256.h" // Legacy code
#include "txdb.h"
#include "chain.h"
#include "main.h"
#include "pow.h"
#include "support/csviterator.h"
#include "uint256.h"
#ifdef ZEROCOIN
#include "accumulators.h"
using namespace libzerocoin;
#endif

#include <iostream>
#include <fstream>

#include <stdint.h>

#include <boost/thread.hpp>

using namespace std;

static const char DB_COINS = 'c';
static const char DB_BLOCK_FILES = 'f';
static const char DB_TXINDEX = 't';
static const char DB_BLOCK_INDEX = 'b';

static const char DB_BEST_BLOCK = 'B';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';

void static BatchWriteCoins(CLevelDBBatch& batch, const uint256& hash, const CCoins& coins)
{
    if (coins.IsPruned())
        batch.Erase(make_pair(DB_COINS, hash));
    else
        batch.Write(make_pair(DB_COINS, hash), coins);
}

void static BatchWriteHashBestChain(CLevelDBBatch& batch, const uint256& hash)
{
    batch.Write(DB_BEST_BLOCK, hash);
}

CCoinsViewDB::CCoinsViewDB(size_t nCacheSize, bool fMemory, bool fWipe) : db(GetDataDir() / "chainstate", nCacheSize, fMemory, fWipe)
{
}

bool CCoinsViewDB::GetCoins(const uint256& txid, CCoins& coins) const
{
    return db.Read(make_pair(DB_COINS, txid), coins);
}

bool CCoinsViewDB::HaveCoins(const uint256& txid) const
{
    return db.Exists(make_pair(DB_COINS, txid));
}

uint256 CCoinsViewDB::GetBestBlock() const
{
    uint256 hashBestChain;
    if (!db.Read(DB_BEST_BLOCK, hashBestChain))
        return uint256(0);
    return hashBestChain;
}

bool CCoinsViewDB::BatchWrite(CCoinsMap& mapCoins, const uint256& hashBlock)
{
    CLevelDBBatch batch(&db.GetObfuscateKey());
    size_t count = 0;
    size_t changed = 0;
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end();) {
        if (it->second.flags & CCoinsCacheEntry::DIRTY) {
            BatchWriteCoins(batch, it->first, it->second.coins);
            changed++;
        }
        count++;
        CCoinsMap::iterator itOld = it++;
        mapCoins.erase(itOld);
    }
    if (hashBlock != uint256(0))
        BatchWriteHashBestChain(batch, hashBlock);

    LogPrint("coindb", "Committing %u changed transactions (out of %u) to coin database...\n", (unsigned int)changed, (unsigned int)count);
    return db.WriteBatch(batch);
}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) : CLevelDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe)
{
    // Legacy code, using salt
    if (!Read('S', salt)) {
        salt = GetRandHash();
        Write('S', salt);
    }
}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex& blockindex)
{
    return Write(make_pair(DB_BLOCK_INDEX, blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo& info)
{
    return Write(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo& info)
{
    return Read(make_pair(DB_BLOCK_FILES, nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile)
{
    return Write(DB_LAST_BLOCK, nFile);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing)
{
    if (fReindexing)
        return Write(DB_REINDEX_FLAG, '1');
    else
        return Erase(DB_REINDEX_FLAG);
}

bool CBlockTreeDB::ReadReindexing(bool& fReindexing)
{
    fReindexing = Exists(DB_REINDEX_FLAG);
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int& nFile)
{
    return Read(DB_LAST_BLOCK, nFile);
}

bool CCoinsViewDB::GetStats(CCoinsStats& stats) const
{
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    boost::scoped_ptr<CLevelDBIterator> pcursor(const_cast<CLevelDBWrapper*>(&db)->NewIterator());
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    stats.hashBlock = GetBestBlock();
    ss << stats.hashBlock;
    CAmount nTotalAmount = 0;
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            std::pair<char, uint256> key;
            CCoins coins;
            if (pcursor->GetKey(key) && key.first == DB_COINS) {
                if (pcursor->GetValue(coins)) {
                    stats.nTransactions++;
                    for (unsigned int i = 0; i < coins.vout.size(); i++) {
                        const CTxOut& out = coins.vout[i];
                        if (!out.IsNull()) {
                            stats.nTransactionOutputs++;
                            ss << VARINT(i + 1);
                            ss << out;
                            nTotalAmount += out.nValue;
                        }
                    }
                    stats.nSerializedSize += 32 + pcursor->GetValueSize();
                    ss << VARINT(0);
                } else {
                    return error("CCoinsViewDB::GetStats() : unable to read value");
                }
            }
            pcursor->Next();
        } catch (std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    {
        LOCK(cs_main);
        stats.nHeight = mapBlockIndex.find(stats.hashBlock)->second->nHeight;
    }
    stats.hashSerialized = ss.GetHash();
    stats.nTotalAmount = nTotalAmount;
    return true;
}

bool CCoinsViewDB::DumpUTXO(string &fileSaved, string fileBaseName)
{
    /* It seems that there are no "const iterators" for LevelDB.  Since we
       only need read operations on it, use a const-cast to get around
       that restriction.  */
    boost::scoped_ptr<CLevelDBIterator> pcursor(const_cast<CLevelDBWrapper*>(&db)->NewIterator());
    pcursor->SeekToFirst();

    CHashWriter ss(SER_GETHASH, PROTOCOL_VERSION);
    uint256 hashBlock = GetBestBlock();
    CAmount nTotalAmount = 0;
    int nTransactionOutputs = 0;
    uint64_t serializedSize = 0;

    fileSaved = fileBaseName;
    string nHeight;
    ofstream myfile;
    {    
        LOCK(cs_main);
    
        fileSaved += itostr(mapBlockIndex.find(hashBlock)->second->nHeight);
        fileSaved += ".csv";
    }
    myfile.open(fileSaved);
    
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            std::pair<char, uint256> key;
            CCoins coins;
            if (pcursor->GetKey(key) && key.first == DB_COINS) {
                if (pcursor->GetValue(coins)) {
                    for (unsigned int i = 0; i < coins.vout.size(); i++) {
                        const CTxOut& out = coins.vout[i];
                        if (!out.IsNull()) {
                            stringstream scriptStream(out.scriptPubKey.ToString());
                            for(CSVIterator loop(scriptStream, ' '); loop != CSVIterator(); ++loop)
                            {
                                for (int i = 0; i < (*loop).size(); i++)
                                {
                                    myfile << (*loop)[i] << ',';
                                    ss <<  (*loop)[i];
                                }
                            }

                            myfile << out.nValue << ",|,";
                            ss << out.nValue;

                            myfile << (coins.fCoinBase ? 'c' : 'n') << ',';
                            ss << (coins.fCoinBase ? 'c' : 'n');

                            myfile << coins.nVersion << ',';
                            ss << VARINT(coins.nVersion);

                            myfile << coins.nHeight;
                            ss << VARINT(coins.nHeight);

                            myfile << std::endl;

                            nTransactionOutputs++;
                            nTotalAmount += out.nValue;
                        }
                    }
                    serializedSize += 32 + pcursor->GetValueSize();
                } else {
                    return error("CCoinsViewDB::GetStats() : unable to read value");
                }
            }
            pcursor->Next();
        } catch (std::exception& e) {
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    myfile << "Outputs: " << nTransactionOutputs << std::endl;
    myfile << "Total Value: " << nTotalAmount << std::endl;
    myfile << "Dumped at block: " << mapBlockIndex.find(GetBestBlock())->second->nHeight << std::endl;
    myfile << "Hash: " << ss.GetHash().GetHex() << std::endl;

    myfile.close();
    return true;
}

bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<int, const CBlockFileInfo*> >& fileInfo, int nLastFile, const std::vector<const CBlockIndex*>& blockinfo)
{
    CLevelDBBatch batch(&GetObfuscateKey());
    for (std::vector<std::pair<int, const CBlockFileInfo*> >::const_iterator it = fileInfo.begin(); it != fileInfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_FILES, it->first), *it->second);
    }
    batch.Write(DB_LAST_BLOCK, nLastFile);
    for (std::vector<const CBlockIndex*>::const_iterator it = blockinfo.begin(); it != blockinfo.end(); it++) {
        batch.Write(make_pair(DB_BLOCK_INDEX, (*it)->GetBlockHash()), CDiskBlockIndex(*it));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::ReadTxIndex(const uint256& txid, CDiskTxPos& pos)
{
    return Read(make_pair(DB_TXINDEX, txid), pos);
}

bool CBlockTreeDB::WriteTxIndex(const std::vector<std::pair<uint256, CDiskTxPos> >& vect)
{
    CLevelDBBatch batch(&GetObfuscateKey());
    for (std::vector<std::pair<uint256, CDiskTxPos> >::const_iterator it = vect.begin(); it != vect.end(); it++)
        batch.Write(make_pair(DB_TXINDEX, it->first), it->second);
    return WriteBatch(batch);
}

bool CBlockTreeDB::ReadAddrIndex(uint160 addrid, std::vector<CExtDiskTxPos> &list) {
    boost::scoped_ptr<CLevelDBIterator> pcursor(NewIterator());

    uint64_t lookupid;
    {
        CHashWriter ss(SER_GETHASH, 0);
        ss << salt;
        ss << addrid;
        lookupid = UintToArith256(ss.GetHash()).GetLow64();
    }

    pcursor->Seek(make_pair('a', lookupid));

    while (pcursor->Valid()) {
        std::pair<std::pair<char, uint64_t>, CExtDiskTxPos> key;
        if (pcursor->GetKey(key) && key.first.first == 'a' && key.first.second == lookupid) {
            list.push_back(key.second);
        } else {
            break;
        }
        pcursor->Next();
    }
    return true;
}

bool CBlockTreeDB::AddAddrIndex(const std::vector<std::pair<uint160, CExtDiskTxPos> > &list) {
    unsigned char foo[0];
    CLevelDBBatch batch(&GetObfuscateKey());
    for (std::vector<std::pair<uint160, CExtDiskTxPos> >::const_iterator it=list.begin(); it!=list.end(); it++) {
        CHashWriter ss(SER_GETHASH, 0);
        ss << salt;
        ss << it->first;
        batch.Write(make_pair(make_pair('a', UintToArith256(ss.GetHash()).GetLow64()), it->second), FLATDATA(foo));
    }
    return WriteBatch(batch, true);
}

bool CBlockTreeDB::WriteFlag(const std::string& name, bool fValue)
{
    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const std::string& name, bool& fValue)
{
    char ch;
    if (!Read(std::make_pair(DB_FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::WriteInt(const std::string& name, int nValue)
{
    return Write(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::ReadInt(const std::string& name, int& nValue)
{
    return Read(std::make_pair('I', name), nValue);
}

bool CBlockTreeDB::LoadBlockIndexGuts()
{
    LogPrintf("LoadBlockIndexGuts --> \n");
    boost::scoped_ptr<CLevelDBIterator> pcursor(NewIterator());

    pcursor->Seek(make_pair(DB_BLOCK_INDEX, uint256()));

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            std::pair<char, uint256> key;
            if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
                CDiskBlockIndex diskindex;
                if (pcursor->GetValue(diskindex)) {
                    // Construct block index object
                    bool useLegacyCode = UseLegacyCode(diskindex.nHeight);
                    LogPrintf("Reading Block: %d \n", diskindex.nHeight);
                    LogPrintf("BlockInfo %s \n", diskindex.ToString());
                    CBlockIndex* pindexNew    = InsertBlockIndex(diskindex.GetBlockHash());
                    pindexNew->pprev          = InsertBlockIndex(diskindex.hashPrev);
                    pindexNew->pnext          = useLegacyCode ? NULL : InsertBlockIndex(diskindex.hashNext);
                    pindexNew->nHeight        = diskindex.nHeight;
                    pindexNew->nFile          = diskindex.nFile;
                    pindexNew->nDataPos       = diskindex.nDataPos;
                    pindexNew->nUndoPos       = diskindex.nUndoPos;
                    pindexNew->nVersion       = diskindex.nVersion;
                    pindexNew->hashMerkleRoot = diskindex.hashMerkleRoot;
                    pindexNew->nTime          = diskindex.nTime;
                    pindexNew->nBits          = diskindex.nBits;
                    pindexNew->nNonce         = diskindex.nNonce;
                    pindexNew->nBirthdayA     = diskindex.nBirthdayA;
                    pindexNew->nBirthdayB     = diskindex.nBirthdayB;
                    pindexNew->nStatus        = diskindex.nStatus;
                    pindexNew->nTx            = diskindex.nTx;

                    //Proof Of Stake
                    pindexNew->nMint             = diskindex.nMint;
                    pindexNew->nMoneySupply      = diskindex.nMoneySupply;
                    pindexNew->nFlags            = diskindex.nFlags;
                    pindexNew->nStakeModifier    = diskindex.nStakeModifier;
                    pindexNew->nStakeModifierOld = diskindex.nStakeModifierOld;
                    pindexNew->prevoutStake      = diskindex.prevoutStake;
                    pindexNew->nStakeTime        = diskindex.nStakeTime;
                    pindexNew->hashProofOfStake  = diskindex.hashProofOfStake;
                    bool isProofOfStake = useLegacyCode ? pindexNew->IsProofOfStake_Legacy() : pindexNew->IsProofOfStake(); 
                    if (!isProofOfStake && (pindexNew->nStatus & BLOCK_HAVE_DATA)) {
                        if (!CheckProofOfWork(pindexNew->GetBlockHash(), pindexNew->nBits, pindexNew->nHeight))
                            return error("LoadBlockIndexGuts() : CheckProofOfWork failed: %s", pindexNew->ToString());
                    }
                    // ppcoin: build setStakeSeen
                    if (!useLegacyCode && isProofOfStake)
                        setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));

                    pcursor->Next();
                } else {
                    return error("LoadBlockIndexGuts() : failed to read value");
                }
            } else {
                break; // if shutdown requested or finished loading block index
            }
        } catch (std::exception& e) {
            LogPrintf("LoadBlockIndexGuts Error !!!");
            return error("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }

    LogPrintf("LoadBlockIndexGuts <-- \n");
    return true;
}
