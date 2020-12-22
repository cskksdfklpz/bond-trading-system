/**
 * pricingservice.hpp
 * Defines the data types and Service for internal prices.
 *
 * @author Breman Thuraisingham, Quanzhi Bi
 */
#ifndef PRICING_SERVICE_HPP
#define PRICING_SERVICE_HPP

#include <boost/date_time/gregorian/gregorian.hpp>
#include <fstream>
#include <map>
#include <string>
#include <utility>

#include "products.hpp"
#include "soa.hpp"
#include "bondinfo.hpp"

/**
 * A price object consisting of mid and bid/offer spread.
 * Type T is the product type.
 */
template <typename T>
class Price {
   public:
    // default ctor
    Price() {}

    // ctor for a price
    Price(const T& _product, double _mid, double _bidOfferSpread) : product(_product) {
        mid = _mid;
        bidOfferSpread = _bidOfferSpread;
    }

    // Get the product
    const T& GetProduct() const { return product; }

    // Get the mid price
    double GetMid() const { return mid; }

    // Get the bid/offer spread around the mid
    double GetBidOfferSpread() const { return bidOfferSpread; }

   private:
    const T& product;
    double mid;
    double bidOfferSpread;
};

/**
 * Bond Pricing Service managing mid prices and bid/offers.
 * Keyed on product identifier (CUSIPS string).
 * Type T is the product type (Price<Bond>).
 */
class BondPricingService : public Service<string, Price<Bond> > {
   private:
    std::map<string, Price<Bond> > prices;

   public:
    //getting data
    virtual Price<Bond>& GetData(string key) {
        return prices.find(key)->second;
    }
    // called by the connector
    // update the map and them notifty the listeners (BondAlgoStreamingService)
    virtual void OnMessage(Price<Bond>& _price) {
        // since we didn't implement the copy ctor/copy assignment operator
        // we use insert method instead of [] operator to update price
        prices.erase(_price.GetProduct().GetProductId());
        prices.insert(make_pair(_price.GetProduct().GetProductId(), _price));
        Service<string, Price<Bond> >::Notify(_price);
    }
};

/**
 * Bond Pricing Connector to get data via TCP/IP.
 * Keyed on product identifier (CUSIPS string).
 * Type T is the product type (Price<Bond>).
 */
class BondPricingConnector : public Connector<Price<Bond> > {
   private:
    string file_name;
    BondPricingService* pricing_service;

   public:
    explicit BondPricingConnector(string file_name_, BondPricingService* pricing_service_) : file_name(file_name_), pricing_service(pricing_service_) {}
    virtual void Publish(Price<Bond>& data) {}

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
            int digitPartLength = tokens[1].size();
            if (tokens[1][digitPartLength - 1] == '+')
                tokens[1][digitPartLength - 1] = '4';

            double price = BondInfo::CalculatePrice(tokens[1]);
            double spread = (double)(tokens[2][0] - '0') / 128.0;
            double coupon = BondInfo::CUSIPToCoupon(tokens[0]);

            boost::gregorian::date* maturityPtr = BondInfo::CUSIPToDate(tokens[0]);

            Bond bond(tokens[0], CUSIP, "T", coupon, *maturityPtr);
            Price<Bond> bondPrice(bond, price, spread);
            DEBUG_TEST("price = %.3lf -> BondPricingService\n", price);

            // For each price, call Service.OnMessage() once to pass this piece of data.
            pricing_service->OnMessage(bondPrice);
            // request for another line to the server
            send_socket(socket, file_name);
            line = read_socket(socket);
            trim(line);
        }
    }
};

#endif
