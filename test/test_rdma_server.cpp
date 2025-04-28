#include <infiniband/verbs.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

#define PORT 18515

struct qp_info_t {
    uint32_t qp_num;
    uint16_t lid;
};

int main() {
    int sockfd, newsockfd;
    struct sockaddr_in serv_addr{}, cli_addr{};
    socklen_t clilen = sizeof(cli_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    listen(sockfd, 1);
    std::cout << "Server: Waiting for connection..." << std::endl;
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    std::cout << "Server: Connection accepted!" << std::endl;

    // Setup RDMA
    struct ibv_device **dev_list = ibv_get_device_list(nullptr);
    struct ibv_context *ctx = ibv_open_device(dev_list[0]);
    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    struct ibv_cq *cq = ibv_create_cq(ctx, 16, nullptr, nullptr, 0);

    struct ibv_qp_init_attr qp_init_attr{};
    qp_init_attr.send_cq = cq;
    qp_init_attr.recv_cq = cq;
    qp_init_attr.qp_type = IBV_QPT_RC;
    qp_init_attr.cap.max_send_wr = 16;
    qp_init_attr.cap.max_recv_wr = 16;
    qp_init_attr.cap.max_send_sge = 1;
    qp_init_attr.cap.max_recv_sge = 1;

    struct ibv_qp *qp = ibv_create_qp(pd, &qp_init_attr);

    struct ibv_port_attr port_attr;
    ibv_query_port(ctx, 1, &port_attr);

    // Send my qp_info to client
    qp_info_t my_info = {qp->qp_num, port_attr.lid};
    write(newsockfd, &my_info, sizeof(my_info));

    // Receive client's qp_info
    qp_info_t client_info;
    read(newsockfd, &client_info, sizeof(client_info));

    // Modify QP state: INIT
    struct ibv_qp_attr attr{};
    attr.qp_state = IBV_QPS_INIT;
    attr.pkey_index = 0;
    attr.port_num = 1;
    attr.qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
    ibv_modify_qp(qp, &attr,
                  IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                  IBV_QP_PORT | IBV_QP_ACCESS_FLAGS);

    // Modify QP state: RTR (Ready to Receive)
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTR;
    attr.path_mtu = IBV_MTU_1024;
    attr.dest_qp_num = client_info.qp_num;
    attr.rq_psn = 0;
    attr.max_dest_rd_atomic = 1;
    attr.min_rnr_timer = 12;
    attr.ah_attr.is_global = 0;
    attr.ah_attr.dlid = client_info.lid;
    attr.ah_attr.sl = 0;
    attr.ah_attr.src_path_bits = 0;
    attr.ah_attr.port_num = 1;
    ibv_modify_qp(qp, &attr,
                  IBV_QP_STATE | IBV_QP_AV |
                  IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                  IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC |
                  IBV_QP_MIN_RNR_TIMER);

    // Modify QP state: RTS (Ready to Send)
    memset(&attr, 0, sizeof(attr));
    attr.qp_state = IBV_QPS_RTS;
    attr.timeout = 14;
    attr.retry_cnt = 7;
    attr.rnr_retry = 7;
    attr.sq_psn = 0;
    attr.max_rd_atomic = 1;
    ibv_modify_qp(qp, &attr,
                  IBV_QP_STATE | IBV_QP_TIMEOUT |
                  IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
                  IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC);

    std::cout << "Server: RDMA connection established!" << std::endl;

    close(newsockfd);
    close(sockfd);
    ibv_destroy_qp(qp);
    ibv_destroy_cq(cq);
    ibv_dealloc_pd(pd);
    ibv_close_device(ctx);
    ibv_free_device_list(dev_list);

    return 0;
}

