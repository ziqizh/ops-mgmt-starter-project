#include <memory>
#include <string>
#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

class VendorClient {
  public:
  VendorClient(std::shared_ptr<grpc::Channel> vendor_channel):
  vendor_stub_(supplyfinder::Vendor::NewStub(vendor_channel)) {}
  
  supplyfinder::InventoryInfo InquireInventoryInfo (uint32_t);
  
  private:
  std::unique_ptr<supplyfinder::Vendor::Stub> vendor_stub_;
};

class SupplierClient {
 public:
  SupplierClient(std::shared_ptr<grpc::Channel> supplier_channel)
      : supplier_stub_(supplyfinder::Supplier::NewStub(supplier_channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHelloToSupplier(const std::string&);
  
  std::vector<supplyfinder::VendorInfo> InquireVendorInfo (uint32_t food_id);
 private:
  std::unique_ptr<supplyfinder::Supplier::Stub> supplier_stub_;
};

class FinderServiceImpl final : public supplyfinder::Finder::Service {
  public:
  FinderServiceImpl(std::string supplier_target_str): supplier_client_(grpc::CreateChannel(
      supplier_target_str, grpc::InsecureChannelCredentials())) {}
  grpc::Status CheckFood (grpc::ServerContext*, const supplyfinder::Request*, 
        grpc::ServerWriter<supplyfinder::ShopInfo>*);
  static void PrintResult (const uint32_t, 
        const std::vector<std::pair<supplyfinder::VendorInfo, supplyfinder::InventoryInfo>>&);
  static void PrintVendorInfo (const uint32_t, const std::vector<supplyfinder::VendorInfo>&);
  std::vector<supplyfinder::ShopInfo> 
        ProcessRequest(uint32_t);

  private:
  SupplierClient supplier_client_;
};