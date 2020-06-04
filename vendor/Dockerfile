FROM grpc/cxx:1.12.0

WORKDIR /data

COPY vendor/vendor.* ./vendor/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile helpers.cc ./

RUN make supplyfinder-vendor && make clean

CMD ["./supplyfinder-vendor"]
