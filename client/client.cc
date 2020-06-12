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

#include <grpcpp/grpcpp.h>
#include <grpcpp/opencensus.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/strings/str_cat.h"
#include "absl/time/clock.h"
#include "exporters.h"
#include "helpers.h"
#include "opencensus/trace/context_util.h"
#include "opencensus/trace/sampler.h"
#include "opencensus/trace/trace_config.h"
#include "opencensus/trace/with_span.h"

#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#include "proto/supplyfinder.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#include "supplyfinder.pb.h"
#endif

using google::protobuf::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using std::pair;
using std::vector;
using supplyfinder::Finder;
using supplyfinder::FinderRequest;
using supplyfinder::FoodID;
using supplyfinder::InventoryInfo;
using supplyfinder::ShopInfo;
using supplyfinder::ShopResponse;
using supplyfinder::Supplier;
using supplyfinder::SupplierInfo;
using supplyfinder::Vendor;
using supplyfinder::VendorInfo;

class FinderClient {
 public:
  FinderClient(std::shared_ptr<Channel> channel)
      : stub_(Finder::NewStub(channel)) {}

  void InquireFoodInfo(std::string& food_name, uint32_t quantity) {
    auto span = opencensus::trace::Span::StartSpan(
        "Supplyfinder-Client", /*parent=*/nullptr);
    std::cout << span.context().trace_id().Value();
    {
      opencensus::trace::WithSpan ws(span);
      FinderRequest request;
      request.set_food_name(food_name);
      request.set_quantity(quantity);
      ShopInfo reply;
      ClientContext context;
      context.AddMetadata("key1", "value1");
      ShopResponse response;
      opencensus::trace::GetCurrentSpan().AddAnnotation("Sending request.");
      Status status = stub_->CheckFood(&context, request, &response);
      if (!status.ok()) {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        opencensus::trace::GetCurrentSpan().SetStatus(
            opencensus::trace::StatusCode::UNKNOWN, status.error_message());
        opencensus::trace::GetCurrentSpan().End();
        return;
      } else if (response.shopinfo_size() == 0) {
        std::cout << "No shop found." << std::endl;
      }
      opencensus::trace::GetCurrentSpan().AddAnnotation("Printing Shop Information.");
      std::cout << "Receiving Shop Information" << std::endl;
      for (size_t i = 0; i < response.shopinfo_size(); ++i) {
        PrintResult(response.shopinfo(i));
      }
      opencensus::trace::GetCurrentSpan().End();
    }
  }

 private:
  std::unique_ptr<Finder::Stub> stub_;
};

int main(int argc, char* argv[]) {
  // The Client gets the finder address from the argument,
  // and create a finder client.
  std::string finder_addr = "0.0.0.0:50051";
  int c;

  // option 'f' specifies the Finder server it talks to.
  while ((c = getopt(argc, argv, "f:")) != -1) {
    switch (c) {
      case 'f':
        if (optarg) finder_addr = optarg;
        break;
    }
  }
  std::cout << "Finder address: " << finder_addr << std::endl;
  FinderClient client(
      grpc::CreateChannel(finder_addr, grpc::InsecureChannelCredentials()));
  opencensus::trace::TraceConfig::SetCurrentTraceParams(
      {128, 128, 128, 128, opencensus::trace::ProbabilitySampler(1.0)});
  grpc::RegisterOpenCensusPlugin();
  RegisterExporters();
  // Testing
  std::cout << "Try to find some food! Input the food name and quantity"
            << " separated by newline.\n You can try something like: flour, "
            << "apple, egg, milk, water, or even quail.\n Or q to stop."
            << std::endl;

  int quantity;
  std::string food_name;
  while (getline(std::cin, food_name) && food_name != "q") {
    std::string input;
    getline(std::cin, input);
    quantity = std::stoi(input);
    if (quantity < 0) {
      std::cout << "Please input a positive quantity." << std::endl;
      continue;
    }
    std::cout << "========== Querying Food: " << food_name
              << " Quantity = " << quantity << " ==========" << std::endl;
    client.InquireFoodInfo(food_name, quantity);
  }

  std::cout << "See you again!" << std::endl;
  return 0;
}
