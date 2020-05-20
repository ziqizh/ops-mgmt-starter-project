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
#include <cstdint>
#include <utility>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "finder_server.h"

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

using std::vector;
using std::pair;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using supplyfinder::FoodID;
using supplyfinder::Finder;
using supplyfinder::Request;
using supplyfinder::VendorInfo;
using supplyfinder::Vendor;
using supplyfinder::Supplier;
using supplyfinder::ShopInfo;
using supplyfinder::InventoryInfo;

class FinderClient {
  public:
    FinderClient(std::shared_ptr<Channel> channel):
      stub_(Finder::NewStub(channel)) {}
    
    vector<ShopInfo> InquireFoodInfo (uint32_t food_id, uint32_t quantity) {
      Request request;
      request.set_food_id(food_id);
      request.set_quantity(quantity);
      
      ShopInfo reply;
      vector<ShopInfo> info;
      
      ClientContext context;
      
      std::unique_ptr<ClientReader<ShopInfo>> reader(stub_->CheckFood(&context, request));

      while (reader->Read(&reply)) {
        info.push_back(reply);
      }

      Status status = reader->Finish();
      if (status.ok()) {
        return info;
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                    << std::endl;
        return info;
      }
    }
  private:
    std::unique_ptr<Finder::Stub> stub_;
};

vector<ShopInfo> ProcessRequest(const std::string& target_str, 
    uint32_t food_id, uint32_t quantity) {
  /*
   * Initiate client with supplier channel. Retrieve a list of vendor address
   * Then create client
   * return a vector of <Vendor Info, Inventory Info>
   */
  
  FinderClient client(grpc::CreateChannel(
      target_str, grpc::InsecureChannelCredentials()));
      
  vector<ShopInfo> result = client.InquireFoodInfo(food_id, quantity);
  
  return result;
}

void PrintResult (const uint32_t id, const vector<ShopInfo>& result) {
  std::cout << "The food ID is " << id << std::endl;
  for (auto & p : result) {
    std::cout << "\tVendor url: " << p.vendor().url() << "; name: " << p.vendor().name()
      << "; location: " << p.vendor().location() << std::endl;
    std::cout << "\tInventory price: " << p.inventory().price() << "; quantity: "
      << p.inventory().quantity() << std::endl;
  }
}

int main(int argc, char** argv) {
  // Parse Argument
  std::string target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target=" << std::endl;
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
  for (int i = 0; i < 3; i++) {
    std::cout << "========== Querying FoodID = " << i << " Quantity = 60 ==========" << std::endl;
    vector<ShopInfo> result = ProcessRequest(target_str, i, 60);
    PrintResult(i, result);
  }
  
  return 0;
}