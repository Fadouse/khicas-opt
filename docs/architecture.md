# 源码与构建

| 位置 | 职责 |
| --- | --- |
| `src/cas/` | Giac 计算内核、积分、导数、化简与方程转换 |
| `src/ui/` | 计算器界面、编辑器、菜单和参数方程显示 |
| `src/platform/` | CG50 平台配置、设备接口和控制台适配 |
| `src/runtime/` | 大数、正则表达式、Unicode 与脚本运行库 |
| `src/generated/` | 已生成的解析器、命令索引和帮助表 |
| `build/` | 原生 Makefile 和 SH4 链接脚本 |
| `assets/`、`vendor/` | 图标及构建所需的预编译库 |
| `tests/`、`tests/device/` | 主机测试、题库及计算器测试脚本 |
| `tools/` | 构建、路径解析、格式化、签名及打包工具 |

`tools/repository.py` 统一解析源码位置。构建在 `.build/` 内按上游 Makefile 所需布局准备输入，不在源码目录生成对象文件。重复文件名和断开的符号链接会阻止构建。

ESP32-S3 是独立 ESP-IDF 目标：源码在 `src/platform/esp32/`，配置在 `build/esp32/`，两者不进入 CG50 的平铺构建输入。`tools/vm/esp32.py` 管理固定 SDK、QEMU 补丁、固件镜像和 CLI/GDB 入口。

计算器 `ai(...)` → SH7305 USBHS → S3 DWC2 描述符 DMA/ESP-IDF USB Host → HTTPS Chat Completions → 原生文本查看器。两台 QEMU 通过 USB token socket 连接；代理不计算答案、不替代固件 USB 驱动。S3 运行 NVS、TLS、配置校验、API 请求和 lwIP NAPT。实机使用 AP+STA，QEMU 使用两个 OpenCores 网口，隔离网络中的客户端数据经过 S3 的 NAPT 后才进入上游 slirp。无线电层未模拟。

HTTPS 管理页限制在 LAN 地址，使用独立生成并保存在 NVS 的证书、设置密码及写入型 API Key 字段。上游 TLS 验证使用证书包或用户提供的 CA，不跟随 API 重定向。USB 请求/回复分别限制为 1,024/2,048 字节，回复只显示文本。

CG50 使用固定 ROM/AC2 代码区和 1,572,864 字节 CAS 堆。资源数据见 [resources.json](bench/resources.json)。主机探针链接实际积分、导数、化简、转换及部分辅助实现；其余依赖主机 Giac，不模拟 SH4/MMU。

手写 C/C++ 使用 `.clang-format`；生成表、解析器和纯许可证文件保留原格式。数值运算保持精度及分支语义，不启用 `fast-math`。
