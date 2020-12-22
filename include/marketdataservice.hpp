/**
 * marketdataservice.hpp
 * Defines the data types and Service for order book market data.
 *
 * @author Breman Thuraisingham
 */
#ifndef MARKET_DATA_SERVICE_HPP
#define MARKET_DATA_SERVICE_HPP

#include <string>
#include <vector>

#include "products.hpp"
#include "soa.hpp"
#include "bondinfo.hpp"

using namespace std;

// Side for market data
enum PricingSide { BID,
                   OFFER };

/**
 * A market data order with price, quantity, and side.
 */
class Order {
   public:
    // ctor for an order
    Order(double _price, long _quantity, PricingSide _side) {
        price = _price;
        quantity = _quantity;
        side = _side;
    }

    // Get the price on the order
    double GetPrice() const { return price; }

    // Get the quantity on the order
    long GetQuantity() const { return quantity; }

    // Get the side on the order
    PricingSide GetSide() const { return side; }

   private:
    double price;
    long quantity;
    PricingSide side;
};

/**
 * Class representing a bid and offer order
 */
class BidOffer {
   public:
    // ctor for bid/offer
    BidOffer(const Order& _bidOrder,
             const Order& _offerOrder) : bidOrder(_bidOrder),
                                         offerOrder(_offerOrder) {}

    // Get the bid order
    const Order& GetBidOrder() const { return bidOrder; }

    // Get the offer order
    const Order& GetOfferOrder() const { return offerOrder; }

   private:
    Order bidOrder;
    Order offerOrder;
};

/**
 * Order book with a bid and offer stack.
 * Type T is the product type.
 */
template <typename T>
class OrderBook {
   public:
    // ctor for the order book
    OrderBook(const T& _product,
              const vector<Order>& _bidStack,
              const vector<Order>& _offerStack) : product(_product),
                                                  bidStack(_bidStack),
                                                  offerStack(_offerStack) {}

    // Get the product
    const T& GetProduct() const { return product; }

    // Get the bid stack
    const vector<Order>& GetBidStack() const { return bidStack; }

    // Get the offer stack
    const vector<Order>& GetOfferStack() const { return offerStack; }

    // Get the spread
    const double GetSpread() const {
        return offerStack[0].GetPrice() - bidStack[0].GetPrice();
    }

   private:
    T product;
    vector<Order> bidStack;
    vector<Order> offerStack;
};

/**
 * Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class MarketDataService : public Service<string, OrderBook<T> > {
   public:
    // Get the best bid/offer order
    virtual const BidOffer& GetBestBidOffer(const string& productId) = 0;

    // Aggregate the order book
    // I don't understand this method
    // thus I will leave it commented 
    // until I figure out what does it mean
    // virtual const OrderBook<T>& AggregateDepth(const string& productId) = 0;
};

/**
 * Bond Market Data Service which distributes market data
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondMarketDataService : public MarketDataService<Bond> {

   private:

    std::map<std::string, OrderBook<Bond> > orderbooks;

   public:
    virtual const BidOffer& GetBestBidOffer(const string& productId) {
        auto itr = orderbooks.find(productId);
        if (itr == orderbooks.end()) {
            std::cout << "Can't find orderbook of " << productId << std::endl;
            exit(0);
        }
        Order offer_order = itr->second.GetOfferStack()[0];
        Order bid_order = itr->second.GetOfferStack()[0];
        BidOffer* bid_offer = new BidOffer(bid_order, offer_order);
        return *bid_offer;
    }
    // update the map and notify the listeners
    virtual void OnMessage(OrderBook<Bond>& _orderbook) {
        orderbooks.erase(_orderbook.GetProduct().GetProductId());
        orderbooks.insert(make_pair(_orderbook.GetProduct().GetProductId(), _orderbook));
        this->Notify(_orderbook);
    }
};

/**
 * Bond Market Data Connector to get marketdata via TCP/IP.
 * Keyed on product identifier (CUSIPS string).
 * Type T is the product type (OrderBook<Bond>).
 */
class BondMarketDataConnector : public Connector<OrderBook<Bond> > {
   private:
    string file_name;
    BondMarketDataService* marketdata_service;

   public:
    explicit BondMarketDataConnector
    (string file_name_, BondMarketDataService* _marketdata_service) : file_name(file_name_), 
                                                                      marketdata_service(_marketdata_service) {}
    virtual void Publish(OrderBook<Bond>& data) {}

    void Subscribe(int port) {
        boost::asio::io_service io_service;
        // socket creation
        tcp::socket socket(io_service);
        // connection on localhost
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        
        std::cout << "connecting to the " << file_name << "...";
        file_name += "\n";
        // send the file request message to the server
        send_socket(socket, file_name);

        string line = read_socket(socket);
        
        trim(line);
        std::cout << "success" << std::endl;
        while (line != "EOF") {
            std::vector<std::string> tokens = split(line, ',');
            // Transform data.
            std::string productId = tokens[0];
            std::vector<Order> bidStack;
            std::vector<Order> offerStack;
            // tokens 1,2,3,4,5 -> bid 4,3,2,1,0
            // tokens 6,7,8,9,10 -> offer 0,1,2,3,4
            for (int i=0; i<=4; ++i) {
                double bid_price = BondInfo::CalculatePrice(tokens[5-i]);
                double offer_price = BondInfo::CalculatePrice(tokens[6+i]);
                // L millions quantity for L-level
                double quantity = 1000000*(i+1);
                bidStack.push_back(Order(bid_price,quantity,BID));
                offerStack.push_back(Order(offer_price,quantity,OFFER));
            }
            Bond bond = *BondInfo::GetBond(productId);
            OrderBook<Bond> orderbook(bond, bidStack, offerStack);
            // For each price, call Service.OnMessage() once to pass this piece of data.
            marketdata_service->OnMessage(orderbook);
            DEBUG_TEST("OrderBook of %s -> BondMarketDataService\n", productId.c_str());
            // request for another line to the server
            send_socket(socket, file_name);
            line = read_socket(socket);
            trim(line);
        }
    }
};

#endif
