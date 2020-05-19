/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <thread>
#include <vector>
#include <stdlib.h> 
#include <random>
#include <algorithm>
#include <iterator>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

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
using supplyfinder::HelloRequest;
using supplyfinder::HelloReply;
using supplyfinder::Vendor;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;

// Logic and data behind the server's behavior.
class VendorServiceImpl final : public Vendor::Service {
  public:
  VendorServiceImpl () {
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
      // std:: cout << "ID: " << idx << " Price: " << price << " Quantity: " << quantity << std::endl;
      inv.set_price(price);
      inv.set_quantity(quantity);
      inventory_db[idx] = inv;
    }
  }
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }
  
  Status CheckInventory (ServerContext* context, const FoodID* request,
                  InventoryInfo* info) {
    uint32_t food_id = request->food_id();
    if (inventory_db.find(food_id) == inventory_db.end()) {
      Status status(StatusCode::NOT_FOUND, "Food ID not found.");
      return status;
    }
    info->set_price(inventory_db[food_id].price());
    info->set_quantity(inventory_db[food_id].quantity());
    return Status::OK;
  }
  private:
  std::unordered_map<uint32_t, InventoryInfo> inventory_db;
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
  int server_number = 4;
  int base_port = 50053;
  std::vector<std::thread> threads;
  for (int i = 0; i < server_number; i++) {
    int port = base_port + i;
    std::string addr = "localhost:" + std::to_string(port);
    std::cout << "Running " << addr << std::endl;
    threads.emplace_back(RunServer, addr);
  }
  
  for (auto & t : threads) {
    t.join();
  }
  
  threads.clear();
  return 0;
}
