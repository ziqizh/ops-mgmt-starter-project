FROM grpc/cxx:1.12.0

WORKDIR /data

ARG SUPPLIER
ENV SUPPLIER_ADDRESS=$SUPPLIER

COPY finder/finder.* ./finder/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile ./

RUN make supplyfinder-finder && make clean

CMD ["/bin/bash", "-c", "./supplyfinder-finder -s $SUPPLIER_ADDRESS"]
