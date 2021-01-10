CXX      := -c++
CXXFLAGS := -std=c++14 -stdlib=libc++
LDFLAGS  := -L /opt/homebrew/Cellar/boost/1.75.0/lib 
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
DAT_DIR  := ./data
OUT_DIR  := ./output
TARGET   := bond_trading_system
INCLUDE  := -Iinclude/ -I/opt/homebrew/Cellar/boost/1.75.0/include
SRC      := $(wildcard src/main.cpp)

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ 

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

	$(CXX) $(CXXFLAGS) $(INCLUDE) ./src/data_reader.cpp -o $(APP_DIR)/data_reader
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./src/data_writer.cpp -o $(APP_DIR)/data_writer

.PHONY: all build clean debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG_TEST\(fmt,arg...\)=printf\(\"[debug]:\ \"\ fmt,\#\#arg\) -g
debug: all

release: CXXFLAGS += -O3 -DDEBUG_TEST\(fmt,arg...\)=\{\}
release: all

run: 
	clear
	
	# generate txt data
	python data_generator.py

	# server process reading from prices.txt 
	$(APP_DIR)/data_reader 1234 & 

	# server process reading from trades.txt 
	$(APP_DIR)/data_reader 1236 & 

	# server process reading from marketdata.txt 
	$(APP_DIR)/data_reader 1237 & 

	# server process reading from inquiries.txt 
	$(APP_DIR)/data_reader 1242 & 

	# server process writing to executions.txt 
	$(APP_DIR)/data_writer 1238 &
	
	# server process writing to positions.txt 
	$(APP_DIR)/data_writer 1239 &

	# server process writing to risk.txt 
	$(APP_DIR)/data_writer 1240 &

	# server process writing to streaming.txt 
	$(APP_DIR)/data_writer 1241 &

	# server process writing to gui.txt 
	$(APP_DIR)/data_writer 1235 &

	# server process writing to allinquiries.txt on port=1243
	$(APP_DIR)/data_writer 1243 &

	# launch the bond trading system
	build/apps/$(TARGET)

clean:
	clear
	# cleaning the binary files and data files
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(DAT_DIR)/*
	-@rm -rvf $(OUT_DIR)/*
	
