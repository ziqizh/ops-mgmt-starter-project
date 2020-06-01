#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// #include <grpcpp/grpcpp.h>
#include "finder.h"

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
using std::pair;
using std::string;
using std::vector;
using supplyfinder::Finder;
using supplyfinder::FinderRequest;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;
using supplyfinder::ShopInfo;
using supplyfinder::Supplier;
using supplyfinder::Vendor;
using supplyfinder::VendorInfo;

Status FinderServiceImpl::CheckFood(ServerContext* context,
                                    const FinderRequest* request,
                                    ServerWriter<ShopInfo>* writer) {
  std::cout << "========== Receving Request Food: " << request->food_name()
            << " ==========" << std::endl;

  vector<ShopInfo> result = ProcessRequest(request->food_name());
  long quantity = request->quantity();

  if (result.empty()) {
    Status status(StatusCode::NOT_FOUND,
                  "Food " + request->food_name() + " not found.");
    return status;
  }

  std::sort(result.begin(), result.end(), Comp());

  for (const auto& info : result) {
    writer->Write(info);
    quantity -= info.inventory().quantity();
    if (quantity <= 0) break;
  }
  return Status::OK;
}

VendorClient::VendorClient(std::shared_ptr<grpc::Channel> vendor_channel)
    : vendor_stub_(supplyfinder::Vendor::NewStub(vendor_channel)) {}

InventoryInfo VendorClient::InquireInventoryInfo(uint32_t food_id) {
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

SupplierClient::SupplierClient(std::shared_ptr<grpc::Channel> supplier_channel)
    : supplier_stub_(supplyfinder::Supplier::NewStub(supplier_channel)) {}

std::unique_ptr<ClientReader<VendorInfo>>& SupplierClient::InitReader(
    ClientContext* context_ptr, FoodID& request) {
  // Release previous reader
  reader_.reset();
  // Use the context and request on stack to create new reader
  // The context and request will be released when ProcessRequest
  // exits
  reader_ = supplier_stub_->CheckVendor(context_ptr, request);
  return reader_;
}

FinderServiceImpl::FinderServiceImpl(std::string supplier_target_str)
    : supplier_client_(grpc::CreateChannel(
          supplier_target_str, grpc::InsecureChannelCredentials())) {
  vector<string> food_names = {"apple", "egg",   "milk",
                               "flour", "water", "butter"};
  InitFoodID(food_names);
}

void FinderServiceImpl::PrintVendorInfo(const uint32_t id,
                                        const VendorInfo& info) {
  std::cout << "This vendor might have food " << id << std::endl;
  std::cout << "\tVendor url: " << info.url() << "; name: " << info.name()
            << "; location: " << info.location() << std::endl;
}

long FinderServiceImpl::GetFoodID(const string& food_name) {
  // convert food name to lowercase, then look up
  string name_lowercase = food_name;
  transform(name_lowercase.begin(), name_lowercase.end(),
            name_lowercase.begin(),
            [](unsigned char c) { return std::tolower(c); });
  auto it = food_id_.find(name_lowercase);
  if (it == food_id_.end()) {
    return -1;
  }
  return it->second;
}

Status FinderServiceImpl::UpdateSupplier(grpc::ServerContext* context,
                                         const supplyfinder::SupplierInfo* info,
                                         google::protobuf::Empty* empty) {
  supplier_client_ = SupplierClient(grpc::CreateChannel(
      info->url(), grpc::InsecureChannelCredentials()));
  std::cout << "supplier client updated to " << info->url() << std::endl;
  return Status::OK;
}

void FinderServiceImpl::InitFoodID(vector<string>& food_names) {
  int idx = 0;
  for (const string& name : food_names) {
    food_id_[name] = idx++;
  }
}

vector<ShopInfo> FinderServiceImpl::ProcessRequest(const string& food_name) {
  /*
   * Initiate client with supplier channel. Retrieve a list of vendor address
   * Then create client
   * return a vector of <Vendor Info, Inventory Info>
   */
  vector<ShopInfo> result;
  VendorInfo vendor_info;
  ClientContext context;
  FoodID request;
  long food_id = GetFoodID(food_name);
  if (food_id < 0) {
    // if no corresponding food id, return empty vector
    std::cout << "Food " << food_name << " cannot be found." << std::endl;
    return result;
  }
  request.set_food_id(food_id);
  std::unique_ptr<ClientReader<VendorInfo>>& reader =
      supplier_client_.InitReader(&context, request);

  while (reader->Read(&vendor_info)) {
    // if never connected before, create a new client.
    FinderServiceImpl::PrintVendorInfo(food_id, vendor_info);
    const string& url = vendor_info.url();
    auto client = vendor_clients_.find(url);
    if (client == vendor_clients_.end()) {
      auto element = vendor_clients_.emplace(
          url, VendorClient(grpc::CreateChannel(
                   url, grpc::InsecureChannelCredentials())));
      client = element.first;  // assign the newly created client iterator
    }

    InventoryInfo inventory_info = client->second.InquireInventoryInfo(food_id);
    // error checking: if no inventory, price == -1
    if (inventory_info.price() < 0)
      std::cout << "vendor at " << url << " doesn't have food " << food_id
                << std::endl;
    else {
      ShopInfo info;
      *info.mutable_vendor() = vendor_info;
      *info.mutable_inventory() = inventory_info;
      result.push_back(info);
    }
  }

  Status status = reader->Finish();

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  }
  return result;
}

void FinderServiceImpl::PrintResult(
    const uint32_t id, const vector<pair<VendorInfo, InventoryInfo>>& result) {
  std::cout << "The following " << id << std::endl;
  for (auto& p : result) {
    std::cout << "Vendor url: " << p.first.url() << "; name: " << p.first.name()
              << "; location: " << p.first.location() << std::endl;
    std::cout << "Inventory price: " << p.second.price()
              << "; quantity: " << p.second.quantity() << std::endl;
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
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else if (argc > 2) {
    std::cout << "Too much arguments: The only acceptable argument is --target="
              << std::endl;
    return 0;
  } else {
    supplier_target_str = "localhost:50052";
  }

  RunServer(supplier_target_str);

  return 0;
}
