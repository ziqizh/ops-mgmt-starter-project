#include <algorithm>
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
using supplyfinder::FoodID;
using supplyfinder::Finder;
using supplyfinder::Request;
using supplyfinder::VendorInfo;
using supplyfinder::Vendor;
using supplyfinder::Supplier;
using supplyfinder::ShopInfo;
using supplyfinder::InventoryInfo;

Status FinderServiceImpl::CheckFood (ServerContext* context, const Request* request,
 ServerWriter<ShopInfo>* writer) {

  std::cout << "========== Receving Request Food Id=" << request->food_id() 
    << " ==========" << std::endl;

  vector<ShopInfo> result = ProcessRequest(request->food_id());
  long quantity = request->quantity();
  
  std::sort(result.begin(), result.end(), Comp());

  for (const auto& info : result){
    writer->Write(info);
    quantity -= info.inventory().quantity();
    if (quantity <= 0)  break;
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

vector<VendorInfo> SupplierClient::InquireVendorInfo (uint32_t food_id) {
  FoodID request;
  request.set_food_id(food_id);
  
  VendorInfo reply;
  vector<VendorInfo> info;
  
  ClientContext context;
  std::unique_ptr<ClientReader<VendorInfo>> reader(supplier_stub_->CheckVendor(&context, request));

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

void FinderServiceImpl::PrintVendorInfo (const uint32_t id, 
                                         const vector<VendorInfo>& info) {
  std::cout << "The following vendors might have food ID " << id << std::endl;
  for (auto& i : info) {
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
  
  for (const auto& vendor : vendor_info) {
    // if never connected before, create a new client.
    const string& url = vendor.url();
    if (vendor_clients_.find(url) == vendor_clients_.end())
      vendor_clients_.emplace(url,
                             VendorClient(grpc::CreateChannel(url, grpc::InsecureChannelCredentials())));
    auto client = vendor_clients_.find(url);

    InventoryInfo inventory_info = client->second.InquireInventoryInfo(food_id);
    // error checking: if no inventory, price == -1
    if (inventory_info.price() < 0)
      std::cout << "vendor at " << url << " doesn't have food " << food_id << std::endl;
    else {
      ShopInfo info;
      VendorInfo* v_ptr = info.mutable_vendor();
      InventoryInfo* i_ptr = info.mutable_inventory();
      *v_ptr = vendor;
      *i_ptr = inventory_info;
      result.push_back(info);
    }
  }
  return result;
}

void FinderServiceImpl::PrintResult (const uint32_t id, 
                                     const vector<pair<VendorInfo, InventoryInfo>>& result) {
  std::cout << "The following " << id << std::endl;
  for (auto& p : result) {
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
  std::string arg_str = "--target";
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
