# KhiCAS CG50

Casio fx-CG50 的符号计算程序，基于官方源码；当前版本 **2026a / 1.8.0**。

- 符号与数值积分、导数、化简、方程求解、矩阵和精确算术。
- 普通方程、参数方程、极坐标方程的六向转换；极坐标优先显示 `r=…`，参数结果显示 `x(t)=…`、`y(t)=…`。
- 有界积分规则、表达式展开限制、实数定义域与复数分支处理。
- `Li2`、`EllipticF(phi,m)` 等特殊函数；椭圆积分第二参数采用 Legendre 参数 `m`。

仍有未求出积分、参数退化和曲线完整像集缺口，见[当前测试状态](docs/tests-set.md)。主机通过不等于 CG50 实机通过。

## 使用

```xcas
integrate(x/sin(x)^2,x,pi/4,pi/2)
cart2param(x^2+y^2=4,[x,y],t)
cart2polar(x^2+y^2=4,[x,y],[r,theta])
param2cart([cos(t),sin(t)],t,[x,y])
polar2param(r=1+cos(theta),[r,theta],t)
```

## 构建与检查

需要 Python 3.12+、make、SH4 工具链及 `mkg3a`。主机数学测试另需 C++ 编译器、Giac、GMP、MPFR 和 SymPy。

```bash
TOOLS_DIR=/path/to/khicas-toolchain python3 tools/build.py optimized
python3 tools/format-code.py --check
python3 tools/check-signatures.py HEAD
```

构建产物为 `.build/optimized/khicas50.g3a` 和 `khicas50.ac2`，须成对使用。已签名源码可用 `python3 tools/package.py` 打包到 `dist/current/`。

## 维护要求

- 代码、测试和文档只维护当前有效内容，不为上一版本保留副本；能精简就精简，能删除就删除，保留核心功能、必要依赖和有效验证。历史由 Git 保存。
- 不做无意义的 smoke test；仅在核心改动关联重点行为时执行必要、最少的测试，通过后不重复扩测。
- 任何有歧义或模糊的要求必须先向用户确认，确认前不得执行相关操作。
- 文档及固定名称的 bench 报告仅在提交前更新；提交和 checkpoint 标签必须使用用户的 OpenPGP 签名。

[源码结构](docs/architecture.md) · [当前变更](docs/changelog.md) · [通过题目](docs/passed-tests.md) · [测试与缺口](docs/tests-set.md) · [上游来源](UPSTREAM.md)
