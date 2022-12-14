/*================================================================================                               
*     Source: ../RCM/StrategyStudio/examples/strategies/SimpleMomentumStrategy/SimpleMomentumStrategy.cpp                                                        
*     Last Update: 2013/6/1 13:55:14                                                                            
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

#ifdef _WIN32
    #include "stdafx.h"
#endif

#include "resistance.h"


using namespace RCM::StrategyStudio;
using namespace RCM::StrategyStudio::MarketModels;
using namespace RCM::StrategyStudio::Utilities;

ResistanceStrategy::ResistanceStrategy(StrategyID strategyID, const std::string& strategyName, const std::string& groupName):
    Strategy(strategyID, strategyName, groupName)
{

}

ResistanceStrategy::~ResistanceStrategy()
{
}

void ResistanceStrategy::OnResetStrategyState()
{
}

void ResistanceStrategy::DefineStrategyParams()
{
}

void ResistanceStrategy::DefineStrategyCommands()
{
}

void ResistanceStrategy::RegisterForStrategyEvents(StrategyEventRegister* eventRegister, DateType currDate)
{
    for (SymbolSetConstIter it = symbols_begin(); it != symbols_end(); ++it) {
        std::string symbol = *it;
        std::cout << symbol << '\n';

        std::string fname = "/vagrant/ie421/python_script/data/" + symbol + ".csv";

        std::vector<std::string> row;
        std::string line, word;

        std::fstream file (fname, std::ios::in);
        if(file.is_open())
        {
            while(getline(file, line))
            {
                row.clear();

                std::stringstream str(line);

                while(getline(str, word, ','))
                {
                    row.push_back(word);
                }
                SRLine line = SRLine(row);
                symbol_to_srlines[symbol].push_back(line);
            }
        }
        else {
            std::cout << "Not found" << '\n';
        }
    }

}
void ResistanceStrategy::OnTrade(const TradeDataEventMsg& msg)
{
    // find out how much capital
    // store instrument state
    // portfolio().cash_balance();

    double price = msg.trade().price();

    for (SRLine& line: symbol_to_srlines[msg.instrument().symbol()]) {
        // if top bid dips below a support line
        if (!line.is_resistance) {
            // if not broken, check if new price breaks support line
            if (line.state == 0) {
                if (price <= line.price){
                    line.state = 1;
                }
            }
                // if already broken, check if we rebound above support line
            else if (line.state == 1){
                if (price > line.price){
                    line.state = 2;
                    // calculate qty
                    double max_risked = portfolio().cash_balance() * per_trade_risk;
                    double max_risked_per_share = line.weight * price;
                    int quantity = floor(max_risked / max_risked_per_share);
                    SendBuyOrder(&msg.instrument(), quantity);
                    line.holdings += quantity;
                }
            }
                // if we are already holding, check if we want to sell
            else if (line.state == 2){
                // if profit taker or stop loss satisfied we sell
                // todo set stoploss/profit taker price to price that we actually bought at
                // might want to place resting orders  on the order book for profit taker instead
                if (price <=  (1-line.weight) * line.price || price >= (1+2*line.weight) * line.price){
                    SendSellOrder(&msg.instrument(), line.holdings);
                    line.holdings = 0;
                    line.state = 3;
                }
            }
        } else {
            if (line.state == 0) {
                if (price >= line.price){
                    line.state = 1;
                }
            }
                // if already broken, check if we rebound above support line
            else if (line.state == 1){
                if (price < line.price){
                    line.state = 2;
                    // calculate qty
                    double max_risked = portfolio().cash_balance() * per_trade_risk;
                    double max_risked_per_share = line.weight * price;
                    int quantity = floor(max_risked / max_risked_per_share);
                    SendSellOrder(&msg.instrument(), quantity);
                    line.holdings += quantity;
                }
            }
                // if we are already holding, check if we want to sell
            else if (line.state == 2){
                // if profit taker or stop loss satisfied we sell
                // todo set stoploss/profit taker price to price that we actually bought at
                // might want to place resting orders  on the order book for profit taker instead
                if (price <=  (1-line.weight) * line.price || price >= (1+2*line.weight) * line.price){
                    SendBuyOrder(&msg.instrument(), line.holdings);
                    line.holdings = 0;
                    line.state = 3;
                }
            }
        }
    }
}

void ResistanceStrategy::OnTopQuote(const QuoteEventMsg& msg)
{

}

void ResistanceStrategy::SendBuyOrder(const Instrument* instrument, int unitsNeeded)
{
    OrderParams params(*instrument,
                       unitsNeeded,
                       (instrument->top_quote().ask() != 0) ? instrument->top_quote().ask() : instrument->last_trade().price(),
                       (instrument->type() == INSTRUMENT_TYPE_EQUITY) ? MARKET_CENTER_ID_NASDAQ : ((instrument->type() == INSTRUMENT_TYPE_OPTION) ? MARKET_CENTER_ID_CBOE_OPTIONS : MARKET_CENTER_ID_CME_GLOBEX),
                       ORDER_SIDE_BUY,
                       ORDER_TIF_DAY,
                       ORDER_TYPE_MARKET);

    trade_actions()->SendNewOrder(params);
}

void ResistanceStrategy::SendSellOrder(const Instrument* instrument, int unitsNeeded)
{
    OrderParams params(*instrument,
                       unitsNeeded,
                       (instrument->top_quote().bid() != 0) ? instrument->top_quote().bid() : instrument->last_trade().price(),
                       (instrument->type() == INSTRUMENT_TYPE_EQUITY) ? MARKET_CENTER_ID_NASDAQ : ((instrument->type() == INSTRUMENT_TYPE_OPTION) ? MARKET_CENTER_ID_CBOE_OPTIONS : MARKET_CENTER_ID_CME_GLOBEX),
                       ORDER_SIDE_SELL,
                       ORDER_TIF_DAY,
                       ORDER_TYPE_MARKET);

    trade_actions()->SendNewOrder(params);
}
void ResistanceStrategy::OnQuote(const QuoteEventMsg& msg)
{
}

void ResistanceStrategy::OnDepth(const MarketDepthEventMsg& msg)
{
}

void ResistanceStrategy::OnBar(const BarEventMsg& msg)
{
}

void ResistanceStrategy::OnOrderUpdate(const OrderUpdateEventMsg& msg)
{
}

void ResistanceStrategy::OnStrategyCommand(const StrategyCommandEventMsg& msg)
{
}

void ResistanceStrategy::OnParamChanged(StrategyParam& param)
{
}

