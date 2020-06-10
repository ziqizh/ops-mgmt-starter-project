// #include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>
#include <cstdint>
#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/opencensus.h>

#include "absl/strings/escaping.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "opencensus/stats/stats.h"
#include "opencensus/tags/context_util.h"
#include "opencensus/tags/tag_map.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/span.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/with_span.h"

#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

#include "exporters.h"

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
                         supplyfinder::ShopResponse*);
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
