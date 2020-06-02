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
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "supplyfinder.grpc.pb.h"

using google::protobuf::Empty;
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
using supplyfinder::SupplierInfo;
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

  void UpdateSupplier(const std::string& addr) {
    SupplierInfo info;
    Empty empty;
    ClientContext context;
    info.set_url(addr);
    Status status = stub_->UpdateSupplier(&context, info, &empty);
    if (!status.ok()) {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
  }

 private:
  std::unique_ptr<Finder::Stub> stub_;
};

class Client {
 public:
  Client(std::string& finder_addr)
      : finder_client_(grpc::CreateChannel(
            finder_addr, grpc::InsecureChannelCredentials())) {}

  void ProcessRequest(std::string& food_name, uint32_t quantity) {
    finder_client_.InquireFoodInfo(food_name, quantity);
  }

  void InitSupplier(const std::string& supplier_addr) {
    std::cout << "========== Initializing Supplier Server at " << supplier_addr
              << " ==========" << std::endl;
    finder_client_.UpdateSupplier(supplier_addr);

    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(supplier_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<Supplier::Stub> supplier_stub = Supplier::NewStub(channel);

    VendorInfo info;
    Empty empty;
    std::string url, name, location;
    std::cout
        << "please input server url, name and location, separated by newlines."
        << std::endl;
    while (std::cin >> url >> name >> location) {
      info.set_url(url);
      info.set_name(name);
      info.set_location(location);
      ClientContext context;
      Status status = supplier_stub->AddVendor(&context, info, &empty);
      if (!status.ok()) {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return;
      }
    }
  }

 private:
  FinderClient finder_client_;
};

int main(int argc, char* argv[]) {
  // Parse Argument
  std::string finder_addr = "0.0.0.0:50051";
  std::string supplier_addr;
  bool init_supplier = false;
  int c;
  while ((c = getopt(argc, argv, "s:f:")) != -1) {
    switch (c) {
      case 's':
        if (optarg) {
          init_supplier = true;
          supplier_addr = optarg;
        }
        break;
      case 'f':
        if (optarg) finder_addr = optarg;
        break;
    }
  }
  std::cout << "finder addr: " << finder_addr << std::endl;
  Client client(finder_addr);

  if (init_supplier) {
    client.InitSupplier(supplier_addr);
  }

  // Test
  std::string food_name = "flour";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  client.ProcessRequest(food_name, 50);

  food_name = "egg";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  client.ProcessRequest(food_name, 50);

  food_name = "milk";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  client.ProcessRequest(food_name, 50);

  food_name = "quail";
  std::cout << "========== Querying Food: " << food_name
            << " Quantity = 50 ==========" << std::endl;
  client.ProcessRequest(food_name, 50);

  return 0;
}
