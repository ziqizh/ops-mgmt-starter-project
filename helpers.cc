#include <iostream>
#include <string>

#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif

supplyfinder::VendorInfo MakeVendor(const std::string& url,
                                    const std::string& name,
                                    const std::string& location) {
  supplyfinder::VendorInfo vendor;
  vendor.set_url(url);
  vendor.set_name(name);
  vendor.set_location(location);
  return vendor;
}

void PrintResult(const supplyfinder::ShopInfo& info) {
  std::cout << "\tVendor url: " << info.vendor().url()
            << "; name: " << info.vendor().name()
            << "; location: " << info.vendor().location() << std::endl;
  std::cout << "\tInventory price: " << info.inventory().price()
            << "; quantity: " << info.inventory().quantity() << "\n\n";
}