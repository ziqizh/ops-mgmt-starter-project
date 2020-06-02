#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;
using supplyfinder::Vendor;

// Logic and data behind the server's behavior.
class VendorServiceImpl final : public Vendor::Service {
 public:
  VendorServiceImpl() {
    /*
     * Randomly generate inventory information
     * Each vendor has five items, with price [0, 20.0] and quantity [0, 99]
     */
    std::vector<int> indices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);
    InventoryInfo inv;
    for (int i = 0; i < 5; i++) {
      int idx = indices[i];
      double price = rand() % 200 / 10.0;
      uint32_t quantity = rand() % 100;
      inv.set_price(price);
      inv.set_quantity(quantity);
      inventory_db_[idx] = inv;
    }
  }

  Status CheckInventory(ServerContext* context, const FoodID* request,
                        InventoryInfo* info) {
    uint32_t food_id = request->food_id();
    std::cout << "Food " << food_id;
    auto inventory = inventory_db_.find(food_id);
    if (inventory == inventory_db_.end()) {
      std::cout << " Not Found" << std::endl;
      Status status(StatusCode::NOT_FOUND, "Food ID not found.");
      return status;
    }
    info->set_price(inventory->second.price());
    info->set_quantity(inventory->second.quantity());
    std::cout << " has price " << inventory->second.price() << " and quantity "
              << inventory->second.quantity() << std::endl;
    return Status::OK;
  }

 private:
  // Maps food ID to inventory information
  std::unordered_map<uint32_t, InventoryInfo> inventory_db_;
};

void RunServer(std::string addr) {
  std::string server_address(addr);
  VendorServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  int server_number = 1;
  int base_port = 50053;
  std::string addr = "0.0.0.0:50053";
  std::cout << "Running " << addr << std::endl;
  RunServer(addr);
  return 0;
}
