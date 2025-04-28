#include <infiniband/verbs.h>
#include <iostream>
#include <cstring>

int main() {
    int num_devices = 0;
    struct ibv_device **dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) {
        std::cerr << "No RDMA devices found!" << std::endl;
        return 1;
    }

    std::cout << "Found " << num_devices << " RDMA device(s)." << std::endl;

    struct ibv_context *ctx = ibv_open_device(dev_list[0]);
    if (!ctx) {
        std::cerr << "Failed to open RDMA device." << std::endl;
        return 1;
    }

    struct ibv_device_attr dev_attr;
    if (ibv_query_device(ctx, &dev_attr)) {
        std::cerr << "Failed to query device attributes." << std::endl;
        return 1;
    }

    if (dev_attr.transport_type != IBV_TRANSPORT_IB) {
        std::cerr << "Warning: Device is not using InfiniBand transport!" << std::endl;
    } else {
        std::cout << "Device is using InfiniBand transport." << std::endl;
    }

    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    if (!pd) {
        std::cerr << "Failed to allocate Protection Domain." << std::endl;
        return 1;
    }

    struct ibv_cq *cq = ibv_create_cq(ctx, 16, nullptr, nullptr, 0);
    if (!cq) {
        std::cerr << "Failed to create Completion Queue." << std::endl;
        return 1;
    }

    struct ibv_qp_init_attr qp_init_attr = {};
    qp_init_attr.send_cq = cq;
    qp_init_attr.recv_cq = cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 16;
    qp_init_attr.cap.max_recv_wr = 16;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;

    struct ibv_qp *qp = ibv_create_qp(pd, &qp_init_attr);
    if (!qp) {
        std::cerr << "Failed to create Queue Pair." << std::endl;
        return 1;
    }

    std::cout << "Successfully created a Reliable Connection (RC) Queue Pair!" << std::endl;

    struct ibv_port_attr port_attr;
    if (ibv_query_port(ctx, 1, &port_attr)) {
        std::cerr << "Failed to query port attributes." << std::endl;
        return 1;
    }

    if (port_attr.link_layer == IBV_LINK_LAYER_INFINIBAND) {
        std::cout << "Link layer is InfiniBand." << std::endl;
    } else if (port_attr.link_layer == IBV_LINK_LAYER_ETHERNET) {
        std::cout << "Warning: Link layer is Ethernet (RoCE detected)." << std::endl;
    }

    // Clean up
    ibv_destroy_qp(qp);
    ibv_destroy_cq(cq);
    ibv_dealloc_pd(pd);
    ibv_close_device(ctx);
    ibv_free_device_list(dev_list);

    std::cout << "RDMA connection initialization test passed!" << std::endl;
    return 0;
}

