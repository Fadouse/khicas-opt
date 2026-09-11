# 已通过的积分题目列表

验证源码：`checkpoint/polar-stability-cycle12-2026a`。
正式题库与用户题目共 **669 条通过记录**：**664 条精确验证、5 条导数采样通过、0 条数值常量检查通过**。
保留各题库的编号和重复项；仅去除输入空白后有 646 种输入文本，这不是数学意义的去重。

“核对参考结果”来自已验证题库，用于阅读和比对，并不保证与计算器实际打印的写法相同。
不定积分省略积分常数；实根、对数和反三角函数须遵守原题定义域。采样检查不能代替完整符号证明。
主机使用仓库积分、归一化和 FXCG 化简入口；这些通过记录不代表 CG50 实机耗时或实机全部通过。

本表列出完整的正式积分题库；其他算法参数变体、拒绝非法输入、方程转换和崩溃保护回归另见 [当前验收报告](USER-ACCEPTANCE-MATRIX.md)。
安全保留未求出的积分不计入本表。后续新题只有完成验证后才应追加。

## 极坐标优化第十二轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle12-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle12-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC12-I1` | `simplify(integrate(x*atan(x)/(1+x^2)^2,x))` | `((x^2-1)*atan(x)+x)/(4*(1+x^2))` | 精确验证 |
| `PC12-I2` | `simplify(integrate(ln(x)/(sqrt(x)*(1+sqrt(x))^2),x))` | `2*sqrt(x)*ln(x)/(1+sqrt(x))-4*ln(1+sqrt(x))` | 精确验证 |

## 极坐标优化第十一轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle11-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle11-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC11-I1` | `simplify(integrate(exp(2*x)/(exp(2*x)+exp(x)+1),x))` | `ln(exp(2*x)+exp(x)+1)/2-atan((2*exp(x)+1)/sqrt(3))/sqrt(3)` | 精确验证 |
| `PC11-I2` | `simplify(integrate(sqrt(x^2-1)/x,x))` | `sqrt(x^2-1)-atan(sqrt(x^2-1))` | 精确验证 |

## 极坐标优化第十轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle10-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle10-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC10-I1` | `simplify(integrate(sin(x)^3/(2+cos(x)),x))` | `cos(x)^2/2-2*cos(x)+3*ln(2+cos(x))` | 精确验证 |
| `PC10-I2` | `simplify(integrate(ln(x)/(x*(1+sqrt(ln(x)))),x))` | `2*sqrt(ln(x))^3/3-ln(x)+2*sqrt(ln(x))-2*ln(1+sqrt(ln(x)))` | 精确验证 |

## 极坐标优化第九轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle9-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle9-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC9-I1` | `simplify(integrate(ln(1+surd(x,3)^2),x))` | `x*ln(1+surd(x,3)^2)-2*x/3+2*surd(x,3)-2*atan(surd(x,3))` | 精确验证 |
| `PC9-I2` | `simplify(integrate(exp(x)*ln(1+exp(x))/(1+exp(x))^2,x))` | `-(ln(1+exp(x))+1)/(1+exp(x))` | 精确验证 |

## 极坐标优化第八轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle8-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle8-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC8-I1` | `simplify(integrate(2*x*ln(1+x^2)/((1+x^2)*(1+ln(1+x^2))),x))` | `ln(1+x^2)-ln(1+ln(1+x^2))` | 精确验证 |
| `PC8-I2` | `simplify(integrate(1/(x*sqrt((x-1)/(x+1))),x))` | `ln(abs((1+sqrt((x-1)/(x+1)))/(1-sqrt((x-1)/(x+1)))))+2*atan(sqrt((x-1)/(x+1)))` | 精确验证 |

## 极坐标优化第七轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle7-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle7-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC7-I1` | `simplify(integrate((1-x^2)*ln((sqrt(x^4+3*x^2+1)+x)/(1+x^2))/((1+x^2)*sqrt(x^4+3*x^2+1)),x))` | `ln((sqrt(x^4+3*x^2+1)+x)/(1+x^2))^2/2` | 精确验证 |
| `PC7-I2` | `simplify(integrate(sqrt(x/(1-x))/(1+x),x))` | `2*atan(sqrt(x/(1-x)))-sqrt(2)*atan(sqrt(2)*sqrt(x/(1-x)))` | 精确验证 |

## 极坐标优化第六轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle6-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle6-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC6-I1` | `simplify(integrate(surd(x-2,3)^2/(1+surd(x-2,3)^2)^2,x))` | `3*surd(x-2,3)-9*atan(surd(x-2,3))/2+3*surd(x-2,3)/(2*(1+surd(x-2,3)^2))` | 精确验证 |
| `PC6-I2` | `simplify(integrate(1/(sqrt(x^2+4)*(1+ln((x+sqrt(x^2+4))/2)^2)),x))` | `atan(ln((x+sqrt(x^2+4))/2))` | 精确验证 |

## 极坐标优化第五轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle5-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle5-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC5-I1` | `simplify(integrate(surd(x,3)/(1+x),x))` | `3*surd(x,3)-ln(abs(1+surd(x,3)))+ln(surd(x,3)^2-surd(x,3)+1)/2-sqrt(3)*atan((2*surd(x,3)-1)/sqrt(3))` | 精确验证 |
| `PC5-I2` | `simplify(integrate(ln(abs(x))/(x*(1+ln(abs(x))^4)),x))` | `atan(ln(abs(x))^2)/2` | 精确验证 |

## 极坐标优化第四轮新增积分（2 条）

