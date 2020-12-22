/**
 * executionservice.hpp
 * Defines the data types and Service for executions.
 *
 * @author Breman Thuraisingham
 */
#ifndef EXECUTION_SERVICE_HPP
#define EXECUTION_SERVICE_HPP

#include <chrono>
#include <string>

#include "bondinfo.hpp"
#include "marketdataservice.hpp"
#include "products.hpp"
#include "soa.hpp"

enum OrderType { FOK,
                 IOC,
                 MARKET,
                 LIMIT,
                 STOP };

enum Market { BROKERTEC,
              ESPEED,
              CME };

/**
 * An execution order that can be placed on an exchange.
 * Type T is the product type.
 */
template <typename T>
class ExecutionOrder {
   public:
    // ctor for an order
    ExecutionOrder(const T &_product, PricingSide _side, string _orderId, OrderType _orderType, double _price, double _visibleQuantity, double _hiddenQuantity, string _parentOrderId, bool _isChildOrder) : product(_product) {
        side = _side;
        orderId = _orderId;
        orderType = _orderType;
        price = _price;
        visibleQuantity = _visibleQuantity;
        hiddenQuantity = _hiddenQuantity;
        parentOrderId = _parentOrderId;
        isChildOrder = _isChildOrder;
    }

    // Get the product
    const T &GetProduct() const { return product; }

    // Get the order ID
    const string &GetOrderId() const { return orderId; }

    // Get the order type on this order
    OrderType GetOrderType() const { return orderType; }

    // Get the price on this order
    double GetPrice() const { return price; }

    // Get the visible quantity on this order
    long GetVisibleQuantity() const { return visibleQuantity; }

    // Get the hidden quantity
    long GetHiddenQuantity() const { return hiddenQuantity; }

    // Get the parent order ID
    const string &GetParentOrderId() const { return parentOrderId; }

    // Get the prcing side
    PricingSide GetPricingSide() const { return side; }

    // Is child order?
    bool IsChildOrder() const { return isChildOrder; }

   private:
    T product;
    PricingSide side;
    string orderId;
    OrderType orderType;
    double price;
    double visibleQuantity;
    double hiddenQuantity;
    string parentOrderId;
    bool isChildOrder;
};

/**
 * Service for executing orders on an exchange.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class ExecutionService : public Service<string, ExecutionOrder<T> > {
   public:
    // Execute an order on a market
    virtual void ExecuteOrder(const ExecutionOrder<T> &order, Market market) = 0;
};

/**
 * Service for executing orders on an exchange for Bond.
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondExecutionService : public ExecutionService<Bond> {
   public:
    // Execute an order on a market
    void ExecuteOrder(const ExecutionOrder<Bond> &_order, Market market) {
        ExecutionOrder<Bond> order = _order;
        this->Notify(order);
    }
};

/**
 * Service for algo executing orders on an exchange for Bond.
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondAlgoExecutionService : public ExecutionService<Bond> {
   private:
    // counter to alternate between BID and OFFER
    int count;

   public:
    // ctor to initialize counter
    BondAlgoExecutionService() : count(0) {}

    // Algorithm to generate execution
    // alternating between bid and offer
    // (taking the opposite side of the order book to cross the spread)
    // and only aggressing when the spread is at its tightest
    // (i.e. 1/128th) to reduce the cost of crossing the spread.
    void AlgoExecute(OrderBook<Bond> &orderbook) {
        count = (count + 1);
        PricingSide side = (count % 2) ? BID : OFFER;
        double spread = orderbook.GetSpread();
        if (spread > 1.0 / 128) return;

        string orderId = to_string(count);
        double price = (side == BID) ? orderbook.GetBidStack()[0].GetPrice() : orderbook.GetOfferStack()[0].GetPrice();
        double quantity = (side == BID) ? orderbook.GetOfferStack()[0].GetQuantity() : orderbook.GetBidStack()[0].GetQuantity();
        double hidden_quantity = quantity;

        ExecutionOrder<Bond> order(orderbook.GetProduct(),
                                   side,
                                   orderId,
                                   MARKET,
                                   price,
                                   quantity,
                                   hidden_quantity,
                                   orderId,
                                   false);
        this->Notify(order);
    }

    // we don't need this method
    void ExecuteOrder(const ExecutionOrder<Bond> &order, Market market) {}
};

/**
 * Bond Algo Execution service listener
 * to listen the BondMarketDataService
 * then publish the orderbook data to BondAlgoExecutionService
 */
class BondAlgoExecutionListener : public ServiceListener<OrderBook<Bond> > {
   private:
    BondAlgoExecutionService *service;

   public:
    explicit BondAlgoExecutionListener(BondAlgoExecutionService *_service) : service(_service) {}
    virtual void ProcessAdd(OrderBook<Bond> &_orderbook) {
        DEBUG_TEST("BondMarketDataService -> BondAlgoExecutionService\n");
        service->AlgoExecute(_orderbook);
    }
    virtual void ProcessRemove(OrderBook<Bond> &_orderbook) {}
    virtual void ProcessUpdate(OrderBook<Bond> &_orderbook) {}
};

/**
 * Bond Execution service listener
 * to listen the BondAlgoExecutionService
 * then publish the execution order data to BondExecutionService
 */
class BondExecutionListener : public ServiceListener<ExecutionOrder<Bond> > {
   private:
    BondExecutionService *service;

   public:
    explicit BondExecutionListener(BondExecutionService *_service) : service(_service) {}
    virtual void ProcessAdd(ExecutionOrder<Bond> &_order) {
        DEBUG_TEST("BondAlgoExecutionService -> BondExecutionService\n");
        // we didn't use Market argument?
        // just randomly pass one, won't be used anyway
        service->ExecuteOrder(_order, CME);
    }
    // we don't need these methods
    virtual void ProcessRemove(ExecutionOrder<Bond> &_order) {}
    virtual void ProcessUpdate(ExecutionOrder<Bond> &_order) {}
};

/**
 * Bond Execution Connector
 * to publish the execution to another process via TCP/IP
 */

class BondExecutionConnector : public Connector<ExecutionOrder<Bond> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit BondExecutionConnector(string file_name_, int port = 1237) : file_name(file_name_), socket(io_service) {
        
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The BondExecutionService should use a Connector to publish
    // executions via socket into a separate process which listens
    // to the executions on the socket via its own Connector
    // and prints them when it receives them.
    virtual void Publish(ExecutionOrder<Bond> &_order) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string timestamp = std::to_string(ms.count());
        std::string side = (_order.GetPricingSide() == BID) ? "BUY" : "SELL";
        std::string productId = _order.GetProduct().GetProductId();
        std::string orderId = _order.GetOrderId();
        std::string orderType = "MARKET";
        std::string price = BondInfo::FormatPrice(_order.GetPrice());
        std::string visibleQuantity = std::to_string(_order.GetVisibleQuantity());
        std::string hiddenQuantity = std::to_string(_order.GetHiddenQuantity());
        std::string line = timestamp + "," + productId + "," + orderId + "," + orderType + "," + side + "," + price + "," + visibleQuantity + "," + hiddenQuantity + "\n";
        this->send_socket(socket, line + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("ExecutionOrder -> BondExecutionConnector\n");
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~BondExecutionConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};

#endif
