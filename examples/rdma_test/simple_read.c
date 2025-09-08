//==============================================================================
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//==============================================================================

/**
 * @file simple_read.c
 * @brief Simplified RDMA Read Test Program for RecoNIC
 * 
 * This is a streamlined version of the RDMA read functionality that is easier
 * to understand and use. It provides both server and client functionality for
 * testing RDMA read operations.
 * 
 * Key improvements over the original:
 * - Cleaner code structure with better error handling
 * - More detailed logging and debugging information
 * - ARM platform optimizations
 * - Simplified configuration and usage
 */

#define _GNU_SOURCE
#include <time.h>
#include "reconic.h"
#include "rdma_api.h"
#include "rdma_test.h"

// Program configuration constants
#define DEVICE_NAME_DEFAULT "/dev/reconic-mm"
#define PCIE_RESOURCE_DEFAULT "/sys/bus/pci/devices/0005:01:00.0/resource2"
#define DEFAULT_PAYLOAD_SIZE 1024
#define DEFAULT_QP_DEPTH 64
#define DEFAULT_QP_ID 2
#define MAX_RETRIES 10

// Global configuration structure
typedef struct {
    char *device_name;
    char *pcie_resource;
    char *src_ip_str;
    char *dst_ip_str;
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t tcp_port;
    uint16_t udp_port;
    uint32_t payload_size;
    uint32_t qp_id;
    uint32_t dst_qp_id;
    char *qp_location;
    int is_server;
    int is_client;
    int verbose;
    int debug;
} rdma_config_t;

// Global variables
static rdma_config_t config;
static struct rn_dev_t* rn_dev = NULL;
static struct rdma_dev_t* rdma_dev = NULL;

// Function prototypes
static void init_config(rdma_config_t *cfg);
static int parse_arguments(int argc, char *argv[], rdma_config_t *cfg);
static void print_usage(const char *program_name);
static int setup_rdma_environment(rdma_config_t *cfg);
static int run_rdma_server(rdma_config_t *cfg);
static int run_rdma_client(rdma_config_t *cfg);
static void cleanup_resources(void);
static int get_remote_mac_address(const char *ip_str, struct mac_addr_t *mac_addr);
static void print_test_summary(rdma_config_t *cfg, double bandwidth, double latency);

/**
 * @brief Initialize configuration with default values
 */
static void init_config(rdma_config_t *cfg) {
    memset(cfg, 0, sizeof(rdma_config_t));
    cfg->device_name = DEVICE_NAME_DEFAULT;
    cfg->pcie_resource = PCIE_RESOURCE_DEFAULT;
    cfg->payload_size = DEFAULT_PAYLOAD_SIZE;
    cfg->qp_id = DEFAULT_QP_ID;
    cfg->dst_qp_id = DEFAULT_QP_ID;
    cfg->qp_location = HOST_MEM;
    cfg->tcp_port = 11111;
    cfg->udp_port = 22222;
}

/**
 * @brief Print program usage information
 */
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Simplified RDMA Read Test Program for RecoNIC\n\n");
    
    printf("Required Options:\n");
    printf("  -r, --src_ip IP        Source IP address\n");
    printf("  -i, --dst_ip IP        Destination IP address\n");
    printf("  -s, --server           Run as server (data provider)\n");
    printf("  -c, --client           Run as client (data reader)\n\n");
    
    printf("Optional Parameters:\n");
    printf("  -d, --device DEVICE    Character device (default: %s)\n", DEVICE_NAME_DEFAULT);
    printf("  -p, --pcie_resource    PCIe resource file (default: %s)\n", PCIE_RESOURCE_DEFAULT);
    printf("  -t, --tcp_port PORT    TCP port for coordination (default: 11111)\n");
    printf("  -u, --udp_port PORT    UDP port for RDMA (default: 22222)\n");
    printf("  -z, --payload_size N   Payload size in bytes (default: %d)\n", DEFAULT_PAYLOAD_SIZE);
    printf("  -q, --qp_id N          Queue Pair ID (default: %d)\n", DEFAULT_QP_ID);
    printf("  -l, --qp_location LOC  QP location: host_mem|dev_mem (default: host_mem)\n");
    printf("  -v, --verbose          Verbose output\n");
    printf("  -g, --debug            Debug mode\n");
    printf("  -h, --help             Show this help\n\n");
    
    printf("Examples:\n");
    printf("  Server (192.168.1.100):\n");
    printf("    %s -r 192.168.1.100 -i 192.168.1.101 -s -v\n\n", program_name);
    printf("  Client (192.168.1.101):\n");
    printf("    %s -r 192.168.1.101 -i 192.168.1.100 -c -v\n\n", program_name);
    
    printf("ARM Platform Notes:\n");
    printf("  - Optimized for NVIDIA Jetson and other ARM platforms\n");
    printf("  - Uses proper memory barriers for reliable operation\n");
    printf("  - Default PCIe path assumes ARM device numbering (0005:01:00.0)\n\n");
}

