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

// #include <grpcpp/grpcpp.h>
#include "finder_server.h"

// #ifdef BAZEL_BUILD
// #include "examples/protos/supplyfinder.grpc.pb.h"
// #else
// #include "supplyfinder.grpc.pb.h"
// #endif

using std::vector;
using std::pair;
using std::string;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using namespace supplyfinder;

Status FinderServiceImpl::CheckFood (ServerContext* context, const Request* request,
 ServerWriter<ShopInfo>* writer) {

  std::cout << "========== Receving Request Food Id=" << request->food_id() << " ==========" << std::endl;

  vector<ShopInfo> result = ProcessRequest(request->food_id());
  // FinderServiceImpl::PrintResult(request->food_id(), result);
  for (const auto & info : result){
    writer->Write(info);
  }
  return Status::OK;
}


InventoryInfo VendorClient::InquireInventoryInfo (uint32_t food_id) {
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


string SupplierClient::SayHelloToSupplier(const string& user) {
  // Data we are sending to the server.
  HelloRequest request;
  request.set_name(user);

  HelloReply reply;

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

vector<VendorInfo> SupplierClient::InquireVendorInfo (uint32_t food_id) {
  FoodID request;
  request.set_food_id(food_id);
  
  VendorInfo reply;
  vector<VendorInfo> info;
  
  ClientContext context;
  std::unique_ptr<ClientReader<VendorInfo>> reader (supplier_stub_->CheckVendor(&context, request));
  // Status status = supplier_stub_->CheckVendor(&context, request, &info);

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

void FinderServiceImpl::PrintVendorInfo (const uint32_t id, const vector<VendorInfo>& info) {
  std::cout << "The following vendors might have food ID " << id << std::endl;
  for (auto & i : info) {
    std::cout << "\tVendor url: " << i.url() << "; name: " << i.name()
      << "; location: " << i.location() << std::endl;
  }
}

vector<ShopInfo> FinderServiceImpl::ProcessRequest(uint32_t food_id) {
  /*
   * Initiate client with supplier channel. Retrieve a list of vendor address
   * Then create client
   * return a vector of <Vendor Info, Inventory Info>
   */
  vector<ShopInfo> result;
        
  vector<VendorInfo> vendor_info = supplier_client_.InquireVendorInfo(food_id);
  
  FinderServiceImpl::PrintVendorInfo(food_id, vendor_info);
  
  for (const auto & vendor : vendor_info) {
    // TODO: save these clients 
    VendorClient vendor_client(grpc::CreateChannel(
      vendor.url(), grpc::InsecureChannelCredentials()));
    InventoryInfo inventory_info = vendor_client.InquireInventoryInfo(food_id);
    // error checking: if no inventory, price == -1
    if (inventory_info.price() < 0)
      std::cout << "vendor at " << vendor.url() << " doesn't have food " << food_id << std::endl;
    else {
      VendorInfo* v_ptr = new VendorInfo(vendor);
      InventoryInfo* i_ptr = new InventoryInfo(inventory_info);
      ShopInfo info;
      info.set_allocated_vendor(v_ptr);
      info.set_allocated_inventory(i_ptr);
      result.push_back(info);
    }
  }
  return result;
}

void FinderServiceImpl::PrintResult (const uint32_t id, const vector<pair<VendorInfo, InventoryInfo>>& result) {
  std::cout << "The following " << id << std::endl;
  for (auto & p : result) {
    std::cout << "Vendor url: " << p.first.url() << "; name: " << p.first.name()
      << "; location: " << p.first.location() << std::endl;
    std::cout << "Inventory price: " << p.second.price() << "; quantity: " << p.second.quantity()
      << std::endl;
  }
}



void RunServer(string& supplier_target_str) {
  std::string server_address("0.0.0.0:50051");
  FinderServiceImpl service(supplier_target_str);

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
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
    supplier_target_str = "localhost:50052";
  }

  RunServer(supplier_target_str);

  return 0;
}
