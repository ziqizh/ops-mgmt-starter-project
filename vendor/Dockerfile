FROM grpc/cxx:1.12.0

WORKDIR /data

ARG VENDOR1
ARG VENDOR2
ARG VENDOR3
ARG SUPPLIER
ENV SUPPLIER_ADDRESS=$SUPPLIER
ENV vendor_1=$VENDOR1
ENV vendor_2=$VENDOR2
ENV vendor_3=$VENDOR3

COPY vendor/vendor.* ./vendor/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile helpers.cc ./

RUN make supplyfinder-vendor && make clean

CMD ["/bin/bash", "-c", "./supplyfinder-vendor -s $SUPPLIER_ADDRESS -v ${!SVC} -n $NAME -l $LOCATION"]
