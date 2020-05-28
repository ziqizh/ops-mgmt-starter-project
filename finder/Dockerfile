FROM grpc/cxx:1.12.0

WORKDIR /data

COPY finder/finder.* ./finder/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile ./

RUN make supplyfinder-finder && make clean

CMD ["./supplyfinder-finder --target=34.72.6.187:50052"]
