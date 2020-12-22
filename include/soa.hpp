/**
 * soa.hpp
 * Definition of our Service Oriented Architecture (SOA) Service base class
 *
 * @author Breman Thuraisingham, Quanzhi Bi
 */

#ifndef SOA_HPP
#define SOA_HPP

#include <algorithm>
#include <boost/asio.hpp>
#include <fstream>
#include <string>
#include <vector>

using namespace std;
using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

/**
 * Definition of a generic base class ServiceListener to listen to add, update, and remve
 * events on a Service. This listener should be registered on a Service for the Service
 * to notify all listeners for these events.
 */
template <typename V>
class ServiceListener {
   public:
    // Listener callback to process an add event to the Service
    virtual void ProcessAdd(V &data) = 0;

    // Listener callback to process a remove event to the Service
    virtual void ProcessRemove(V &data) = 0;

    // Listener callback to process an update event to the Service
    virtual void ProcessUpdate(V &data) = 0;
};

/**
 * Definition of a generic base class Service.
 * Uses key generic type K and value generic type V.
 */
template <typename K, typename V>
class Service {
   public:
    // The callback that a Connector should invoke for any new or updated data
    virtual void OnMessage(V &data) {}

    // Add a listener to the Service for callbacks on add, remove, and update events
    // for data to the Service.
    virtual void AddListener(ServiceListener<V> *listener) {
        listeners.push_back(listener);
    }

    // Get all listeners on the Service.
    virtual const vector<ServiceListener<V> *> &GetListeners() const {
        return listeners;
    }

    // Notify all the listeners
    virtual void Notify(V &data) {
        for (auto listener : listeners)
            listener->ProcessAdd(data);
    }

   protected:
    // vector of listeners
    vector<ServiceListener<V> *> listeners;
};

/**
 * Definition of a Connector class.
 * This will invoke the Service.OnMessage() method for subscriber Connectors
 * to push data to the Service.
 * Services can invoke the Publish() method on this Service to publish data to the Connector
 * for a publisher Connector.
 * Note that a Connector can be publisher-only, subscriber-only, or both publisher and susbcriber.
 */
template <typename V>
class Connector {
   public:
    // Publish data to the Connector
    virtual void Publish(V &data) = 0;

   protected:
    // split the string
    std::vector<std::string> split(const std::string &s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
    // remove the \n from the string
    void trim(string &str) {
        str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    }
    // read the data from the socket
    string read_socket(tcp::socket &socket) {
        boost::asio::streambuf buf;
        boost::asio::read_until(socket, buf, "\n");
        string data = boost::asio::buffer_cast<const char *>(buf.data());
        return data;
    }
    // send the data to the socket
    void send_socket(tcp::socket &socket, const string &message) {
        const string msg = message + "\n";
        boost::asio::write(socket, boost::asio::buffer(message));
    }
};

#endif
