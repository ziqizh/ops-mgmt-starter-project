#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif
#include <string>

supplyfinder::VendorInfo MakeVendor(const std::string& url,
                                    const std::string& name,
                                    const std::string& location);

void PrintResult(const supplyfinder::ShopInfo& info);