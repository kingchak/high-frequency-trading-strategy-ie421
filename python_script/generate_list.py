
#!pip install yfinance
import numpy as np
import pandas as pd
import yfinance as yf

import matplotlib.pyplot as plt


def support(df, index, n1, n2):
    # n1 (n2) candles before (after) the index candle resp.
    # test whether the index candle is support or not
    for i in range(index - n1 + 1, index + 1):
        if( df['Low'][i] > df['Low'][i-1]):
            return False
    for i in range(index+1, index + n2 + 1):
        if( df['Low'][i] < df['Low'][i-1] ):
            return False
    
    return True

def resistance(df, index, n1, n2):
    # n1 (n2) candles before (after) the index candle resp.
    # test whether the index candle is resistance or not
    for i in range(index-n1+1, index+1):
        if(df['High'][i] < df['High'][i-1]):
            return False
    for i in range(index + 1, index + n2 + 1):
        if(df['High'][i] > df['High'][i-1]):
            return False
    
    return True



def generate_csv(stock):
    df= yf.download(stock, '2021-03-01', '2022-03-01') 
    df.reset_index( inplace = True)
    # using two list support_list, resistance_list
    support_list = []
    resistance_list = []
    # add a volume and  time weighing
    volsums=0
    volsumr=0
    vollists=[]
    vollistr=[]
    timelists=[]
    timelistr=[]

    # paramater for changing sensitivity of the algorithm
    n1 = 2
    n2 = 2

    for row in range(n1, len(df) - n2):

        if support(df, row, n1, n2 )&((max(df.index)-365)<=row):
            support_list.append((row , df['Low'][row], 0, df['Volume'][row], df['Date'][row]))
            volsums+=df['Volume'][row]
            timelists.append(df['Date'][row])
            vollists.append(df['Volume'][row])
        if resistance(df , row, n1, n2)&((max(df.index)-365)<=row):
            resistance_list.append((row, df['High'][row],1, df['Volume'][row], df['Date'][row]))
            volsumr+=df['Volume'][row]
            timelistr.append(df['Date'][row])
            vollistr.append(df['Volume'][row])


    support_list.sort()
    resistance_list.sort()

    # merge lines that are close to each other
    for i in range(1 , len(support_list)):
        if ( i >= len(support_list)):
            break
        if abs( support_list[i][1] - support_list[i-1][1] <= 1):
            support_list.pop(i)
            vollists.pop(i)
            timelists.pop(i)

    for i in range(1, len(resistance_list)):
        if( i >= len(resistance_list)):
            break
        if abs( resistance_list[i][1] - resistance_list[i-1][1] <= 1):
            resistance_list.pop(i)
            vollistr.pop(i)
            timelistr.pop(i)

    # support weights
    volws=[]
    timews=[]
    target=vollists
    target=pd.Series(target)
    ttt=pd.to_datetime(timelists)
    a=target.ewm(halflife='252 days', times=ttt).mean()


    for i in range(len(support_list)):
        vw=a[i]/sum(a)
        volws.append(vw)

    # resistance weights
    volwr=[]
    timewr=[]
    target=vollistr
    target=pd.Series(target)
    ttt=pd.to_datetime(timelistr)
    a=target.ewm(halflife='252 days', times=ttt).mean()


    for i in range(len(resistance_list)):
        vw=a[i]/sum(a)
        volwr.append(vw)

    sigmaS = np.std(volws)
    muS = np.mean(volws)

    volSupportClass = []

    # Support Price Level Weight
    # Classify raw weight number into three position classes(less aggressive, normal, aggressive)
    for weight in volws:
        if weight < muS -sigmaS:
            # weight is less than one standard deviation so categorize to less aggressive class
            # 0 means less aggressive position sizing
            volSupportClass.append(0)
        elif weight > muS + sigmaS:
            #weight more than mean + one standard deviation,
            # more aggressive positon sizing
            volSupportClass.append(2)
        else:
            #normal position size
            volSupportClass.append(1)

    # resistance weight 
    sigmaR = np.std(volwr)
    muR = np.mean(volwr)

    #print(mu-sigma)
    #print(mu+sigma)

    # For Resistance Price Level Weight
    volResistanceClass = []

    for weight in volwr:
        if weight < muR -sigmaR:
            # weight is less than one standard deviation so categorize to less aggressive class
            # 0 means less aggressive position sizing
            volResistanceClass.append(0)
        elif weight > muR + sigmaR:
            #weight more than mean + one standard deviation,
            # more aggressive positon sizing
            volResistanceClass.append(2)
        else:
            #normal position size
            volResistanceClass.append(1)

    support_df = pd.DataFrame(support_list, columns =['row', 'price', 'signal', 'volume', 'date'])
    support_df.drop(columns =['row'] , inplace =True)
    support_df['weight'] = volSupportClass

    resistance_df = pd.DataFrame(resistance_list, columns =['row', 'price', 'signal', 'volume', 'date'])
    resistance_df.drop(columns =['row'] , inplace =True)
    resistance_df['weight'] = volResistanceClass

    # concat the support and resistance dataframe
    df_concat = pd.concat([support_df, resistance_df] ,axis =0 )
    df_concat.reset_index(inplace = True)
     
    sorted_df = df_concat.sort_values(by=["date"], ascending=False)
    #sorted_df.to_csv('homes_sorted.csv', index=False)

    sorted_df.to_csv(f"./data/{stock}.csv" ,index =False, header=False)


stockList = ["DIA", "AAPL", "TSLA"]

for stock in stockList:
    generate_csv(stock)

# Signal column in the csv: 
# 0 indicates support level, 1 indicates resistance level

# Weight column in the csv: 
# 0 indicates less aggressive position size( e.g. max risk 0.5% total equity)
# 1 indicates normal position size  (e.g. max risk 1% total equity)
# 2 indicates aggressive position size (e.g. max risk 2% total equity)
