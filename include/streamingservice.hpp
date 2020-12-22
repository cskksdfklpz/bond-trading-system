/**
 * streamingservice.hpp
 * Defines the data types and Service for price streams.
 *
 * @author Breman Thuraisingham, Quanzhi Bi
 */
#ifndef STREAMING_SERVICE_HPP
#define STREAMING_SERVICE_HPP

#include <map>

#include "marketdataservice.hpp"
#include "products.hpp"
#include "bondinfo.hpp"
#include "soa.hpp"

/**
 * A price stream order with price and quantity (visible and hidden)
 */
class PriceStreamOrder {
   public:
    // ctor for an order
    PriceStreamOrder(double _price, long _visibleQuantity, long _hiddenQuantity, PricingSide _side) {
        price = _price;
        visibleQuantity = _visibleQuantity;
        hiddenQuantity = _hiddenQuantity;
        side = _side;
    }

    // The side on this order
    PricingSide GetSide() const { return side; }

    // Get the price on this order
    double GetPrice() const { return price; }

    // Get the visible quantity on this order
    long GetVisibleQuantity() const { return visibleQuantity; }

    // Get the hidden quantity on this order
    long GetHiddenQuantity() const { return hiddenQuantity; }

   private:
    double price;
    long visibleQuantity;
    long hiddenQuantity;
    PricingSide side;
};

/**
 * Price Stream with a two-way market.
 * Type T is the product type.
 */
template <typename T>
class PriceStream {
   public:
    // ctor
    PriceStream(const T& _product,
                const PriceStreamOrder& _bidOrder,
                const PriceStreamOrder& _offerOrder) : product(_product),
                                                       bidOrder(_bidOrder),
                                                       offerOrder(_offerOrder) {}

    // Get the product
    const T& GetProduct() const { return product; }

    // Get the bid order
    const PriceStreamOrder& GetBidOrder() const { return bidOrder; }

    // Get the offer order
    const PriceStreamOrder& GetOfferOrder() const { return offerOrder; }

   private:
    T product;
    PriceStreamOrder bidOrder;
    PriceStreamOrder offerOrder;
};

/**
 * Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class StreamingService : public Service<string, PriceStream<T> > {
   public:
    // Publish two-way prices
    void PublishPrice(PriceStream<T>& priceStream) {
        Service<string, PriceStream<T> >::Notify(priceStream);
    }
};

// I think we can just use the template StreamingService<Bond>
// defining another BondStreamingService will waste all of our template effort
// and we don't need the PublishPrice method to be virtual here
// (it's not virtual in the given code)

using BondStreamingService = StreamingService<Bond>;

/**
 * Bond Algo Streaming service to publish two-way prices.
 * Keyed on product identifier.
 * with value an AlgoStream object.
 * (what's the point of AlgoStream object?)
 */
class BondAlgoStreamingService : public Service<string, PriceStream<Bond> > {
   private:
    // do we need this?
    std::map<string, PriceStream<Bond> > algo_stream;
    // counter to alternate the order size
    int count;

   public:
    // ctor to initailize count
    BondAlgoStreamingService() { count = 0; }

    // method to generate algo streams and notify all the listeners
    void PublishPrice(Price<Bond>& _price) {
        // get the bid/offer price
        double spread = _price.GetBidOfferSpread();
        double mid_price = _price.GetMid();
        double bid_price = mid_price - spread * 0.5;
        double offer_price = mid_price + spread * 0.5;
        // Alternate visible sizes between 1000000 and 2000000
        // on subsequent updates for both sides
        int visible_size = 1000000;
        if (count == 0) {
            visible_size = 2000000;
            count = 1;
        } else {
            count = 0;
        }
        // Hidden size should be twice the visible size at all times.
        int hidden_size = 2 * visible_size;
        // send the priceStreamOrder to the listeners
        PriceStreamOrder bid_order(bid_price, visible_size, hidden_size, BID);
        PriceStreamOrder offer_order(offer_price, visible_size, hidden_size, OFFER);
        PriceStream<Bond> price_stream(_price.GetProduct(), bid_order, offer_order);
        Service<string, PriceStream<Bond> >::Notify(price_stream);
    }
};

/**
 * Bond Algo Streaming service listener
 * to listen the BondPricingService
 * then publish the price data to BondAlgoStreamingService
 */
class BondAlgoStreamingListener : public ServiceListener<Price<Bond> > {
   private:
    BondAlgoStreamingService* service;

   public:
    explicit BondAlgoStreamingListener(BondAlgoStreamingService* _service) : service(_service) {}
    // publish the price data
    virtual void ProcessAdd(Price<Bond>& _price) {
        DEBUG_TEST("BondPricingService -> BondAlgoStreamingService\n");
        service->PublishPrice(_price);
    }
    // we don't need these methods
    virtual void ProcessRemove(Price<Bond>& _price) {}
    virtual void ProcessUpdate(Price<Bond>& _price) {}
};


/**
 * Bond Streaming service listener
 * to listen the BondAlgoStreamingService
 * then publish the price data to BondHistoricalDataService
 */
class BondStreamingListener : public ServiceListener<PriceStream<Bond> > {
   private:
    BondStreamingService* service;

   public:
    explicit BondStreamingListener(StreamingService<Bond>* _service) : service(_service) {}
    // publish the price stream data to the listeners
    virtual void ProcessAdd(PriceStream<Bond>& _priceStream) {
        DEBUG_TEST("BondAlgoStreamingService -> BondStreamingService\n");
        service->PublishPrice(_priceStream);
    }
    // we don't need these methods
    virtual void ProcessRemove(PriceStream<Bond>& _priceStream) {}
    virtual void ProcessUpdate(PriceStream<Bond>& _priceStream) {}
};


/**
 * Bond Streaming Connector
 * to publish the streaming to another process via TCP/IP
 */

class BondStreamingConnector : public Connector<PriceStream<Bond> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit BondStreamingConnector(string file_name_, int port = 1237) : file_name(file_name_), socket(io_service) {
        
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The BondStreamingService should use a Connector to publish
    // price stream via socket into a separate process which listens
    // to the price stream on the socket via its own Connector
    // and prints them when it receives them.
    virtual void Publish(PriceStream<Bond> &_stream) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string timestamp = std::to_string(ms.count());
        std::string productId = _stream.GetProduct().GetProductId();
        std::string bidPrice = BondInfo::FormatPrice(_stream.GetBidOrder().GetPrice());
        std::string offerPrice = BondInfo::FormatPrice(_stream.GetOfferOrder().GetPrice());
        std::string line = timestamp + "," + productId + "," + bidPrice + "," + offerPrice + "\n";
        this->send_socket(socket, line + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("PriceStream<Bond> -> BondStreamingConnector\n");
        
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~BondStreamingConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};

#endif
