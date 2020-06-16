#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

#include "helpers.h"

using google::protobuf::Empty;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;
using std::vector;
using supplyfinder::FoodID;
using supplyfinder::Supplier;
using supplyfinder::VendorInfo;

// Logic and data behind the server's behavior.
class SupplierServiceImpl final : public Supplier::Service {
 private:
  // DB maintains a mapping ID -> vector<VendorInfo*>.
  // Stores pointers to VendorInfos, which are stored in an array
  std::unordered_map<uint32_t, vector<VendorInfo*>> vendor_db_;
  // use C style array to ensure the validity of the address
  VendorInfo vendor_[10]; 
  int vendor_count_;
  // Store existing vendor addresses to prevent double counting
  std::unordered_set<std::string> vendor_addr_;

 public:
  SupplierServiceImpl() : vendor_count_(0) {}

  Status CheckVendor(ServerContext* context, const FoodID* request,
                     ServerWriter<VendorInfo>* writer) override {
    uint32_t food_id = request->food_id();
    std::cout << "supplier server received id: " << food_id
              << std::endl;
    auto vendors = vendor_db_.find(food_id);
    if (vendors == vendor_db_.end()) {
      return Status(StatusCode::NOT_FOUND, "Food ID not found.");
    }
    for (const auto vendor : vendors->second) {
      writer->Write(*vendor);
    }
    return Status::OK;
  }

  Status RegisterVendor(ServerContext* context, const VendorInfo* request,
                   Empty* info) {
    if (vendor_count_ > 9) {
      return Status(
          StatusCode::OUT_OF_RANGE,
          "Reached maximum vendor. Please consider resetting the vendors");
    }
    if (vendor_addr_.count(request->url())) {
      return Status(
          StatusCode::ALREADY_EXISTS,
          "Current vendor address already exists.");
    }
    std::cout << "Adding Vendor " << request->url() << " " << request->name()
              << " " << request->location() << std::endl;
    vendor_[vendor_count_] =
        MakeVendor(request->url(), request->name(), request->location());
    for (uint32_t i = 0; i < 10; i++) {
      vendor_db_[i].push_back(&vendor_[vendor_count_]);
    }
    vendor_addr_.insert(request->url());
    vendor_count_++;
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address = "0.0.0.0:50052";
  SupplierServiceImpl service;

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

int main(int argc, char** argv) {
  RunServer();
  return 0;
}
