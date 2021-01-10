import numpy as np
from tqdm import tqdm


cusips = ['91282CAX9',  #  2Y
          '91282CBA80', # 3Y
          '91282CAZ4', # 5Y
          '91282CAY7', # 7Y
          '91282CAV3', # 10Y
          '912810ST6', # 20Y
          '912810SS8'] # 30Y


def increase(integer, xy, z, dz=1):
    '''
    the fractional notation is integer-xyz
    where xy in [0, 31], z in [0,7]
    we want to increase this price by 1/256 (z+1)
    '''

    z += dz
    if z >= 8:
        z -= 8
        xy += 1
        if xy >= 32:
            xy -= 32
            integer += 1
    return integer, xy, z

def decrease(integer, xy, z, dz=1):
    '''
    the fractional notation is integer-xyz
    where xy in [0, 31], z in [0,7]
    we want to decrease this price by 1/256 (z-1)
    '''

    z -= dz
    if z < 0:
        z += 8
        xy -= 1
        if xy < 0:
            xy += 32
            integer -= 1

    return integer, xy, z

def generate_price(n):
    print("\n")
    print("generating n=" + str(n) + " pieces of price data\n")
    f =  open('./data/prices.txt', 'w', newline = '')
    # generate n pieces of data.
    increasing = True
    # the mid price start with 99-000
    price = (98,31,7)
    count = 0
    for i in tqdm(range(n)):

        # for each security
        for cusip in cusips:
            #  The file should create prices which oscillate between
            # 99 and 101, moving by the smallest increment each
            # time up from 99 and then down from 101
            if (increasing == True):
                price = increase(*price)
                if price == (101,0,0):
                    increasing = False
            else:
                price = decrease(*price)
                if price == (99,0,0):
                    increasing = True
            # The bid/offer spread should oscillate between 1/128 and 1/64.
            spread = 1+(count%2)
            count += 1

            spread = str(np.random.randint(1, 3))
            f.write(cusip+","+str(price[0])+"-"+format(price[1],'02d')+format(price[2],'01d')+","+spread+"\n")

    f.close()

def generate_trade(n):
    print("\n")
    print("generating trading data\n")
    f =  open('./data/trades.txt', 'w', newline = '')
    # generate n pieces of data.
    idx = 0
    for i in tqdm(range(n)):

        # for each security
        for cusip in cusips:
            tradeId = 'TradeId' + str(idx)
            idx += 1
            book = 'TRSY' + str(np.random.randint(1, 4))

            # The price should oscillate between 99.0 (BUY) and 100.0 (SELL).
            price1 = str(np.random.randint(99, 101))
            price2 = str(np.random.randint(0, 32)).zfill(2)
            price3 = str(np.random.randint(0, 8))
            price = price1 + '-' + price2 + price3

            # Trades for each security should alternate between BUY and SELL
            side = 'BUY' if (i%2==0) else 'SELL'
            # and cycle from 1000000, 2000000, 3000000, 4000000, and 5000000
            # for quantity, and then repeat back from 1000000.
            quantity = str((1+(i%5)) * 1000000)
            # The price should oscillate between 99.0 (BUY) and 100.0 (SELL).
            price = "99.0" if side=="BUY" else "100.0"

            f.write(cusip+","+tradeId+","+book+","+price+","+side+","+quantity+"\n")

    f.close()

def generate_market_data(n):
    print("\n")
    print("generating market data\n")
    f =  open('./data/marketdata.txt', 'w', newline = '')
    # generate n pieces of data.
    # generate n pieces of data.

    increasing = True
    # the mid price start with 99-000
    mid_price = (98,31,7)
    # the spread is from 2/256 -> 8/256 -> 2/256
    spreads = [2,4,6,8,6,4]
    count = 0

    for i in tqdm(range(n)):

        # for each security
        for cusip in cusips:

            # The file should create mid prices which oscillate
            # between 99 and 101, moving by the smallest increment
            # each time up from 99 and then down from 101
            # (bearing in mind that US Treasuries trade in
            # 1/256th increments) with a bid/offer spread starting
            # at 1/128th on the top of book (and widening in the smallest
            # increment from there for subsequent levels in the order book).
            if (increasing == True):
                mid_price = increase(*mid_price)
                if mid_price == (101,0,0):
                    increasing = False
            else:
                mid_price = decrease(*mid_price)
                if mid_price == (99,0,0):
                    increasing = True
            # The top of book spread itself should widen out on
            # successive updates by 1/128 until it reaches 1/32 ,
            # and then decrease back down to 1/128 in 1/128 intervals
            spread = spreads[count % 6]
            count += 1

            bids = [None] * 5
            offers = [None] * 5

            # bid4 bid3 bid2 bid1 bid0 offer0 offer1 offer2 offer3 offer4
            for i in range(5):
                bids[i] = decrease(*mid_price, dz=(i+int(spread/2)))
                offers[i] = increase(*mid_price, dz=(i+int(spread/2)))

            line = cusip+","
            for i in range(4,-1,-1):
                bid = bids[i]
                line += (str(bid[0])+"-"+format(bid[1],'02d')+format(bid[2],'01d')+",")
            for i in range(5):
                offer = offers[i]
                line += (str(offer[0])+"-"+format(offer[1],'02d')+format(offer[2],'01d')+",")

            line = line[:-1]
            f.write(line + "\n")

    f.close()

def generate_inquiry(n):
    print("\n")
    print("generating inquriy data\n")
    f =  open('./data/inquiries.txt', 'w', newline = '')
    # generate n pieces of data.
    idx = 0
    for i in tqdm(range(n)):

        # for each security
        for cusip in cusips:
            # Trades for each security should alternate between BUY and SELL
            side = 'BUY' if (i%2==0) else 'SELL'
            f.write(str(idx)+","+cusip+","+side+","+"\n")
            idx += 1
    f.close()

# change the number of data generated here
generate_price(10000)
generate_trade(10)
generate_market_data(10000)
generate_inquiry(10)
