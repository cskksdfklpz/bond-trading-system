/**
 * riskservice.hpp
 * Defines the data types and Service for fixed income risk.
 *
 * @author Breman Thuraisingham
 */
#ifndef RISK_SERVICE_HPP
#define RISK_SERVICE_HPP

#include "bondinfo.hpp"
#include "positionservice.hpp"
#include "soa.hpp"

/**
 * PV01 risk.
 * Type T is the product type.
 */
template <typename T>
class PV01 {
   public:
    // ctor for a PV01 value
    PV01(const T& _product, double _pv01, long _quantity) : product(_product) {
        pv01 = _pv01;
        quantity = _quantity;
    }

    // Get the product on this PV01 value
    const T& GetProduct() const { return product; }

    // Get the PV01 value
    double GetPV01() const { return pv01; }

    // Get the quantity that this risk value is associated with
    long GetQuantity() const { return quantity; }

   private:
    T product;
    double pv01;
    long quantity;
};

/**
 * A bucket sector to bucket a group of securities.
 * We can then aggregate bucketed risk to this bucket.
 * Type T is the product type.
 */
template <typename T>
class BucketedSector {
   public:
    // ctor for a bucket sector
    BucketedSector(const vector<T>& _products, string _name) : products(_products) {
        name = _name;
    }

    // Get the products associated with this bucket
    const vector<T>& GetProducts() const { return products; }

    // Get the name of the bucket
    const string& GetName() const { return name; }

   private:
    vector<T> products;
    string name;
};

/**
 * Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type.
 */
template <typename T>
class RiskService : public Service<string, PV01<T> > {
   public:
    // Add a position that the service will risk
    virtual void AddPosition(Position<T>& position) = 0;

    // Get the bucketed risk for the bucket sector
    virtual PV01<BucketedSector<T> >& GetBucketedRisk(BucketedSector<T>& sector) = 0;
};

/**
 * Bond Risk Service to vend out risk for a particular security and across a risk bucketed sector.
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondRiskService : public RiskService<Bond> {
   private:
    std::map<std::string, PV01<Bond> > risks;

   public:
    // add a position will increase the risk
    void AddPosition(Position<Bond>& position) {
        long quantity = position.GetAggregatePosition();
        std::string cusip = position.GetProduct().GetProductId();
        // get pv01 value from BondInfo class
        double pv01_value = BondInfo::GetPV01(cusip);
        PV01<Bond> pv01_object(position.GetProduct(), pv01_value, quantity);
        this->Notify(pv01_object);
    }
    // return the bucketed sector's pv01
    PV01<BucketedSector<Bond> >& GetBucketedRisk(BucketedSector<Bond>& sector) {
        double pv01_value = 0;
        double quantity = 0;
        auto products = sector.GetProducts();
        // pv01_value is the weighted sum of pv01 of each product
        for (auto& product : products) {
            std::string cusip = product.GetProductId();
            PV01<Bond>& pv01_bond = this->GetData(cusip);
            quantity += pv01_bond.GetQuantity();
            pv01_value += pv01_bond.GetQuantity() * pv01_bond.GetPV01();
        }
        pv01_value /= quantity;
        auto ret = new PV01<BucketedSector<Bond> >(sector, pv01_value, quantity);
        return *ret;
    }
    // get the PV01 of a product (bond)
    virtual PV01<Bond>& GetData(string key) {
        auto itr = risks.find(key);
        if (itr != risks.end())
            return itr->second;
        else {
            std::cout << "Can't find bond " << key << std::endl;
            exit(0);
        }
    }
};

/**
 * Bond Risk Listener to listen the BondPositionService and
 * notify the BondRiskService
 * Keyed on product identifier.
 * Type T is the product type (Bond).
 */
class BondRiskListener : public ServiceListener<Position<Bond> > {
   private:
    BondRiskService* service;

   public:
    explicit BondRiskListener(BondRiskService* _service) : service(_service) {}
    virtual void ProcessAdd(Position<Bond>& _pos) {
        DEBUG_TEST("BondPositionService -> BondRiskService\n");
        service->AddPosition(_pos);
    }
    virtual void ProcessRemove(Position<Bond>& _pos) {}
    virtual void ProcessUpdate(Position<Bond>& _pos) {}
};

/**
 * Bond Risk Connector
 * to publish the risk to another process via TCP/IP
 */

class BondRiskConnector : public Connector<PV01<Bond> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit BondRiskConnector(string file_name_, int port = 1237) : file_name(file_name_), socket(io_service) {
        
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The BondRiskService should use a Connector to publish
    // risk via socket into a separate process which listens
    // to the risk on the socket via its own Connector
    // and prints them when it receives them.
    virtual void Publish(PV01<Bond> &_risk) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string timestamp = std::to_string(ms.count());
        std::string productId = _risk.GetProduct().GetProductId();
        std::string pv01 = std::to_string(_risk.GetPV01() * _risk.GetQuantity());
        std::string line = timestamp + "," + productId + "," + pv01 + "\n";
        this->send_socket(socket, line + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("PV01<Bond> -> BondRiskConnector\n");
        
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~BondRiskConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};

#endif
