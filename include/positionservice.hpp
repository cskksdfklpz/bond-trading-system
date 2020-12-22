/**
 * positionservice.hpp
 * Defines the data types and Service for positions.
 *
 * @author Breman Thuraisingham
 */
#ifndef POSITION_SERVICE_HPP
#define POSITION_SERVICE_HPP

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "bondinfo.hpp"
#include "products.hpp"
#include "soa.hpp"
#include "tradebookingservice.hpp"

using namespace std;

/**
 * Position class in a particular book.
 * Type T is the product type.
 */
template <typename T>
class Position {
   public:
    // ctor for a position
    Position(const T &_product) : product(_product) {}

    // Get the product
    const T &GetProduct() const { return product; }

    // Get the position quantity
    long GetPosition(string &book) { return positions[book]; }

    // Get the aggregate position
    // I assume this means we add position of different books?
    long GetAggregatePosition() {
        long position = 0;
        for (auto const &pos : positions) position += pos.second;
        return position;
    }

    // Add a new position with side into the specific book
    void AddPosition(string book, long position, Side side) {
        position = (side == BUY) ? position : -position;
        auto itr = positions.find(book);
        if (itr != positions.end())
            itr->second += position;
        else
            positions.insert(std::make_pair(book, position));
    }

   private:
    T product;
    map<string, long> positions;
};

/**
 * Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class PositionService : public Service<string, Position<T> > {
   public:
    // Add a trade to the service
    virtual void AddTrade(const Trade<T> &trade) = 0;
};

/**
 * Bond Position Service to manage positions across multiple books and secruties.
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondPositionService : public PositionService<Bond> {
   private:
    map<string, Position<Bond> > positions;

   public:
    // initailize the map cusip -> position(bond)
    BondPositionService() {
        auto cusips = BondInfo::GetCUSIP();
        for (auto cusip : cusips) {
            auto bond = BondInfo::GetBond(cusip);
            auto position = Position<Bond>(*bond);
            positions.insert(make_pair(cusip, position));
        }
    }
    // Add a trade to the service
    virtual void AddTrade(const Trade<Bond> &_trade) {
        string cusip = _trade.GetProduct().GetProductId();
        string book = _trade.GetBook();
        long quantity = _trade.GetQuantity();
        Side side = _trade.GetSide();
        // first update the positions map
        auto itr = positions.find(cusip);
        if (itr != positions.end()) {
            Position<Bond> &position = itr->second;
            position.AddPosition(book, quantity, side);
            // then notify all the listeners
            this->Notify(itr->second);
        } else {
            std::cout << "Can't find bond " << cusip << " in the BondPossitionService" << std::endl;
            exit(0);
        }
    }

    // GetData method, the Service's original job!
    virtual Position<Bond> &GetData(string key) {
        auto itr = positions.find(key);
        if (itr == positions.end()) {
            std::cout << "Can't find position " << key << " in the BondPossitionService" << std::endl;
            exit(0);
        } else {
            return positions.find(key)->second;
        }
    }
};

/**
 * Bond Position Listener to listen to BondTradeBookingService
 * and notify BondPositionService
 * Type T is the product type (Bond).
 */
class BondPositionListener : public ServiceListener<Trade<Bond> > {
   private:
    BondPositionService *service;

   public:
    explicit BondPositionListener(BondPositionService *_service) : service(_service) {}
    virtual void ProcessAdd(Trade<Bond> &_trade) {
        DEBUG_TEST("BondTradeBookingService -> BondPositionService\n");
        service->AddTrade(_trade);
    }
    virtual void ProcessRemove(Trade<Bond> &_trade) {}
    virtual void ProcessUpdate(Trade<Bond> &_trade) {}
};

/**
 * Bond Position Connector
 * to publish the position to another process via TCP/IP
 */

class BondPositionConnector : public Connector<Position<Bond> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit BondPositionConnector(string file_name_, int port = 1237) : file_name(file_name_), socket(io_service) {
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The BondPositionService should use a Connector to publish
    // positions via socket into a separate process which listens
    // to the positions on the socket via its own Connector
    // and prints them when it receives them.
    virtual void Publish(Position<Bond> &_position) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string timestamp = std::to_string(ms.count());
        std::string aggregate_position = std::to_string(_position.GetAggregatePosition());
        std::string productId = _position.GetProduct().GetProductId();
        // cycle through the books above in order TRSY1, TRSY2, TRSY3
        std::vector<std::string> books = {"TRSY1", "TRSY2", "TRSY3"};
        std::string position1 = std::to_string(_position.GetPosition(books[0]));
        std::string position2 = std::to_string(_position.GetPosition(books[1]));
        std::string position3 = std::to_string(_position.GetPosition(books[2]));
        std::string line = timestamp + "," + productId + "," + position1 + "," + position2 + "," + position3 + "," + aggregate_position + "\n";
        this->send_socket(socket, line + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("Position<Bond> -> BondPositionConnector\n");
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~BondPositionConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};

#endif
