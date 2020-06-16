// #include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <grpcpp/grpcpp.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
// #include <grpcpp/health_check_service_interface.h>
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
  VendorClient(std::shared_ptr<grpc::Channel> vendor_channel);
  supplyfinder::InventoryInfo InquireInventoryInfo(uint32_t food_id);

 private:
  std::unique_ptr<supplyfinder::Vendor::Stub> vendor_stub_;
};

class SupplierClient {
  /*
   * SupplierClient talks to the supplier server.
   */
 public:
  SupplierClient(std::shared_ptr<grpc::Channel> supplier_channel);
  bool GetVendorInfo(supplyfinder::VendorInfo* vendor_info);
  std::unique_ptr<grpc::ClientReader<supplyfinder::VendorInfo>>& InitReader(
      grpc::ClientContext* context_ptr, supplyfinder::FoodID& request);

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
  FinderServiceImpl(const std::string& supplier_target_str);
  // Receive gRPC request and use the corresponding food id 
  // to query supplier and vendors.
  // Reture a minimum satisfying list of shop info and food info.
  grpc::Status CheckFood(grpc::ServerContext* context,
                         const supplyfinder::FinderRequest* request,
                         supplyfinder::ShopResponse* response);
  // Get corresponding food ID given food name
  long GetFoodID(const std::string& food_name);
  // Given the food name, return a full list of shop info.
  // Part of the CheckFood Span.
  std::vector<supplyfinder::ShopInfo> ProcessRequest(const std::string& food_name,
                                                     const opencensus::trace::Span* parent);

 private:
  // Helper functions
  static void PrintVendorInfo(const uint32_t id, const supplyfinder::VendorInfo& info);
  void InitFoodID(std::vector<std::string>& food_names);

  SupplierClient supplier_client_;
  // maps server address to the client instance
  std::unordered_map<std::string, VendorClient> vendor_clients_;
  // maps food name to food id
  std::unordered_map<std::string, uint32_t> food_id_;
};
