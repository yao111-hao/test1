# RecoNIC Simple RDMA Read Test

## 概述

这是一个简化和改进的RDMA读取测试程序，基于原始的`read.c`重新设计，具有更好的代码结构、错误处理和用户体验。

## 主要改进

### 相比原始`read.c`的优势

1. **更清晰的代码结构**
   - 模块化设计，功能分离明确
   - 统一的配置管理
   - 更好的错误处理和恢复

2. **更简单的使用方式**
   - 直观的命令行参数
   - 自动化的系统检查
   - 详细的帮助信息和示例

3. **更好的调试支持**
   - 分级的日志输出（info、verbose、debug）
   - 详细的性能指标
   - 完整的数据验证和错误报告

4. **ARM平台优化**
   - 针对NVIDIA Jetson等ARM平台的默认配置
   - 内存屏障和缓存优化
   - 更好的错误诊断

## 文件结构

```
/workspace/examples/rdma_test/
├── simple_read.c              # 简化的RDMA读取程序
├── Makefile_simple            # 专用Makefile
├── test_simple_rdma.sh        # 自动化测试脚本
├── README_SIMPLE.md           # 使用说明（本文档）
└── read.c                     # 原始程序（保留作参考）
```

## 编译

### 编译简化版本

```bash
cd /workspace/examples/rdma_test
make -f Makefile_simple
```

### 清理编译文件

```bash
make -f Makefile_simple clean
```

## 使用方法

### 1. 基本使用

#### 服务器端（数据提供者）

```bash
# 基本用法
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s

# 详细输出
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s -v

# 调试模式
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s -g
```

#### 客户端（数据读取者）

```bash
# 基本用法
sudo ./simple_read -r 192.168.1.101 -i 192.168.1.100 -c

# 详细输出
sudo ./simple_read -r 192.168.1.101 -i 192.168.1.100 -c -v

# 调试模式
sudo ./simple_read -r 192.168.1.101 -i 192.168.1.100 -c -g
```

### 2. 使用测试脚本

测试脚本提供了更方便的使用方式：

```bash
# 查看帮助
./test_simple_rdma.sh -h

# 运行服务器（终端1）
sudo ./test_simple_rdma.sh -s 192.168.1.100 -c 192.168.1.101 -v server

# 运行客户端（终端2）
sudo ./test_simple_rdma.sh -s 192.168.1.100 -c 192.168.1.101 -v client

# 查看演示说明
./test_simple_rdma.sh demo

# 查看测试配置
./test_simple_rdma.sh test
```

## 命令行参数

### 必需参数

| 参数 | 说明 | 示例 |
|------|------|------|
| `-r, --src_ip` | 源IP地址 | `-r 192.168.1.100` |
| `-i, --dst_ip` | 目标IP地址 | `-i 192.168.1.101` |
| `-s, --server` | 服务器模式 | `-s` |
| `-c, --client` | 客户端模式 | `-c` |

### 可选参数

| 参数 | 说明 | 默认值 | 示例 |
|------|------|--------|------|
| `-d, --device` | 字符设备文件 | `/dev/reconic-mm` | `-d /dev/reconic-mm` |
| `-p, --pcie_resource` | PCIe资源文件 | `/sys/bus/pci/devices/0005:01:00.0/resource2` | `-p /sys/bus/...` |
| `-t, --tcp_port` | TCP协调端口 | `11111` | `-t 11111` |
| `-u, --udp_port` | UDP RDMA端口 | `22222` | `-u 22222` |
| `-z, --payload_size` | 负载大小（字节） | `1024` | `-z 4096` |
| `-q, --qp_id` | 队列对ID | `2` | `-q 2` |
| `-l, --qp_location` | QP位置 | `host_mem` | `-l dev_mem` |
| `-v, --verbose` | 详细输出 | 关闭 | `-v` |
| `-g, --debug` | 调试模式 | 关闭 | `-g` |
| `-h, --help` | 显示帮助 | - | `-h` |

## 操作流程

### 完整的RDMA读取流程

1. **系统准备**
   ```bash
   # 检查PCIe设备
   lspci | grep Xilinx
   
   # 确保网络连通性
   ping -c 1 <remote_ip>
   
   # 建立ARP缓存
   arping -c 1 <remote_ip>
   ```

2. **启动服务器**（终端1）
   ```bash
   sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s -v
   ```
   
   服务器会：
   - 初始化RDMA环境
   - 分配和初始化测试数据
   - 监听TCP连接
   - 等待客户端连接

3. **启动客户端**（终端2）
   ```bash
   sudo ./simple_read -r 192.168.1.101 -i 192.168.1.100 -c -v
   ```
   
   客户端会：
   - 连接到服务器
   - 获取远程缓冲区地址
   - 执行RDMA读取操作
   - 验证数据完整性
   - 显示性能指标

### 典型输出示例

