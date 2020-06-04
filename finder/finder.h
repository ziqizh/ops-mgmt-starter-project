#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef BAZEL_BUILD
#include "examples/protos/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

struct Comp {
  inline bool operator()(const supplyfinder::ShopInfo& lhs,
                         const supplyfinder::ShopInfo& rhs) {
    return (lhs.inventory().price() < rhs.inventory().price());
  }
};

class VendorClient {
  /*
   * VendorClient talks to vendor servers. Created when the Finder receives
   * VendorInfo from Supplier Server
   */
 public:
  VendorClient(std::shared_ptr<grpc::Channel>);
  supplyfinder::InventoryInfo InquireInventoryInfo(uint32_t);

 private:
  std::unique_ptr<supplyfinder::Vendor::Stub> vendor_stub_;
};

class SupplierClient {
  /*
   * SupplierClient talks to the supplier server.
   */
 public:
  SupplierClient(std::shared_ptr<grpc::Channel>);
  bool GetVendorInfo(supplyfinder::VendorInfo* vendor_info);
  std::unique_ptr<grpc::ClientReader<supplyfinder::VendorInfo>>& InitReader(
      grpc::ClientContext*, supplyfinder::FoodID&);

 private:
  std::unique_ptr<supplyfinder::Supplier::Stub> supplier_stub_;
  std::unique_ptr<grpc::ClientReader<supplyfinder::VendorInfo>> reader_;
};

class FinderServiceImpl final : public supplyfinder::Finder::Service {
  /*
   * Finder service receive request (food_id, quantity) from clients.
   * It first query a list of vendors with food id from the supplier server
   * Then it creates SupplierClients for each vendor server and query inventory
   * information
   */
 public:
  FinderServiceImpl(std::string supplier_target_str);
  grpc::Status CheckFood(grpc::ServerContext*,
                         const supplyfinder::FinderRequest*,
                         grpc::ServerWriter<supplyfinder::ShopInfo>*);
  // Get corresponding food ID given food name
  long GetFoodID(const std::string&);
  std::vector<supplyfinder::ShopInfo> ProcessRequest(const std::string&);

 private:
  // Helper functions
  static void PrintVendorInfo(const uint32_t, const supplyfinder::VendorInfo&);
  void InitFoodID(std::vector<std::string>&);
  
  SupplierClient supplier_client_;
  // maps server address to the client instance
  std::unordered_map<std::string, VendorClient> vendor_clients_;
  // maps food name to food id
  std::unordered_map<std::string, uint32_t> food_id_;
};
