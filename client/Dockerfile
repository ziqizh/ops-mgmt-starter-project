FROM grpc/cxx:1.12.0

WORKDIR /data
ARG FINDER
ENV FINDER_ADDRESS=$FINDER

COPY client/client.* ./client/
COPY proto/supplyfinder.proto ./proto/
COPY Makefile helpers.cc ./

RUN make supplyfinder-client && make clean

CMD ["/bin/bash", "-c", "./supplyfinder-client -f $FINDER_ADDRESS"]
