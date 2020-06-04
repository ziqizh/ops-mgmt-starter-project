FROM grpc/cxx:1.12.0

WORKDIR /data

COPY supplier/supplier.* ./supplier/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile helpers.cc ./

RUN make supplyfinder-supplier && make clean

CMD ["./supplyfinder-supplier"]
