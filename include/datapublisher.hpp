/**
 * datapublisher.hpp
 * Defines DataPublisher derived from Connector
 * Reading the data from the local file 
 * and publish the data via TCP/IP
 *
 * @author Quanzhi Bi
 */
#ifndef DATAPUBLISH_HPP
#define DATAPUBLISH_HPP

#include <algorithm>
#include <boost/asio.hpp>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "soa.hpp"

using namespace boost::asio;
using ip::tcp;
using std::cout;
using std::endl;
using std::string;

class DataPublisher : public Connector<int> {
   private:
    std::string file_name;

   public:
    // read from local file and publish the data via TCP/IP
    virtual void Publish(int& port) {
        std::cout << "Using port " << port << std::endl;
        boost::asio::io_service io_service;
        //listen for new connection
        tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port));
        //socket creation
        tcp::socket socket_(io_service);
        //waiting for connection
        acceptor_.accept(socket_);
        //read operation
        file_name = read_socket(socket_);
        // std::cout << file_name << std::endl;

        trim(file_name);
        std::ifstream f;
        f.open(file_name);
        if (f.is_open() == false) {
            std::cout << "Failed to open file " << file_name << std::endl;
        }
        std::string line;
        while (std::getline(f, line)) {
            // std::cout << line << std::endl;
            line += "\n";
            send_socket(socket_, line);
            file_name = read_socket(socket_);
        }
        line = "EOF\n";
        send_socket(socket_, line);
        f.close();
    }
    // receive data from TCP/IP and write to a local file
    void Subscribe(int port) {
        std::cout << "Using port " << port << std::endl;
        boost::asio::io_service io_service;
        //listen for new connection
        tcp::acceptor acceptor_(io_service, tcp::endpoint(tcp::v4(), port));
        //socket creation
        tcp::socket socket_(io_service);
        //waiting for connection
        acceptor_.accept(socket_);
        //read operation
        string file_name = read_socket(socket_);
        // std::cout << file_name << std::endl;

        trim(file_name);
        std::ofstream out;

        // std::ios::app is the open mode "append" meaning
        // new data will be written to the end of the file.
        out.open(file_name, std::ios::app);
        if (out.is_open() == false) {
            std::cout << "Failed to open file " << file_name << std::endl;
        }
        // send success message to request data
        send_socket(socket_, "success\n");
        std::string line = read_socket(socket_);
        trim(line);
        while (line != "EOF") {
            out << line << std::endl;
            // std::cout << line << std::endl;
            send_socket(socket_, "success\n");
            line = read_socket(socket_);
            trim(line);
        }

        out.close();
    }
};

#endif