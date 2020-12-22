#include <iostream>
#include <string>

#include "datapublisher.hpp"

int main(int argc, char** argv) {
    int port = atoi(argv[1]);
    DataPublisher data_publisher;
    data_publisher.Subscribe(port);
    return 0;
}