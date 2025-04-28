CXX = g++
CXXFLAGS = -Wall -O2
LDLIBS = -libverbs -lrdmacm

TARGETS = test_rdma_init test_rdma_server test_rdma_client

all: $(TARGETS)

test_rdma_init: test_rdma_init.cpp
        $(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test_rdma_server: test_rdma_server.cpp
        $(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

test_rdma_client: test_rdma_client.cpp
        $(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

clean:
        rm -f $(TARGETS)
