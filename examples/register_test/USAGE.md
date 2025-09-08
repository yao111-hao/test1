# RecoNIC Register Test - 使用说明

## 项目概述

这是为RecoNIC RDMA智能网卡开发的寄存器测试工具，允许用户通过PCIe BAR空间读写网卡中的任意寄存器。该工具基于RecoNIC的用户空间API库，特别是`control_api.c`中的寄存器控制函数。

## 主要功能

- **寄存器读取**: 读取PCIe BAR空间中任意地址的寄存器值
- **寄存器写入**: 向可写寄存器写入指定数值
- **预定义寄存器列表**: 显示常用寄存器地址及其功能描述
- **读写验证**: 写入操作后自动读回验证
- **调试支持**: 提供详细的调试和verbose输出模式

## 编译和安装

### 1. 前置条件

确保RecoNIC库已正确编译：
```bash
cd ../../lib
make
```

### 2. 编译寄存器测试工具

```bash
cd /workspace/examples/register_test
make
```

### 3. 清理编译文件

```bash
make clean          # 清理测试程序编译文件
make distclean       # 清理所有文件包括库文件
```

## 使用方法

### 基本命令格式

```bash
sudo ./register_test [选项]
```

**注意**: 由于需要访问PCIe设备资源，通常需要root权限。

### 主要选项

| 选项 | 说明 | 示例 |
|------|------|------|
| `-d, --device` | 字符设备名称 | `-d /dev/reconic-mm` |
| `-p, --pcie_resource` | PCIe资源文件路径 | `-p /sys/bus/pci/devices/0005\:01\:00.0/resource2` |
| `-a, --address` | 寄存器地址偏移 (十六进制) | `-a 0x102000` |
| `-v, --value` | 写入的数值 (十六进制) | `-v 0x12345678` |
| `-r, --read` | 执行读操作 | `-r` |
| `-w, --write` | 执行写操作 | `-w` |
| `-l, --list` | 列出预定义寄存器 | `-l` |
| `-V, --verbose` | 详细输出 | `-V` |
| `-g, --debug` | 调试模式 | `-g` |
| `-h, --help` | 显示帮助信息 | `-h` |

## 使用示例

### 1. 查看帮助信息

```bash
./register_test -h
```

### 2. 列出所有预定义寄存器

```bash
./register_test -l
```

输出示例：
```
=== RecoNIC Predefined Register Addresses ===

Statistics and Configuration Registers (SCR):
  RN_SCR_VERSION         : 0x00102000 (Version register - Read only)
  RN_SCR_TEMPLATE_REG    : 0x00102200 (Template register - Read/Write)

Compute Logic Registers (CLR):
  RN_CLR_CTL_CMD         : 0x00103000 (Control command register)
  RN_CLR_TEMPLATE        : 0x00103200 (Template register - Read/Write)
...
```

### 3. 读取版本寄存器

```bash
sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102000 -r
```

输出示例：
```
=== RecoNIC Register Test ===
Register Read Result:
  Address: 0x00102000
  Value  : 0x12345678 (305419896)

=== Test Complete ===
```

### 4. 写入并验证模板寄存器

```bash
sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102200 -v 0xDEADBEEF -w
```

输出示例：
```
=== RecoNIC Register Test ===
Register Write Result:
  Address    : 0x00102200
  Written    : 0xDEADBEEF (3735928559)
  Read back  : 0xDEADBEEF (3735928559)
  Status     : SUCCESS - Values match

=== Test Complete ===
```

### 5. 详细模式读取寄存器

```bash
sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102000 -r -V
```

### 6. 调试模式

```bash
sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102000 -r -g
```

## 自动化测试

### 使用测试脚本

项目提供了自动化测试脚本 `test_registers.sh`：

```bash
# 运行基本测试
sudo ./test_registers.sh

# 运行详细测试
sudo ./test_registers.sh -v

# 运行调试测试
sudo ./test_registers.sh -g

# 指定PCIe资源路径
sudo ./test_registers.sh -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -v
```

测试脚本会自动执行以下测试：
1. 列出预定义寄存器
2. 读取版本寄存器
3. 读写SCR模板寄存器
4. 读写CLR模板寄存器
5. 读取RDMA配置寄存器
6. 边界地址测试
7. 连续寄存器读取测试

## 寄存器地址空间

### 统计和配置寄存器 (SCR): 0x102000 - 0x102FFF

