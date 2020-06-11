#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/opencensus.h>
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
#include <unistd.h>

#include "absl/strings/escaping.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
// #include "opencensus/exporters/stats/prometheus/prometheus_exporter.h"
#include "opencensus/stats/stats.h"
#include "opencensus/tags/context_util.h"
#include "opencensus/tags/tag_map.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include "opencensus/trace/trace_config.h"


#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

#include "helpers.h"
#include "exporters.h"

using google::protobuf::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;
using supplyfinder::Vendor;
using supplyfinder::VendorInfo;
using supplyfinder::Supplier;

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

void RegisterVendor(std::string& supplier_addr, std::string vendor_addr,
                    std::string name, std::string location) {
  std::shared_ptr<Channel> channel =
        grpc::CreateChannel(supplier_addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<Supplier::Stub> supplier_stub = Supplier::NewStub(channel);

  VendorInfo info = MakeVendor(vendor_addr, name, location);
  ClientContext context;
  Empty empty;
  Status status = supplier_stub->RegisterVendor(&context, info, &empty);
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
  }
}

void RunServer(std::string addr) {
  std::string server_address(addr);
  VendorServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
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

int main(int argc, char* argv[]) {
  // The vendor address is the public address of itself
  // The supplier address is the supplier server it talks to
  std::string vendor_addr = "0.0.0.0:50053";
  std::string supplier_addr = "0.0.0.0:50052";
  std::string name = "Wegmans";
  std::string port = "50053";
  std::string location = "NY";
  int c;
  while ((c = getopt(argc, argv, "s:v:n:l:p:")) != -1) {
    switch (c) {
      case 's':
        if (optarg) supplier_addr = optarg;
        break;
      case 'v':
        if (optarg) vendor_addr = optarg;
        break;
      case 'n':
        if (optarg) name = optarg;
        break;
      case 'l':
        if (optarg) location = optarg;
        break;
      case 'p':
        if (optarg) port = optarg;
        break;
    }
  }
  std::cout << "Running " << vendor_addr << std::endl;
  grpc::RegisterOpenCensusPlugin();
  grpc::RegisterOpenCensusViewsForExport();
  RegisterExporters();
  RegisterVendor(supplier_addr, vendor_addr, name, location);
  RunServer("0.0.0.0:" + port);
  return 0;
}
