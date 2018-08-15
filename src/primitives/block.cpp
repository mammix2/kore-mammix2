// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "script/standard.h"
#include "script/sign.h"
#include "tinyformat.h"
#include "momentum.h"
#include "utilstrencodings.h"
#include "util.h"

uint256 CBlockHeader::GetHash() const
{
    //return HashQuark(BEGIN(nVersion), END(nNonce));
    return Hash(BEGIN(nVersion), END(nBirthdayB));
}

uint256 CBlockHeader::GetMidHash() const
{
    return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CBlockHeader::GetVerifiedHash() const
{
 
 	uint256 midHash = GetMidHash();
 		    	
	uint256 r = Hash(BEGIN(nVersion), END(nBirthdayB));

 	if(!bts::momentum_verify( midHash, nBirthdayA, nBirthdayB)){
 		return uint256S("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeeee");
 	}
   
     return r;
}

uint256 CBlockHeader::CalculateBestBirthdayHash()
{
    uint256 midHash = GetMidHash();
    std::vector<std::pair<uint32_t, uint32_t> > results = bts::momentum_search(midHash);
    uint32_t candidateBirthdayA = 0;
    uint32_t candidateBirthdayB = 0;
    uint256 smallestHashSoFar = uint256S("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffdddd");
    for (unsigned i = 0; i < results.size(); i++) {
        nBirthdayA = results[i].first;
        nBirthdayB = results[i].second;
        uint256 fullHash = Hash(BEGIN(nVersion), END(nBirthdayB));
        if (fullHash < smallestHashSoFar) {
            smallestHashSoFar = fullHash;
            candidateBirthdayA = results[i].first;
            candidateBirthdayB = results[i].second;
        }
        nBirthdayA = candidateBirthdayA;
        nBirthdayB = candidateBirthdayB;
    }

    return GetHash();
}

#ifdef LICO
uint256 CBlock::BlockMerkleRoot(bool* mutated) const
{
    std::vector<uint256> leaves;
    leaves.resize(vtx.size());
    for (size_t s = 0; s < vtx.size(); s++) {
        leaves[s] = vtx[s].GetHash();
    }
    return ComputeMerkleRoot(leaves, mutated);
}

uint256 CBlock::ComputeMerkleRoot(const std::vector<uint256>& leaves, bool* mutated) const
{
    uint256 hash;
    MerkleComputation(leaves, &hash, mutated, -1, NULL);
    return hash;
}

/*     WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
*/

/* This implements a constant-space merkle root/path calculator, limited to 2^32 leaves. */
void CBlock::MerkleComputation(const std::vector<uint256>& leaves, uint256* proot, bool* pmutated, uint32_t branchpos, std::vector<uint256>* pbranch) const
{
    if (pbranch) pbranch->clear();
    if (leaves.size() == 0) {
        if (pmutated) *pmutated = false;
        if (proot) *proot = uint256();
        return;
    }
    bool mutated = false;
    // count is the number of leaves processed so far.
    uint32_t count = 0;
    // inner is an array of eagerly computed subtree hashes, indexed by tree
    // level (0 being the leaves).
    // For example, when count is 25 (11001 in binary), inner[4] is the hash of
    // the first 16 leaves, inner[3] of the next 8 leaves, and inner[0] equal to
    // the last leaf. The other inner entries are undefined.
    uint256 inner[32];
    // Which position in inner is a hash that depends on the matching leaf.
    int matchlevel = -1;
    // First process all leaves into 'inner' values.
    while (count < leaves.size()) {
        uint256 h = leaves[count];
        bool matchh = count == branchpos;
        count++;
        int level;
        // For each of the lower bits in count that are 0, do 1 step. Each
        // corresponds to an inner value that existed before processing the
        // current leaf, and each needs a hash to combine it.
        for (level = 0; !(count & (((uint32_t)1) << level)); level++) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            mutated |= (inner[level] == h);
            CHash256().Write(inner[level].begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
        }
        // Store the resulting hash at inner position level.
        inner[level] = h;
        if (matchh) {
            matchlevel = level;
        }
    }
    // Do a final 'sweep' over the rightmost branch of the tree to process
    // odd levels, and reduce everything to a single top value.
    // Level is the level (counted from the bottom) up to which we've sweeped.
    int level = 0;
    // As long as bit number level in count is zero, skip it. It means there
    // is nothing left at this level.
    while (!(count & (((uint32_t)1) << level))) {
        level++;
    }
    uint256 h = inner[level];
    bool matchh = matchlevel == level;
    while (count != (((uint32_t)1) << level)) {
        // If we reach this point, h is an inner value that is not the top.
        // We combine it with itself (Kore's special rule for odd levels in
        // the tree) to produce a higher level one.
        if (pbranch && matchh) {
            pbranch->push_back(h);
        }
        CHash256().Write(h.begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
        // Increment count to the value it would have if two entries at this
        // level had existed.
        count += (((uint32_t)1) << level);
        level++;
        // And propagate the result upwards accordingly.
        while (!(count & (((uint32_t)1) << level))) {
            if (pbranch) {
                if (matchh) {
                    pbranch->push_back(inner[level]);
                } else if (matchlevel == level) {
                    pbranch->push_back(h);
                    matchh = true;
                }
            }
            CHash256().Write(inner[level].begin(), 32).Write(h.begin(), 32).Finalize(h.begin());
            level++;
        }
    }
    // Return result.
    if (pmutated) *pmutated = mutated;
    if (proot) *proot = h;
}
#endif

uint256 CBlock::BuildMerkleTree(bool* fMutated) const
{
    /* WARNING! If you're reading this because you're learning about crypto
       and/or designing a new system that will use merkle trees, keep in mind
       that the following merkle tree algorithm has a serious flaw related to
       duplicate txids, resulting in a vulnerability (CVE-2012-2459).

       The reason is that if the number of hashes in the list at a given time
       is odd, the last one is duplicated before computing the next level (which
       is unusual in Merkle trees). This results in certain sequences of
       transactions leading to the same merkle root. For example, these two
       trees:

                    A               A
                  /  \            /   \
                B     C         B       C
               / \    |        / \     / \
              D   E   F       D   E   F   F
             / \ / \ / \     / \ / \ / \ / \
             1 2 3 4 5 6     1 2 3 4 5 6 5 6

       for transaction lists [1,2,3,4,5,6] and [1,2,3,4,5,6,5,6] (where 5 and
       6 are repeated) result in the same root hash A (because the hash of both
       of (F) and (F,F) is C).

       The vulnerability results from being able to send a block with such a
       transaction list, with the same merkle root, and the same block hash as
       the original without duplication, resulting in failed validation. If the
       receiving node proceeds to mark that block as permanently invalid
       however, it will fail to accept further unmodified (and thus potentially
       valid) versions of the same block. We defend against this by detecting
       the case where we would hash two identical hashes at the end of the list
       together, and treating that identically to the block having an invalid
       merkle root. Assuming no double-SHA256 collisions, this will detect all
       known ways of changing the transactions without affecting the merkle
       root.
    */
    vMerkleTree.clear();
    vMerkleTree.reserve(vtx.size() * 2 + 16); // Safe upper bound for the number of total nodes.
    for (std::vector<CTransaction>::const_iterator it(vtx.begin()); it != vtx.end(); ++it)
        vMerkleTree.push_back(it->GetHash());
    int j = 0;
    bool mutated = false;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        for (int i = 0; i < nSize; i += 2)
        {
            int i2 = std::min(i+1, nSize-1);
            if (i2 == i + 1 && i2 + 1 == nSize && vMerkleTree[j+i] == vMerkleTree[j+i2]) {
                // Two identical hashes at the end of the list at a particular level.
                mutated = true;
            }
            vMerkleTree.push_back(Hash(BEGIN(vMerkleTree[j+i]),  END(vMerkleTree[j+i]),
                                       BEGIN(vMerkleTree[j+i2]), END(vMerkleTree[j+i2])));
        }
        j += nSize;
    }
    if (fMutated) {
        *fMutated = mutated;
    }
    return (vMerkleTree.empty() ? uint256() : vMerkleTree.back());
}

std::vector<uint256> CBlock::GetMerkleBranch(int nIndex) const
{
    if (vMerkleTree.empty())
        BuildMerkleTree();
    std::vector<uint256> vMerkleBranch;
    int j = 0;
    for (int nSize = vtx.size(); nSize > 1; nSize = (nSize + 1) / 2)
    {
        int i = std::min(nIndex^1, nSize-1);
        vMerkleBranch.push_back(vMerkleTree[j+i]);
        nIndex >>= 1;
        j += nSize;
    }
    return vMerkleBranch;
}

uint256 CBlock::CheckMerkleBranch(uint256 hash, const std::vector<uint256>& vMerkleBranch, int nIndex)
{
    if (nIndex == -1)
		return uint256();
    for (std::vector<uint256>::const_iterator it(vMerkleBranch.begin()); it != vMerkleBranch.end(); ++it)
    {
        if (nIndex & 1)
            hash = Hash(BEGIN(*it), END(*it), BEGIN(hash), END(hash));
        else
            hash = Hash(BEGIN(hash), END(hash), BEGIN(*it), END(*it));
        nIndex >>= 1;
    }
    return hash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << "CBlock ============================>>>>\n";    
    s << strprintf("    hash=%s \n", GetHash().ToString());
    s << strprintf("    ver=%d \n", nVersion);
    s << strprintf("    hashPrevBlock=%s, \n", hashPrevBlock.ToString());
    s << strprintf("    hashMerkleRoot=%s, \n", hashMerkleRoot.ToString());
    s << strprintf("    nTime=%u, \n", nTime);
    s << strprintf("    nBits=%x, \n", nBits);
    s << strprintf("    nNonce=%u, \n", nNonce);
    s << strprintf("    nBirthdayA=%u, \n", nBirthdayA);
    s << strprintf("    nBirthdayB=%u, \n", nBirthdayB);
	s << strprintf("    vchBlockSig=%s, \n", HexStr(vchBlockSig.begin(), vchBlockSig.end()));    

    s << strprintf("    Vtx : size %u \n",vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "    " << vtx[i].ToString() << "\n";
    }
    s << "  vMerkleTree: ";
    s << strprintf("    vMerkleTree : size %u \n",vMerkleTree.size());
    for (unsigned int i = 0; i < vMerkleTree.size(); i++)
        s << "    " << vMerkleTree[i].ToString();

    s << "CBlock <<<<============================\n";
    return s.str();
}

void CBlock::print() const
{
    LogPrintf("%s", ToString());
}