[原题与定义域](../tests/polar-cycle4-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle4-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC4-I1` | `simplify(integrate(1/(surd(x,3)*(1+surd(x,3))),x))` | `3*surd(x,3)-3*ln(abs(1+surd(x,3)))` | 精确验证 |
| `PC4-I2` | `simplify(integrate(1/(1+surd(x-2,5)^2),x))` | `5*surd(x-2,5)^3/3-5*surd(x-2,5)+5*atan(surd(x-2,5))` | 精确验证 |

## 极坐标优化第三轮新增积分（3 条）

[原题与定义域](../tests/polar-cycle3-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle3-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC3-I1` | `simplify(integrate(1/(1+surd(x,3)),x))` | `3*surd(x,3)^2/2-3*surd(x,3)+3*ln(abs(1+surd(x,3)))` | 精确验证 |
| `PC3-I2` | `simplify(integrate(1/(1+exp(i*x)),x))` | `x/2+i*ln(abs(cos(x/2)))` | 精确验证 |
| `PC3-I3` | `simplify(integrate(x^2/(1+x^2)^2,x))` | `atan(x)/2-x/(2*(1+x^2))` | 精确验证 |

## 极坐标优化第二轮新增积分（3 条）

[原题与定义域](../tests/polar-cycle2-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle2-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC2-I1` | `simplify(integrate(sqrt((1-x)/(1+x))/(1-x^2),x))` | `-sqrt((1-x)/(1+x))` | 精确验证 |
| `PC2-I2` | `simplify(integrate((2*x+2*i)/(x^2+2*i*x-2),x))` | `ln(x-1+i)+ln(x+1+i)` | 精确验证 |
| `PC2-I3` | `simplify(integrate(x*exp(x)*cos(x),x))` | `exp(x)*(x*cos(x)+(x-1)*sin(x))/2` | 精确验证 |

## 极坐标优化首轮新增积分（4 条）

[原题与定义域](../tests/polar-cycle1-integrals.json) · [实际输出和验证记录](benchmarks/polar-cycle1-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `PC1-I1` | `simplify(integrate((3*x^2-1)*ln(abs(2*sin((x^3-x)/2))),x))` | `-im(Li2(exp(i*(x^3-x))))` | 精确验证 |
| `PC1-I2` | `simplify(integrate(ln(1-2*exp(-3*i*x/2)),x))` | `-2*i*Li2(2*exp(-3*i*x/2))/3` | 精确验证 |
| `PC1-I3` | `simplify(integrate(sin(x)*cos(x)/(sin(x)^4+3*sin(x)^2*cos(x)^2+2*cos(x)^4),x))` | `-ln(1+cos(x)^2)/2` | 精确验证 |
| `PC1-I4` | `simplify(integrate(x/sqrt((x^2-4)^2),x))` | `sign(x^2-4)*ln(abs(x^2-4))/2` | 精确验证 |

## 用户原始 8 题（8 条）

[原题与定义域](../tests/user-integrals.json) · [实际输出和验证记录](benchmarks/user-eight-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `1` | `integrate(x^5*(1-x^3)^50, x, 0, 1)` | `1/7956` | 精确验证 |
| `2` | `integrate((x^2-1)/((x^2+1)*sqrt(x^4+1)), x)` | `-asin(sqrt(2)*x/(x^2+1))/sqrt(2)` | 精确验证 |
| `3` | `integrate(x*sin(x)/(1+cos(x)^2), x, 0, pi)` | `pi^2/4` | 精确验证 |
| `4` | `simplify(integrate(1/(sin(x)^4+cos(x)^4), x))` | `atan(tan(2*x)/sqrt(2))/sqrt(2)` | 精确验证 |
| `5` | `simplify(integrate((1+x^(1/4))^(1/3)/sqrt(x), x))` | `12/7*(1+x^(1/4))^(7/3)-3*(1+x^(1/4))^(4/3)` | 精确验证 |
| `6` | `integrate(ln(1+x)/(1+x^2), x, 0, 1)` | `pi*ln(2)/8` | 精确验证 |
| `7` | `integrate(sin(x)^3/x, x, 0, +infinity)` | `pi/4` | 精确验证 |
| `8` | `integrate(x^2/(exp(x)+exp(-x)+2), x, -infinity, +infinity)` | `pi^2/3` | 精确验证 |

## 用户追加 2 题（2 条）

[原题与定义域](../tests/user-extra-integrals.json) · [实际输出和验证记录](benchmarks/user-extra-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `USER-EXTRA-01` | `integrate(atan(sqrt(x^2+2))/((x^2+1)*sqrt(x^2+2)),x,0,1)` | `5*pi^2/96` | 精确验证 |
| `USER-EXTRA-02` | `integrate(x*ln(sin(x)),x,0,pi)` | `-pi^2*ln(2)/2` | 精确验证 |

## 用户追加 5 题（5 条）

[原题与定义域](../tests/user-challenge-integrals.json) · [实际输出和验证记录](benchmarks/user-challenge-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `USER-CHALLENGE-01` | `integrate(atan(sqrt(x^2+2))/((x^2+1)*sqrt(x^2+2)),x,0,1)` | `5*pi^2/96` | 精确验证 |
| `USER-CHALLENGE-02` | `integrate(1/(5-3*cos(x)),x,0,2*pi)` | `pi/2` | 精确验证 |
| `USER-CHALLENGE-03` | `integrate(acos(cos(x)/(1+2*cos(x))),x,0,pi/2)` | `5*pi^2/24` | 精确验证 |
| `USER-CHALLENGE-04` | `integrate((atan(pi*x)-atan(x))/x,x,0,+infinity)` | `pi*ln(pi)/2` | 精确验证 |
| `USER-CHALLENGE-05` | `integrate(ln(ln(1/x))/(1+x^2),x,0,1)` | `pi*ln(2*pi)/4+pi*ln(Gamma(3/4)/Gamma(1/4))/2` | 精确验证 |

## MIT / Princeton 题库（168 条）

[原题与定义域](../tests/calculus-corpus.json) · [实际输出和验证记录](benchmarks/calculus-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `2025-Q2` | `integrate(exp(x+1)/(exp(x)+1),x)` | `exp(1)*ln(exp(x)+1)` | 精确验证 |
| `2025-Q3` | `integrate(surd(3*sin(x)-sin(3*x),3),x)` | `-surd(4,3)*cos(x)` | 导数采样通过 |
| `2025-Q5` | `integrate(cos(20*x)*sin(25*x),x,-pi/2,pi/2)` | `0` | 精确验证 |
| `2025-Q7` | `integrate((x*ln(x)*cos(x)-sin(x))/(x*ln(x)^2),x)` | `sin(x)/ln(x)` | 精确验证 |
| `2025-Q9` | `integrate(x^2024*(1-x^2025)^2025,x,0,1)` | `1/(2025*2026)` | 精确验证 |
| `2025-Q11` | `integrate(ceil(floor(x)/2),x,0,20)` | `100` | 精确验证 |
| `2025-Q13` | `integrate(exp(2*x)*(x^2+x)/((x*exp(x))^4+1),x)` | `atan(x^2*exp(2*x))/2` | 精确验证 |
| `2025-Q14` | `integrate(sec(x)^4-tan(x)^4,x)` | `2*tan(x)-x` | 精确验证 |
| `2025-Q15` | `integrate(sqrt(x*(1-x)),x,0,1)` | `pi/8` | 精确验证 |
| `2025-Q16` | `integrate(sin(4*x)*cos(x)/(cos(2*x)*sin(x)),x)` | `2*x+sin(2*x)` | 精确验证 |
| `2025-Q17` | `integrate(sin(x)*sinh(x),x)` | `(sin(x)*cosh(x)-cos(x)*sinh(x))/2` | 精确验证 |
| `2025-Q19` | `integrate((cos(x)+cos(x+2*pi/3)+cos(x-2*pi/3))^2,x)` | `0` | 精确验证 |
| `2026-Q1` | `integrate(sin(x)^2025*cos(x)^2026,x,-pi,pi)` | `0` | 精确验证 |
| `2026-Q2` | `integrate(exp(2026*exp(x)+x),x)` | `exp(2026*exp(x))/2026` | 精确验证 |
| `2026-Q3` | `integrate(floor(x)/3-floor(floor(x)/3),x,0,2026)` | `675` | 精确验证 |
| `2026-Q5` | `integrate(1/(sqrt(x+1)-sqrt(x-1)),x)` | `((x+1)^(3/2)+(x-1)^(3/2))/3` | 精确验证 |
| `2026-Q6` | `integrate(sqrt(1+cosh(x)),x)` | `2*sqrt(2)*sinh(x/2)` | 精确验证 |
| `2026-Q7` | `integrate(2^(ln(x))/x^2,x)` | `x^(ln(2)-1)/(ln(2)-1)` | 导数采样通过 |
| `2026-Q9` | `integrate(x^2*sin(x),x)` | `2*x*sin(x)-(x^2-2)*cos(x)` | 精确验证 |
| `2026-Q10` | `integrate((x-1)^2/(2*exp(x)+x^2+1),x)` | `x-ln(2*exp(x)+x^2+1)` | 精确验证 |
| `2026-Q13` | `integrate(cos(x)^5-10*cos(x)^3*sin(x)^2+5*cos(x)*sin(x)^4,x)` | `sin(5*x)/5` | 精确验证 |
| `2026-Q14` | `integrate(atan(sqrt(x)),x)` | `(x+1)*atan(sqrt(x))-sqrt(x)` | 精确验证 |
| `2026-Q17` | `integrate(exp(-x^2)/(1+exp(2*x)),x,-infinity,+infinity)` | `sqrt(pi)/2` | 精确验证 |
| `2026-Q18` | `integrate(sin(x)^2/x^2-sin(2*x)/x,x)` | `-sin(x)^2/x` | 精确验证 |
| `2026-Q19` | `integrate(ln(ln(x))*ln(ln(ln(x)))/(x*ln(x)),x)` | `ln(ln(x))^2*(2*ln(ln(ln(x)))-1)/4` | 精确验证 |
| `2026-F5` | `integrate(1/((1/(x-2)+3/(x-4)+5/(x-6))^(-2)+1),x,-infinity,+infinity)` | `9*pi` | 精确验证 |
| `Princeton-I1` | `integrate(sin(x)^5/cos(x),x)` | `cos(x)^2-cos(x)^4/4+ln(abs(sec(x)))` | 精确验证 |
| `Princeton-I2` | `integrate(1/(4+x^2)^(5/2),x)` | `x/(16*sqrt(4+x^2))-x^3/(48*(4+x^2)^(3/2))` | 精确验证 |
| `Princeton-I3` | `integrate(sin(sqrt(1+x)),x)` | `-2*sqrt(1+x)*cos(sqrt(1+x))+2*sin(sqrt(1+x))` | 精确验证 |
| `Princeton-I4` | `integrate(atan(x),x)` | `x*atan(x)-ln(1+x^2)/2` | 精确验证 |
| `Princeton-I5` | `integrate(cos(x)^4,x)` | `3*x/8+sin(2*x)/4+sin(4*x)/32` | 精确验证 |
| `Princeton-I6` | `integrate(cos(x)/(4-sin(x)^2),x,0,pi/2)` | `ln(3)/4` | 精确验证 |
| `Princeton-I7` | `integrate(ln(1+ln(x))/x,x)` | `(1+ln(x))*ln(1+ln(x))-ln(x)` | 精确验证 |
| `Princeton-I8` | `integrate(x^2*atan(x),x)` | `x^3*atan(x)/3-x^2/6+ln(1+x^2)/6` | 精确验证 |
| `Princeton-I9` | `integrate(1/(4+2*x+x^2)^(5/2),x,-1,2)` | `1/(8*sqrt(3))` | 精确验证 |
| `Princeton-I10` | `integrate(x*sin(x^2)*exp(x^2),x)` | `exp(x^2)*(sin(x^2)-cos(x^2))/4` | 精确验证 |
| `Princeton-I11` | `integrate(1/sqrt(x^2+25),x)` | `ln(x+sqrt(x^2+25))` | 精确验证 |
| `Princeton-I12` | `integrate((2+x)/(surd(x+2,3)+x),x)` | `surd(x+2,3)^3-3*surd(x+2,3)+3/4*ln(abs(surd(x+2,3)-1))+21/8*ln(surd(x+2,3)^2+surd(x+2,3)+2)+39/(4*sqrt(7))*atan((2*surd(x+2,3)+1)/sqrt(7))` | 精确验证 |
| `Princeton-I13` | `integrate(3*x^2/(x^2+x-2),x)` | `3*x-4*ln(abs(x+2))+ln(abs(x-1))` | 精确验证 |
| `Princeton-I14` | `integrate(cos(surd(x,3))/surd(x,3),x)` | `3*surd(x,3)*sin(surd(x,3))+3*cos(surd(x,3))` | 精确验证 |
| `Princeton-I15` | `integrate(1/sqrt(x^2+2*x),x)` | `ln(x+1+sqrt(x^2+2*x))` | 精确验证 |
| `Princeton-I16` | `integrate((x^2+3*x-3)/((x+1)*(x^2+6*x+10)),x)` | `ln(x^2+6*x+10)+atan(x+3)-ln(abs(x+1))` | 精确验证 |
| `Princeton-I17` | `integrate(1/(x*sqrt(1-x^2)),x)` | `-ln(abs((1+sqrt(1-x^2))/x))` | 精确验证 |
| `Princeton-I18` | `integrate(x^3*exp(x^2),x)` | `(x^2-1)*exp(x^2)/2` | 精确验证 |
| `Princeton-I19` | `integrate(x^2*ln(x),x)` | `x^3*ln(x)/3-x^3/9` | 精确验证 |
| `Princeton-I20` | `integrate(x^3/sqrt(1-x^2),x)` | `(1-x^2)^(3/2)/3-sqrt(1-x^2)` | 精确验证 |
| `Princeton-I21` | `integrate(tan(x)^4,x)` | `tan(x)^3/3-tan(x)+x` | 精确验证 |
| `Princeton-I22` | `integrate((x+1)/(x^2+4*x+13),x)` | `ln(x^2+4*x+13)/2-atan((x+2)/3)/3` | 精确验证 |
| `Princeton-I23` | `integrate(cos(x)/(sin(x)^2+5*sin(x)+6),x,0,pi/2)` | `ln(9/8)` | 精确验证 |
| `Princeton-I24` | `integrate(exp(x/2)/(1+exp(x)),x)` | `2*atan(exp(x/2))` | 精确验证 |
| `Princeton-I25` | `integrate((2*x^2+5*x+10)/(x^3+2*x^2+10*x),x)` | `ln(abs(x))+ln(x^2+2*x+10)/2+2*atan((x+1)/3)/3` | 精确验证 |
| `Princeton-I26` | `integrate((x-2)*sqrt(9-x^2),x)` | `-(9-x^2)^(3/2)/3-9*asin(x/3)-x*sqrt(9-x^2)` | 精确验证 |
| `Princeton-I27` | `integrate(asin(ln(x))/x,x,1,sqrt(exp(1)))` | `pi/12+sqrt(3)/2-1` | 精确验证 |
| `Princeton-I28` | `integrate(x*exp(-x),x,0,1)` | `1-2/exp(1)` | 精确验证 |
| `Princeton-I29` | `integrate(ln(x)^2,x)` | `x*ln(x)^2-2*x*ln(x)+2*x` | 精确验证 |
| `Princeton-I30` | `integrate(sin(x)/sqrt(1+cos(x)),x)` | `-2*sqrt(1+cos(x))` | 精确验证 |
| `Princeton-I31` | `integrate(x^2/(x^6-1),x)` | `ln(abs((x^3-1)/(x^3+1)))/6` | 精确验证 |
| `Princeton-I32` | `integrate(sin(x)^5*cos(x)^2,x)` | `2*cos(x)^5/5-cos(x)^7/7-cos(x)^3/3` | 精确验证 |
| `Princeton-I33` | `integrate((1+exp(x))/(1-exp(x)),x)` | `x-2*ln(abs(1-exp(x)))` | 精确验证 |
| `Princeton-I34` | `integrate(sin(ln(x)),x,1,exp(1))` | `exp(1)*(sin(1)-cos(1))/2+1/2` | 精确验证 |
| `Princeton-I35` | `integrate(exp(sqrt(x)),x)` | `2*(sqrt(x)-1)*exp(sqrt(x))` | 精确验证 |
| `Princeton-I36` | `integrate(1/(4-x^2)^(3/2),x)` | `x/(4*sqrt(4-x^2))` | 精确验证 |
| `Princeton-I37` | `integrate(exp(1/x)/x^2,x,2,3)` | `sqrt(exp(1))-exp(1/3)` | 精确验证 |
| `Princeton-I38` | `integrate(sqrt(x^2-4)/x^3,x)` | `acos(2/x)/4-sqrt(x^2-4)/(2*x^2)` | 导数采样通过 |
| `Princeton-I39` | `integrate((x-1)/(x^3+x),x)` | `-ln(abs(x))+ln(x^2+1)/2+atan(x)` | 精确验证 |
| `Princeton-I40` | `integrate(1/(x^2*sqrt(x^2+4)),x)` | `-sqrt(x^2+4)/(4*x)` | 精确验证 |
| `Princeton-I41` | `integrate(sin(sqrt(x)),x)` | `-2*sqrt(x)*cos(sqrt(x))+2*sin(sqrt(x))` | 精确验证 |
| `Princeton-I42` | `integrate(1/(x*(1-x)^2),x)` | `ln(abs(x))-ln(abs(x-1))-1/(x-1)` | 精确验证 |
| `Princeton-I43` | `integrate((x-5)*(sqrt(x-1)+3)/(sqrt(x-1)+2),x)` | `x^2/2-7*x+2*(x-1)^(3/2)/3` | 精确验证 |
| `Princeton-I44` | `integrate((2*x+3)*ln(x),x)` | `(x^2+3*x)*ln(x)-x^2/2-3*x` | 精确验证 |
| `Princeton-I45` | `integrate(sqrt(9+x^2)/x^2,x)` | `-sqrt(9+x^2)/x+ln(abs((sqrt(9+x^2)+x)/3))` | 精确验证 |
| `Princeton-I46` | `integrate(x/((x^2+1)*(x+1)),x)` | `ln(x^2+1)/4+atan(x)/2-ln(abs(x+1))/2` | 精确验证 |
| `Princeton-I47` | `integrate((exp(x)+1)^20*exp(x),x,0,1)` | `((1+exp(1))^21-2^21)/21` | 精确验证 |
| `Princeton-I48` | `integrate(x^2/(x^2+4*x+5),x)` | `x-2*ln(x^2+4*x+5)+3*atan(x+2)` | 精确验证 |
| `Princeton-I49` | `integrate((x+1)/(x^2+2*x+3),x)` | `ln(x^2+2*x+3)/2` | 精确验证 |
| `Princeton-Imp1` | `integrate(1/(x^3+2),x,0,+infinity)` | `2*pi/(3*sqrt(3)*2^(2/3))` | 精确验证 |
| `Princeton-Imp2` | `integrate(1/(x+sqrt(x)),x,0,1)` | `2*ln(2)` | 精确验证 |
| `Princeton-Imp4` | `integrate(x^2/(x^3+1),x,0,+infinity)` | `+infinity` | 精确验证 |
| `Princeton-Imp5` | `integrate(ln(x),x,0,1)` | `-1` | 精确验证 |
| `Princeton-Imp6` | `integrate(1/(exp(x)-1),x,0,1)` | `+infinity` | 精确验证 |
| `Princeton-Imp7` | `integrate(1/(x^2+2*x+2),x,0,+infinity)` | `pi/4` | 精确验证 |
| `Princeton-Imp10` | `integrate(1/(1-cos(x)),x,0,1)` | `+infinity` | 精确验证 |
| `Princeton-Imp11` | `integrate(exp(-x)*cos(x),x,0,+infinity)` | `1/2` | 精确验证 |
| `Princeton-Imp12` | `integrate(exp(-x^2)/x^2,x,0,+infinity)` | `+infinity` | 精确验证 |
| `Princeton-Imp15` | `integrate(exp(x)/x,x,0,1)` | `+infinity` | 精确验证 |
| `Princeton-Imp18` | `integrate((1-x)^(-2/3),x,0,1)` | `3` | 精确验证 |
| `Princeton-Imp20` | `integrate(tan(x),x,0,pi/2)` | `+infinity` | 精确验证 |
| `Princeton-Imp21` | `integrate((exp(x)-1)/(exp(2*x)+1),x,0,+infinity)` | `pi/4-ln(2)/2` | 精确验证 |
| `Princeton-Imp25` | `integrate(1/abs(x-1),x,0,2)` | `+infinity` | 精确验证 |
| `Princeton-Imp26` | `integrate(x^(-99/100),x,1,+infinity)` | `+infinity` | 精确验证 |
| `Princeton-Imp28` | `integrate(x^3*exp(-x),x,0,+infinity)` | `6` | 精确验证 |
| `Princeton-Imp30` | `integrate(1/(x^2*ln(x)),x,1,+infinity)` | `+infinity` | 精确验证 |
| `Princeton-Imp32` | `integrate(exp(x)*(1+exp(-2*x)),x,0,+infinity)` | `+infinity` | 精确验证 |
| `Princeton-Imp33` | `integrate(sqrt(x)*ln(x),x,0,1)` | `-4/9` | 精确验证 |
| `Princeton-Imp35` | `integrate((1+cos(x))/x,x,0,pi/2)` | `+infinity` | 精确验证 |
| `Princeton-Imp37` | `integrate(1/((1+x)*sqrt(x)),x,0,+infinity)` | `pi` | 精确验证 |
| `Princeton-ImpOther2` | `integrate(x*exp(-x),x,0,+infinity)` | `1` | 精确验证 |
| `Princeton-ImpOther4` | `integrate(1/(x^2+1),x,1,+infinity)` | `pi/4` | 精确验证 |
| `2010-Q5` | `integrate(sin(ln(x))^2,x,0,1)` | `2/5` | 精确验证 |
| `2010-Q8` | `integrate(1/(x*sqrt(x^4-1)),x,1,+infinity)` | `pi/4` | 精确验证 |
| `2010-Q11` | `integrate(ln(1+x)/(1+x^2),x,0,1)` | `pi*ln(2)/8` | 精确验证 |
| `2010-Q19` | `integrate((cos(x)+x*sin(x))/(x*(x+cos(x))),x)` | `ln(abs(x))-ln(abs(x+cos(x)))` | 精确验证 |
| `2010-Q24` | `integrate(1/ln(x)-1/ln(x)^2,x)` | `x/ln(x)` | 精确验证 |
| `2011-Q1` | `integrate((x^6-1)/(x^4+x^3-x-1),x)` | `x^3/3-x^2/2+x` | 精确验证 |
| `2011-Q7` | `integrate(sec(x)^4*tan(x)^2,x)` | `tan(x)^5/5+tan(x)^3/3` | 精确验证 |
| `2011-Q10` | `integrate(1/(1+2*x^2+x^4),x)` | `atan(x)/2+x/(2+2*x^2)` | 精确验证 |
| `2011-Q11` | `integrate(cos(ln(x)),x)` | `x*(cos(ln(x))+sin(ln(x)))/2` | 精确验证 |
| `2011-Q22` | `integrate(sin(101*x)*sin(x)^99,x)` | `sin(100*x)*sin(x)^100/100` | 导数采样通过 |
| `2012-Q2` | `integrate(x^(1/4)*ln(x),x)` | `4*x^(5/4)*ln(x)/5-16*x^(5/4)/25` | 精确验证 |
| `2012-Q8` | `integrate(1/(1+sin(x)),x,0,pi/2)` | `1` | 精确验证 |
| `2012-Q15` | `integrate(x*(1-x)^99,x,0,1)` | `1/10100` | 精确验证 |
| `2012-Q20` | `integrate(x/(x^4+4),x)` | `atan(x^2/2)/4` | 精确验证 |
| `2012-Q24` | `integrate((x^7-1)/ln(x),x,0,1)` | `ln(8)` | 精确验证 |
| `2013-Q2` | `integrate(exp(abs(x)),x,-1,3)` | `exp(3)+exp(1)-2` | 精确验证 |
| `2013-Q8` | `integrate((x^5-x^3+x^2-1)/(x^4-x^3+x-1),x)` | `x^2/2+x` | 精确验证 |
| `2013-Q12` | `integrate(pi*sin(pi*sqrt(x))/sqrt(x),x,0,441)` | `4` | 精确验证 |
| `2013-Q14` | `integrate((x-floor(x))^2,x,0,256)` | `256/3` | 精确验证 |
| `2013-Q23` | `integrate(sec(x)^5*tan(x)^3,x)` | `sec(x)^5*(5*sec(x)^2-7)/35` | 精确验证 |
| `2014-Q5` | `integrate(sqrt(x)*exp(sqrt(x)),x)` | `2*exp(sqrt(x))*(x-2*sqrt(x)+2)` | 精确验证 |
| `2014-Q7` | `integrate(abs(1+2*sin(x)),x,0,2*pi)` | `2*pi/3+4*sqrt(3)` | 精确验证 |
| `2014-Q8` | `integrate(x*(1-x)^2014,x)` | `(1-x)^2016/2016-(1-x)^2015/2015` | 精确验证 |
| `2014-Q13` | `integrate(exp(x)*(ln(1+x^2)-2*(1+x)*atan(x)),x)` | `exp(x)*(ln(1+x^2)-2*x*atan(x))` | 精确验证 |
| `2014-Q20` | `integrate(1/(2+cos(x)),x,0,5*pi/2)` | `7*sqrt(3)*pi/9` | 精确验证 |
| `2015-Q7` | `integrate(1/(5+4*sqrt(x)+x),x)` | `ln(x+4*sqrt(x)+5)-4*atan(sqrt(x)+2)` | 精确验证 |
| `2015-Q9` | `integrate(x/((x-3)*(x+5)^2),x,0,2)` | `1/28-3*ln(21/5)/64` | 精确验证 |
| `2015-Q14` | `integrate(exp(3*x)*atan(exp(x)),x)` | `exp(3*x)*atan(exp(x))/3-exp(2*x)/6+ln(1+exp(2*x))/6` | 精确验证 |
| `2015-Q15` | `integrate(abs(x-1)/(abs(x-2)+abs(x-3)),x,0,4)` | `2+9*ln(3)/4-3*ln(5)/4` | 精确验证 |
| `2015-Q16` | `integrate(1/(sin(x)^4+cos(x)^4),x,0,2*pi)` | `2*pi*sqrt(2)` | 精确验证 |
| `2016-Q2` | `integrate(abs(x^3-x),x,-4,4)` | `113` | 精确验证 |
| `2016-Q5` | `integrate(ln(ln(x))/(x*ln(x)),x)` | `ln(ln(x))^2/2` | 精确验证 |
| `2016-Q10` | `integrate(x^3*exp(-x^2),x,0,+infinity)` | `1/2` | 精确验证 |
| `2016-Q16` | `integrate(x/(x^4+x^2+1),x)` | `atan((2*x^2+1)/sqrt(3))/sqrt(3)` | 精确验证 |
| `2016-Q20` | `integrate(1/(2+cosh(x)),x,0,+infinity)` | `ln(2+sqrt(3))/sqrt(3)` | 精确验证 |
| `2017-Q2` | `integrate(ln(x)/x^2,x,1,+infinity)` | `1` | 精确验证 |
| `2017-Q3` | `integrate(1/cosh(x),x)` | `2*atan(exp(x))` | 精确验证 |
| `2017-Q5` | `integrate(1/(x*sqrt(x^2-1)),x,1,2)` | `pi/3` | 精确验证 |
| `2017-Q6` | `integrate(1/(x*(x^2+1)),x,1,+infinity)` | `ln(2)/2` | 精确验证 |
| `2017-Q8` | `integrate(exp(-2*x^2-5*x-3),x,-infinity,+infinity)` | `exp(1/8)*sqrt(pi/2)` | 精确验证 |
| `2018-Q3` | `integrate(abs(sin(2018*x)),x,0,2018*pi)` | `4036` | 精确验证 |
| `2018-Q5` | `integrate(x^5/(2+x^12),x)` | `atan(x^6/sqrt(2))/(6*sqrt(2))` | 精确验证 |
| `2018-Q8` | `integrate(sin(cos(sin(x)))*sin(sin(x))*cos(x),x)` | `cos(cos(sin(x)))` | 精确验证 |
| `2018-Q13` | `integrate((2017*x^2016+2018*x^2017)/(1+x^4034+2*x^4035+x^4036),x)` | `atan(x^2018+x^2017)` | 精确验证 |
| `2018-Q18` | `integrate(1/sqrt(x*sqrt(x)-x^2),x)` | `4*asin(x^(1/4))` | 精确验证 |
| `2019-Q1` | `integrate(tan(cos(x)),x,0,2*pi)` | `0` | 精确验证 |
| `2019-Q6` | `integrate((cos(3*x)+sin(2*x))*(-sin(2019*x)+cos(3*x)),x,-2*pi,2*pi)` | `2*pi` | 精确验证 |
| `2019-Q7` | `integrate(cos(x)*cos(sin(x))*cos(sin(sin(x))),x)` | `sin(sin(sin(x)))` | 精确验证 |
| `2019-Q8` | `integrate(exp(-2019/(4*x^2))/x^2,x,0,+infinity)` | `sqrt(pi/2019)` | 精确验证 |
| `2019-Q17` | `integrate(1/(x+surd(x,3)),x)` | `3*ln(1+surd(x,3)^2)/2` | 精确验证 |
| `2020-Q2` | `integrate(1/(exp(x)+1),x,0,+infinity)` | `ln(2)` | 精确验证 |
| `2020-Q9` | `integrate(cos(x)^2020,x,0,2*pi)` | `2^(-2019)*pi*comb(2020,1010)` | 精确验证 |
| `2020-Q15` | `integrate(1/(tan(x)^sqrt(2020)+1),x,0,pi/2)` | `pi/4` | 精确验证 |
| `2020-Q16` | `integrate(x*(1-x)^2020,x)` | `(1-x)^2022/2022-(1-x)^2021/2021` | 精确验证 |
| `2020-Q20` | `integrate(x^5*exp(-x^4),x,0,+infinity)` | `sqrt(pi)/8` | 精确验证 |
| `2022-Q5` | `integrate(x/(sqrt(x-1)+sqrt(x+1)),x)` | `(x+1)^(5/2)/5-(x+1)^(3/2)/3-(x-1)^(5/2)/5-(x-1)^(3/2)/3` | 精确验证 |
| `2022-Q7` | `integrate(x^3*sin(x^2),x)` | `(sin(x^2)-x^2*cos(x^2))/2` | 精确验证 |
| `2022-Q12` | `integrate(sqrt(1-sqrt(x)),x,0,1)` | `8/15` | 精确验证 |
| `2022-Q14` | `integrate(sin(x+sin(x))-sin(x-sin(x)),x)` | `-2*cos(sin(x))` | 精确验证 |
| `2022-Q17` | `integrate(1/(1+sin(x))+1/(1+cos(x))+1/(1+tan(x))+1/(1+cot(x))+1/(1+sec(x))+1/(1+csc(x)),x)` | `3*x` | 精确验证 |
| `2023-Q3` | `integrate(exp(x)/((1+exp(x))*ln(1+exp(x))),x)` | `ln(ln(1+exp(x)))` | 精确验证 |
| `2023-Q9` | `integrate((2*ln(x)+1)*exp(ln(x)^2),x)` | `x*exp(ln(x)^2)` | 精确验证 |
| `2023-Q15` | `integrate((1+2*x^2022)/(x+x^2023),x)` | `ln(abs(x^2022+x^4044))/2022` | 精确验证 |
| `2023-Q16` | `integrate(3*sin(20*x)*cos(23*x)+20*sin(43*x),x)` | `sin(20*x)*sin(23*x)` | 精确验证 |
| `2023-Q18` | `integrate(sin(x)/(2*exp(x)+cos(x)+sin(x)),x)` | `(x-ln(abs(2*exp(x)+cos(x)+sin(x))))/2` | 精确验证 |
| `2024-Q5` | `integrate(acos(sin(x)),x,0,2*pi)` | `pi^2` | 精确验证 |
| `2024-Q7` | `integrate((x^2024-1)/(x^506-1),x)` | `x+x^507/507+x^1013/1013+x^1519/1519` | 精确验证 |
| `2024-Q11` | `integrate(csc(x)^2*tan(x)^2024,x)` | `tan(x)^2023/2023` | 精确验证 |
| `2024-Q13` | `integrate(exp(-(x-2024)^2/4),x,-infinity,+infinity)` | `2*sqrt(pi)` | 精确验证 |
| `2024-Q19` | `integrate(x^4/(3-6*x+6*x^2-4*x^3+2*x^4),x)` | `x/2+ln(3-6*x+6*x^2-4*x^3+2*x^4)/4` | 精确验证 |

## 独立泛化题库（152 条）

[原题与定义域](../tests/generalization-corpus.json) · [实际输出和验证记录](benchmarks/generalization-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `GEN-binomial-chain-01` | `integrate(x^1*(2+(3)*x^2)^(7),x)` | `(1/48)*(2+(3)*x^2)^(8)` | 精确验证 |
| `GEN-binomial-chain-02` | `integrate(x^5*(1+(2)*x^3)^(9),x)` | `(-1/120)*(1+(2)*x^3)^(10)+(1/132)*(1+(2)*x^3)^(11)` | 精确验证 |
| `GEN-binomial-chain-03` | `integrate(x^5*(3+(1)*x^2)^(4),x)` | `(9/10)*(3+(1)*x^2)^(5)+(-1/2)*(3+(1)*x^2)^(6)+(1/14)*(3+(1)*x^2)^(7)` | 精确验证 |
| `GEN-binomial-chain-04` | `integrate(x^15*(2+(1)*x^4)^(3),x)` | `(-1/2)*(2+(1)*x^4)^(4)+(3/5)*(2+(1)*x^4)^(5)+(-1/4)*(2+(1)*x^4)^(6)+(1/28)*(2+(1)*x^4)^(7)` | 精确验证 |
| `GEN-binomial-chain-05` | `integrate(x^9*(1+(1)*x^2)^(2),x)` | `(1/6)*(1+(1)*x^2)^(3)+(-1/2)*(1+(1)*x^2)^(4)+(3/5)*(1+(1)*x^2)^(5)+(-1/3)*(1+(1)*x^2)^(6)+(1/14)*(1+(1)*x^2)^(7)` | 精确验证 |
| `GEN-binomial-chain-06` | `integrate(x^11*(2+(1)*x^2)^(3),x)` | `(-4)*(2+(1)*x^2)^(4)+(8)*(2+(1)*x^2)^(5)+(-20/3)*(2+(1)*x^2)^(6)+(20/7)*(2+(1)*x^2)^(7)+(-5/8)*(2+(1)*x^2)^(8)+(1/18)*(2+(1)*x^2)^(9)` | 精确验证 |
| `GEN-binomial-chain-07` | `integrate(x^13*(1+(2)*x^2)^(2),x)` | `(1/768)*(1+(2)*x^2)^(3)+(-3/512)*(1+(2)*x^2)^(4)+(3/256)*(1+(2)*x^2)^(5)+(-5/384)*(1+(2)*x^2)^(6)+(15/1792)*(1+(2)*x^2)^(7)+(-3/1024)*(1+(2)*x^2)^(8)+(1/2304)*(1+(2)*x^2)^(9)` | 精确验证 |
| `GEN-binomial-chain-08` | `integrate(x^15*(3+(1)*x^2)^(1),x)` | `(-2187/4)*(3+(1)*x^2)^(2)+(1701/2)*(3+(1)*x^2)^(3)+(-5103/8)*(3+(1)*x^2)^(4)+(567/2)*(3+(1)*x^2)^(5)+(-315/4)*(3+(1)*x^2)^(6)+(27/2)*(3+(1)*x^2)^(7)+(-21/16)*(3+(1)*x^2)^(8)+(1/18)*(3+(1)*x^2)^(9)` | 精确验证 |
| `GEN-binomial-chain-09` | `integrate(x^4*(3+(-1)*x^5)^(11),x)` | `(-1/60)*(3+(-1)*x^5)^(12)` | 精确验证 |
| `GEN-binomial-chain-10` | `integrate(x^8*(-2+(3)*x^3)^(5),x)` | `(2/243)*(-2+(3)*x^3)^(6)+(4/567)*(-2+(3)*x^3)^(7)+(1/648)*(-2+(3)*x^3)^(8)` | 精确验证 |
| `GEN-binomial-chain-11` | `integrate(x^3*(1+(3)*x^2)^(1/2),x)` | `(-1/27)*(1+(3)*x^2)^(3/2)+(1/45)*(1+(3)*x^2)^(5/2)` | 精确验证 |
| `GEN-binomial-chain-12` | `integrate(x^5*(2+(1)*x^3)^(2/3),x)` | `(-2/5)*(2+(1)*x^3)^(5/3)+(1/8)*(2+(1)*x^3)^(8/3)` | 精确验证 |
| `GEN-binomial-chain-13` | `integrate(x^3*(1+(2)*x^4)^(3/2),x)` | `(1/20)*(1+(2)*x^4)^(5/2)` | 精确验证 |
| `GEN-binomial-chain-14` | `integrate(x^1*(1+(1)*x^2)^(257),x)` | `(1/516)*(1+(1)*x^2)^(258)` | 精确验证 |
| `GEN-binomial-chain-15` | `integrate(x^5*(1+(-1)*x^3)^(4096),x)` | `(-1/12291)*(1+(-1)*x^3)^(4097)+(1/12294)*(1+(-1)*x^3)^(4098)` | 精确验证 |
| `GEN-binomial-chain-16` | `integrate(x^5*(1-x^3)^17,x,0,1)` | `1/1026` | 精确验证 |
| `GEN-binomial-chain-17` | `integrate(x^7*(1-x^2)^9,x,1,0)` | `-1/5720` | 精确验证 |
| `GEN-binomial-chain-18` | `integrate(x^4*(1-x^5)^23,x,-1,0)` | `1118481/8` | 精确验证 |
| `GEN-polynomial-expansion-01` | `integrate(x^2*(1+2*x^2)^5,x)` | `(1/3)*x^3+(2)*x^5+(40/7)*x^7+(80/9)*x^9+(80/11)*x^11+(32/13)*x^13` | 精确验证 |
| `GEN-polynomial-expansion-02` | `integrate(x^4*(1+2*x^3)^4,x)` | `(1/5)*x^5+(1)*x^8+(24/11)*x^11+(16/7)*x^14+(16/17)*x^17` | 精确验证 |
| `GEN-polynomial-expansion-03` | `integrate(x^17*(1+2*x^2)^3,x)` | `(1/18)*x^18+(3/10)*x^20+(6/11)*x^22+(1/3)*x^24` | 精确验证 |
| `GEN-polynomial-expansion-04` | `integrate(x^26*(1+2*x^3)^2,x)` | `(1/27)*x^27+(2/15)*x^30+(4/33)*x^33` | 精确验证 |
| `GEN-binomial-neighbor-01` | `integrate(x/(1+x^2),x)` | `ln(1+x^2)/2` | 精确验证 |
| `GEN-binomial-neighbor-02` | `integrate(x^3/(1+x^2),x)` | `(x^2-ln(1+x^2))/2` | 精确验证 |
| `GEN-binomial-neighbor-03` | `integrate(x/(1+x^2)^2,x)` | `-1/(2*(1+x^2))` | 精确验证 |
| `GEN-binomial-neighbor-04` | `integrate(x*(1+x^2+x^4)^2,x)` | `x^2/2+x^4/2+x^6/2+x^8/4+x^10/10` | 精确验证 |
| `GEN-reciprocal-quartic-01` | `integrate((1)*(x^2-1)/((x^2+1)*sqrt(x^4+(0)*x^2+1)),x)` | `-(1)*asin(sqrt(2)*x/(x^2+1))/sqrt(2)` | 精确验证 |
| `GEN-reciprocal-quartic-02` | `integrate((3)*(x^2-2)/((x^2+2)*sqrt(x^4+(1)*x^2+4)),x)` | `-(3)*asin(sqrt(3)*x/(x^2+2))/sqrt(3)` | 精确验证 |
| `GEN-reciprocal-quartic-03` | `integrate((1)*(x^2-3)/((x^2+3)*sqrt(x^4+(6)*x^2+9)),x)` | `-(1)*x/(x^2+3)` | 精确验证 |
| `GEN-reciprocal-quartic-04` | `integrate((-2)*(x^2-2)/((x^2+2)*sqrt(x^4+(7)*x^2+4)),x)` | `-(-2)*asinh(sqrt(3)*x/(x^2+2))/sqrt(3)` | 精确验证 |
| `GEN-reciprocal-quartic-05` | `integrate((2)*(x^2-1)/((x^2+1)*sqrt(x^4+(-1)*x^2+1)),x)` | `-(2)*asin(sqrt(3)*x/(x^2+1))/sqrt(3)` | 精确验证 |
| `GEN-reciprocal-quartic-06` | `integrate((1)*(x^2-4)/((x^2+4)*sqrt(x^4+(12)*x^2+16)),x)` | `-(1)*asinh(sqrt(4)*x/(x^2+4))/sqrt(4)` | 精确验证 |
| `GEN-quartic-neighbor-01` | `integrate((x^2+(1))/((x^2+1)*sqrt(x^4+2*x^2+1)),x)` | `(1)*atan(x/sqrt(1))/sqrt(1)+(0)*x/(x^2+1)` | 精确验证 |
| `GEN-quartic-neighbor-02` | `integrate((x^2+(3))/((x^2+2)*sqrt(x^4+4*x^2+4)),x)` | `(5/4)*atan(x/sqrt(2))/sqrt(2)+(1/4)*x/(x^2+2)` | 精确验证 |
| `GEN-quartic-neighbor-03` | `integrate((x^2+(-2))/((x^2+3)*sqrt(x^4+6*x^2+9)),x)` | `(1/6)*atan(x/sqrt(3))/sqrt(3)+(-5/6)*x/(x^2+3)` | 精确验证 |
| `GEN-quartic-neighbor-04` | `integrate((x^2-1)/((x^2+1)*sqrt(x^4+4*x^2+4)),x)` | `(-2)*atan(x/sqrt(1))/sqrt(1)+(3)*atan(x/sqrt(2))/sqrt(2)` | 精确验证 |
| `GEN-quartic-neighbor-05` | `integrate((x^2-3)/((x^2+3)*sqrt(x^4+2*x^2+1)),x)` | `(3)*atan(x/sqrt(3))/sqrt(3)+(-2)*atan(x/sqrt(1))/sqrt(1)` | 精确验证 |
| `GEN-quartic-neighbor-06` | `integrate((x^2+1)/((x^2-1)*sqrt(x^4-2*x^2+1)),x)` | `-x/(x^2-1)` | 导数采样通过 |
| `GEN-quartic-trig-01` | `integrate(1/(sin(((1)*x+(0)))^4+cos(((1)*x+(0)))^4),x)` | `(sqrt(2)*((1)*x+(0))-atan(sin(4*((1)*x+(0)))/(3+2*sqrt(2)+cos(4*((1)*x+(0)))))/sqrt(2))/(1)` | 精确验证 |
| `GEN-quartic-trig-02` | `integrate(1/(sin(((2)*x+(1)))^4+cos(((2)*x+(1)))^4),x)` | `(sqrt(2)*((2)*x+(1))-atan(sin(4*((2)*x+(1)))/(3+2*sqrt(2)+cos(4*((2)*x+(1)))))/sqrt(2))/(2)` | 精确验证 |
| `GEN-quartic-trig-03` | `integrate(1/(sin(((-3)*x+(2)))^4+cos(((-3)*x+(2)))^4),x)` | `(sqrt(2)*((-3)*x+(2))-atan(sin(4*((-3)*x+(2)))/(3+2*sqrt(2)+cos(4*((-3)*x+(2)))))/sqrt(2))/(-3)` | 精确验证 |
| `GEN-quartic-trig-04` | `integrate(1/(sin(((1/2)*x+(-1)))^4+cos(((1/2)*x+(-1)))^4),x)` | `(sqrt(2)*((1/2)*x+(-1))-atan(sin(4*((1/2)*x+(-1)))/(3+2*sqrt(2)+cos(4*((1/2)*x+(-1)))))/sqrt(2))/(1/2)` | 精确验证 |
| `GEN-quartic-trig-05` | `integrate(1/(sin(((-2/3)*x+(1/3)))^4+cos(((-2/3)*x+(1/3)))^4),x)` | `(sqrt(2)*((-2/3)*x+(1/3))-atan(sin(4*((-2/3)*x+(1/3)))/(3+2*sqrt(2)+cos(4*((-2/3)*x+(1/3)))))/sqrt(2))/(-2/3)` | 精确验证 |
| `GEN-quartic-trig-06` | `integrate(1/(sin((1*x+0))^4+cos((1*x+0))^4),x,-pi,pi)` | `sqrt(2)*((pi)-(-pi))` | 精确验证 |
| `GEN-quartic-trig-07` | `integrate(1/(sin((2*x+1))^4+cos((2*x+1))^4),x,0,pi)` | `sqrt(2)*((pi)-(0))` | 精确验证 |
| `GEN-quartic-trig-08` | `integrate(1/(sin((-2*x+0))^4+cos((-2*x+0))^4),x,pi,0)` | `sqrt(2)*((0)-(pi))` | 精确验证 |
| `GEN-trig-identity-neighbor-01` | `integrate(1/(sin(x)^2+cos(x)^2),x)` | `x` | 精确验证 |
| `GEN-trig-identity-neighbor-02` | `integrate(1/(sin(x)^4-cos(x)^4),x)` | `-ln(abs(1/cos(2*x)+tan(2*x)))/2` | 精确验证 |
| `GEN-trig-identity-neighbor-03` | `integrate(1/(sin(x)^4+cos(x)^4+2*sin(x)^2*cos(x)^2),x)` | `x` | 精确验证 |
| `GEN-sine-dirichlet-01` | `integrate(sin((1)*x)^1/x,x,0,+infinity)` | `(1/2)*pi` | 精确验证 |
| `GEN-sine-dirichlet-02` | `integrate(sin((2)*x)^3/x,x,0,+infinity)` | `(1/4)*pi` | 精确验证 |
| `GEN-sine-dirichlet-03` | `integrate(sin((-3)*x)^5/x,x,0,+infinity)` | `(-3/16)*pi` | 精确验证 |
| `GEN-sine-dirichlet-04` | `integrate(sin((1/2)*x)^7/x,x,-infinity,0)` | `(5/32)*pi` | 精确验证 |
| `GEN-sine-dirichlet-05` | `integrate(sin((2)*x)^9/x,x,-infinity,+infinity)` | `(35/128)*pi` | 精确验证 |
| `GEN-sine-dirichlet-06` | `integrate(sin((-1)*x)^11/x,x,+infinity,0)` | `(63/512)*pi` | 精确验证 |
| `GEN-sine-dirichlet-07` | `integrate(sin((3)*x)^15/x,x,0,+infinity)` | `(429/4096)*pi` | 精确验证 |
| `GEN-sine-dirichlet-08` | `integrate(sin((1)*x)^31/x,x,0,+infinity)` | `(9694845/134217728)*pi` | 精确验证 |
| `GEN-sine-dirichlet-09` | `integrate(sin((2)*x)^33/x,x,0,+infinity)` | `(300540195/4294967296)*pi` | 精确验证 |
| `GEN-squared-sinc-01` | `integrate(sin(1*x)^2/x^2,x,0,+infinity)` | `1*pi/2` | 精确验证 |
| `GEN-squared-sinc-02` | `integrate(sin(2*x)^2/x^2,x,0,+infinity)` | `2*pi/2` | 精确验证 |
| `GEN-squared-sinc-03` | `integrate(sin(3*x)^2/x^2,x,0,+infinity)` | `3*pi/2` | 精确验证 |
| `GEN-damped-dirichlet-01` | `integrate(exp(-2*x)*sin(1*x)/x,x,0,+infinity)` | `atan(1/2)` | 精确验证 |
| `GEN-damped-dirichlet-02` | `integrate(exp(-1*x)*sin(3*x)/x,x,0,+infinity)` | `atan(3/1)` | 精确验证 |
| `GEN-shifted-dirichlet-01` | `integrate(sin(x+pi/2)/x,x,1,+infinity)` | `-Ci(1)` | 精确验证 |
| `GEN-logistic-moment-01` | `integrate(x^0/(exp((1)*x)+exp(-(1)*x)+2),x,-infinity,+infinity)` | `(1)/(1)^1` | 精确验证 |
| `GEN-logistic-moment-02` | `integrate(x^2/(exp((2)*x)+exp(-(2)*x)+2),x,-infinity,+infinity)` | `(pi^2/3)/(2)^3` | 精确验证 |
| `GEN-logistic-moment-03` | `integrate(x^4/(exp((3)*x)+exp(-(3)*x)+2),x,-infinity,+infinity)` | `(7*pi^4/15)/(3)^5` | 精确验证 |
| `GEN-logistic-moment-04` | `integrate(x^6/(exp((1/2)*x)+exp(-(1/2)*x)+2),x,-infinity,+infinity)` | `(31*pi^6/21)/(1/2)^7` | 精确验证 |
| `GEN-logistic-moment-05` | `integrate(x^8/(exp((-2)*x)+exp(-(-2)*x)+2),x,-infinity,+infinity)` | `(127*pi^8/15)/(2)^9` | 精确验证 |
| `GEN-logistic-moment-06` | `integrate(x^1/(exp((3)*x)+exp(-(3)*x)+2),x,-infinity,+infinity)` | `0` | 精确验证 |
| `GEN-logistic-moment-07` | `integrate(x^3/(exp((-1)*x)+exp(-(-1)*x)+2),x,-infinity,+infinity)` | `0` | 精确验证 |
| `GEN-logistic-moment-08` | `integrate(x^7/(exp((2)*x)+exp(-(2)*x)+2),x,-infinity,+infinity)` | `0` | 精确验证 |
| `GEN-logistic-moment-09` | `integrate(x^10/(exp((1)*x)+exp(-(1)*x)+2),x,-infinity,+infinity)` | `(2555*pi^10/33)/(1)^11` | 精确验证 |
| `GEN-logistic-moment-10` | `integrate(x^2/(exp(2*x)+exp(-2*x)+2),x,+infinity,-infinity)` | `-pi^2/24` | 精确验证 |
| `GEN-logistic-shift-01` | `integrate(x^2/(exp(x-(1/2))+exp(-x+(1/2))+2),x,-infinity,+infinity)` | `pi^2/3+(1/2)^2` | 精确验证 |
| `GEN-logistic-shift-02` | `integrate(x^2/(exp(x-(2))+exp(-x+(2))+2),x,-infinity,+infinity)` | `pi^2/3+(2)^2` | 精确验证 |
| `GEN-logistic-neighbor-01` | `integrate(1/(exp(x)+exp(-x)),x,-infinity,+infinity)` | `pi/2` | 精确验证 |
| `GEN-weighted-reflection-01` | `integrate(((1)*x+(0))*sin(((1)*x+(0)))/(1+1*cos(((1)*x+(0)))^2),x,-(0)/(1),(pi-(0))/(1))` | `((1)*(((-(0)/(1))+((pi-(0))/(1)))/2)+(0))*2*atan(sqrt(1/1))/((1)*sqrt(1))` | 精确验证 |
| `GEN-weighted-reflection-02` | `integrate(((3)*x+(2))*sin(((2)*x+(1)))/(2+3*cos(((2)*x+(1)))^2),x,-(1)/(2),(pi-(1))/(2))` | `((3)*(((-(1)/(2))+((pi-(1))/(2)))/2)+(2))*2*atan(sqrt(3/2))/((2)*sqrt(6))` | 精确验证 |
| `GEN-weighted-reflection-03` | `integrate(((1)*x+(-1))*sin(((-2)*x+(1)))/(3+2*cos(((-2)*x+(1)))^2),x,-(1)/(-2),(pi-(1))/(-2))` | `((1)*(((-(1)/(-2))+((pi-(1))/(-2)))/2)+(-1))*2*atan(sqrt(2/3))/((-2)*sqrt(6))` | 精确验证 |
| `GEN-weighted-reflection-04` | `integrate(((-1)*x+(4))*sin(((3)*x+(-2)))/(1+4*cos(((3)*x+(-2)))^2),x,(pi-(-2))/(3),-(-2)/(3))` | `-(((-1)*(((-(-2)/(3))+((pi-(-2))/(3)))/2)+(4))*2*atan(sqrt(4/1))/((3)*sqrt(4)))` | 精确验证 |
| `GEN-weighted-reflection-05` | `integrate(((2)*x+(1))*sin(((1/2)*x+(0)))/(4+1*cos(((1/2)*x+(0)))^2),x,-(0)/(1/2),(pi-(0))/(1/2))` | `((2)*(((-(0)/(1/2))+((pi-(0))/(1/2)))/2)+(1))*2*atan(sqrt(1/4))/((1/2)*sqrt(4))` | 精确验证 |
| `GEN-weighted-reflection-neighbor-01` | `integrate(x^2*sin(x),x,0,pi)` | `pi^2-4` | 精确验证 |
| `GEN-weighted-reflection-neighbor-02` | `integrate(x*sin(x)/(2-cos(x)^2),x,0,pi)` | `pi*atanh(1/sqrt(2))/sqrt(2)` | 精确验证 |
| `GEN-weighted-reflection-neighbor-03` | `integrate(x*sin(x),x,0,pi/2)` | `1` | 精确验证 |
| `GEN-gaussian-moment-01` | `integrate(x^0*exp(-(1)*x^2),x,-infinity,+infinity)` | `(1)*sqrt(pi)/(1)^(1/2)` | 精确验证 |
| `GEN-gaussian-moment-02` | `integrate(x^2*exp(-(2)*x^2),x,-infinity,+infinity)` | `(1/2)*sqrt(pi)/(2)^(3/2)` | 精确验证 |
| `GEN-gaussian-moment-03` | `integrate(x^4*exp(-(3)*x^2),x,-infinity,+infinity)` | `(3/4)*sqrt(pi)/(3)^(5/2)` | 精确验证 |
| `GEN-gaussian-moment-04` | `integrate(x^6*exp(-(1/2)*x^2),x,-infinity,+infinity)` | `(15/8)*sqrt(pi)/(1/2)^(7/2)` | 精确验证 |
| `GEN-gaussian-shift-01` | `integrate(x^2*exp(-2*(x-3)^2),x,-infinity,+infinity)` | `(37/4)*sqrt(pi/2)` | 精确验证 |
| `GEN-gaussian-fourier-01` | `integrate(exp(-x^2)*cos(3*x),x,-infinity,+infinity)` | `sqrt(pi)*exp(-9/4)` | 精确验证 |
| `GEN-gaussian-neighbor-01` | `integrate(exp(-x^4),x,-infinity,+infinity)` | `Gamma(1/4)/2` | 精确验证 |
| `GEN-parts-exponential-01` | `integrate(x*exp(2*x),x)` | `exp(2*x)*(2*x-1)/4` | 精确验证 |
| `GEN-parts-exponential-02` | `integrate(x^2*exp(-x),x)` | `-exp(-x)*(x^2+2*x+2)` | 精确验证 |
| `GEN-parts-trigonometric-01` | `integrate(x*cos(3*x),x)` | `x*sin(3*x)/3+cos(3*x)/9` | 精确验证 |
| `GEN-parts-trigonometric-02` | `integrate(x^2*sin(x),x)` | `-x^2*cos(x)+2*x*sin(x)+2*cos(x)` | 精确验证 |
| `GEN-parts-logarithmic-01` | `integrate(ln(x),x)` | `x*ln(x)-x` | 精确验证 |
| `GEN-parts-logarithmic-02` | `integrate(x*ln(x)^2,x)` | `x^2*(ln(x)^2-ln(x)+1/2)/2` | 精确验证 |
| `GEN-inverse-function-parts-01` | `integrate(atan(x),x)` | `x*atan(x)-ln(1+x^2)/2` | 精确验证 |
| `GEN-inverse-function-parts-02` | `integrate(asin(x),x)` | `x*asin(x)+sqrt(1-x^2)` | 精确验证 |
| `GEN-rational-simple-poles-01` | `integrate(1/((x+1)*(x+3)),x)` | `(ln(abs(x+1))-ln(abs(x+3)))/2` | 精确验证 |
| `GEN-rational-repeated-pole-01` | `integrate(1/(x+2)^3,x)` | `-1/(2*(x+2)^2)` | 精确验证 |
| `GEN-rational-irreducible-01` | `integrate(1/(x^2+4),x)` | `atan(x/2)/2` | 精确验证 |
| `GEN-rational-irreducible-02` | `integrate(1/(x^2+1)^2,x)` | `x/(2*(x^2+1))+atan(x)/2` | 精确验证 |
| `GEN-rational-division-01` | `integrate(x^4/(1+x^2),x)` | `x^3/3-x+atan(x)` | 精确验证 |
| `GEN-rational-log-derivative-01` | `integrate((3*x^2+2)/(x^3+2*x+5),x)` | `ln(abs(x^3+2*x+5))` | 精确验证 |
| `GEN-trig-substitution-01` | `integrate(sqrt(4-x^2),x)` | `x*sqrt(4-x^2)/2+2*asin(x/2)` | 精确验证 |
| `GEN-trig-substitution-02` | `integrate(1/sqrt(9-x^2),x)` | `asin(x/3)` | 精确验证 |
| `GEN-hyperbolic-substitution-01` | `integrate(sqrt(x^2+4),x)` | `x*sqrt(x^2+4)/2+2*asinh(x/2)` | 精确验证 |
| `GEN-hyperbolic-substitution-02` | `integrate(1/sqrt(x^2+9),x)` | `asinh(x/3)` | 精确验证 |
| `GEN-radical-logarithm-01` | `integrate(1/sqrt(x^2-1),x)` | `ln(x+sqrt(x^2-1))` | 精确验证 |
| `GEN-exponential-substitution-01` | `integrate(exp(x)/(1+exp(2*x)),x)` | `atan(exp(x))` | 精确验证 |
| `GEN-exponential-substitution-02` | `integrate(exp(2*x)/(1+exp(x)),x)` | `exp(x)-ln(1+exp(x))` | 精确验证 |
| `GEN-logarithmic-substitution-01` | `integrate(ln(x)^3/x,x)` | `ln(x)^4/4` | 精确验证 |
| `GEN-logarithmic-substitution-02` | `integrate(1/(x*(1+ln(x)^2)),x)` | `atan(ln(x))` | 精确验证 |
| `GEN-trig-product-01` | `integrate(sin(2*x)*cos(3*x),x)` | `-cos(5*x)/10+cos(x)/2` | 精确验证 |
| `GEN-trig-power-01` | `integrate(sin(x)^3,x)` | `-cos(x)+cos(x)^3/3` | 精确验证 |
| `GEN-trig-power-02` | `integrate(cos(x)^4,x)` | `3*x/8+sin(2*x)/4+sin(4*x)/32` | 精确验证 |
| `GEN-trig-logarithm-01` | `integrate(tan(x),x)` | `-ln(abs(cos(x)))` | 精确验证 |
| `GEN-hyperbolic-product-01` | `integrate(sinh(x)*cosh(x)^3,x)` | `cosh(x)^4/4` | 精确验证 |
| `GEN-absolute-value-01` | `integrate(abs(x),x)` | `x*abs(x)/2` | 精确验证 |
| `GEN-absolute-value-02` | `integrate(abs(2*x-1),x)` | `(2*x-1)*abs(2*x-1)/4` | 精确验证 |
| `GEN-real-odd-root-01` | `integrate(surd(x,3),x)` | `3*x*surd(x,3)/4` | 精确验证 |
| `GEN-real-odd-root-02` | `integrate(1/surd(x,3),x)` | `3*surd(x,3)^2/2` | 精确验证 |
| `GEN-principal-power-01` | `integrate(x^(1/3),x)` | `3*x^(4/3)/4` | 精确验证 |
| `GEN-composition-01` | `integrate(2*x*exp(x^2)/(1+exp(x^2)),x)` | `ln(1+exp(x^2))` | 精确验证 |
| `GEN-composition-02` | `integrate(cos(x)*ln(2+sin(x)),x)` | `(2+sin(x))*ln(2+sin(x))-(2+sin(x))` | 精确验证 |
| `GEN-composition-03` | `integrate(x*cos(x^2)*exp(sin(x^2)),x)` | `exp(sin(x^2))/2` | 精确验证 |
| `GEN-special-function-01` | `integrate(exp(-x^2),x)` | `sqrt(pi)*erf(x)/2` | 精确验证 |
| `GEN-special-function-02` | `integrate(sin(x)/x,x)` | `Si(x)` | 精确验证 |
| `GEN-endpoint-beta-01` | `integrate(1/sqrt(x*(1-x)),x,0,1)` | `pi` | 精确验证 |
| `GEN-endpoint-beta-02` | `integrate(sqrt(x)*(1-x)^2,x,0,1)` | `16/105` | 精确验证 |
| `GEN-endpoint-log-01` | `integrate(ln(x),x,0,1)` | `-1` | 精确验证 |
| `GEN-endpoint-log-02` | `integrate(ln(x)^2,x,0,1)` | `2` | 精确验证 |
| `GEN-endpoint-log-03` | `integrate(ln(x)/sqrt(x),x,0,1)` | `-4` | 精确验证 |
| `GEN-gamma-decay-01` | `integrate(x^4*exp(-2*x),x,0,+infinity)` | `3/4` | 精确验证 |
| `GEN-gamma-decay-02` | `integrate(sqrt(x)*exp(-x),x,0,+infinity)` | `sqrt(pi)/2` | 精确验证 |
| `GEN-rational-tail-01` | `integrate(1/(1+x^2),x,0,+infinity)` | `pi/2` | 精确验证 |
| `GEN-rational-tail-02` | `integrate(1/(1+x^4),x,0,+infinity)` | `pi/(2*sqrt(2))` | 精确验证 |
| `GEN-rational-tail-03` | `integrate(x/(1+x^4),x,0,+infinity)` | `pi/4` | 精确验证 |
| `GEN-mellin-log-01` | `integrate(ln(x)/(1+x^2),x,0,+infinity)` | `0` | 精确验证 |
| `GEN-mellin-log-02` | `integrate(ln(x)^2/(1+x^2),x,0,+infinity)` | `pi^3/8` | 精确验证 |
| `GEN-absolute-value-03` | `integrate(abs(x-1),x,-2,3)` | `13/2` | 精确验证 |
| `GEN-absolute-value-04` | `integrate(abs(x^2-1),x,-2,2)` | `4` | 精确验证 |
| `GEN-absolute-trig-01` | `integrate(abs(sin(x)),x,0,2*pi)` | `4` | 精确验证 |
| `GEN-absolute-trig-02` | `integrate(abs(cos(2*x)),x,-pi,pi)` | `4` | 精确验证 |
| `GEN-log-trig-01` | `integrate(ln(sin(x)),x,0,pi)` | `-pi*ln(2)` | 精确验证 |
| `GEN-log-trig-02` | `integrate(ln(cos(x)),x,0,pi/2)` | `-pi*ln(2)/2` | 精确验证 |
| `GEN-frullani-01` | `integrate((exp(-2*x)-exp(-5*x))/x,x,0,+infinity)` | `ln(5/2)` | 精确验证 |
| `GEN-rational-exponential-01` | `integrate(1/(exp(x)+1),x,0,+infinity)` | `ln(2)` | 精确验证 |
| `GEN-rational-exponential-02` | `integrate(x/(exp(x)+1),x,0,+infinity)` | `pi^2/12` | 精确验证 |
| `GEN-root-substitution-01` | `integrate(surd(x,3)^2,x,-8,1)` | `99/5` | 精确验证 |

## 第二轮泛化题库（61 条）

[原题与定义域](../tests/generalization-cycle2.json) · [实际输出和验证记录](benchmarks/cycle2-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C2-gaussian-convolution-01` | `integrate(x^0*exp(-(1)*(x-(1))^2-(2)*(x-(-2))^2),x,-infinity,+infinity)` | `(1)*sqrt(pi/(3))*exp(-(6))` | 精确验证 |
| `C2-gaussian-convolution-02` | `integrate(x^1*exp(-(2)*(x-(-1))^2-(3)*(x-(2))^2),x,-infinity,+infinity)` | `(4/5)*sqrt(pi/(5))*exp(-(54/5))` | 精确验证 |
| `C2-gaussian-convolution-03` | `integrate(x^6*exp(-(1/2)*(x-(2))^2-(3/2)*(x-(-1))^2),x,-infinity,+infinity)` | `(1741/4096)*sqrt(pi/(2))*exp(-(27/8))` | 精确验证 |
| `C2-gaussian-convolution-04` | `integrate(x^12*exp(-(1)*(x-(0))^2-(1)*(x-(1))^2),x,-infinity,+infinity)` | `(17519/512)*sqrt(pi/(2))*exp(-(1/2))` | 精确验证 |
| `C2-cauchy-convolution-01` | `integrate(1/((x^2+(1)^2)*((x-(3))^2+(2)^2)),x,-infinity,+infinity)` | `(1/12)*pi` | 精确验证 |
| `C2-cauchy-convolution-02` | `integrate(1/((x^2+(2)^2)*((x-(-1))^2+(3)^2)),x,-infinity,+infinity)` | `(5/156)*pi` | 精确验证 |
| `C2-cauchy-convolution-03` | `integrate(1/((x^2+(1/2)^2)*((x-(2))^2+(3/2)^2)),x,-infinity,+infinity)` | `(1/3)*pi` | 精确验证 |
| `C2-cauchy-convolution-04` | `integrate(x/((x^2+1)*((x-(3))^2+4)),x,-infinity,+infinity)` | `(1/12)*pi` | 精确验证 |
| `C2-cauchy-convolution-05` | `integrate(x/((x^2+9)*((x-(-2))^2+1)),x,-infinity,+infinity)` | `(-1/10)*pi` | 精确验证 |
| `C2-mellin-rational-01` | `integrate(x^(1/2-1)/(1+x^(2)),x,0,+infinity)` | `pi/((2)*sin(pi*(1/2)/(2)))` | 精确验证 |
| `C2-mellin-rational-02` | `integrate(x^(2/3-1)/(1+x^(5/3)),x,0,+infinity)` | `pi/((5/3)*sin(pi*(2/3)/(5/3)))` | 精确验证 |
| `C2-mellin-rational-03` | `integrate(x^(5/4-1)/(1+x^(3)),x,0,+infinity)` | `pi/((3)*sin(pi*(5/4)/(3)))` | 精确验证 |
| `C2-mellin-rational-04` | `integrate(x^(2-1)/(1+x^(5)),x,0,+infinity)` | `pi/((5)*sin(pi*(2)/(5)))` | 精确验证 |
| `C2-mellin-log-weight-01` | `integrate(x^(1-1)*ln(x)^1/(1+x^(3)),x,0,+infinity)` | `-pi^2/(3)^2*cos((pi*(1)/(3)))/sin((pi*(1)/(3)))^2` | 精确验证 |
| `C2-mellin-log-weight-02` | `integrate(x^(2-1)*ln(x)^1/(1+x^(3)),x,0,+infinity)` | `-pi^2/(3)^2*cos((pi*(2)/(3)))/sin((pi*(2)/(3)))^2` | 精确验证 |
| `C2-mellin-log-weight-03` | `integrate(x^(1/2-1)*ln(x)^1/(1+x^(2)),x,0,+infinity)` | `-pi^2/(2)^2*cos((pi*(1/2)/(2)))/sin((pi*(1/2)/(2)))^2` | 精确验证 |
| `C2-mellin-log-weight-04` | `integrate(x^(1-1)*ln(x)^2/(1+x^(4)),x,0,+infinity)` | `pi^3/(4)^3*(1+2*(cos((pi*(1)/(4)))/sin((pi*(1)/(4))))^2)/sin((pi*(1)/(4)))` | 精确验证 |
| `C2-fractional-beta-01` | `integrate(x^(1/3-1)*(1-x)^(2/3-1),x,0,1)` | `Gamma(1/3)*Gamma(2/3)/Gamma(1)` | 精确验证 |
| `C2-fractional-beta-02` | `integrate(x^(2/3-1)*(1-x)^(4/3-1),x,0,1)` | `Gamma(2/3)*Gamma(4/3)/Gamma(2)` | 精确验证 |
| `C2-fractional-beta-03` | `integrate(x^(3/4-1)*(1-x)^(1/4-1),x,0,1)` | `Gamma(3/4)*Gamma(1/4)/Gamma(1)` | 精确验证 |
| `C2-fractional-beta-04` | `integrate(x^(3/2-1)*(1-x)^(5/3-1),x,0,1)` | `Gamma(3/2)*Gamma(5/3)/Gamma(19/6)` | 精确验证 |
| `C2-beta-log-weight-01` | `integrate((ln(x))/sqrt(x*(1-x)),x,0,1)` | `-2*pi*ln(2)` | 精确验证 |
| `C2-beta-log-weight-02` | `integrate((ln(x)^2)/sqrt(x*(1-x)),x,0,1)` | `pi*(4*ln(2)^2+pi^2/3)` | 精确验证 |
| `C2-beta-log-weight-03` | `integrate((ln(x)*ln(1-x))/sqrt(x*(1-x)),x,0,1)` | `pi*(4*ln(2)^2-pi^2/6)` | 精确验证 |
| `C2-bose-moment-01` | `integrate(x^2/(exp((2)*x)-1),x,0,+infinity)` | `(1/4)*Zeta(3)` | 精确验证 |
| `C2-bose-moment-02` | `integrate(x^4/(exp((1/2)*x)-1),x,0,+infinity)` | `(768)*Zeta(5)` | 精确验证 |
| `C2-bose-moment-03` | `integrate(x^9/(exp((3)*x)-1),x,0,+infinity)` | `(4480/729)*Zeta(10)` | 精确验证 |
| `C2-fermi-moment-01` | `integrate(x^3/(exp((2)*x)+1),x,0,+infinity)` | `(21/64)*Zeta(4)` | 精确验证 |
| `C2-fermi-moment-02` | `integrate(x^6/(exp((3/2)*x)+1),x,0,+infinity)` | `(1120/27)*Zeta(7)` | 精确验证 |
| `C2-fermi-moment-03` | `integrate(x^11/(exp((1)*x)+1),x,0,+infinity)` | `(319178475/8)*Zeta(12)` | 精确验证 |
| `C2-logistic-high-moment-01` | `integrate(x^12/(exp((3/2)*x)+exp(-(3/2)*x)+2),x,-infinity,+infinity)` | `(11587395584/2176250895)*pi^12` | 精确验证 |
| `C2-logistic-high-moment-02` | `integrate(x^14/(exp((2)*x)+exp(-(2)*x)+2),x,-infinity,+infinity)` | `(57337/98304)*pi^14` | 精确验证 |
| `C2-logistic-shifted-moment-01` | `integrate(x^4/(exp((2)*(x-(3)))+exp(-(2)*(x-(3)))+2),x,-infinity,+infinity)` | `(81/2)*(1)+(27/4)*(pi^2/3)+(1/32)*(7*pi^4/15)` | 精确验证 |
| `C2-logistic-shifted-moment-02` | `integrate(x^6/(exp((1/2)*(x-(-2)))+exp(-(1/2)*(x-(-2)))+2),x,-infinity,+infinity)` | `(128)*(1)+(1920)*(pi^2/3)+(1920)*(7*pi^4/15)+(128)*(31*pi^6/21)` | 精确验证 |
| `C2-phase-laplace-01` | `integrate(exp(-3*x)*(sin((2)*x+(pi/3))-sin(pi/3))/x,x,0,+infinity)` | `cos(pi/3)*atan((2)/3)-sin(pi/3)*ln(1+((2)/3)^2)/2` | 精确验证 |
| `C2-phase-laplace-02` | `integrate(exp(-2*x)*(sin((-3)*x+(pi/4))-sin(pi/4))/x,x,0,+infinity)` | `cos(pi/4)*atan((-3)/2)-sin(pi/4)*ln(1+((-3)/2)^2)/2` | 精确验证 |
| `C2-phase-laplace-03` | `integrate(exp(-1*x)*(sin((1/2)*x+(-pi/6))-sin(-pi/6))/x,x,0,+infinity)` | `cos(-pi/6)*atan((1/2)/1)-sin(-pi/6)*ln(1+((1/2)/1)^2)/2` | 精确验证 |
| `C2-cosine-frullani-01` | `integrate(exp(-2*x)*(cos((1)*x)-cos((3)*x))/x,x,0,+infinity)` | `ln((4+(3)^2)/(4+(1)^2))/2` | 精确验证 |
| `C2-cosine-frullani-02` | `integrate(exp(-3*x)*(cos((1/2)*x)-cos((2)*x))/x,x,0,+infinity)` | `ln((9+(2)^2)/(9+(1/2)^2))/2` | 精确验证 |
| `C2-conditional-cosine-difference-01` | `integrate((cos(1*x)-cos(2*x))/x,x,0,+infinity)` | `ln(2/1)` | 精确验证 |
| `C2-conditional-cosine-difference-02` | `integrate((cos(3*x)-cos(1*x))/x,x,0,+infinity)` | `ln(1/3)` | 精确验证 |
| `C2-cross-sinc-01` | `integrate(sin(1*x)*sin(3*x)/x^2,x,0,+infinity)` | `1*pi/2` | 精确验证 |
| `C2-cross-sinc-02` | `integrate(sin(2*x)*sin(5*x)/x^2,x,0,+infinity)` | `2*pi/2` | 精确验证 |
| `C2-cauchy-fourier-01` | `integrate(cos(2*x)/((x-(0))^2+(1)^2),x,-infinity,+infinity)` | `pi/(1)*exp(-(1)*2)*cos(2*(0))` | 精确验证 |
| `C2-cauchy-fourier-02` | `integrate(cos(3*x)/((x-(0))^2+(2)^2),x,-infinity,+infinity)` | `pi/(2)*exp(-(2)*3)*cos(3*(0))` | 精确验证 |
| `C2-cauchy-fourier-03` | `integrate(cos(1*x)/((x-(3))^2+(2)^2),x,-infinity,+infinity)` | `pi/(2)*exp(-(2)*1)*cos(1*(3))` | 精确验证 |
| `C2-cauchy-fourier-04` | `integrate(cos(2*x)/((x-(-1))^2+(1/2)^2),x,-infinity,+infinity)` | `pi/(1/2)*exp(-(1/2)*2)*cos(2*(-1))` | 精确验证 |
| `C2-rational-power-01` | `integrate(x^(-2/3)/(1+x^(1/3)),x)` | `3*ln(1+x^(1/3))` | 精确验证 |
| `C2-real-root-rational-01` | `integrate(1/(surd(x,3)^2*(1+surd(x,3))^2),x)` | `-3/(1+surd(x,3))` | 精确验证 |
| `C2-rational-power-02` | `integrate(x^(-1/3)*sqrt(2+3*x^(2/3)),x)` | `(2+3*x^(2/3))^(3/2)/3` | 精确验证 |
| `C2-rational-power-03` | `integrate(x^(-1/2)/(1+sqrt(x))^2,x)` | `-2/(1+sqrt(x))` | 精确验证 |
| `C2-shifted-gaussian-primitive-01` | `integrate(exp(-2*x^2+3*x),x)` | `sqrt(pi)*exp(9/8)*erf(sqrt(2)*x-3/(2*sqrt(2)))/(2*sqrt(2))` | 精确验证 |
| `C2-hyperbolic-parts-01` | `integrate(asinh(x),x)` | `x*asinh(x)-sqrt(1+x^2)` | 精确验证 |
| `C2-exponential-trig-01` | `integrate(exp(x)*cos(2*x),x)` | `exp(x)*(cos(2*x)+2*sin(2*x))/5` | 精确验证 |
| `C2-nested-log-01` | `integrate(1/(x*ln(x)*ln(ln(x))),x)` | `ln(abs(ln(ln(x))))` | 精确验证 |
| `C2-nested-log-02` | `integrate(1/(x*sqrt(ln(x))),x)` | `2*sqrt(ln(x))` | 精确验证 |
| `C2-fractional-log-parts-01` | `integrate(sqrt(x)*ln(x),x)` | `x^(3/2)*(2*ln(x)/3-4/9)` | 精确验证 |
| `C2-fractional-log-parts-02` | `integrate(ln(x)/x^(3/2),x)` | `-(2*ln(x)+4)/sqrt(x)` | 精确验证 |
| `C2-absolute-exponential-01` | `integrate(abs(x-2)*exp(abs(x-2)),x)` | `sign(x-2)*((abs(x-2)-1)*exp(abs(x-2))+1)` | 精确验证 |
| `C2-inverse-gaussian-01` | `integrate(exp(-1*x-1/x)/sqrt(x),x,0,+infinity)` | `sqrt(pi/1)*exp(-2*sqrt(1))` | 精确验证 |
| `C2-inverse-gaussian-02` | `integrate(exp(-2*x-3/x)/sqrt(x),x,0,+infinity)` | `sqrt(pi/2)*exp(-2*sqrt(6))` | 精确验证 |

## 第三轮泛化题库（30 条）

[原题与定义域](../tests/generalization-cycle3.json) · [实际输出和验证记录](benchmarks/cycle3-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C3-mellin-denominator-power-01` | `integrate(x^(-1/2)/(1+x^(3/2))^2,x,0,+infinity)` | `8*pi/(9*sqrt(3))` | 精确验证 |
| `C3-mellin-denominator-power-02` | `integrate(x^(3/2)/(1+x^2)^3,x,0,+infinity)` | `Gamma(5/4)*Gamma(7/4)/4` | 精确验证 |
| `C3-mellin-log-denominator-power-01` | `integrate(ln(x)/(1+x^2)^2,x,0,+infinity)` | `-pi/4` | 精确验证 |
| `C3-mellin-log-denominator-power-02` | `integrate(ln(x)^2/(1+x^2)^2,x,0,+infinity)` | `pi^3/16` | 精确验证 |
| `C3-beta-logit-01` | `integrate(ln(x/(1-x))^2/sqrt(x*(1-x)),x,0,1)` | `pi^3` | 精确验证 |
| `C3-beta-logit-02` | `integrate(sqrt(x/(1-x))*ln(x/(1-x)),x,0,1)` | `pi` | 精确验证 |
| `C3-beta-mixed-log-01` | `integrate(ln(x)^3/sqrt(x*(1-x)),x,0,1)` | `-pi*(8*ln(2)^3+2*pi^2*ln(2)+12*Zeta(3))` | 精确验证 |
| `C3-beta-mixed-log-02` | `integrate(sqrt(x*(1-x))*ln(x)*ln(1-x),x,0,1)` | `pi/8*((1/2-2*ln(2))^2-pi^2/6+5/4)` | 精确验证 |
| `C3-laplace-second-cancellation-01` | `integrate(exp(-2*x)*(1-cos(3*x))/x^2,x,0,+infinity)` | `3*atan(3/2)-ln(13/4)` | 精确验证 |
| `C3-laplace-second-cancellation-02` | `integrate(exp(-3*x)*(sin(2*x)-2*x)/x^2,x,0,+infinity)` | `2-3*atan(2/3)-ln(13/9)` | 精确验证 |
| `C3-laplace-second-cancellation-03` | `integrate(exp(-2*x)*(sin(3*x+pi/3)-sin(pi/3)-3*x*cos(pi/3))/x^2,x,0,+infinity)` | `(3-2*atan(3/2)-3*ln(13/4)/2)/2-sqrt(3)*(3*atan(3/2)-ln(13/4))/2` | 精确验证 |
| `C3-inverse-gaussian-weight-01` | `integrate(x^(-3/2)*exp(-2*x-3/x),x,0,+infinity)` | `sqrt(pi/3)*exp(-2*sqrt(6))` | 精确验证 |
| `C3-inverse-gaussian-weight-02` | `integrate(sqrt(x)*exp(-2*x-3/x),x,0,+infinity)` | `sqrt(pi)*exp(-2*sqrt(6))*(1/(4*sqrt(2))+sqrt(3)/2)` | 精确验证 |
| `C3-inverse-gaussian-quadratic-01` | `integrate(x^2*exp(-3*x^2-2/x^2),x,0,+infinity)` | `sqrt(pi)*exp(-2*sqrt(6))*(1/(12*sqrt(3))+sqrt(2)/6)` | 精确验证 |
| `C3-quartic-beta-radical-01` | `integrate(1/sqrt(1+x^4),x,0,+infinity)` | `Gamma(1/4)^2/(4*sqrt(pi))` | 精确验证 |
| `C3-quartic-beta-radical-02` | `integrate(1/sqrt(1-x^4),x,0,1)` | `Gamma(1/4)*sqrt(pi)/(4*Gamma(3/4))` | 精确验证 |
| `C3-logarithmic-zeta-kernel-01` | `integrate(ln(1+x)/x,x,0,1)` | `pi^2/12` | 精确验证 |
| `C3-logarithmic-zeta-kernel-02` | `integrate(ln(x)*ln(1-x)/x,x,0,1)` | `Zeta(3)` | 精确验证 |
| `C3-hyperbolic-log-moment-01` | `integrate(ln(cosh(x))/cosh(x),x,-infinity,+infinity)` | `pi*ln(2)` | 精确验证 |
| `C3-inverse-trig-log-moment-01` | `integrate(atan(x)/(x*(1+x^2)),x,0,+infinity)` | `pi*ln(2)/2` | 精确验证 |
| `C3-composed-parts-01` | `integrate(x*asinh(x)/sqrt(1+x^2),x)` | `sqrt(1+x^2)*asinh(x)-x` | 精确验证 |
| `C3-composed-parts-02` | `integrate(ln(1+x^2)/x^2,x)` | `-ln(1+x^2)/x+2*atan(x)` | 精确验证 |
| `C3-root-rational-primitive-01` | `integrate(1/(sqrt(x)+x),x)` | `2*ln(1+sqrt(x))` | 精确验证 |
| `C3-root-inverse-trig-primitive-01` | `integrate(atan(sqrt(x))/sqrt(x),x)` | `2*sqrt(x)*atan(sqrt(x))-ln(1+x)` | 精确验证 |
| `C3-exterior-radical-primitive-01` | `integrate(1/(x*sqrt(x^2-1)),x)` | `atan(sqrt(x^2-1))` | 精确验证 |
| `C3-stress-beta-polynomial-01` | `integrate(2^512*x^255*(1-x)^255,x,0,1)` | `2^512*factorial(255)^2/factorial(511)` | 精确验证 |
| `C3-stress-binomial-substitution-01` | `integrate(2^128*x^63*(1-x^2)^127,x,0,1)` | `2^128*factorial(31)*factorial(127)/(2*factorial(159))` | 精确验证 |
| `C3-stress-beta-logit-01` | `integrate(ln(x/(1-x))^16/sqrt(x*(1-x)),x,0,1)` | `19391512145*pi^17` | 精确验证 |
| `C3-stress-fermi-moment-01` | `integrate(x^24/(exp(2*x)+1),x,0,+infinity)` | `(1-2^(-24))*factorial(24)*Zeta(25)/2^25` | 精确验证 |
| `C3-stress-harmonic-cancellation-01` | `integrate((sin(31*x)-31*sin(x))/x^3,x,0,+infinity)` | `-465*pi/2` | 精确验证 |

## 第四轮泛化题库（16 条）

[原题与定义域](../tests/generalization-cycle4.json) · [实际输出和验证记录](benchmarks/cycle4-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C4-beta-mixed-third-01` | `integrate(ln(x)*ln(1-x)*ln(x*(1-x))/sqrt(x*(1-x)),x,0,1)` | `pi*(4*Zeta(3)-16*ln(2)^3)` | 精确验证 |
| `C4-beta-skew-logit-01` | `integrate(sqrt(x/(1-x))*ln(x/(1-x))^3,x,0,1)` | `3*pi^3` | 精确验证 |
| `C4-mellin-distinct-factors-01` | `integrate(x^(-1/3)/((1+x^(2/3))*(1+4*x^(2/3))),x,0,+infinity)` | `ln(2)` | 精确验证 |
| `C4-mellin-unmatched-log-scale-01` | `integrate(ln(x)^2/(1+9*x^2)^2,x,0,+infinity)` | `pi^3/48+pi*ln(3)/6+pi*ln(3)^2/12` | 精确验证 |
| `C4-laplace-squared-cancellation-01` | `integrate(exp(-x)*(1-cos(2*x))^2/x^2,x,0,+infinity)` | `4*atan(2)-2*atan(4)-ln(5)+ln(17)/4` | 精确验证 |
| `C4-laplace-decay-difference-01` | `integrate((exp(-x)-2*exp(-2*x)+exp(-3*x))/x^2,x,0,+infinity)` | `3*ln(3)-4*ln(2)` | 精确验证 |
| `C4-gaussian-erf-shift-01` | `integrate(exp(-2*x^2)*erf(x+1),x,-infinity,+infinity)` | `sqrt(pi/2)*erf(sqrt(2/3))` | 精确验证 |
| `C4-gaussian-erf-product-01` | `integrate(exp(-(x-1)^2)*erf(x-1)*erf(2*x-2),x,-infinity,+infinity)` | `2*asin(2/sqrt(10))/sqrt(pi)` | 精确验证 |
| `C4-inverse-gaussian-square-01` | `integrate(exp(-(sqrt(x)-2/sqrt(x))^2)/sqrt(x),x,0,+infinity)` | `sqrt(pi)` | 精确验证 |
| `C4-arc-affine-01` | `integrate(acos((1+2*cos(2*x+1))/(2+3*cos(2*x+1))),x,-1/2,atan(sqrt(2))-1/2)` | `pi*atan(sqrt(2))-atan(sqrt(2))^2-pi*atan(sqrt(5))+pi^2/4` | 精确验证 |
| `C4-arc-complement-rational-01` | `integrate(2*asin((3-x^2)/(5-x^2))/(1+x^2),x,0,sqrt(2))` | `-pi*atan(sqrt(2))+2*atan(sqrt(2))^2+2*pi*atan(sqrt(5))-pi^2/2` | 精确验证 |
| `C4-atan-square-symmetry-01` | `integrate(atan(2/sqrt(x^2+2))/((1+x^2)*sqrt(x^2+2)),x,0,2)` | `atan(2)^2/2` | 精确验证 |
| `C4-frullani-log-weight-01` | `integrate((atan(2*x)-atan(3*x))*ln(x)/x,x,0,+infinity)` | `pi*(ln(3)^2-ln(2)^2)/4` | 精确验证 |
| `C4-frullani-log-weight-02` | `integrate((atan(2*x)-atan(x))*ln(x)^2/x,x,0,+infinity)` | `pi^3*ln(2)/8+pi*ln(2)^3/6` | 精确验证 |
| `C4-loglog-fractional-scale-01` | `integrate(sqrt(x)*ln(-ln(x))/(1+x^3),x,0,1)` | `(2/3)*((pi*ln(2*pi)/4+pi*ln(Gamma(3/4)/Gamma(1/4))/2)-pi*ln(3/2)/4)` | 精确验证 |
| `C4-loglog-frullani-01` | `integrate((x-x^3)*ln(-ln(x))/(-ln(x)),x,0,1)` | `-3*ln(2)^2/2-euler_gamma*ln(2)` | 精确验证 |

## 第五轮泛化题库（8 条）

[原题与定义域](../tests/generalization-cycle5.json) · [实际输出和验证记录](benchmarks/cycle5-cycle8-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C5-gaussian-erf-exponential-01` | `integrate(exp(-x^2+erf(x)),x,-infinity,+infinity)` | `sqrt(pi)*sinh(1)` | 精确验证 |
| `C5-beta-affine-logit-01` | `integrate(ln((1+x)/(1-x))^2/sqrt(1-x^2),x,-1,1)` | `pi^3` | 精确验证 |
| `C5-frullani-root-log-01` | `integrate((atan(2*sqrt(x))-atan(3*sqrt(x)))*ln(x)/x,x,0,+infinity)` | `pi*(ln(3)^2-ln(2)^2)` | 精确验证 |
| `C5-loglog-frullani-second-01` | `integrate((x-x^3)*ln(-ln(x))^2/(-ln(x)),x,0,1)` | `7*ln(2)^3/3+3*euler_gamma*ln(2)^2+(euler_gamma^2+pi^2/6)*ln(2)` | 精确验证 |
| `C5-laplace-second-log-01` | `integrate((exp(-x)-2*exp(-2*x)+exp(-3*x))*ln(x)/x^2,x,0,+infinity)` | `(1-euler_gamma)*(3*ln(3)-4*ln(2))-3*ln(3)^2/2+2*ln(2)^2` | 精确验证 |
| `C5-reciprocal-gaussian-erf-01` | `integrate((x+1)*exp(-(sqrt(x)-1/sqrt(x))^2)*erf((sqrt(x)-1/sqrt(x)))^2/x^(3/2),x,0,+infinity)` | `2*sqrt(pi)/3` | 精确验证 |
| `C5-arc-quartic-pullback-01` | `integrate(4*x*acos((3-x^4)/(5-x^4))/(1+x^4),x,0,2^(1/4))` | `2*pi*atan(sqrt(2))-2*atan(sqrt(2))^2-2*pi*atan(sqrt(5))+pi^2/2` | 精确验证 |
| `C5-atan-rectangle-pair-01` | `integrate(2*atan(3/sqrt(4*x^2+2))/((1+4*x^2)*sqrt(4*x^2+2))+3*atan(2/sqrt(9*x^2+2))/((1+9*x^2)*sqrt(9*x^2+2)),x,0,1)` | `atan(2)*atan(3)` | 精确验证 |

## 第六轮泛化题库（8 条）

[原题与定义域](../tests/generalization-cycle6.json) · [实际输出和验证记录](benchmarks/cycle6-cycle8-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C6-gaussian-atan-parts-01` | `integrate(x*exp(-x^2)*atan(x),x,0,+infinity)` | `pi*exp(1)*erfc(1)/4` | 精确验证 |
| `C6-half-period-log-sine-moment-01` | `integrate(x*ln(sin(x)),x,0,pi/2)` | `7*Zeta(3)/16-pi^2*ln(2)/8` | 精确验证 |
| `C6-opposite-logarithm-product-01` | `integrate(ln(1-x)*ln(1+x)/x,x,0,1)` | `-5*Zeta(3)/8` | 精确验证 |
| `C6-mixed-degree-mellin-log-01` | `integrate(ln(x)/((1+x)*(1+x^2)),x,0,+infinity)` | `-pi^2/16` | 精确验证 |
| `C6-trig-log-quarter-period-01` | `integrate(ln(1+sin(x)),x,0,pi/2)` | `(Psi(1/4,1)-Psi(3/4,1))/8-pi*ln(2)/2` | 精确验证 |
| `C6-mobius-atan-log-measure-01` | `integrate(atan(x)/(1+x),x,0,1)` | `pi*ln(2)/8` | 精确验证 |
| `C6-oscillatory-third-order-cancellation-01` | `integrate((sin(x)-x*cos(x))/x^3,x,0,+infinity)` | `pi/4` | 精确验证 |
| `C6-fermi-logarithm-first-moment-01` | `integrate(x*ln(1+exp(-x)),x,0,+infinity)` | `3*Zeta(3)/4` | 精确验证 |

## 第七轮泛化题库（8 条）

[原题与定义域](../tests/generalization-cycle7.json) · [实际输出和验证记录](benchmarks/cycle7-cycle8-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C7-quadratic-radical-atan-parts-01` | `integrate((2*x+1)*atan(sqrt(x^2+x+1))/sqrt(x^2+x+1),x)` | `2*sqrt(x^2+x+1)*atan(sqrt(x^2+x+1))-ln(x^2+x+2)` | 精确验证 |
| `C7-nested-root-degree-boundary-01` | `integrate((1+x^(1/17))^(1/19)/x^(16/17),x)` | `323*(1+x^(1/17))^(20/19)/20` | 精确验证 |
| `C7-composed-high-power-beta-boundary-01` | `integrate((2*x+1)*(x^2+x)^15*(1-(x^2+x)^16)^2048,x,0,(sqrt(5)-1)/2)` | `1/32784` | 精确验证 |
| `C7-erf-erfc-product-01` | `integrate(erf(x)*erfc(x),x,0,+infinity)` | `(sqrt(2)-1)/sqrt(pi)` | 精确验证 |
| `C7-radical-cauchy-log-moment-01` | `integrate(ln(1+x^2)/(1+x^2)^(3/2),x,0,+infinity)` | `2-2*ln(2)` | 精确验证 |
| `C7-damped-sine-linear-cancellation-01` | `integrate(exp(-x)*(sin(2*x)-2*x)/x^2,x,0,+infinity)` | `2-atan(2)-ln(5)` | 精确验证 |
| `C7-mobius-logit-cauchy-01` | `integrate(ln((1+x)/(1-x))^2/(1+x^2),x,0,1)` | `pi^3/16` | 精确验证 |
| `C7-log-sine-cosine-sum-01` | `integrate(ln(sin(x)+cos(x)),x,0,pi/2)` | `(Psi(1/4,1)-Psi(3/4,1))/16-pi*ln(2)/4` | 精确验证 |

## 第八轮泛化题库（8 条）

[原题与定义域](../tests/generalization-cycle8.json) · [实际输出和验证记录](benchmarks/cycle8-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C8-quadratic-asinh-parts-01` | `integrate((2*x+1)*asinh(x^2+x+1),x)` | `(x^2+x+1)*asinh(x^2+x+1)-sqrt(1+(x^2+x+1)^2)` | 精确验证 |
| `C8-composed-binomial-parts-boundary-01` | `integrate((2*x+1)*(x^2+x)^63*(1+(x^2+x)^32)^1024,x)` | `(1+(x^2+x)^32)^1026/32832-(1+(x^2+x)^32)^1025/32800` | 精确验证 |
| `C8-exponential-beta-boundary-01` | `integrate(exp(-2*x)*(1-exp(-x))^4096,x,0,+infinity)` | `1/(4097*4098)` | 精确验证 |
| `C8-laplace-erf-square-root-01` | `integrate(exp(-x)*erf(sqrt(x)),x,0,+infinity)` | `1/sqrt(2)` | 精确验证 |
| `C8-atan-log-cauchy-moment-01` | `integrate(atan(x)*ln(x)/(1+x^2),x,0,1)` | `7*Zeta(3)/16-pi*(Psi(1/4,1)-Psi(3/4,1))/64` | 精确验证 |
| `C8-opposite-trig-logarithm-product-01` | `integrate(ln(1+sin(x))*ln(1-sin(x)),x,0,pi/2)` | `pi*ln(2)^2/2-pi^3/12` | 精确验证 |
| `C8-atan-square-cauchy-square-01` | `integrate(atan(x)^2/(1+x^2)^2,x,0,+infinity)` | `pi^3/48-pi/8` | 精确验证 |
| `C8-gaussian-log-square-moment-01` | `integrate(exp(-x^2)*ln(x)^2,x,0,+infinity)` | `sqrt(pi)*(Psi(1/2)^2+Psi(1/2,1))/8` | 精确验证 |

## 基础有限区间积分（14 条）

[原题与定义域](../tests/basic-finite-integrals.json) · [实际输出和验证记录](benchmarks/basic-finite-cycle8-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `constant` | `integrate(1,x,0,1)` | `1` | 精确验证 |
| `linear` | `integrate(x,x,0,1)` | `1/2` | 精确验证 |
| `quadratic` | `integrate(x^2,x,0,1)` | `1/3` | 精确验证 |
| `cubic` | `integrate(x^3,x,-1,1)` | `0` | 精确验证 |
| `sparse-polynomial` | `integrate(3*x^10-5*x^3+2,x,0,1)` | `45/44` | 精确验证 |
| `reverse-polynomial` | `integrate(x^2,x,2,-1)` | `-3` | 精确验证 |
| `rational-bounds` | `integrate(x^2+1,x,1/2,3/2)` | `25/12` | 精确验证 |
| `simple-sine` | `integrate(sin(x),x,0,pi)` | `2` | 精确验证 |
| `simple-cosine` | `integrate(cos(x),x,0,pi/2)` | `1` | 精确验证 |
| `exponential` | `integrate(exp(x),x,0,1)` | `exp(1)-1` | 精确验证 |
| `positive-reciprocal` | `integrate(1/x,x,1,2)` | `ln(2)` | 精确验证 |
| `basic-arctan` | `integrate(1/(1+x^2),x,0,1)` | `pi/4` | 精确验证 |
| `basic-root` | `integrate(sqrt(x),x,0,1)` | `2/3` | 精确验证 |
| `basic-log` | `integrate(ln(x),x,1,exp(1))` | `1` | 精确验证 |

## 误差函数与分母对数变体（15 条）

[原题与定义域](../tests/cycle7-tail-mellin.json) · [实际输出和验证记录](benchmarks/cycle7-tail-mellin-cycle8-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C7-tail-mellin-01` | `integrate(erf(x)*erfc(x),x,0,+infinity)` | `(sqrt(2)-1)/sqrt(pi)` | 精确验证 |
| `C7-tail-mellin-02` | `integrate(erf(2*x)*erfc(3*x),x,0,+infinity)` | `(sqrt(13)-3)/(6*sqrt(pi))` | 精确验证 |
| `C7-tail-mellin-03` | `integrate(erf(-2*x)*erfc(3*x),x,0,+infinity)` | `-(sqrt(13)-3)/(6*sqrt(pi))` | 精确验证 |
| `C7-tail-mellin-04` | `integrate(erfc(x)^2,x,0,+infinity)` | `(2-sqrt(2))/sqrt(pi)` | 精确验证 |
| `C7-tail-mellin-05` | `integrate(erfc(2*x)*erfc(3*x),x,0,+infinity)` | `(5-sqrt(13))/(6*sqrt(pi))` | 精确验证 |
| `C7-tail-mellin-06` | `integrate(x*erf(2*x^2)*erfc(3*x^2),x,0,+infinity)` | `(sqrt(13)-3)/(12*sqrt(pi))` | 精确验证 |
| `C7-tail-mellin-07` | `integrate(erf(sqrt(x))*erfc(2*sqrt(x))/sqrt(x),x,0,+infinity)` | `(sqrt(5)-2)/sqrt(pi)` | 精确验证 |
| `C7-tail-mellin-08` | `integrate(ln(1+x^2)/(1+x^2)^(3/2),x,0,+infinity)` | `2-2*ln(2)` | 精确验证 |
| `C7-tail-mellin-09` | `integrate(ln(1+4*x^2)/(1+4*x^2)^(3/2),x,0,+infinity)` | `1-ln(2)` | 精确验证 |
| `C7-tail-mellin-10` | `integrate(x*ln(1+x^2)/(1+x^2)^2,x,0,+infinity)` | `1/2` | 精确验证 |
| `C7-tail-mellin-11` | `integrate(x*ln(1+x^2)^2/(1+x^2)^2,x,0,+infinity)` | `1` | 精确验证 |
| `C7-tail-mellin-12` | `integrate(ln(1+x)/(1+x)^2,x,0,+infinity)` | `1` | 精确验证 |
| `C7-tail-mellin-13` | `integrate(ln(2+2*x)/(2+2*x)^2,x,0,+infinity)` | `(1+ln(2))/4` | 精确验证 |
| `C7-tail-mellin-14` | `integrate(ln(1+x^2)^2/(1+x^2)^(3/2),x,0,+infinity)` | `(2-2*ln(2))^2+4-pi^2/3` | 精确验证 |
| `C7-tail-mellin-15` | `integrate(x*ln(1+x^2)^4/(1+x^2)^2,x,0,+infinity)` | `12` | 精确验证 |

## Gamma 对数矩变体（15 条）

[原题与定义域](../tests/cycle8-gamma-log.json) · [实际输出和验证记录](benchmarks/cycle8-gamma-log-stack-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `C8-gamma-01` | `integrate(exp(-x^2)*ln(x)^2,x,0,+infinity)` | `sqrt(pi)*(Psi(1/2)^2+Psi(1/2,1))/8` | 精确验证 |
| `C8-gamma-02` | `integrate(exp(-x),x,0,+infinity)` | `1` | 精确验证 |
| `C8-gamma-03` | `integrate(exp(-2*x^2),x,0,+infinity)` | `sqrt(pi)/(2*sqrt(2))` | 精确验证 |
| `C8-gamma-04` | `integrate(exp(-x)*ln(x),x,0,+infinity)` | `Psi(1)` | 精确验证 |
| `C8-gamma-05` | `integrate(exp(-x)*ln(x)^2,x,0,+infinity)` | `Psi(1)^2+pi^2/6` | 精确验证 |
| `C8-gamma-06` | `integrate(exp(-x)*ln(x)^3,x,0,+infinity)` | `Psi(1)^3+pi^2*Psi(1)/2-2*Zeta(3)` | 精确验证 |
| `C8-gamma-07` | `integrate(exp(-x)*ln(x)^4,x,0,+infinity)` | `Psi(1)^4+pi^2*Psi(1)^2-8*Psi(1)*Zeta(3)+3*pi^4/20` | 精确验证 |
| `C8-gamma-08` | `integrate(x^3*exp(-x^2)*ln(x),x,0,+infinity)` | `Psi(2)/4` | 精确验证 |
| `C8-gamma-09` | `integrate(exp(-2*x^2)*ln(3*x^2)^2,x,0,+infinity)` | `sqrt(pi)*((Psi(1/2)+ln(3/2))^2+Psi(1/2,1))/(2*sqrt(2))` | 精确验证 |
| `C8-gamma-10` | `integrate(exp(-sqrt(x))*ln(x)^2,x,0,+infinity)` | `8*(Psi(2)^2+Psi(2,1))` | 精确验证 |
| `C8-gamma-11` | `integrate(x^(1/3)*exp(-x^(2/3))*ln(x),x,0,+infinity)` | `9*Psi(2)/4` | 精确验证 |
| `C8-gamma-12` | `integrate(exp(-x^2)*ln(1/x)^3,x,0,+infinity)` | `-sqrt(pi)*(Psi(1/2)^3+3*Psi(1/2)*Psi(1/2,1)+Psi(1/2,2))/16` | 精确验证 |
| `C8-gamma-13` | `integrate(ln(x)^2/exp(2*x),x,0,+infinity)` | `((Psi(1)-ln(2))^2+pi^2/6)/2` | 精确验证 |
| `C8-gamma-14` | `integrate(exp(-x)*ln(x)/3,x,0,+infinity)` | `Psi(1)/3` | 精确验证 |
| `C8-gamma-15` | `integrate(x^15*exp(-x),x,0,+infinity)` | `factorial(15)` | 精确验证 |

## 用户 A1–F6 全模式通过项（34 条）

[原题与定义域](../tests/user-acceptance-passed-dilog.json) · [实际输出和验证记录](benchmarks/user-acceptance-passed-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `A1` | `integrate(sqrt(x^2),x)` | `x*abs(x)/2` | 精确验证 |
| `A2` | `integrate(x/sqrt(x^2),x)` | `abs(x)` | 精确验证 |
| `A3` | `integrate(1/(x*ln(x)),x)` | `ln(abs(ln(x)))` | 精确验证 |
| `A4` | `integrate(sqrt((x-1/2)^2),x,-1,3)` | `17/4` | 精确验证 |
| `A5` | `integrate(sqrt(sin(x)^2),x,0,2*pi)` | `4` | 精确验证 |
| `A6` | `integrate(1/(x^2-1),x)` | `ln(abs((x-1)/(x+1)))/2` | 精确验证 |
| `B1` | `integrate(sin(x)^7/(sin(x)^7+cos(x)^7),x,0,pi/2)` | `pi/4` | 精确验证 |
| `B2` | `integrate(x^5/(x^5+(1-x)^5),x,0,1)` | `1/2` | 精确验证 |
| `B3` | `integrate(x/(sin(x)+cos(x)),x,0,pi/2)` | `pi*ln(1+sqrt(2))/(2*sqrt(2))` | 精确验证 |
| `B4` | `integrate(atan(x)/x,x,0,1)` | `(Psi(1/4,1)-Psi(3/4,1))/16` | 精确验证 |
| `B5` | `integrate(ln(x)/(1+x^2),x,0,1)` | `-((Psi(1/4,1)-Psi(3/4,1))/16)` | 精确验证 |
| `B6` | `integrate(ln(cos(x)),x,0,pi/4)` | `-pi*ln(2)/4+((Psi(1/4,1)-Psi(3/4,1))/16)/2` | 精确验证 |
| `C1` | `[assume(a>0),assume(b>0),integrate((exp(-a*x)-exp(-b*x))/x,x,0,+infinity)][2]` | `ln(b/a)` | 精确验证 |
| `C2` | `[assume(a>0),integrate(exp(-a*x)*sin(b*x)/x,x,0,+infinity)][1]` | `atan(b/a)` | 精确验证 |
| `C3` | `[assume(a>0),integrate(exp(-a*x)*(1-cos(b*x))/x,x,0,+infinity)][1]` | `ln(1+b^2/a^2)/2` | 精确验证 |
| `C4` | `integrate(sin(x)/x,x,0,+infinity)` | `pi/2` | 精确验证 |
| `D1` | `[assume(a>0),integrate(exp(-a*x),x,0,+infinity)][1]` | `1/a` | 精确验证 |
| `D2` | `[assume(s>0),integrate(x^(s-1)*exp(-x),x,0,+infinity)][1]` | `Gamma(s)` | 精确验证 |
| `D3` | `[assume(a>0),assume(b>0),integrate(x^(a-1)*(1-x)^(b-1),x,0,1)][2]` | `Gamma(a)*Gamma(b)/Gamma(a+b)` | 精确验证 |
| `D4` | `[assume(s>0 and s<1),integrate(x^(s-1)/(1+x),x,0,+infinity)][1]` | `pi/sin(pi*s)` | 精确验证 |
| `D5` | `[assume(a>0),integrate(cos(b*x)/(x^2+a^2),x,0,+infinity)][1]` | `pi*exp(-a*abs(b))/(2*a)` | 精确验证 |
| `D6` | `[assume(a>abs(b)),integrate(1/(a+b*cos(x)),x,0,2*pi)][1]` | `2*pi/sqrt(a^2-b^2)` | 精确验证 |
| `E1` | `[assume(r>-1 and r<1),integrate(ln(1-2*r*cos(x)+r^2),x,0,pi)][1]` | `0` | 精确验证 |
| `E2` | `[assume(a>abs(b)),integrate(ln(a+b*cos(x)),x,0,2*pi)][1]` | `2*pi*ln((a+sqrt(a^2-b^2))/2)` | 精确验证 |
| `E3` | `integrate(cos(3*x)/(2+cos(x)),x,0,2*pi)` | `2*pi*(sqrt(3)-2)^3/sqrt(3)` | 精确验证 |
| `E4` | `[assume(r>-1 and r<1),integrate(1/(1-2*r*cos(x)+r^2),x,0,2*pi)][1]` | `2*pi/(1-r^2)` | 精确验证 |
| `E5` | `[assume(r>-1 and r<1),assume(n,integer),additionally(n>=0),integrate(cos(n*x)/(1-2*r*cos(x)+r^2),x,0,2*pi)][3]` | `2*pi*r^n/(1-r^2)` | 精确验证 |
| `E6` | `[assume(r>-1 and r<1),assume(n,integer),additionally(n>=1),integrate(ln(1-2*r*cos(x)+r^2)*cos(n*x),x,0,2*pi)][3]` | `-2*pi*r^n/n` | 精确验证 |
| `F1` | `integrate(exp(-x^2),x)` | `sqrt(pi)*erf(x)/2` | 精确验证 |
| `F2` | `integrate(sin(x^2),x)` | `sqrt(pi/2)*FresnelS(sqrt(2/pi)*x)` | 精确验证 |
| `F3` | `integrate(sin(x)/x,x)` | `Si(x)` | 精确验证 |
| `F4` | `integrate(exp(x)/x,x)` | `Ei(x)` | 精确验证 |
| `F5` | `integrate(ln(1-x)/x,x)` | `-Li2(x)` | 精确验证 |
| `F6` | `integrate(1/ln(x),x)` | `Ei(ln(x))` | 精确验证 |

## 用户前五道未解题（5 条）

[原题与定义域](../tests/user-reported-five-gaps.json) · [实际输出和验证记录](benchmarks/user-reported-five-gaps-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `GAP-TRIG-LOG-QUARTER` | `integrate(ln(3*cos(x)^2+sin(x)^2),x,0,pi/2)` | `pi*ln((sqrt(3)+1)/2)` | 精确验证 |
| `GAP-SYMMETRIC-QUARTIC` | `integrate((1+x^2)/((1-x^2)*sqrt(1+x^4)),x)` | `atanh(sqrt(2)*x/sqrt(1+x^4))/sqrt(2)` | 精确验证 |
| `GAP-ATAN-CIRCLE` | `integrate(atan(x)/(x*sqrt(1-x^2)),x,0,1)` | `pi*ln(1+sqrt(2))/2` | 精确验证 |
| `GAP-OSCILLATORY-FRULLANI` | `integrate((exp(-x)-exp(-3*x))*cos(x)/x,x,0,+infinity)` | `ln(5)/2` | 精确验证 |
| `GAP-TRIG-LOG-HALF` | `integrate(ln(2+cos(x)),x,0,pi)` | `pi*ln((2+sqrt(3))/2)` | 精确验证 |

## 验收错题结构变体（33 条）

[原题与定义域](../tests/user-matrix-next.json) · [实际输出和验证记录](benchmarks/user-matrix-next-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `NEXT-ABS-AFFINE` | `integrate(sqrt((x-1/2)^2),x,-1,3)` | `17/4` | 精确验证 |
| `NEXT-ABS-NEGATIVE-SLOPE` | `integrate(sqrt((2-3*x)^2),x,-1,2)` | `41/6` | 精确验证 |
| `NEXT-ABS-REVERSE` | `integrate(abs(3*x-2),x,2,-1)` | `-41/6` | 精确验证 |
| `NEXT-ABS-SIN` | `integrate(sqrt(sin(x)^2),x,0,2*pi)` | `4` | 精确验证 |
| `NEXT-ABS-COS-SCALED` | `integrate(-3*sqrt(cos(2*x+1)^2),x,0,3*pi)` | `-18` | 精确验证 |
| `NEXT-LOG-COS-QUARTER` | `integrate(ln(cos(x)),x,0,pi/4)` | `-pi*ln(2)/4+((Psi(1/4,1)-Psi(3/4,1))/16)/2` | 精确验证 |
| `NEXT-LOG-SIN-QUARTER` | `integrate(ln(sin(3*x)),x,0,pi/12)` | `-pi*ln(2)/12-((Psi(1/4,1)-Psi(3/4,1))/16)/6` | 精确验证 |
| `NEXT-LOG-ABS-NEGATIVE-WAVE` | `integrate(ln(abs(sin(x))),x,-pi/4,pi/4)` | `-pi*ln(2)/2-((Psi(1/4,1)-Psi(3/4,1))/16)` | 精确验证 |
| `NEXT-LOG-COS-REVERSE` | `integrate(ln(cos(2*x)),x,pi/8,0)` | `pi*ln(2)/8-((Psi(1/4,1)-Psi(3/4,1))/16)/4` | 精确验证 |
| `NEXT-LOG-MEAN-COS` | `integrate(ln(2+cos(x)),x,0,pi)` | `pi*ln((2+sqrt(3))/2)` | 精确验证 |
| `NEXT-LOG-MEAN-SIN` | `integrate(ln(5-3*sin(2*x)),x,0,pi)` | `pi*ln(9/2)` | 精确验证 |
| `NEXT-LOG-MEAN-SQUARES` | `integrate(ln(3*cos(x)^2+sin(x)^2),x,0,pi/2)` | `pi*ln((sqrt(3)+1)/2)` | 精确验证 |
| `NEXT-LOG-MEAN-SQUARES-SCALE` | `integrate(2*ln(4*cos(3*x)^2+9*sin(3*x)^2),x,0,pi/6)` | `2*pi*ln(5/2)/3` | 精确验证 |
| `NEXT-LOG-MEAN-SINGLE-SQUARE` | `integrate(ln(1+3*cos(x)^2),x,0,pi)` | `2*pi*ln(3/2)` | 精确验证 |
| `NEXT-FRULLANI-COS` | `integrate((exp(-x)-exp(-3*x))*cos(x)/x,x,0,+infinity)` | `ln(5)/2` | 精确验证 |
| `NEXT-FRULLANI-COS-ENVELOPE` | `integrate(exp(-2*x)*(exp(-x)-exp(-3*x))*cos(-4*x)/x,x,0,+infinity)` | `ln(41/25)/2` | 精确验证 |
| `NEXT-FRULLANI-COS-MULTI` | `integrate((2*exp(-x)-3*exp(-2*x)+exp(-4*x))*cos(3*x)/x,x,0,+infinity)` | `(-2*ln(10)+3*ln(13)-ln(25))/2` | 精确验证 |
| `NEXT-QUARTIC-1` | `integrate((-1*x^2+-1)/((1*x^2-1)*sqrt(x^4+(0)*x^2+1)),x)` | `-(-1)*atanh(sqrt(2)*x/sqrt(x^4+(0)*x^2+1))/(1*sqrt(2))` | 精确验证 |
| `NEXT-QUARTIC-4` | `integrate((3*x^2+12)/((2*x^2-8)*sqrt(x^4+(1)*x^2+16)),x)` | `-(3)*atanh(sqrt(9)*x/sqrt(x^4+(1)*x^2+16))/(2*sqrt(9))` | 精确验证 |
| `NEXT-QUARTIC-9` | `integrate((-2*x^2+-18)/((3*x^2-27)*sqrt(x^4+(-5)*x^2+81)),x)` | `-(-2)*atanh(sqrt(13)*x/sqrt(x^4+(-5)*x^2+81))/(3*sqrt(13))` | 精确验证 |
| `NEXT-CATALAN-ATAN-CUBIC` | `integrate(atan(8*x^3)/x,x,0,1/2)` | `((Psi(1/4,1)-Psi(3/4,1))/16)/3` | 精确验证 |
| `NEXT-CATALAN-ATAN-NEGATIVE` | `integrate(atan(-4*x^2)/x,x,0,1/2)` | `-((Psi(1/4,1)-Psi(3/4,1))/16)/2` | 精确验证 |
| `NEXT-CATALAN-LOG-CUBIC` | `integrate(x^2*ln(8*x^3)/(1+64*x^6),x,0,1/2)` | `-((Psi(1/4,1)-Psi(3/4,1))/16)/24` | 精确验证 |
| `NEXT-ATAN-CIRCLE` | `integrate(atan(x)/(x*sqrt(1-x^2)),x,0,1)` | `pi*ln(1+sqrt(2))/2` | 精确验证 |
| `NEXT-ATAN-CIRCLE-SCALE` | `integrate(atan(6*x^2)/(x*sqrt(1-16*x^4)),x,0,1/2)` | `pi*asinh(3/2)/4` | 精确验证 |
| `NEXT-ATAN-CIRCLE-NEGATIVE` | `integrate(atan(-3*x)/(x*sqrt(1-4*x^2)),x,0,1/2)` | `-pi*asinh(3/2)/2` | 精确验证 |
| `NEXT-WEIGHTED-SINE-COSINE` | `integrate(x/(sin(x)+cos(x)),x,0,pi/2)` | `pi*ln(1+sqrt(2))/(2*sqrt(2))` | 精确验证 |
| `NEXT-WEIGHTED-SINE-COSINE-SCALE` | `integrate((3*x+2)/(sin(2*x)+cos(2*x)),x,0,pi/4)` | `(3*pi/8+2)*ln(1+sqrt(2))/sqrt(2)` | 精确验证 |
| `NEXT-POISSON-MEAN` | `integrate(ln(1-2*(1/3)*cos(x)+1/9),x,0,pi)` | `0` | 精确验证 |
| `NEXT-COSINE-HARMONIC` | `integrate(cos(3*x)/(2+cos(x)),x,0,2*pi)` | `2*pi*(sqrt(3)-2)^3/sqrt(3)` | 精确验证 |
| `NEXT-POISSON-HARMONIC` | `integrate(cos(5*x)/(1+cos(x)+1/4),x,0,2*pi)` | `-pi/12` | 精确验证 |
| `NEXT-LOG-POISSON-HARMONIC` | `integrate(ln(1+cos(x)+1/4)*cos(3*x),x,0,2*pi)` | `pi/12` | 精确验证 |
| `NEXT-LOG-POISSON-SCALED` | `integrate(ln(1-2*cos(2*x)/3+1/9)*cos(8*x),x,0,pi)` | `-pi/324` | 精确验证 |

## Li2 换元与端点变体（29 条）

[原题与定义域](../tests/dilogarithm-corpus.json) · [实际输出和验证记录](benchmarks/dilogarithm-corpus-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `L1` | `integrate(ln(1+x)/x,x)` | `-Li2(-x)` | 精确验证 |
| `L2` | `integrate(ln(x)/(1-x),x)` | `Li2(1-x)` | 精确验证 |
| `L3` | `integrate(ln(1-exp(-x)),x)` | `Li2(exp(-x))` | 精确验证 |
| `L4` | `integrate(ln(1-x)/(x*(1-x)),x)` | `-Li2(x)-ln(1-x)^2/2` | 精确验证 |
| `L5` | `integrate(ln(1-x)/x,x,0,1)` | `-pi^2/6` | 精确验证 |
| `L6` | `integrate(ln(1+x)/x,x,0,1)` | `pi^2/12` | 精确验证 |
| `Q6` | `integrate(ln(1-x^2)/x,x)` | `-Li2(x^2)/2` | 精确验证 |
| `power-2` | `integrate(ln(1-x^2)/x,x)` | `-Li2(x^2)/2` | 精确验证 |
| `power-plus-2` | `integrate(3*ln(1+2*x^2)/x,x)` | `-3*Li2(-2*x^2)/2` | 精确验证 |
| `endpoint-2` | `integrate(ln(1-x^2)/x,x,0,1)` | `-pi^2/12` | 精确验证 |
| `power-3` | `integrate(ln(1-x^3)/x,x)` | `-Li2(x^3)/3` | 精确验证 |
| `power-plus-3` | `integrate(3*ln(1+2*x^3)/x,x)` | `-3*Li2(-2*x^3)/3` | 精确验证 |
| `endpoint-3` | `integrate(ln(1-x^3)/x,x,0,1)` | `-pi^2/18` | 精确验证 |
| `power-7` | `integrate(ln(1-x^7)/x,x)` | `-Li2(x^7)/7` | 精确验证 |
| `power-plus-7` | `integrate(3*ln(1+2*x^7)/x,x)` | `-3*Li2(-2*x^7)/7` | 精确验证 |
| `endpoint-7` | `integrate(ln(1-x^7)/x,x,0,1)` | `-pi^2/42` | 精确验证 |
| `power-16` | `integrate(ln(1-x^16)/x,x)` | `-Li2(x^16)/16` | 精确验证 |
| `power-plus-16` | `integrate(3*ln(1+2*x^16)/x,x)` | `-3*Li2(-2*x^16)/16` | 精确验证 |
| `endpoint-16` | `integrate(ln(1-x^16)/x,x,0,1)` | `-pi^2/96` | 精确验证 |
| `fractional` | `integrate(ln(1-sqrt(x))/x,x)` | `-2*Li2(sqrt(x))` | 精确验证 |
| `negative-power` | `integrate(ln(1-1/x)/x,x)` | `Li2(1/x)` | 精确验证 |
| `shifted` | `integrate(ln(3-2*x)/(2*x-2),x)` | `-Li2(2*x-2)/2` | 精确验证 |
| `shifted-square` | `integrate(ln(3-2*x)/((2*x-2)*(3-2*x)),x)` | `-Li2(2*x-2)/2-ln(3-2*x)^2/4` | 精确验证 |
| `exponential-scaled` | `integrate(5*ln(1-exp(-3*x)),x)` | `5*Li2(exp(-3*x))/3` | 精确验证 |
| `exponential-plus` | `integrate(ln(1+2*exp(3*x)),x)` | `-Li2(-2*exp(3*x))/3` | 精确验证 |
| `half-endpoint` | `integrate(ln(1-x)/x,x,0,1/2)` | `ln(2)^2/2-pi^2/12` | 精确验证 |
| `reverse-endpoint` | `integrate(ln(1+x)/x,x,1,0)` | `-pi^2/12` | 精确验证 |
| `negative-argument-endpoint` | `integrate(ln(1+2*x)/x,x,0,1)` | `-Li2(-2)` | 精确验证 |
| `finite-exponential` | `integrate(ln(1-exp(-x)),x,0,1)` | `Li2(exp(-1))-pi^2/6` | 精确验证 |

## 三角对数与主值分支（14 条）

[原题与定义域](../tests/trig-log-corpus.json) · [实际输出和验证记录](benchmarks/trig-log-corpus-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `TL1` | `integrate(3*ln((1)*sin((2)*x+(0))),x)` | `(3)*(x*ln(1/2)-im(Li2(exp(2*i*(((2)*x+(0))))))/(2*(2)))+(3)*i*pi*(((((2)*x+(0)))-pi+abs((((2)*x+(0)))-2*pi*floor((((2)*x+(0)))/(2*pi))-pi))/(2*(2)))` | 精确验证 |
| `TL2` | `integrate(1*ln((1)*cos((3)*x+(1))),x)` | `(1)*(x*ln(1/2)-im(Li2(exp(2*i*(((3)*x+(1))+pi/2))))/(2*(3)))+(1)*i*pi*(((((3)*x+(1))+pi/2)-pi+abs((((3)*x+(1))+pi/2)-2*pi*floor((((3)*x+(1))+pi/2)/(2*pi))-pi))/(2*(3)))` | 精确验证 |
| `TL3` | `integrate(1*ln((1)*sin((-2)*x+(1))),x)` | `(1)*(x*ln(1/2)-im(Li2(exp(2*i*(((-2)*x+(1))))))/(2*(-2)))+(1)*i*pi*(((((-2)*x+(1)))-pi+abs((((-2)*x+(1)))-2*pi*floor((((-2)*x+(1)))/(2*pi))-pi))/(2*(-2)))` | 精确验证 |
| `TL4` | `integrate(1*ln(abs((1)*sin((1)*x+(0)))),x)` | `(1)*(x*ln(1/2)-im(Li2(exp(2*i*(((1)*x+(0))))))/(2*(1)))` | 精确验证 |
| `TL5` | `integrate(2*ln(abs((1)*cos((-3)*x+(1)))),x)` | `(2)*(x*ln(1/2)-im(Li2(exp(2*i*(((-3)*x+(1))+pi/2))))/(2*(-3)))` | 精确验证 |
| `TL6` | `integrate(ln(sin(x))+ln(sin(x+pi)),x)` | `-2*x*ln(2)-im(Li2(exp(2*i*x)))+i*pi*x` | 精确验证 |
| `shift-abs-1` | `integrate(2*ln(abs((3)*sin((1)*x+(pi/3)))),x)` | `(2)*(x*ln(3/2)-im(Li2(exp(2*i*(((1)*x+(pi/3))))))/(2*(1)))` | 精确验证 |
| `shift-principal-1` | `integrate(-2*ln((-3)*cos((1)*x+(pi/3))),x)` | `(-2)*(x*ln(3/2)-im(Li2(exp(2*i*(((1)*x+(pi/3))+pi/2+pi))))/(2*(1)))+(-2)*i*pi*(((((1)*x+(pi/3))+pi/2+pi)-pi+abs((((1)*x+(pi/3))+pi/2+pi)-2*pi*floor((((1)*x+(pi/3))+pi/2+pi)/(2*pi))-pi))/(2*(1)))` | 精确验证 |
| `shift-abs--1` | `integrate(2*ln(abs((3)*sin((-1)*x+(-2)))),x)` | `(2)*(x*ln(3/2)-im(Li2(exp(2*i*(((-1)*x+(-2))))))/(2*(-1)))` | 精确验证 |
| `shift-principal--1` | `integrate(-2*ln((-3)*cos((-1)*x+(-2))),x)` | `(-2)*(x*ln(3/2)-im(Li2(exp(2*i*(((-1)*x+(-2))+pi/2+pi))))/(2*(-1)))+(-2)*i*pi*(((((-1)*x+(-2))+pi/2+pi)-pi+abs((((-1)*x+(-2))+pi/2+pi)-2*pi*floor((((-1)*x+(-2))+pi/2+pi)/(2*pi))-pi))/(2*(-1)))` | 精确验证 |
| `shift-abs-7` | `integrate(2*ln(abs((3)*sin((7)*x+(3*pi/2)))),x)` | `(2)*(x*ln(3/2)-im(Li2(exp(2*i*(((7)*x+(3*pi/2))))))/(2*(7)))` | 精确验证 |
| `shift-principal-7` | `integrate(-2*ln((-3)*cos((7)*x+(3*pi/2))),x)` | `(-2)*(x*ln(3/2)-im(Li2(exp(2*i*(((7)*x+(3*pi/2))+pi/2+pi))))/(2*(7)))+(-2)*i*pi*(((((7)*x+(3*pi/2))+pi/2+pi)-pi+abs((((7)*x+(3*pi/2))+pi/2+pi)-2*pi*floor((((7)*x+(3*pi/2))+pi/2+pi)/(2*pi))-pi))/(2*(7)))` | 精确验证 |
| `shift-abs-1/3` | `integrate(2*ln(abs((3)*sin((1/3)*x+(-pi/7)))),x)` | `(2)*(x*ln(3/2)-im(Li2(exp(2*i*(((1/3)*x+(-pi/7))))))/(2*(1/3)))` | 精确验证 |
| `shift-principal-1/3` | `integrate(-2*ln((-3)*cos((1/3)*x+(-pi/7))),x)` | `(-2)*(x*ln(3/2)-im(Li2(exp(2*i*(((1/3)*x+(-pi/7))+pi/2+pi))))/(2*(1/3)))+(-2)*i*pi*(((((1/3)*x+(-pi/7))+pi/2+pi)-pi+abs((((1/3)*x+(-pi/7))+pi/2+pi)-2*pi*floor((((1/3)*x+(-pi/7))+pi/2+pi)/(2*pi))-pi))/(2*(1/3)))` | 精确验证 |

## 混合首轮新增积分（4 条）

[原题与定义域](../tests/mixed-round1-integrals.json) · [实际输出和验证记录](benchmarks/mixed1-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `MR1-I1` | `simplify(integrate(-3*ln(abs(4*sin(-2*x/3+pi/5))),x))` | `-3*x*ln(2)-9/4*im(Li2(exp(2*i*(-2*x/3+pi/5))))` | 精确验证 |
| `MR1-I2` | `simplify(integrate(2*ln(-2*cos(3*x/2-pi/7)),x))` | `-2/3*im(Li2(exp(2*i*(3*x/2+19*pi/14))))+2*i*pi/3*(3*x/2+19*pi/14-pi+abs(3*x/2+19*pi/14-2*pi*floor((3*x/2+19*pi/14)/(2*pi))-pi))` | 精确验证 |
| `MR1-I3` | `simplify(integrate(ln(abs(2*sin(x/2+pi/6)))+ln(abs(2*cos(x/2+pi/6))),x))` | `-1/2*im(Li2(exp(2*i*(x+pi/3))))` | 精确验证 |
| `MR1-I4` | `simplify(integrate(ln(1+x^2)/(x*(1+x^2)),x))` | `-Li2(-x^2)/2-ln(1+x^2)^2/4` | 精确验证 |

## 混合第二轮新增积分（4 条）

[原题与定义域](../tests/mixed-round2-integrals.json) · [实际输出和验证记录](benchmarks/mixed2-integrals-polar12-2026a.json)

| 编号 | 可输入的题目 | 核对参考结果 | 验证 |
| --- | --- | --- | --- |
| `MR2-I1` | `simplify(integrate(2*ln(abs(3*sin(3*x/4+pi/8)))-ln(abs(2*cos(x/2-pi/9))),x))` | `2*x*ln(3/2)-4/3*im(Li2(exp(2*i*(3*x/4+pi/8))))+im(Li2(exp(2*i*(x/2+7*pi/18))))` | 精确验证 |
| `MR2-I2` | `simplify(integrate(ln(-3*sin(2*x/3+pi/7)),x))` | `x*ln(3/2)-3/4*im(Li2(exp(2*i*(2*x/3+pi/7))))+piecewise(sin(2*x/3+pi/7)>0,i*pi*x,0)` | 精确验证 |
| `MR2-I3` | `simplify(integrate((2*x+1)*ln(1+(x^2+x)^2)/(x^2+x),x))` | `-Li2(-(x^2+x)^2)/2` | 精确验证 |
| `MR2-I4` | `simplify(integrate(x*atan(x^2),x))` | `x^2*atan(x^2)/2-ln(1+x^4)/4` | 精确验证 |

---

由 `python3 tests/write-passed-integrals.py` 根据已保存报告生成。用 `--check` 检查文件与报告一致。
