/**
 * historicaldataservice.hpp
 * Defines the data types and Service for historical data.
 *
 * @author Breman Thuraisingham
 */
#ifndef HISTORICAL_DATA_SERVICE_HPP
#define HISTORICAL_DATA_SERVICE_HPP

#include "soa.hpp"

/**
 * Service for processing and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist.
 */
template <typename T>
class HistoricalDataService : Service<string, T> {
   private:
    Connector<T> *connector;
    std::string dataType;  // T
   public:
    // ctor
    explicit HistoricalDataService(Connector<T> *_connector, std::string _dataType) : connector(_connector), dataType(_dataType) {}
    // Persist data to a store
    void PersistData(string persistKey, T &_data) {
        DEBUG_TEST("Persisting historical %s data\n", dataType.c_str());
        connector->Publish(_data);
    }
};

/**
 * Service for listening and persisting historical data to a persistent store.
 * Keyed on some persistent key.
 * Type T is the data type to persist (Price<Bond>/ExecutionOrder<Bond>/...).
 */
template <typename T>
class HistoricalDataListener : public ServiceListener<T> {
   private:
    HistoricalDataService<T> *service;
    int count;  // persistent key

   public:
    explicit HistoricalDataListener(HistoricalDataService<T> *_service) : service(_service), count(0) {}
    virtual void ProcessAdd(T &_data) {
        std::string key = std::to_string(count++);
        service->PersistData(key, _data);
    }
    virtual void ProcessRemove(T &_data) {}
    virtual void ProcessUpdate(T &_data) {}
};

#endif