| 寄存器名称 | 地址 | 访问权限 | 功能描述 |
|------------|------|----------|----------|
| RN_SCR_VERSION | 0x102000 | 只读 | 版本寄存器 |
| RN_SCR_FATAL_ERR | 0x102004 | 只读 | 致命错误寄存器 |
| RN_SCR_TEMPLATE_REG | 0x102200 | 读写 | 模板寄存器 |

### 计算逻辑寄存器 (CLR): 0x103000 - 0x103FFF

| 寄存器名称 | 地址 | 访问权限 | 功能描述 |
|------------|------|----------|----------|
| RN_CLR_CTL_CMD | 0x103000 | 读写 | 控制命令寄存器 |
| RN_CLR_KER_STS | 0x103004 | 只读 | 内核状态寄存器 |
| RN_CLR_TEMPLATE | 0x103200 | 读写 | 模板寄存器 |

### RDMA全局控制状态寄存器 (GCSR): 0x60000 - 0x8FFFF

| 寄存器名称 | 地址 | 访问权限 | 功能描述 |
|------------|------|----------|----------|
| RN_RDMA_GCSR_XRNICCONF | 0x60000 | 读写 | XRNIC配置寄存器 |
| RN_RDMA_GCSR_MACXADDLSB | 0x60010 | 读写 | MAC地址低32位 |
| RN_RDMA_GCSR_IPV4XADD | 0x60070 | 读写 | IPv4地址 |

## 故障排除

### 常见问题及解决方案

1. **权限被拒绝**
   ```
   Error: Unable to open PCIe resource file
   ```
   **解决方案**: 使用 `sudo` 运行程序

2. **PCIe资源文件未找到**
   ```
   Error: PCIe resource file not found
   ```
   **解决方案**: 检查PCIe设备路径
   ```bash
   lspci | grep Xilinx
   ls -la /sys/bus/pci/devices/0000:d8:00.0/resource*
   ```

3. **内存映射失败**
   ```
   Error: Failed to mmap PCIe resource
   ```
   **解决方案**: 确保FPGA正确配置且可访问

4. **写入后数值不匹配**
   ```
   Status: WARNING - Values don't match
   ```
   **解决方案**: 这对于只读寄存器是正常的

### 系统配置检查

```bash
# 查找Xilinx设备
lspci | grep Xilinx

# 查看详细设备信息
sudo lspci -vv -s d8:00.0

# 检查NUMA节点 (性能优化)
sudo lspci -vv -s d8:00.0 | grep 'NUMA node'

# 检查权限
ls -la /sys/bus/pci/devices/0000:d8:00.0/resource*
ls -la /dev/reconic-mm
```

## 性能优化

### CPU亲和性设置

对于大量寄存器操作，建议将进程绑定到与PCIe设备相同NUMA节点的CPU：

```bash
# 查找NUMA节点
sudo lspci -vv -s d8:00.0 | grep 'NUMA node'

# 查看NUMA节点的CPU列表
cat /sys/devices/system/node/node1/cpulist

# 使用CPU亲和性运行
taskset -c 1,3,5,7 sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102000 -r
```

## 安全注意事项

⚠️ **重要安全信息**:

1. **只读寄存器**: 某些寄存器是只读的，写入可能无效或导致意外行为
2. **系统寄存器**: 谨慎写入系统配置寄存器，错误的值可能影响系统稳定性
3. **并发访问**: 避免多个进程同时访问相同寄存器
4. **硬件状态**: 确保FPGA处于正确状态后再访问寄存器

## 与其他测试的集成

该寄存器测试工具可以与其他RecoNIC测试结合使用：

```bash
# 1. 首先运行DMA测试
cd ../dma_test
make && sudo ./dma_test -d /dev/reconic-mm

# 2. 然后运行寄存器测试
cd ../register_test
make && sudo ./register_test -p /sys/bus/pci/devices/0005\:01\:00.0/resource2 -a 0x102000 -r

# 3. 最后运行RDMA测试
cd ../rdma_test
make && sudo ./read -r 192.168.1.1 -i 192.168.1.2 -p /sys/bus/pci/devices/0005\:01\:00.0/resource2
```

## 技术支持

如遇到问题，请：

1. 检查主要RecoNIC文档
2. 验证PCIe设备设置
3. 确保驱动程序正确安装
4. 查看系统日志以获取错误消息

## 扩展功能

该工具可以进一步扩展以支持：

- 批量寄存器操作
- 寄存器监控和轮询
- 配置文件驱动的测试
- 与硬件仿真器集成
- 自动化回归测试
