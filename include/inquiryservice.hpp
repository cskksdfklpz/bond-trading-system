/**
 * inquiryservice.hpp
 * Defines the data types and Service for customer inquiries.
 *
 * @author Breman Thuraisingham
 */
#ifndef INQUIRY_SERVICE_HPP
#define INQUIRY_SERVICE_HPP

#include "bondinfo.hpp"
#include "soa.hpp"
#include "tradebookingservice.hpp"

// Various inqyury states
enum InquiryState { RECEIVED,
                    QUOTED,
                    DONE,
                    REJECTED,
                    CUSTOMER_REJECTED };

/**
 * Inquiry object modeling a customer inquiry from a client.
 * Type T is the product type.
 */
template <typename T>
class Inquiry {
   public:
    // ctor for an inquiry
    Inquiry(string _inquiryId, const T& _product, Side _side, long _quantity, double _price, InquiryState _state) : product(_product) {
        inquiryId = _inquiryId;
        side = _side;
        quantity = _quantity;
        price = _price;
        state = _state;
    }

    // Get the inquiry ID
    const string& GetInquiryId() const { return inquiryId; }

    // Get the product
    const T& GetProduct() const { return product; }

    // Get the side on the inquiry
    Side GetSide() const { return side; }

    // Get the quantity that the client is inquiring for
    long GetQuantity() const { return quantity; }

    // Get the price that we have responded back with
    double GetPrice() const { return price; }

    // Set the price that we have responded back with
    void SetPrice(double _price) { price = _price; }

    // Get the current state on the inquiry
    InquiryState GetState() const { return state; }

    // Set the current state on the inquiry
    void SetState(InquiryState _state) { state = _state; }

   private:
    string inquiryId;
    T product;
    Side side;
    long quantity;
    double price;
    InquiryState state;
};

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type.
 */
template <typename T>
class InquiryService : public Service<string, Inquiry<T> > {
   public:
    // Send a quote back to the client
    virtual void SendQuote(Inquiry<T>& inquiry) = 0;

    // Reject an inquiry from the client
    virtual void RejectInquiry(Inquiry<T>& inquiry) = 0;
};


/**
 * Quote Connector to get data via TCP/IP.
 * Keyed on some identifier.
 * Type T is the product type (Inquiry<Bond>).
 * It change the inquiry from RECEIVED to QUOTED
 */
class QuoteConnector : public Connector<Inquiry<Bond> > {
   public:
    virtual void Publish(Inquiry<Bond>& _inquiry) {
        if (_inquiry.GetState() == RECEIVED) {
            _inquiry.SetState(QUOTED);
            DEBUG_TEST("Inquiry QUOTED -> BondInquiryService\n");
        } else if (_inquiry.GetState() == DONE) {
            DEBUG_TEST("Inquiry DONE\n");
        }
    }
};

/**
 * Service for customer inquirry objects.
 * Keyed on inquiry identifier (NOTE: this is NOT a product identifier since each inquiry must be unique).
 * Type T is the product type (Bond).
 */
class BondInquiryService : public InquiryService<Bond> {
   private:
    QuoteConnector* connector;

   public:
    explicit BondInquiryService(QuoteConnector* _connector) : connector(_connector) {}
    // Send a quote back to the client, just in-method simulation
    void SendQuote(Inquiry<Bond>& inquiry) {
        connector->Publish(inquiry);
        if (inquiry.GetState() == QUOTED) this->OnMessage(inquiry);
    }

    // Reject an inquiry from the client by ignoring it
    void RejectInquiry(Inquiry<Bond>& inquiry) {}

    // Read the inquires from a file
    virtual void OnMessage(Inquiry<Bond>& _inquiry) {
        std::string inquiryId = _inquiry.GetInquiryId();
        InquiryState state = _inquiry.GetState();
        if (state == RECEIVED) {
            // return quote as the face value
            _inquiry.SetPrice(100);
            this->SendQuote(_inquiry);
        } else if (state == QUOTED) {
            // change state to DONE
            _inquiry.SetState(DONE);
            // send an update to the connector
            this->SendQuote(_inquiry);
            // notify the listeners
            this->Notify(_inquiry);
        } else {
            std::cout << "Invalid inquiry state" << std::endl;
            _inquiry.SetState(REJECTED);
            this->RejectInquiry(_inquiry);
            this->Notify(_inquiry);
        }
    }
};

/**
 * Bond Inquiry Connector to get data via TCP/IP.
 * Keyed on some identifier.
 * Type T is the product type (Inquiry<Bond>).
 */
class BondInquiryConnector : public Connector<Inquiry<Bond> > {
   private:
    string file_name;
    BondInquiryService* service;

   public:
    explicit BondInquiryConnector(string file_name_, BondInquiryService* _service) : file_name(file_name_), service(_service) {}
    virtual void Publish(Inquiry<Bond>& _inquiry) {}

    void Subscribe(int port) {
        boost::asio::io_service io_service;
        // socket creation
        tcp::socket socket(io_service);
        // connection on localhost
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        file_name += "\n";
        std::cout << "connecting to the data server...";
        // send the file request message to the server
        send_socket(socket, file_name);

        string line = read_socket(socket);

        trim(line);
        std::cout << "success" << std::endl;
        while (line != "EOF") {
            std::vector<std::string> tokens = split(line, ',');
            std::string inquiryId = tokens[0];
            std::string productId = tokens[1];
            Side side = (tokens[2] == "BUY") ? BUY : SELL;
            Bond product = *BondInfo::GetBond(productId);
            Inquiry<Bond> inquiry(inquiryId, product, side, 0, 0, RECEIVED);
            service->OnMessage(inquiry);

            DEBUG_TEST("Inquiry RECEIVED -> BondInquiryService\n");

            // request for another line to the server
            send_socket(socket, file_name);
            line = read_socket(socket);
            trim(line);
        }
    }
};


/**
 * Bond All Inquiries Connector
 * to publish all the inquiries to another process via TCP/IP
 */

class BondAllInquiriesConnector : public Connector<Inquiry<Bond> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit BondAllInquiriesConnector(string file_name_, int port = 1237) : file_name(file_name_), socket(io_service) {
        
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The BondAllInquiriesConnector should use a Connector to publish
    // all the inquiries via socket into a separate process which listens
    // to the all the inquiries on the socket via its own Connector
    // and prints them when it receives them.
    virtual void Publish(Inquiry<Bond> &_inquiry) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string timestamp = std::to_string(ms.count());
        std::string productId = _inquiry.GetProduct().GetProductId();
        std::string price = BondInfo::FormatPrice(_inquiry.GetPrice());
        std::string state = (_inquiry.GetState() == DONE) ? "DONE" : "REJECTED";
        std::string line = timestamp + "," + productId + "," + price + "," + state + "\n";
        this->send_socket(socket, line + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("Inquiry<Bond> -> BondAllInquiriesConnector\n");
        
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~BondAllInquiriesConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};


#endif
