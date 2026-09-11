# 上游来源

源码基线为作者发布的 [giacbf.tgz](https://www-fourier.univ-grenoble-alpes.fr/~parisse/casio/giacbf.tgz)，归档标识及逐文件 SHA-256 保存在 [UPSTREAM.json](UPSTREAM.json)。官方基线引用为 `upstream/khicas-2026-07-31`；该归档没有独立发行标签。

当前生产源码位于 `src/`，保留原版权与许可证。`vendor/libmicropy.a` 为构建所需的上游静态库；`src/platform/iostream` 是指向 `iostream.new` 的相对链接。

`python3 tools/build.py official` 从固定 Git 引用提取并校验官方输入；`optimized` 构建当前源码。
