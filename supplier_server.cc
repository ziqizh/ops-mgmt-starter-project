#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

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

VendorInfo CreateVendor(std::string url, std::string name, std::string location) {
  VendorInfo vendor;
  vendor.set_url(url);
  vendor.set_name(name);
  vendor.set_location(location);
  return vendor;
}

// Logic and data behind the server's behavior.
class SupplierServiceImpl final : public Supplier::Service {
 private:
  // DB maintains a mapping ID -> vector<VendorInfo>.
  // Stores instance of VendorInfo for simplicity. May cause data redundency.
  std::unordered_map<uint32_t, vector<VendorInfo*>> vendor_db_;
  VendorInfo vendor[4];

 public:
  SupplierServiceImpl() {
    vendor[0] = CreateVendor("localhost:50053", "Kroger", "Ann Arbor, MI");
    vendor[1] = CreateVendor("localhost:50054", "Trader Joes", "Pittsburgh, PA");
    vendor[2] = CreateVendor("localhost:50055", "Walmart", "Beijing, CN");
    vendor[3] = CreateVendor("localhost:50056", "Costco", "Rochester, NY");

    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 4; j++) {
        vendor_db_[i].push_back(&vendor[j]);
      }
    }
  }

  Status CheckVendor(ServerContext* context, const FoodID* request,
                     ServerWriter<VendorInfo>* writer) override {
    std::cout << "supplier server received id: " << request->food_id()
              << std::endl;
    uint32_t food_id = request->food_id();
    auto vendors = vendor_db_.find(request->food_id());
    if (vendors == vendor_db_.end()) {
      return Status(StatusCode::NOT_FOUND, "Food ID not found.");
    }
    for (const auto vendor : vendors->second) {
      writer->Write(*vendor);
    }
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address = "0.0.0.0:50052";
  SupplierServiceImpl service;

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
  RunServer();
  return 0;
}
