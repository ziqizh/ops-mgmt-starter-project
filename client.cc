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

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

using std::vector;
using std::pair;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using supplyfinder::HelloRequest;
using supplyfinder::HelloReply;
using supplyfinder::Supplier;
using supplyfinder::Vendor;
using supplyfinder::FoodID;
using supplyfinder::VendorInfo;
using supplyfinder::InventoryInfo;

class VendorClient {
  public:
  VendorClient(std::shared_ptr<Channel> vendor_channel):
  vendor_stub_(Vendor::NewStub(vendor_channel)) {}
  
  std::string SayHelloToVendor(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = vendor_stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
  
  InventoryInfo InquireInventoryInfo (uint32_t food_id) {
    FoodID request;
    request.set_food_id(food_id);
    
    InventoryInfo info;
    
    ClientContext context;
    
    Status status = vendor_stub_->CheckInventory(&context, request, &info);
    
    if (status.ok()) {
      return info;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      info.set_price(-1);
      return info;
    }
  }
  private:
  std::unique_ptr<Vendor::Stub> vendor_stub_;
};

class SupplierClient {
 public:
  SupplierClient(std::shared_ptr<Channel> supplier_channel)
      : supplier_stub_(Supplier::NewStub(supplier_channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHelloToSupplier(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = supplier_stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }
  
  VendorInfo InquireVendorInfo (uint32_t food_id) {
    FoodID request;
    request.set_food_id(food_id);
    
    VendorInfo info;
    
    ClientContext context;
    
    Status status = supplier_stub_->CheckVendor(&context, request, &info);
    
    if (status.ok()) {
      return info;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      info.set_url("RPC failed");
      return info;
    }
  }
  

 private:
  std::unique_ptr<Supplier::Stub> supplier_stub_;
};

vector<pair<VendorInfo, InventoryInfo>> ProcessRequest(const std::string& supplier_target_str, uint32_t food_id) {
  /*
   * Initiate client with supplier channel. Retrieve a list of vendor address
   * Then create client
   * return a vector of <Vendor Info, Inventory Info>
   */
  vector<pair<VendorInfo, InventoryInfo>> result;
  
  SupplierClient supplier_client(grpc::CreateChannel(
      supplier_target_str, grpc::InsecureChannelCredentials()));
      
  VendorInfo vendor_info = supplier_client.InquireVendorInfo(food_id);
  
  std::cout << "Vendor info received: " << vendor_info.url() << std::endl;
  
  VendorClient vendor_client(grpc::CreateChannel(
      vendor_info.url(), grpc::InsecureChannelCredentials()));
      
  InventoryInfo inventory_info = vendor_client.InquireInventoryInfo(food_id);
  
  // error checking: if no inventory, price == -1
  if (inventory_info.price() < 0)
    std::cout << "vendor at " << vendor_info.url() << " doesn't have food " << food_id << std::endl;
  else
    result.push_back(std::make_pair(vendor_info, inventory_info));
  return result;
}

void PrintResult (const uint32_t id, const vector<pair<VendorInfo, InventoryInfo>>& result) {
  std::cout << "The food ID is " << id << std::endl;
  for (auto & p : result) {
    std::cout << "Vendor url: " << p.first.url() << "; name: " << p.first.name()
      << "; location: " << p.first.location() << std::endl;
    std::cout << "Inventory price: " << p.second.price() << "; quantity: " << p.second.quantity()
      << std::endl;
  }
}

int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string supplier_target_str;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        supplier_target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target=" << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    supplier_target_str = "localhost:50051";
  }
  
  
  std::cout << "========== Testing Supplier Server ==========" << std::endl;
  std::cout << "Querying FoodID = 1" << std::endl;
  vector<pair<VendorInfo, InventoryInfo>> result = ProcessRequest(supplier_target_str, 1);
  PrintResult(1, result);
  
  

  return 0;
}