#### 服务器输出
```
RecoNIC Simple RDMA Read Test
=============================

=== RDMA Server Mode ===
Setting up RDMA environment...
  Device: /dev/reconic-mm
  PCIe Resource: /sys/bus/pci/devices/0005:01:00.0/resource2
  Payload Size: 1024 bytes
  QP Location: host_mem
Creating RecoNIC device...
Creating RDMA device...
Test data initialized in host_mem
Buffer physical address: 0x7f8a2c000000
Server listening on 192.168.1.100:11111
Waiting for client connection...
Client connected from 192.168.1.101
Sent buffer offset 0x7f8a2c000000 to client
Server setup complete. Press Enter when client finishes RDMA read...
```

#### 客户端输出
```
RecoNIC Simple RDMA Read Test
=============================

=== RDMA Client Mode ===
Setting up RDMA environment...
Connecting to server 192.168.1.100:11111...
Connected to server
Received remote buffer offset: 0x7f8a2c000000
Creating RDMA read WQE...
  Local buffer: 0x7f8a2d000000
  Remote buffer: 0x7f8a2c000000
  Length: 1024 bytes
Executing RDMA read operation...
RDMA read operation completed
Verifying received data...
✓ Data verification PASSED - All 256 words correct

=== Performance Summary ===
Payload Size:    1024 bytes
Latency:         12.34 microseconds
Bandwidth:       82.94 MB/s
Bandwidth:       0.66 Gb/s
QP Location:     host_mem
==========================

Client operation completed successfully
```

## 高级配置

### 1. 不同负载大小测试

```bash
# 小负载（64字节）
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -z 64 -s

# 中等负载（4KB）
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -z 4096 -s

# 大负载（64KB）
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -z 65536 -s
```

### 2. 设备内存 vs 主机内存

```bash
# 使用主机内存（默认）
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -l host_mem -s

# 使用设备内存
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -l dev_mem -s
```

### 3. 性能调优

```bash
# 使用CPU亲和性（如果适用）
taskset -c 0-3 sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s

# 大负载高性能测试
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -z 1048576 -l dev_mem -s
```

## 故障排除

### 1. 常见错误

**错误**: `Error: Could not resolve MAC address`
```bash
# 解决方案：建立ARP缓存
ping -c 1 <remote_ip>
arping -c 1 <remote_ip>
```

**错误**: `Error: Failed to create RecoNIC device`
```bash
# 检查PCIe资源
ls -la /sys/bus/pci/devices/*/resource*
lspci | grep Xilinx
```

**错误**: `Permission denied`
```bash
# 使用sudo运行
sudo ./simple_read ...
```

### 2. 调试技巧

```bash
# 启用详细日志
sudo ./simple_read -r 192.168.1.100 -i 192.168.1.101 -s -g

# 检查系统日志
dmesg | tail -20

# 监控网络流量
tcpdump -i eth0 host 192.168.1.101
```

### 3. 性能问题

如果性能不理想：

1. **检查网络配置**
   ```bash
   ethtool eth0
   iperf3 -s  # 服务器
   iperf3 -c <server_ip>  # 客户端
   ```

2. **检查PCIe链路**
   ```bash
   lspci -vv -s 0005:01:00.0 | grep -E "(LnkSta|Width|Speed)"
   ```

3. **检查CPU使用率**
   ```bash
   top -p $(pgrep simple_read)
   ```

## 与原版本的对比

| 特性 | 原始read.c | simple_read.c |
|------|-----------|---------------|
| 代码行数 | ~641行 | ~700+行（含注释） |
| 配置管理 | 分散的全局变量 | 统一的配置结构 |
| 错误处理 | 基本 | 详细的错误分类和恢复 |
| 日志输出 | 混乱的stderr输出 | 分级的彩色输出 |
| 参数验证 | 有限 | 全面的参数检查 |
| 帮助文档 | 基本usage | 详细的帮助和示例 |
| 性能监控 | 基本带宽计算 | 详细的性能指标 |
| 数据验证 | 简单比较 | 完整的数据完整性检查 |
| ARM优化 | 无 | 针对ARM平台优化 |

## 扩展功能

### 1. 批量测试

可以轻松扩展为支持批量RDMA操作：

```c
// 在simple_read.c中添加批量支持
for (int i = 0; i < batch_size; i++) {
    create_a_wqe(...);
    rdma_post_send(...);
}
```

### 2. 多QP支持

支持多个队列对并行操作：

```c
// 分配多个QP
for (int qp = 0; qp < num_qps; qp++) {
    allocate_rdma_qp(rdma_dev, qp, ...);
}
```

### 3. 自动化测试

集成到CI/CD流水线中进行自动化测试。

## 总结

简化版的RDMA读取程序提供了：

1. ✅ **更好的用户体验**：直观的参数、详细的帮助、清晰的输出
2. ✅ **更强的可靠性**：完善的错误处理、数据验证、系统检查
3. ✅ **更好的性能监控**：详细的指标、调试信息、性能分析
4. ✅ **更强的可维护性**：模块化设计、清晰的代码结构、充分的注释
5. ✅ **更好的ARM支持**：针对NVIDIA Jetson等ARM平台优化

这个工具为RecoNIC RDMA功能提供了一个易于使用和理解的测试平台。