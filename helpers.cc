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

void PrintResult(const supplyfinder::ShopInfo& p) {
  std::cout << "\tVendor url: " << p.vendor().url()
            << "; name: " << p.vendor().name()
            << "; location: " << p.vendor().location() << std::endl;
  std::cout << "\tInventory price: " << p.inventory().price()
            << "; quantity: " << p.inventory().quantity() << "\n\n";
}