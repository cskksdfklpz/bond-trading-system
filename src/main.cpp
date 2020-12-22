/**
 * main.cpp
 * The main function with class structure diagram
 * @author Quanzhi Bi
 */

#include <iostream>

#include "bondinfo.hpp"
#include "executionservice.hpp"
#include "guiservice.hpp"
#include "historicaldataservice.hpp"
#include "inquiryservice.hpp"
#include "marketdataservice.hpp"
#include "positionservice.hpp"
#include "pricingservice.hpp"
#include "products.hpp"
#include "riskservice.hpp"
#include "streamingservice.hpp"
#include "tradebookingservice.hpp"

std::vector<std::string> BondInfo::cusips = {};
std::map<std::string, boost::gregorian::date *> BondInfo::date_map = {};
std::map<std::string, Bond *> BondInfo::bond_map = {};

int main(int argc, char *argv[]) {
    DEBUG_TEST("Running the program in the debug mode.\n");

    BondInfo::init();

    /* trades.txt 
     *     	|
	 *     	v		  (port=1236)	
	 * (data_reader ->  TCP/IP -> BondTradeBookingConnector) 
	 * 		|
	 *      V
	 * BondTradeBookingService <--------------------|
	 * 	   	|										|
	 * 		V										|
	 * (BondPositionListener)		(BondTradeBookingListener) <- BondExecutionService
	 * 		|										
	 * 		V										
	 * BondPositionService -> (HistoricalDataListener<Position<Bond>)				
	 * 		|									|
	 * 		V									V
	 * (BondRiskListener)	   HistoricalDataService<Position<Bond>> -> BondPositionConnector -> ./output/positions.txt
	 * 		|
	 * 		V
	 * BondRiskService -> (HistoricalDataListener<PV01<Bond>>) 
	 * 									|
	 * 									V
	 * 						HistoricalDataService<PV01<Bond>>
	 * 									|		
	 * 									V
	 * 						BondRiskConnector -> ./output/risk.txt
	 * 								
	 */

    BondPositionConnector bond_position_connector("./output/positions.txt", 1239);
    HistoricalDataService<Position<Bond>> bond_position_HDS(&bond_position_connector, "Position<Bond>");
    HistoricalDataListener<Position<Bond>> bond_position_HDL(&bond_position_HDS);

    BondRiskConnector bond_risk_connector("./output/risk.txt", 1240);
    HistoricalDataService<PV01<Bond>> bond_risk_HDS(&bond_risk_connector, "PV01<Bond>");
    HistoricalDataListener<PV01<Bond>> bond_risk_HDL(&bond_risk_HDS);

    // BondRiskService and Listener
    BondRiskService bond_risk_service;
    BondRiskListener bond_risk_listener(&bond_risk_service);
    bond_risk_service.AddListener(&bond_risk_HDL);

    // BondPositionService, register the BondRiskListener
    BondPositionService bond_position_service;
    bond_position_service.AddListener(&bond_risk_listener);
    bond_position_service.AddListener(&bond_position_HDL);

    // BondPositionListener
    BondPositionListener bond_position_listener(&bond_position_service);
    // BondTradeBookingService, register the BondPositionListener
    BondTradeBookingService bond_trade_booking_service;
    bond_trade_booking_service.AddListener(&bond_position_listener);

    // connector connect to the data server via TCP/IP
    BondTradeBookingConnector bond_trade_booking_connector("./data/trades.txt", &bond_trade_booking_service);
    bond_trade_booking_connector.Subscribe(1236);

    /* marketdata.txt 
	 *     	|
	 *     	v		  (port=1237)	
	 * (data_reader ->  TCP/IP -> BondMarketDataConnector) 
	 * 		|
	 *      V
	 * BondMarketDataService 
	 * 	   	|
	 * 		V
	 * (BondAlgoExecutionListener)
	 * 		|
	 * 		V
	 * BondAlgoExecutionService
	 * 		|
	 * 		V
	 * (BondExecutionListener)
	 * 		|
	 * 		V
	 * BondExecutionService -------------------------------------
	 * 		|													|
	 * 		V													V
	 * (HistoricalDataListener<ExecutionOrder<Bond>)		(BondTradeBookingListener)
	 * 		|													|
	 * 		V													V
	 * 	HistoricalDataService<ExecutionOrder<Bond>>			BondTradeBookingService
	 * 		|
	 * 		V
	 * 	(BondExecutionConnector)
	 * 		|
	 * 		V
	 * 	output/executions.txt
	 */

    BondExecutionConnector bond_execution_connector("./output/executions.txt", 1238);
    HistoricalDataService<ExecutionOrder<Bond>> bond_execution_HDS(&bond_execution_connector, "ExecutionOrder<Bond>");
    HistoricalDataListener<ExecutionOrder<Bond>> bond_execution_HDL(&bond_execution_HDS);

    // BondTradeBookingListener
    BondTradeBookingListener bond_trade_booking_listener(&bond_trade_booking_service);

    // BondExecutionService and listener
    BondExecutionService bond_execution_service;
    BondExecutionListener bond_execution_listener(&bond_execution_service);
    bond_execution_service.AddListener(&bond_trade_booking_listener);
    bond_execution_service.AddListener(&bond_execution_HDL);

    // BondAlgoExecutionService, register the BondExecutionListener
    BondAlgoExecutionService bond_algo_execution_service;
    BondAlgoExecutionListener bond_algo_execution_listener(&bond_algo_execution_service);
    bond_algo_execution_service.AddListener(&bond_execution_listener);

    // BondMarketDataService, register the BondAlgoExecutionListener
    BondMarketDataService bond_marketdata_service;
    bond_marketdata_service.AddListener(&bond_algo_execution_listener);

    // connector connect to the data server via TCP/IP
    BondMarketDataConnector bond_marketdata_connector("./data/marketdata.txt", &bond_marketdata_service);
    bond_marketdata_connector.Subscribe(1237);

    /* prices.txt 
	 *     |
	 *     v		  (port=1234)	
	 * (data_reader ->  TCP/IP -> BondPricingConnector) 
	 *     |
	 *     V
	 * BondPricingService -----------------------------------
	 *     |												|
	 * 	   V												V
	 * (GUIServiceListener) 						(BondAlgoStreamingListener)
	 *     |												|
	 *     V												V
	 * GUIService									BondAlgoStreamingService
	 * 	   |												|
	 * 	   V		   (port=1235)						    V
	 * (GUIConnector -> TCP/IP -> data_wrtier)		(BondStreamingListener)
	 * 	   |												|
	 * 	   V												V
	 * output/gui.txt								BondStreamingService
	 * 														|
	 * 														V
	 * 												(HistoricalDataListener<PriceStream<Bond>)
	 * 														|
	 * 														V
	 * 												HistoricalDataService<PriceStream<Bond>
	 * 														|
	 * 														V
	 * 												BondStreamingConnector -> output/streaming.txt
	 */
    // GUI connector/service/listerner
    GUIConnector<Bond> gui_connector("./output/gui.txt");
    GUIService<Bond> gui_service(&gui_connector, 300);
    GUIServiceListener<Bond> gui_service_listener(&gui_service);

    BondStreamingConnector bond_streaming_connector("./output/streaming.txt", 1241);
    HistoricalDataService<PriceStream<Bond>> bond_streaming_HDS(&bond_streaming_connector, "PriceStream<Bond>");
    HistoricalDataListener<PriceStream<Bond>> bond_streaming_HDL(&bond_streaming_HDS);

    // BondStreaming service/listner
    BondStreamingService bond_streaming_service;
    BondStreamingListener bond_streaming_listener(&bond_streaming_service);
    bond_streaming_service.AddListener(&bond_streaming_HDL);

    // BondAlgoStreaming service/listener, register bond_streaming_service
    BondAlgoStreamingService bond_algo_streaming_service;
    BondAlgoStreamingListener bond_algo_streaming_listener(&bond_algo_streaming_service);
    bond_algo_streaming_service.AddListener(&bond_streaming_listener);

    // BondPricing service, register GUI/BondAlgoStreaming listener
    BondPricingService pricing_service;
    pricing_service.AddListener(&gui_service_listener);
    pricing_service.AddListener(&bond_algo_streaming_listener);

    // Pricing connector
    BondPricingConnector pricing_connector("./data/prices.txt", &pricing_service);
    pricing_connector.Subscribe(1234);

    /* inquiries.txt 
	 *     	|
	 *     	v		  (port=1242)	
	 * (data_reader ->  TCP/IP -> BondInquiryConnector) 
	 * 		|
	 *      V
	 * BondInquiryService <--------------------> QuoteConnector
	 * 	   	|										
	 * 		V										
	 * (HistoricalDataListener<Inquiry<Bond>>)		
	 * 		|										
	 * 		V										
	 * HistoricalDataService<Inquiry<Bond>> 
	 * 		|									
	 * 		V									
	 * BondAllInquiriesConnector 
	 * 		|
	 * 		V
	 * ./output/allinquiries.txt 						
	 */

    BondAllInquiriesConnector bond_allinquiries_connector("./output/allinquiries.txt", 1243);
    HistoricalDataService<Inquiry<Bond>> bond_allinquiries_HDS(&bond_allinquiries_connector, "Inquiry<Bond>");
    HistoricalDataListener<Inquiry<Bond>> bond_allinquiries_HDL(&bond_allinquiries_HDS);

    QuoteConnector quote_connector;
    BondInquiryService bond_inquiry_service(&quote_connector);
    bond_inquiry_service.AddListener(&bond_allinquiries_HDL);
    BondInquiryConnector bond_inquiry_connector("./data/inquiries.txt", &bond_inquiry_service);
    bond_inquiry_connector.Subscribe(1242);

    BondInfo::clean();

    return 0;
}
