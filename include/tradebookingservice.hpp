/**
 * tradebookingservice.hpp
 * Defines the data types and Service for trade booking.
 *
 * @author Breman Thuraisingham
 */
#ifndef TRADE_BOOKING_SERVICE_HPP
#define TRADE_BOOKING_SERVICE_HPP

#include <boost/date_time/gregorian/gregorian.hpp>
#include <map>
#include <string>
#include <vector>

#include "bondinfo.hpp"
#include "executionservice.hpp"
#include "products.hpp"
#include "soa.hpp"

// Trade sides
enum Side { BUY,
            SELL };

/**
 * Trade object with a price, side, and quantity on a particular book.
 * Type T is the product type.
 */
template <typename T>
class Trade {
   public:
    // ctor for a trade
    Trade(const T& _product, string _tradeId, double _price, string _book, long _quantity, Side _side) : product(_product) {
        tradeId = _tradeId;
        price = _price;
        book = _book;
        quantity = _quantity;
        side = _side;
    }

    // Get the product
    const T& GetProduct() const { return product; }

    // Get the trade ID
    const string& GetTradeId() const { return tradeId; }

    // Get the mid price
    double GetPrice() const { return price; }

    // Get the book
    const string& GetBook() const { return book; }

    // Get the quantity
    long GetQuantity() const { return quantity; }

    // Get the side
    Side GetSide() const { return side; }

   private:
    T product;
    string tradeId;
    double price;
    string book;
    long quantity;
    Side side;
};

/**
 * Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type.
 */
template <typename T>
class TradeBookingService : public Service<string, Trade<T> > {
   public:
    // Book the trade
    void BookTrade(const Trade<T>& trade) {}
};

/**
 * Bond Trade Booking Service to book trades to a particular book.
 * Keyed on trade id.
 * Type T is the product type (Bond).
 * Again we derive the class from TradeBookingService as a result
 * of open and close principle since we need to override some methods
 */

class BondTradeBookingService : public TradeBookingService<Bond> {
   private:
    std::map<string, Trade<Bond> > trades;

   public:
    // Book the trade
    void BookTrade(Trade<Bond>& _trade) {
        this->Notify(_trade);
    }
    // get the trade data
    virtual Trade<Bond>& GetData(string key) {
        return trades.find(key)->second;
    }
    // update the trades map and notify the listeners
    virtual void OnMessage(Trade<Bond>& _trade) {
        trades.erase(_trade.GetTradeId());
        trades.insert(std::make_pair(_trade.GetTradeId(), _trade));
        this->Notify(_trade);
    }
};

/**
 * Bond Trade Booking Connector to get date via TCP/IP.
 * Keyed on trade id.
 * Type T is the product type (Bond).
 */
class BondTradeBookingConnector : public Connector<Trade<Bond> > {
   private:
    string file_name;
    BondTradeBookingService* trade_booking_service;

   public:
    explicit BondTradeBookingConnector(string _file_name,
                                       BondTradeBookingService* _service) : file_name(_file_name),
                                                                            trade_booking_service(_service) {}
    // we don't need this method
    virtual void Publish(Trade<Bond>& _trade) {}
    //
    void Subscribe(int port) {
        boost::asio::io_service io_service;
        // socket creation
        tcp::socket socket(io_service);
        // connection on localhost
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));

        // send the file request message to the server
        std::cout << "connecting to the " << file_name << "...";
        file_name += "\n";
        send_socket(socket, file_name);

        string line = read_socket(socket);

        trim(line);
        std::cout << "success" << std::endl;
        while (line != "EOF") {
            // parse the line
            std::vector<std::string> tokens = this->split(line, ',');
            std::string productId = tokens[0];
            std::string tradeId = tokens[1];
            std::string book = tokens[2];
            double price = atof(tokens[3].c_str());
            Side side = tokens[4] == "BUY" ? BUY : SELL;
            long quantity = atol(tokens[5].c_str());

            double coupon = BondInfo::CUSIPToCoupon(productId);
            boost::gregorian::date* maturityPtr = BondInfo::CUSIPToDate(productId);

            Bond bond(productId, CUSIP, "T", coupon, *maturityPtr);
            Trade<Bond> trade(bond, tradeId, price, book, quantity, side);
            // For each trade, call Service.OnMessage() once to pass this piece of data.
            trade_booking_service->OnMessage(trade);
            DEBUG_TEST("side = %s -> BondTradeBookingService\n", tokens[4].c_str());
            // request for another line to the server
            send_socket(socket, file_name);
            line = read_socket(socket);
            trim(line);
        }
    }
};

/**
 * Bond Trade Booking Listener, listening to BondExecutionService.
 * Keyed on trade id.
 * Type T is the product type (Bond).
 */
class BondTradeBookingListener : public ServiceListener<ExecutionOrder<Bond> > {
   private:
    BondTradeBookingService* service;
    int count;  // counter to alternate the book

   public:
    explicit BondTradeBookingListener(BondTradeBookingService* _service) : service(_service), count(0) {}

    // Each execution should result in a trade into
    // the BondTradeBookingService via ServiceListener on BondExectionService
    virtual void ProcessAdd(ExecutionOrder<Bond>& _order) {
        Bond product = _order.GetProduct();
        std::string tradeId = _order.GetOrderId();
        double price = _order.GetPrice();
        count = count + 1;
        std::string book = "TRSY" + std::string(to_string(1 + count % 3));
        long quantity = _order.GetVisibleQuantity();
        PricingSide side = _order.GetPricingSide();
        Side order_side = (side == BID) ? BUY : SELL;
        Trade<Bond> trade(product, tradeId, price, book, quantity, order_side);
        service->BookTrade(trade);
        DEBUG_TEST("BondExecutionService -> BondTradeBookingService\n");
    }

    // Listener callback to process a remove event to the Service
    virtual void ProcessRemove(ExecutionOrder<Bond>& _order) {}

    // Listener callback to process an update event to the Service
    virtual void ProcessUpdate(ExecutionOrder<Bond>& _order) {}
};

#endif
