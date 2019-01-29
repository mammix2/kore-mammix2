// Copyright (c) 2017-2018 The KORE developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KORE_STAKEINPUT_H
#define KORE_STAKEINPUT_H

class CKeyStore;
class CWallet;
class CWalletTx;

class CStakeInput
{
protected:
    CBlockIndex* pindexFrom;

public:
    virtual ~CStakeInput(){};
    virtual CScript GetScriptPubKey(CWallet* pwallet, CScript& scriptPubKey) = 0;
    virtual CBlockIndex* GetIndexFrom() = 0;
    virtual bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) = 0;
    virtual bool GetTxFrom(CTransaction& tx) = 0;
    virtual CAmount GetValue() = 0;
    virtual bool CreateTxOuts(CWallet* pwallet, vector<CTxOut>& vout, bool splitStake) = 0;
    virtual bool GetModifier(uint64_t& nStakeModifier) = 0;
    virtual CDataStream GetUniqueness() = 0;
    virtual int GetPosition() = 0;
};


class CKoreStake : public CStakeInput
{
private:
    CTransaction txFrom;
    unsigned int nPosition;

public:
    CKoreStake()
    {
        this->pindexFrom = nullptr;
    }

    CScript GetScriptPubKey(CWallet* pwallet, CScript& scriptPubKey) override;
    bool SetInput(CTransaction txPrev, unsigned int n);

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransaction& tx) override;
    CAmount GetValue() override;
    bool GetModifier(uint64_t& nStakeModifier) override;
    CDataStream GetUniqueness() override;
    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = 0) override;
    bool CreateTxOuts(CWallet* pwallet, vector<CTxOut>& vout, bool splitStake) override;
    int GetPosition() override;
};


#endif //KORE_STAKEINPUT_H
