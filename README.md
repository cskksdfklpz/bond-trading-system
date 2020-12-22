# Final Project for MTH 9815: A Trading System

by Quanzhi Bi

# Getting Started

To run the bond trading system, first clone the repo to your local machine.

```bash
git clone https://gitlab.com/quanzhibi/bond-trading-system.git
cd bond-trading-system 
```

Then first clean the binary files 

```bash
make clean
```

with the following output

```bash
# cleaning the binary files and data files
./build/objects/src/main.o
./build/objects/src
./build/apps/bond_trading_system
./build/apps/data_reader
./build/apps/data_writer
./data/inquiries.txt
./data/marketdata.txt
./data/prices.txt
./data/trades.txt
./output/allinquiries.txt
./output/executions.txt
./output/gui.txt
./output/positions.txt
./output/risk.txt
./output/streaming.txt
```

Then we compile the system. You need to change the following boost header/lib path of your own in Makefile.

```Makefile
INCLUDE  := -Iinclude/
LDFLAGS  := -L /usr/local/Cellar/boost/1.72.0_2/lib 
```

If you need, you can also change the compiler (to whatever you want, `g++`, `clang`, `MSVC`):

```Makefile
CXX      := -c++
```

You have two options, namely `debug mode` (print a lot of debug info to the terminal, slow) and `release mode` (used -O3, barely no output to the terminal, fast). 

To compile in debug mode, type the following command in the terminal.

```bash
make debug
```

To compile in release mode, type the following command in the terminal.

```bash
make release
```

After that, change the parameter (number of data generated) in the python script `data_generator.py`:

```python
generate_price(10000)
generate_trade(10)
generate_market_data(10000)
generate_inquiry(10)
```

Change the number to be 1 million if you want (for simplicity I used a smaller configuration).

Finally to run the bond-trading-system, just type

```bash
make run
```

You may get the output in the terminal like this (release mode)

```bash
# generate txt data
python data_generator.py


generating n=10000 pieces of price data

100%|███████████████████████| 10000/10000 [00:00<00:00, 10528.23it/s]


generating trading data

100%|██████████████████████████████| 10/10 [00:00<00:00, 2185.33it/s]


generating market data

100%|████████████████████████| 10000/10000 [00:02<00:00, 3959.99it/s]


generating inquriy data

100%|█████████████████████████████| 10/10 [00:00<00:00, 19793.79it/s]
# server process reading from prices.txt 
./build/apps/data_reader 1234 & 
# server process reading from trades.txt 
./build/apps/data_reader 1236 & 
Using port 1234
# server process reading from marketdata.txt 
Using port 1236
./build/apps/data_reader 1237 & 
# server process reading from inquiries.txt 
Using port 1237
./build/apps/data_reader 1242 & 
# server process writing to executions.txt 
Using port 1242
./build/apps/data_writer 1238 &
# server process writing to positions.txt 
./build/apps/data_writer 1239 &
Using port 1238
# server process writing to risk.txt 
Using port 1239
./build/apps/data_writer 1240 &
# server process writing to streaming.txt 
Using port 1240
./build/apps/data_writer 1241 &
# server process writing to gui.txt 
Using port 1241
./build/apps/data_writer 1235 &
# server process writing to allinquiries.txt on port=1243
Using port 1235
./build/apps/data_writer 1243 &
# launch the bond trading system
Using port 1243
build/apps/bond_trading_system
connecting to the ./output/positions.txt...success
connecting to the ./output/risk.txt...success
connecting to the ./data/trades.txt...success
connecting to the ./output/executions.txt...success
connecting to the ./data/marketdata.txt...success
connecting to the ./output/gui.txt...success
connecting to the ./output/streaming.txt...success
connecting to the ./data/prices.txt...success
connecting to the ./output/allinquiries.txt...success
connecting to the data server...success
Finished, killing the data_writer (./output/allinquiries.txt) process
Finished, killing the data_writer (./output/streaming.txt) process
Finished, killing the data_writer (./output/gui.txt) process
Finished, killing the data_writer (./output/executions.txt) process
Finished, killing the data_writer (./output/risk.txt) process
Finished, killing the data_writer (./output/positions.txt) process
```

If you can't run the code, you need to change the port number in the source code `src/main.cpp` and `Makefile` (that means some applications are using port from `1234` to `1243`, change it to free port!).
