/**
 * guiservice.hpp
 * Defines GUIService
 *
 * @author Quanzhi Bi
 */
#ifndef GUISERVICE_HPP
#define GUISERVICE_HPP

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "pricingservice.hpp"
#include "soa.hpp"

template <typename V>
class GUIConnector : public Connector<Price<V> > {
   private:
    std::string file_name;
    boost::asio::io_service io_service;
    tcp::socket socket;

   public:
    // ctor
    explicit GUIConnector(string file_name_, int port = 1235) : file_name(file_name_), socket(io_service) {
        // connection of the socket
        std::cout << "connecting to the " << file_name << "...";
        socket.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port));
        this->send_socket(socket, file_name + "\n");
        string success = this->read_socket(socket);
        std::cout << "success" << std::endl;
    }
    // The GUIService should output those updates with a timestamp
    // with millisecond precision to a file gui.txt.
    virtual void Publish(Price<V> &_price) {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::string info = to_string(ms.count()) + "," + _price.GetProduct().GetProductId() + "," + to_string(_price.GetMid()) + "," + to_string(_price.GetBidOfferSpread()) + "\n";
        this->send_socket(socket, info + "\n");
        auto success = this->read_socket(socket);
        DEBUG_TEST("%s -> GUIConnector\n", _price.GetProduct().GetProductId().c_str());
    }
    // dtor, we need to kill the data_writer process by sending EOF
    ~GUIConnector() {
        std::cout << "Finished, killing the data_writer (" << file_name << ") process" << std::endl;
        this->send_socket(socket, "EOF\n");
    }
};

template <typename T>
class GUIService : public Service<std::string, Price<T> > {
   private:
    uint64_t last_time;
    int throttle;
    int count;
    GUIConnector<T> *gui_connector;

   public:
    explicit GUIService(GUIConnector<T> *gui_connector_, long long int _throttle = 300) : gui_connector(gui_connector_) {
        throttle = _throttle;
        count = 0;
        last_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // we don't need this function
    virtual void onMessage(Price<T> &_price) {}

    void ProvideData(Price<T> data) {
        uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (current_time - last_time >= throttle && count < 100) {
            last_time = current_time;
            gui_connector->Publish(data);
            count++;
        }
    }
};

template <typename T>
class GUIServiceListener : public ServiceListener<Price<T> > {
   private:
    GUIService<T> *gui_service;

   public:
    explicit GUIServiceListener(GUIService<T> *gui) : gui_service(gui) {}
    virtual void ProcessAdd(Price<T> &_price) {
        gui_service->ProvideData(_price);
    }
    virtual void ProcessRemove(Price<T> &_price) {}
    virtual void ProcessUpdate(Price<T> &_price) {}
};

#endif