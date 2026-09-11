# CG50 Address 错误：根式分母积分

状态：checkpoint 21 源码已通过下述主机验证及交叉构建，尚未安装到 CG50。用户确认的错误是 **Address error**，不是 TLB error；根因尚未由实机异常现场确认。

## 原始输入和证据边界

```text
integrate(1/((x^2+1)*sqrt(x+1)),x,0,+infinity)
```

正确数值约为 `1.06023329227074371689880130468339198077`。

- 已安装 checkpoint 20 和修复前开发探针：直接积分 / 外套 simplify、普通栈 / 64 KiB 保护栈，8 次测试均超过 15 秒。
- 随后的主机 GDB 调试还观察到 PARI 扩展内存、内存不足和非法指针信息。主机使用部分旧 Giac 共享库，调试时设置 768 MiB 虚拟地址空间上限；这些信息不能用于断言 CG50 的具体异常原因或内存需求。
- 新增路径在四种主机组合下返回同一闭式表达式，无 PARI 警告，初次测得约 0.5–0.6 ms。这不是 SH4 速度测量，也不是 Address 错误的实机复验。

原始记录：[修复前](benchmarks/user-radical-tail-first-runs-2026a.json)、[当前主机复验](benchmarks/user-radical-tail-working21-2026a.json)。

## 通用换元与分支证明

处理族为

\[
\int\frac{c\,dx}{(Ax^2+Bx+C)\sqrt{kx+b}},\quad
A>0,\quad4AC-B^2>0,\quad k\ne0.
\]

当前匹配范围：有理数系数、有界表达式和多项式次数、实数弧度模式。有限定积分端点须为有理数且满足根式定义域；无穷端点方向须与 `k` 的符号一致。未证明适用条件时返回原有求解流程，不用这条公式给出答案。

令 `u=sqrt(kx+b)`，则 `u>=0`，`dx/sqrt(kx+b)=2 du/k`。二次式成为

\[
Q((u^2-b)/k)=\frac{A}{k^2}(u^4+p u^2+q),\quad
p=Bk/A-2b,\quad q=b^2-Bbk/A+Ck^2/A.
\]

原二次式正定保证 `q>0` 和 `p²<4q`。置

\[
v=\sqrt q,\quad w=\sqrt{2v-p},\quad h=\sqrt{2v+p}.
\]

三者严格为正，`u²±wu+v` 也严格为正。以下原函数在整个实数 `u` 轴连续：

\[
F(u)=\frac{\ln\frac{u^2+wu+v}{u^2-wu+v}}{4vw}
+\frac{\arctan\frac{2u+w}{h}+\arctan\frac{2u-w}{h}}{2vh}.
\]

直接求导得到 `F'(u)=1/(u⁴+p u²+q)`。保留两个反正切之和，避免合并公式引入分支跳跃。原积分的原函数为 `(2kc/A)F(sqrt(kx+b))`。端点取值直接使用 `F(0)=0`、`F(+infinity)=pi/(2vh)`，无需通用无穷端点搜索。

根式零点附近的原 integrand 为常数乘距离的 `-1/2` 次方，可积；无穷端是 `O(|x|^(-5/2))`，绝对可积。负斜率和反向区间保留换元方向。二次式有实零点、重根、根式越出实数域的输入不能使用此证明。

## 已完成的验证

- [根式积分检查](../tests/run-quadratic-affine-root-guards.py)：112/112，包括族公式的独立有理恒等式证明、80 次完整积分/化简运行、32 次实际生产辅助函数的接受/拒绝检查。有限区间和无穷区间另以 55 位精度独立数值积分复核。覆盖正负斜率、平移、有理权重、根式端点、反向区间及极点等反例。
- [同轮 csc/sec 平方分母检查](benchmarks/csc-moment-guards-polar21-2026a.json)：112/112。
- [既有积分与化简回归汇总](benchmarks/polar21-integral-regression-summary-2026a.json)：21 组任务全部成功。checkpoint 21 新题组、旧方程转换和补充回归另有各自报告。
- 积分入口 `_integrate_` 单独使用体积优化后，SH4 静态栈帧从 484 B 降至 404 B；这是单函数静态帧，不是整条调用链峰值。15 个入口、各 9 对交错主机测试的耗时中位数比为 0.978–1.038，没有据此宣称总体提速。见[记录](benchmarks/dispatcher-size-performance-polar21-2026a.json)。

## 未完成事项

开发中曾出现 AC2 超出 3,088 B；经调度体积优化与共享分式处理后，最终交叉链接通过：ROM 使用 2,065,004 / 2,065,152 B，AC2 使用 2,558,928 / 2,559,996 B，余量分别 148 B、1,068 B。静态 RAM 为 424,620 B，配置堆 1,572,864 B。没有扩大固定区域或 CAS 堆。

checkpoint 21 的本轮新题与转换验收已完成，并形成安装包；CG50 的 Address 错误仍需要该版本的实机复验。不能将本次主机通过表述为所有非法访问、对齐或栈越界风险已经解决。
