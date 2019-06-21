// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdpdb.h"

#include "main.h"

bool CCdpMemCache::GetCdps(const double ratio, vector<CUserCdp> &cdps) {
    // TODO
    return true;
}

bool CCdpCacheManager::StakeBcoinsToCdp(const CRegID &regId, const uint64_t bcoinsToStake, const uint64_t mintedScoins,
                                        const int blockHeight, const int txIndex, CUserCdp &cdp, CDbOpLog &cdpDbOpLog) {
    cdpDbOpLog = CDbOpLog(cdpCache.GetPrefixType(), regId.ToRawString(), cdp);

    cdp.lastBlockHeight = blockHeight;
    cdp.mintedScoins    = mintedScoins;
    cdp.totalStakedBcoins += bcoinsToStake;
    cdp.totalOwedScoins += cdp.mintedScoins;

    if (!SaveCdp(regId, TxCord(blockHeight, txIndex), cdp)) {
        return ERRORMSG("CCdpCacheManager::StakeBcoinsToCdp : SetData failed.");
    }

    return true;
}

bool CCdpCacheManager::GetUnderLiquidityCdps(const uint64_t bcoinMedianPrice, vector<CUserCdp> &userCdps) {
    return cdpMemCache.GetCdps(GetOpenLiquidateRatio() * bcoinMedianPrice, userCdps);
}

bool CCdpCacheManager::GetForceSettleCdps(const uint64_t bcoinMedianPrice, vector<CUserCdp> &userCdps) {
    return cdpMemCache.GetCdps(GetForceLiquidateRatio() * bcoinMedianPrice, userCdps);
}

bool CCdpCacheManager::GetCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.GetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

bool CCdpCacheManager::SaveCdp(const CRegID &regId, const TxCord &cdpTxCord, CUserCdp &cdp) {
    if (!cdpCache.SetData(std::make_pair(regId.ToRawString(), cdpTxCord.ToRawString()), cdp))
        return false;

    cdp.UpdateUserCdp(regId, cdpTxCord);

    return true;
}

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *  a = 1, b = 1
 *
 *  ==> ratio = 1/Log10(1+N)
 */
uint64_t CCdpCacheManager::ComputeInterest(int blockHeight, const CUserCdp &cdp) {
    assert(uint64_t(blockHeight) > cdp.lastBlockHeight);

    int interval = blockHeight - cdp.lastBlockHeight;
    double interest = ((double) GetInterestParamA() * cdp.totalOwedScoins / kYearBlockCount)
                    * log10(GetInterestParamB() + cdp.totalOwedScoins) * interval;

    return (uint64_t) interest;
}