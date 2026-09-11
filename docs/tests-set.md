# 测试与当前缺口

## 当前状态

- [积分回归目录](../tests/corpora.json)：690 条已验证记录，完整列表见 [passed-tests.md](passed-tests.md)。其中 5 条只有导数采样验证。
- [当前新题](../tests/acceptance-questions.json)：48 题，涵盖积分、导数、化简、六向转换、极限、求和、矩阵与精确算术。
- 新题 192 次普通栈/64 KiB 保护栈运行未出现进程崩溃；完整数学验收尚未完成，不作为整批通过。
- 待处理：部分含参数积分、不完整的参数退化分支、主平方根化简、导数继承定义域、曲线精确像集及部分符号和/矩阵幂。
- 用户报告的 CG50 Address error 尚未经过本轮实机复验。

## 验收方法

积分检查导数恒等式、原始域、收敛和端点；转换检查代回、分支及完整像集。参数题覆盖退化与不适用规则的反例。精确证明、数值采样、未求出、发散、超时和崩溃分别记录。

主机小栈不是 SH4/MMU 模拟器；主机 RSS 不是计算器 RAM。实际容量以 SH4 链接为准，性能只报告同条件重复测量所得的具体样例结果。

## 固定报告

| 文件 | 内容 |
| --- | --- |
| [acceptance.json](bench/acceptance.json) | 当前新题首测、复测及数学验收状态 |
| [regression.json](bench/regression.json) | 当前保留的独立回归族、实际输出与核验依据 |
| [verification.json](bench/verification.json) | 当前构建、格式、结构及专项检查 |
| [resources.json](bench/resources.json) | SH4 代码区、静态 RAM 和单函数栈帧 |
| [performance.json](bench/performance.json) | 同条件主机样例的重复测量 |

报告文件名不带版本或轮次，后续 checkpoint 更新同名文件。测试 ID 标识独立题目；历史报告由 Git 保存。测试执行期间只写 `.build/` 或临时目录，提交前集中更新文档和上述报告。

## 执行

仅选择核心改动直接影响的必要测试，不运行无意义的 smoke test；通过后不重复扩测。运行器位于 `tests/run-*.py`，可用 `make test TEST=tests/所选运行器.py TEST_ARGS="所需参数"` 执行。计算器脚本位于 `tests/device/`。

题库保留独立输入、参考恒等式、拒绝规则及当前未解决题。重复通过快照、一次性生成器、旧 checkpoint 汇总器和候选源码注入已移除；化简探针复用当前源码的共同依赖提取逻辑。题库中的稳定 ID 不代表需要保留同一测试的多份历史版本。
