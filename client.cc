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

#include <grpcpp/grpcpp.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "finder_server.h"

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using std::pair;
using std::vector;
using supplyfinder::Finder;
using supplyfinder::FinderRequest;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;
using supplyfinder::ShopInfo;
using supplyfinder::Supplier;
using supplyfinder::Vendor;
using supplyfinder::VendorInfo;

void PrintResult(const ShopInfo& p) {
  std::cout << "\tVendor url: " << p.vendor().url()
            << "; name: " << p.vendor().name()
            << "; location: " << p.vendor().location() << std::endl;
  std::cout << "\tInventory price: " << p.inventory().price()
            << "; quantity: " << p.inventory().quantity() << "\n\n";
}

class FinderClient {
 public:
  FinderClient(std::shared_ptr<Channel> channel)
      : stub_(Finder::NewStub(channel)) {}

  void InquireFoodInfo(std::string& food_name, uint32_t quantity) {
    FinderRequest request;
    request.set_food_name(food_name);
    request.set_quantity(quantity);
    ShopInfo reply;
    ClientContext context;
    std::unique_ptr<ClientReader<ShopInfo>> reader(
        stub_->CheckFood(&context, request));

    std::cout << "Receiving Shop Information\n";
    while (reader->Read(&reply)) {
      PrintResult(reply);
    }

    Status status = reader->Finish();
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

 private:
  std::unique_ptr<Finder::Stub> stub_;
};

void ProcessRequest(const std::string& target_str, std::string& food_name,
                    uint32_t quantity) {
  /*
   * Initiate client with supplier channel. Retrieve a list of vendor address
   * Then create client
   * return a vector of <Vendor Info, Inventory Info>
   */

  FinderClient client(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  client.InquireFoodInfo(food_name, quantity);
}

int main(int argc, char** argv) {
  // Parse Argument
  std::string target_str;
  std::string arg_str = "--target";
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50051";
  }

  // Test
  std::string food_name = "flour";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  ProcessRequest(target_str, food_name, 50);

  food_name = "egg";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  ProcessRequest(target_str, food_name, 50);

  food_name = "milk";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  ProcessRequest(target_str, food_name, 50);

  food_name = "quail";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  ProcessRequest(target_str, food_name, 50);

  return 0;
}