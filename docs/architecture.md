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

CG50 使用固定 ROM/AC2 代码区和 1,572,864 字节 CAS 堆。资源数据见 [resources.json](bench/resources.json)。主机探针链接实际积分、导数、化简、转换及部分辅助实现；其余依赖主机 Giac，不模拟 SH4/MMU。

手写 C/C++ 使用 `.clang-format`；生成表、解析器和纯许可证文件保留原格式。数值运算保持精度及分支语义，不启用 `fast-math`。
