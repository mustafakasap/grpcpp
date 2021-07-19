FROM amd64/ubuntu:20.04 AS build-env

USER root

ARG DEPENDENCIES_PATH ../dependencies

ENV TEMP_DIR /tmp/grpcpp
ENV DEV_DIR /dev

RUN mkdir {/dev,/tmp/grpcpp}



# Latest CMAKE install binary
COPY $DEPENDENCIES_PATH/cmake-3.21.0-linux-x86_64.sh $TEMP_DIR

# Install CMAKE
RUN cd $TEMP_DIR \
	&& ./cmake-3.21.0-linux-x86_64.sh --skip-license --prefix=/

RUN apt-get update \
	&& apt-get install -y --no-install-recommends \
			autoconf \
			build-essential \
			curl \
			g++ \
			gcc \
			gdb \
			make \
			pkg-config \
			software-properties-common \
	&& apt-get update \
	&& rm -rf /var/lib/apt/lists/*



ENV GRPC_INSTALL_DIR /root/.local
ENV PATH "$PATH:$GRPC_INSTALL_DIR/bin"

# Before running below commands, must have local clone of grpc repo in parent directory
# Update branch version accordingly
# git clone --recurse-submodules -b v1.38.1 https://github.com/grpc/grpc
COPY $DEPENDENCIES_PATH/grpc/ $TEMP_DIR/grpc

RUN mkdir -p $GRPC_INSTALL_DIR \
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p "third_party/abseil-cpp/cmake/build" \
	&& cd "third_party/abseil-cpp/cmake/build" \
	&& cmake ../.. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_POSITION_INDEPENDENT_CODE=TRUE \
	&& make -j$(nproc --all) install \
	\
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p "third_party/cares/cares/cmake/build" \
	&& cd "third_party/cares/cares/cmake/build" \
	&& cmake ../.. \
	-DCMAKE_BUILD_TYPE=Release \
	&& make -j$(nproc --all) install \
	\
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p "third_party/protobuf/cmake/build" \
	&& cd "third_party/protobuf/cmake/build" \
	&& cmake .. \
	-Dprotobuf_BUILD_TESTS=OFF \
	-DCMAKE_BUILD_TYPE=Release .. \
	&& make -j$(nproc --all) install \
	\
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p "third_party/re2/cmake/build" \
	&& cd "third_party/re2/cmake/build" \
	&& cmake ../.. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_POSITION_INDEPENDENT_CODE=TRUE \
	&& make -j$(nproc --all) install \
	\
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p "third_party/zlib/cmake/build" \
	&& cd "third_party/zlib/cmake/build" \
	&& cmake -DCMAKE_BUILD_TYPE=Release ../.. \
	&& make -j$(nproc --all) install \
	\
	\
	&& cd $TEMP_DIR/grpc \
	&& mkdir -p cmake/build \
	&& cd cmake/build \
	&& cmake ../.. \
	-DCMAKE_INSTALL_PREFIX=$GRPC_INSTALL_DIR \
	-DCMAKE_BUILD_TYPE=Release \
	-DgRPC_INSTALL=ON \
	-DgRPC_BUILD_TESTS=OFF \
	-DgRPC_CARES_PROVIDER=package \
	-DgRPC_ABSL_PROVIDER=package \
	-DgRPC_PROTOBUF_PROVIDER=package \
	-DgRPC_RE2_PROVIDER=package \
	-DgRPC_SSL_PROVIDER=package \
	-DgRPC_ZLIB_PROVIDER=package \
	&& make -j$(nproc --all) install



# Clean-up
RUN rm -rf $TEMP_DIR \
    && apt-get update \
	&& apt-get upgrade \
	&& rm -rf /var/lib/apt/lists/*