/**
 * @brief Parse command line arguments
 */
static int parse_arguments(int argc, char *argv[], rdma_config_t *cfg) {
    int opt;
    static struct option long_options[] = {
        {"device", required_argument, 0, 'd'},
        {"pcie_resource", required_argument, 0, 'p'},
        {"src_ip", required_argument, 0, 'r'},
        {"dst_ip", required_argument, 0, 'i'},
        {"tcp_port", required_argument, 0, 't'},
        {"udp_port", required_argument, 0, 'u'},
        {"payload_size", required_argument, 0, 'z'},
        {"qp_id", required_argument, 0, 'q'},
        {"qp_location", required_argument, 0, 'l'},
        {"server", no_argument, 0, 's'},
        {"client", no_argument, 0, 'c'},
        {"verbose", no_argument, 0, 'v'},
        {"debug", no_argument, 0, 'g'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "d:p:r:i:t:u:z:q:l:scvgh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd':
            cfg->device_name = strdup(optarg);
            break;
        case 'p':
            cfg->pcie_resource = strdup(optarg);
            break;
        case 'r':
            cfg->src_ip_str = strdup(optarg);
            cfg->src_ip = convert_ip_addr_to_uint(optarg);
            break;
        case 'i':
            cfg->dst_ip_str = strdup(optarg);
            cfg->dst_ip = convert_ip_addr_to_uint(optarg);
            break;
        case 't':
            cfg->tcp_port = (uint16_t)atoi(optarg);
            break;
        case 'u':
            cfg->udp_port = (uint16_t)atoi(optarg);
            break;
        case 'z':
            cfg->payload_size = (uint32_t)atoi(optarg);
            break;
        case 'q':
            cfg->qp_id = (uint32_t)atoi(optarg);
            cfg->dst_qp_id = cfg->qp_id; // Default to same QP ID
            break;
        case 'l':
            if (strcmp(optarg, "host_mem") == 0 || strcmp(optarg, "dev_mem") == 0) {
                cfg->qp_location = strdup(optarg);
            } else {
                fprintf(stderr, "Error: Invalid QP location. Use 'host_mem' or 'dev_mem'\n");
                return -1;
            }
            break;
        case 's':
            cfg->is_server = 1;
            break;
        case 'c':
            cfg->is_client = 1;
            break;
        case 'v':
            cfg->verbose = 1;
            break;
        case 'g':
            cfg->debug = 1;
            cfg->verbose = 1; // Debug implies verbose
            break;
        case 'h':
            print_usage(argv[0]);
            return 1;
        default:
            print_usage(argv[0]);
            return -1;
        }
    }
    
    // Validate required parameters
    if (!cfg->src_ip_str || !cfg->dst_ip_str) {
        fprintf(stderr, "Error: Source and destination IP addresses are required\n");
        return -1;
    }
    
    if (!cfg->is_server && !cfg->is_client) {
        fprintf(stderr, "Error: Must specify either --server or --client mode\n");
        return -1;
    }
    
    if (cfg->is_server && cfg->is_client) {
        fprintf(stderr, "Error: Cannot be both server and client\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get MAC address for a given IP address using ARP
 */
static int get_remote_mac_address(const char *ip_str, struct mac_addr_t *mac_addr) {
    char command[128];
    FILE *arp_fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read_len;
    char *mac_ptr;
    char mac_str[18];
    int result = -1;
    
    if (config.verbose) {
        printf("Looking up MAC address for IP: %s\n", ip_str);
    }
    
    snprintf(command, sizeof(command), "arp -a %s", ip_str);
    arp_fp = popen(command, "r");
    if (!arp_fp) {
        perror("Error executing arp command");
        return -1;
    }
    
    while ((read_len = getline(&line, &len, arp_fp)) != -1) {
        if (strstr(line, "no match found")) {
            fprintf(stderr, "Error: No ARP entry for %s. Try: ping -c 1 %s\n", ip_str, ip_str);
            break;
        }
        
        if (strstr(line, "at")) {
            mac_ptr = strstr(line, "at") + 3;
            strncpy(mac_str, mac_ptr, 17);
            mac_str[17] = '\0';
            
            *mac_addr = convert_mac_addr_str_to_uint(mac_str);
            if (config.verbose) {
                printf("Found MAC address: %s\n", mac_str);
            }
            result = 0;
            break;
        }
    }
    
    free(line);
    pclose(arp_fp);
    return result;
}

/**
 * @brief Setup RDMA environment (devices, buffers, queues)
 */
static int setup_rdma_environment(rdma_config_t *cfg) {
    int pcie_resource_fd;
    struct mac_addr_t src_mac;
    int local_fpga_fd;
    
    if (cfg->verbose) {
        printf("Setting up RDMA environment...\n");
        printf("  Device: %s\n", cfg->device_name);
        printf("  PCIe Resource: %s\n", cfg->pcie_resource);
        printf("  Payload Size: %u bytes\n", cfg->payload_size);
        printf("  QP Location: %s\n", cfg->qp_location);
    }
    
    // Get source MAC address
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return -1;
    }
    
    src_mac = get_mac_addr_from_str_ip(sockfd, cfg->src_ip_str);
    close(sockfd);
    
    // Create RecoNIC device
    if (cfg->verbose) {
        printf("Creating RecoNIC device...\n");
    }
    rn_dev = create_rn_dev(cfg->pcie_resource, &pcie_resource_fd, preallocated_hugepages, 8);
    if (!rn_dev) {
        fprintf(stderr, "Error: Failed to create RecoNIC device\n");
        return -1;
    }
    
    // Create RDMA device
    if (cfg->verbose) {
        printf("Creating RDMA device...\n");
    }
    rdma_dev = create_rdma_dev(rn_dev);
    if (!rdma_dev) {
        fprintf(stderr, "Error: Failed to create RDMA device\n");
        return -1;
    }
    
    // Allocate system buffers
    if (cfg->verbose) {
        printf("Allocating system buffers...\n");
    }
    
    // CQ/RQ doorbell buffer
    uint32_t cidb_buffer_size = (1 << HUGE_PAGE_SHIFT);
    struct rdma_buff_t* cidb_buffer = allocate_rdma_buffer(rn_dev, cidb_buffer_size, "host_mem");
    if (!cidb_buffer) {
        fprintf(stderr, "Error: Failed to allocate CIDB buffer\n");
        return -1;
    }
    
    uint64_t cq_cidb_addr = cidb_buffer->dma_addr;
    uint64_t rq_cidb_addr = cidb_buffer->dma_addr + (8 << 2); // 8 QPs * 4 bytes
    
    // System buffers for RDMA engine
    struct rdma_buff_t* data_buf = allocate_rdma_buffer(rn_dev, 4096 * 4096, "host_mem");
    struct rdma_buff_t* ipkterr_buf = allocate_rdma_buffer(rn_dev, 8192, "host_mem");
    struct rdma_buff_t* err_buf = allocate_rdma_buffer(rn_dev, 256 * 256, "host_mem");
    struct rdma_buff_t* resp_err_buf = allocate_rdma_buffer(rn_dev, 65536, "host_mem");
    
    if (!data_buf || !ipkterr_buf || !err_buf || !resp_err_buf) {
        fprintf(stderr, "Error: Failed to allocate system buffers\n");
        return -1;
    }
    
    // Open RDMA engine
    if (cfg->verbose) {
        printf("Opening RDMA engine...\n");
    }
    open_rdma_dev(rdma_dev, src_mac, cfg->src_ip, cfg->udp_port,
                  4096, 4096, data_buf->dma_addr,
                  8192, ipkterr_buf->dma_addr,
                  256, 256, err_buf->dma_addr,
                  65536, resp_err_buf->dma_addr);
    
    // Allocate protection domain
    struct rdma_pd_t* pd = allocate_rdma_pd(rdma_dev, 0);
    if (!pd) {
        fprintf(stderr, "Error: Failed to allocate protection domain\n");
        return -1;
    }
    
    // Open device file for DMA operations (declare locally for server)
    local_fpga_fd = open(cfg->device_name, O_RDWR);
    if (local_fpga_fd < 0) {
        perror("Error opening device file");
        return -1;
    }
    
    if (cfg->verbose) {
        printf("RDMA environment setup completed successfully\n");
    }
    
    return 0;
}

/**
 * @brief Run RDMA server (provides data for reading)
 */
static int run_rdma_server(rdma_config_t *cfg) {
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    struct rdma_buff_t* data_buffer;
    uint64_t buffer_offset;
    uint32_t *test_data;
    struct rdma_pd_t* pd;
    struct mac_addr_t dst_mac;
    int local_fpga_fd;
    
    printf("=== RDMA Server Mode ===\n");
    
    if (get_remote_mac_address(cfg->dst_ip_str, &dst_mac) < 0) {
        fprintf(stderr, "Error: Could not resolve MAC address for %s\n", cfg->dst_ip_str);
        return -1;
    }
    
    // Allocate data buffer
    data_buffer = allocate_rdma_buffer(rn_dev, cfg->payload_size, cfg->qp_location);
    if (!data_buffer) {
        fprintf(stderr, "Error: Failed to allocate data buffer\n");
        return -1;
    }
    
    // Get protection domain
    pd = allocate_rdma_pd(rdma_dev, 0);
    
    // Register memory region
    rdma_register_memory_region(rdma_dev, pd, R_KEY, data_buffer);
    
    // Initialize test data
    if (strcmp(cfg->qp_location, "dev_mem") == 0) {
        // Device memory - need to use DMA to write data
        test_data = malloc(cfg->payload_size);
        for (uint32_t i = 0; i < cfg->payload_size / 4; i++) {
            test_data[i] = i % 256; // Simple test pattern
        }
        
        if (write_from_buffer(cfg->device_name, local_fpga_fd, (char*)test_data, 
                            cfg->payload_size, data_buffer->dma_addr) < 0) {
            fprintf(stderr, "Error: Failed to write test data to device memory\n");
            free(test_data);
            return -1;
        }
        free(test_data);
    } else {
        // Host memory - direct access
        test_data = (uint32_t*)data_buffer->buffer;
        for (uint32_t i = 0; i < cfg->payload_size / 4; i++) {
            test_data[i] = i % 256;
        }
    }
    
    if (cfg->verbose) {
        printf("Test data initialized in %s\n", cfg->qp_location);
        printf("Buffer physical address: 0x%lx\n", data_buffer->dma_addr);
    }
    
    // Setup TCP server for coordination
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("Error creating server socket");
        return -1;
    }
    
    int reuse = 1;
    setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(cfg->src_ip_str);
    server_addr.sin_port = htons(cfg->tcp_port);
    
    if (bind(server_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        close(server_sockfd);
        return -1;
    }
    
    if (listen(server_sockfd, 1) < 0) {
        perror("Error listening on server socket");
        close(server_sockfd);
        return -1;
    }
    
    printf("Server listening on %s:%d\n", cfg->src_ip_str, cfg->tcp_port);
    printf("Waiting for client connection...\n");
    
    client_len = sizeof(client_addr);
    client_sockfd = accept(server_sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (client_sockfd < 0) {
        perror("Error accepting client connection");
        close(server_sockfd);
        return -1;
    }
    
    printf("Client connected from %s\n", inet_ntoa(client_addr.sin_addr));
    
    // Send buffer address to client
    buffer_offset = htonll((uint64_t)data_buffer->buffer);
    if (send(client_sockfd, &buffer_offset, sizeof(buffer_offset), 0) != sizeof(buffer_offset)) {
        fprintf(stderr, "Error sending buffer offset to client\n");
        close(client_sockfd);
        close(server_sockfd);
        return -1;
    }
    
    if (cfg->verbose) {
        printf("Sent buffer offset 0x%lx to client\n", ntohll(buffer_offset));
    }
    
    // Setup QP for RDMA operations
    allocate_rdma_qp(rdma_dev, cfg->qp_id, cfg->dst_qp_id, pd,
                     data_buffer->dma_addr, data_buffer->dma_addr + 64,
                     DEFAULT_QP_DEPTH, cfg->qp_location, &dst_mac, cfg->dst_ip,
                     P_KEY, R_KEY);
    
    config_last_rq_psn(rdma_dev, cfg->qp_id, 0xabc);
    config_sq_psn(rdma_dev, cfg->qp_id, 0xabc + 1);
    
    printf("Server setup complete. Press Enter when client finishes RDMA read...\n");
    getchar();
    
    // Print server-side statistics
    if (cfg->debug) {
        dump_registers(rdma_dev, 0, cfg->qp_id);
    }
    
    close(client_sockfd);
    close(server_sockfd);
    
    printf("Server operation completed successfully\n");
    return 0;
}

/**
 * @brief Run RDMA client (reads data from server)
 */
static int run_rdma_client(rdma_config_t *cfg) {
    int client_sockfd;
    struct sockaddr_in server_addr;
    struct rdma_buff_t* recv_buffer;
    uint64_t remote_offset;
    uint32_t *received_data, *golden_data;
    struct timespec start_time, end_time;
    double elapsed_time, bandwidth;
    struct rdma_pd_t* pd;
    struct mac_addr_t dst_mac;
    int ret;
    int local_fpga_fd;
    
    printf("=== RDMA Client Mode ===\n");
    
    // Open device file for DMA operations
    local_fpga_fd = open(cfg->device_name, O_RDWR);
    if (local_fpga_fd < 0) {
        perror("Error opening device file in client");
        return -1;
    }
    
    if (get_remote_mac_address(cfg->dst_ip_str, &dst_mac) < 0) {
        fprintf(stderr, "Error: Could not resolve MAC address for %s\n", cfg->dst_ip_str);
        return -1;
    }
    
    // Connect to server
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0) {
        perror("Error creating client socket");
        return -1;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(cfg->dst_ip_str);
    server_addr.sin_port = htons(cfg->tcp_port);
    
    printf("Connecting to server %s:%d...\n", cfg->dst_ip_str, cfg->tcp_port);
    
    if (connect(client_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(client_sockfd);
        return -1;
    }
    
    printf("Connected to server\n");
    
    // Receive remote buffer offset
    if (recv(client_sockfd, &remote_offset, sizeof(remote_offset), 0) != sizeof(remote_offset)) {
        fprintf(stderr, "Error receiving remote buffer offset\n");
        close(client_sockfd);
        return -1;
    }
    
    remote_offset = ntohll(remote_offset);
    if (cfg->verbose) {
        printf("Received remote buffer offset: 0x%lx\n", remote_offset);
    }
    
    // Allocate receive buffer
    recv_buffer = allocate_rdma_buffer(rn_dev, cfg->payload_size, cfg->qp_location);
    if (!recv_buffer) {
        fprintf(stderr, "Error: Failed to allocate receive buffer\n");
        close(client_sockfd);
        return -1;
    }
    
    // Get protection domain and setup QP
    pd = allocate_rdma_pd(rdma_dev, 0);
    
    allocate_rdma_qp(rdma_dev, cfg->qp_id, cfg->dst_qp_id, pd,
                     recv_buffer->dma_addr, recv_buffer->dma_addr + 64,
                     DEFAULT_QP_DEPTH, cfg->qp_location, &dst_mac, cfg->dst_ip,
                     P_KEY, R_KEY);
    
    config_last_rq_psn(rdma_dev, cfg->qp_id, 0xabc);
    config_sq_psn(rdma_dev, cfg->qp_id, 0xabc + 1);
    
    // Create RDMA read WQE
    if (cfg->verbose) {
        printf("Creating RDMA read WQE...\n");
        printf("  Local buffer: 0x%lx\n", recv_buffer->dma_addr);
        printf("  Remote buffer: 0x%lx\n", remote_offset);
        printf("  Length: %u bytes\n", cfg->payload_size);
    }
    
    create_a_wqe(rdma_dev, cfg->qp_id, 0, 0, recv_buffer->dma_addr,
                 cfg->payload_size, RNIC_OP_READ, remote_offset, R_KEY,
                 0, 0, 0, 0, 0);
    
    // Execute RDMA read operation
    printf("Executing RDMA read operation...\n");
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    ret = rdma_post_send(rdma_dev, cfg->qp_id);
    if (ret < 0) {
        fprintf(stderr, "Error: RDMA post send failed with code %d\n", ret);
        close(client_sockfd);
        return -1;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate performance metrics
    timespec_sub(&end_time, &start_time);
    elapsed_time = end_time.tv_sec + (double)end_time.tv_nsec / 1000000000.0;
    bandwidth = (double)cfg->payload_size / elapsed_time;
    
    printf("RDMA read operation completed\n");
    
    // Retrieve and verify data
    if (strcmp(cfg->qp_location, "dev_mem") == 0) {
        // Device memory - need DMA read
        received_data = malloc(cfg->payload_size);
        if (read_to_buffer(cfg->device_name, local_fpga_fd, (char*)received_data,
                          cfg->payload_size, recv_buffer->dma_addr) < 0) {
            fprintf(stderr, "Error: Failed to read data from device memory\n");
            free(received_data);
            close(client_sockfd);
            return -1;
        }
    } else {
        // Host memory - direct access
        received_data = (uint32_t*)recv_buffer->buffer;
    }
    
    // Generate golden data for verification
    golden_data = malloc(cfg->payload_size);
    for (uint32_t i = 0; i < cfg->payload_size / 4; i++) {
        golden_data[i] = i % 256;
    }
    
    // Verify data integrity
    printf("Verifying received data...\n");
    int errors = 0;
    for (uint32_t i = 0; i < cfg->payload_size / 4; i++) {
        if (received_data[i] != golden_data[i]) {
            if (errors < 10) { // Limit error output
                fprintf(stderr, "Data mismatch at offset %u: expected %u, got %u\n",
                       i, golden_data[i], received_data[i]);
            }
            errors++;
        }
    }
    
    if (errors == 0) {
        printf("✓ Data verification PASSED - All %u words correct\n", cfg->payload_size / 4);
    } else {
        printf("✗ Data verification FAILED - %d errors out of %u words\n", 
               errors, cfg->payload_size / 4);
    }
    
    // Print performance summary
    print_test_summary(cfg, bandwidth, elapsed_time * 1000000); // Convert to microseconds
    
    // Debug output
    if (cfg->debug) {
        printf("\nFirst 16 received values:\n");
        for (int i = 0; i < 16 && i < cfg->payload_size / 4; i++) {
            printf("  [%d] = %u\n", i, received_data[i]);
        }
        
        dump_registers(rdma_dev, 1, cfg->qp_id);
    }
    
    // Cleanup
    if (strcmp(cfg->qp_location, "dev_mem") == 0) {
        free(received_data);
    }
    free(golden_data);
    close(client_sockfd);
    
    printf("Client operation completed %s\n", errors == 0 ? "successfully" : "with errors");
    return errors == 0 ? 0 : -1;
}

/**
 * @brief Print test performance summary
 */
static void print_test_summary(rdma_config_t *cfg, double bandwidth, double latency) {
    printf("\n=== Performance Summary ===\n");
    printf("Payload Size:    %u bytes\n", cfg->payload_size);
    printf("Latency:         %.2f microseconds\n", latency);
    printf("Bandwidth:       %.2f MB/s\n", bandwidth / (1024 * 1024));
    printf("Bandwidth:       %.2f Gb/s\n", (bandwidth * 8) / 1000000000);
    printf("QP Location:     %s\n", cfg->qp_location);
    printf("==========================\n");
}

/**
 * @brief Cleanup allocated resources
 */
static void cleanup_resources(void) {
    if (rn_dev) {
        destroy_rn_dev(rn_dev);
    }
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    int result;
    
    printf("RecoNIC Simple RDMA Read Test\n");
    printf("=============================\n\n");
    
    // Initialize configuration
    init_config(&config);
    
    // Parse command line arguments
    result = parse_arguments(argc, argv, &config);
    if (result != 0) {
        return result > 0 ? 0 : 1; // Help returns 1, errors return -1
    }
    
    // Setup RDMA environment
    if (setup_rdma_environment(&config) < 0) {
        fprintf(stderr, "Error: Failed to setup RDMA environment\n");
        cleanup_resources();
        return 1;
    }
    
    // Run server or client
    if (config.is_server) {
        result = run_rdma_server(&config);
    } else {
        result = run_rdma_client(&config);
    }
    
    // Cleanup
    cleanup_resources();
    
    return result == 0 ? 0 : 1;
}