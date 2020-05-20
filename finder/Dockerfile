FROM grpc/cxx:1.12.0

WORKDIR /data

COPY finder/finder.* ./server/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile ./

RUN make finder && make clean

CMD ["./greeter-server"]
