FROM grpc/cxx:1.12.0

WORKDIR /data

COPY client/client.* ./client/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile ./

RUN make supplyfinder-client && make clean

CMD ["./supplyfinder-client"]
