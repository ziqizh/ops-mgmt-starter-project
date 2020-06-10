#ifdef BAZEL_BUILD
#include "proto/supplyfinder.grpc.pb.h"
#else
#include "supplyfinder.grpc.pb.h"
#endif
#include <string>


supplyfinder::VendorInfo MakeVendor(const std::string&, const std::string&,
                                    const std::string&);

void PrintResult(const supplyfinder::ShopInfo&);