/*================================================================================                               
*     Source: ../RCM/StrategyStudio/examples/strategies/SimpleMomentumStrategy/SimpleMomentumStrategy.h                                                        
*     Last Update: 2013/06/1 13:55:14                                                                            
*     Contents:                                     
*     Distribution:          
*                                                                                                                
*                                                                                                                
*     Copyright (c) RCM-X, 2011 - 2013.                                                  
*     All rights reserved.                                                                                       
*                                                                                                                
*     This software is part of Licensed material, which is the property of RCM-X ("Company"), 
*     and constitutes Confidential Information of the Company.                                                  
*     Unauthorized use, modification, duplication or distribution is strictly prohibited by Federal law.         
*     No title to or ownership of this software is hereby transferred.                                          
*                                                                                                                
*     The software is provided "as is", and in no event shall the Company or any of its affiliates or successors be liable for any 
*     damages, including any lost profits or other incidental or consequential damages relating to the use of this software.       
*     The Company makes no representations or warranties, express or implied, with regards to this software.                        
/*================================================================================*/ 

#pragma once

#ifndef _STRATEGY_STUDIO_LIB_EXAMPLES_SIMPLE_MOMENTUM_STRATEGY_H_
#define _STRATEGY_STUDIO_LIB_EXAMPLES_SIMPLE_MOMENTUM_STRATEGY_H_

#ifdef _WIN32
    #define _STRATEGY_EXPORTS __declspec(dllexport)
#else
    #ifndef _STRATEGY_EXPORTS
    #define _STRATEGY_EXPORTS
    #endif
#endif

#include <Strategy.h>
#include <Analytics/ScalarRollingWindow.h>
#include <Analytics/InhomogeneousOperators.h>
#include <Analytics/IncrementalEstimation.h>
#include <MarketModels/Instrument.h>
#include <Utilities/ParseConfig.h>

#include <vector>
#include <map>
#include <iostream>
#include "FillInfo.h"
#include "AllEventMsg.h"
#include "ExecutionTypes.h"
#include <Utilities/Cast.h>
#include <Utilities/utils.h>

#include <math.h>
#include <cassert>
#include <fstream>
#include <string>
#include <sstream>

using namespace RCM::StrategyStudio;

class SRLine {
public:
    float weight;
    bool is_resistance;
    float price;
    int holdings = 0;
    // 0 = not broken, 1 = broken, 2 = broken then recovered
    int state = 0;
    SRLine(std::vector<std::string> content)
    {
        switch(stoi(content[5])) {
            case 0:
                weight = 0.005;
                break;
            case 1:
                weight = 0.01;
                break;
            case 2:
                weight = 0.02;
                break;
            default:
                weight = 0.01;
                break;
        }
        is_resistance = stoi(content[2]) == 1;
        price = std::ceil(std::stof(content[1]) * 100.0) / 100.0;
    }


    std::string toString() const {
        std::stringstream ss;
        ss << "Price: " << price << ", is_resistance:" << is_resistance << ", Weight:" << weight << ", state:" << state;
        return ss.str();
    }
};

class ResistanceStrategy : public Strategy {
public:
    ResistanceStrategy(StrategyID strategyID, const std::string& strategyName, const std::string& groupName);
    ~ResistanceStrategy();

public: /* from IEventCallback */

    /**
     * This event triggers whenever trade message arrives from a market data source.
     */
    virtual void OnTrade(const TradeDataEventMsg& msg);

    /**
     * This event triggers whenever aggregate volume at best price changes, based
     * on the best available source of liquidity information for the instrument.
     *
     * If the quote datasource only provides ticks that change the NBBO, top quote will be set to NBBO
     */
    virtual void OnTopQuote(const QuoteEventMsg& msg);

    /**
     * This event triggers whenever a new quote for a market center arrives from a consolidate or direct quote feed,
     * or when the market center's best price from a depth of book feed changes.
     *
     * User can check if quote is from consolidated or direct, or derived from a depth feed. This will not fire if
     * the data source only provides quotes that affect the official NBBO, as this is not enough information to accurately
     * mantain the state of each market center's quote.
     */
    virtual void OnQuote(const QuoteEventMsg& msg);

    /**
     * This event triggers whenever a order book message arrives. This will be the first thing that
     * triggers if an order book entry impacts the exchange's DirectQuote or Strategy Studio's TopQuote calculation.
     */
    virtual void OnDepth(const MarketDepthEventMsg& msg);

    /**
     * This event triggers whenever a Bar interval completes for an instrument
     */
    virtual void OnBar(const BarEventMsg& msg);

    /**
     * This event contains alerts about the state of the market
     */
    virtual void OnMarketState(const MarketStateEventMsg& msg){};

    /**
     * This event triggers whenever new information arrives about a strategy's orders
     */
    virtual void OnOrderUpdate(const OrderUpdateEventMsg& msg);

    /**
     * This event contains strategy control commands arriving from the Strategy Studio client application (eg Strategy Manager)
     */
    virtual void OnStrategyControl(const StrategyStateControlEventMsg& msg){}

    /**
     *  Perform additional reset for strategy state
     */
    void OnResetStrategyState();

    /**
     * This event contains alerts about the status of a market data source
     */
    void OnDataSubscription(const DataSubscriptionEventMsg& msg){}

    /**
     * This event triggers whenever a custom strategy command is sent from the client
     */
    void OnStrategyCommand(const StrategyCommandEventMsg& msg);

    /**
     * Notifies strategy for every succesfull change in the value of a strategy parameter.
     *
     * Will be called any time a new parameter value passes validation, including during strategy initialization when default parameter values
     * are set in the call to CreateParam and when any persisted values are loaded. Will also trigger after OnResetStrategyState
     * to remind the strategy of the current parameter values.
     */
    void OnParamChanged(StrategyParam& param);

private: // Helper functions specific to this strategy
    //helper strategy functions here for sending orders etc
    std::map<std::string, std::vector<SRLine>>  symbol_to_srlines;
    float per_trade_risk = 0.01;
    void SendBuyOrder(const Instrument* instrument, int trade_size);
    void SendSellOrder(const Instrument* instrument, int trade_size);


private: /* from Strategy */
    
    virtual void RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate);

    /**
     * Define any params for use by the strategy
     */
    virtual void DefineStrategyParams();

    /**
     * Define any strategy commands for use by the strategy
     */
    virtual void DefineStrategyCommands();

private:
};

extern "C" {

    _STRATEGY_EXPORTS const char* GetType()
    {
        return "ResistanceStrategy";
    }

    _STRATEGY_EXPORTS IStrategy* CreateStrategy(const char* strategyType, 
                                   unsigned strategyID, 
                                   const char* strategyName,
                                   const char* groupName)
    {
        if (strcmp(strategyType,GetType()) == 0) {
            return *(new ResistanceStrategy(strategyID, strategyName, groupName));
        } else {
            return NULL;
        }
    }

     // must match an existing user within the system 
    _STRATEGY_EXPORTS const char* GetAuthor()
    {
        return "dlariviere";
    }

    // must match an existing trading group within the system 
    _STRATEGY_EXPORTS const char* GetAuthorGroup()
    {
        return "UIUC";
    }

    // used to ensure the strategy was built against a version of the SDK compatible with the server version
    _STRATEGY_EXPORTS const char* GetReleaseVersion()
    {
        return Strategy::release_version();
    }
}

#endif

