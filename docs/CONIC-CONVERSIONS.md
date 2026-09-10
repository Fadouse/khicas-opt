# 圆锥曲线转换与化简定义域

源码 checkpoint：`checkpoint/conic-coverage-2026a`。

26道题通过104次检查：直接转换／外层 `simplify` × 普通栈／64 KiB栈。独立符号验证既检查回代恒等式，也证明参数图覆盖整个实曲线。不能用几个采样点或回代成功替代分支覆盖证明。

## 转换行为

二次曲线分类现在优先于一般有理参数搜索。圆、椭圆返回完整的正弦/余弦参数式；双曲线返回覆盖两支的双曲函数式，矩形双曲线使用较短的指数式。新参数图的参数遍历全体实数，无需排除有限参数点。平移和旋转通过中心与精确正交特征向量处理，避免反正切的象限问题。退化曲线分别返回相交线、平行线、重线、单点或空集合。

例如 `cart2param(x^2+y^2=1,[x,y],t)` 显示 `[[x(t)=cos(t),y(t)=sin(t)]]`。`x^2-y^2=1` 返回两支，分别是 `[cosh(t),sinh(t)]` 与 `[-cosh(t),sinh(t)]`。

短符号系数先检查符号和可能的零因子。`a^2*(x^2+y^2-1)=0` 在未知a时不能直接约去a²；a=0时原式表示整个平面。本轮16次守卫检查覆盖零因子、坐标/参数重名和非曲线输入。更复杂的未决参数条件仍可能被保守拒绝，尚未实现任意参数的完整分类讨论。

## 化简和显示

发现并修复三角化简引入新极点的问题：原来 `simplify(2*sin(x)*cos(x))` 可能转成含 `tan(x)` 的商，在原式正常的π/2处失效；旋转椭圆也可能因半角正切丢失参数点。新规则保留紧凑的仿射正弦/余弦组合，并限制会引入新正切函数的重写。18次独立恒等式及潜在极点检查通过。

显示识别 `simplify`、`normal`、`ratnormal` 和赋值包装，保留自定义坐标、参数名称。标签只增加有限包装节点，共享原坐标表达式，不复制子树。移除标签阶段重复的整树扫描；渲染器自身256节点保护保持不变。本题集最大带标签结果181节点。超大表达式仍可能触发显示资源保护。

## 实际输出

下表保存普通栈、无外层化简的原始坐标列表；界面另加 `x(t)=…`、`y(t)=…` 标签。符号参数条件以原题JSON为准。

| ID | 隐式方程 | 分支数 | 实际坐标列表 |
| --- | --- | --- | --- |
| CQ1 | `(x-2)^2+(y+3)^2=25` | 1 | `[[5*cos(tau)+2,5*sin(tau)-3]]` |
| CQ2 | `4*(x+y-1)^2+9*(x-y-3)^2=72` | 1 | `[[(2*sqrt(2)*cos(s)+3*sqrt(2)*sin(s)+4)/2,(-2*sqrt(2)*cos(s)+3*sqrt(2)*sin(s)-2)/2]]` |
| CQ3 | `9*(x+1)^2-4*(y-2)^2=36` | 2 | `[[2*cosh(t)-1,3*sinh(t)+2],[-2*cosh(t)-1,3*sinh(t)+2]]` |
| CQ4 | `(x+y-2)*(x-y)=4` | 2 | `[[2*cosh(u)+1,2*sinh(u)+1],[-2*cosh(u)+1,2*sinh(u)+1]]` |
| CQ5 | `(x-4)^2+9*(y+1)^2=0` | 1 | `[[4,-1]]` |
| CQ6 | `(x-1)^2-(y+2)^2=0` | 2 | `[[v+1,v-2],[v+1,-v-2]]` |
| unit-circle | `x^2+y^2=1` | 1 | `[[cos(t),sin(t)]]` |
| origin-circle | `x^2+y^2=2*x` | 1 | `[[cos(t)+1,sin(t)]]` |
| axis-ellipse | `x^2/4+y^2/9=1` | 1 | `[[2*cos(t),3*sin(t)]]` |
| vertical-hyperbola | `(y-1)^2/4-(x+2)^2/9=1` | 2 | `[[3*sinh(t)-2,2*cosh(t)+1],[3*sinh(t)-2,-2*cosh(t)+1]]` |
| rotated-hyperbola | `3*x^2+10*x*y+3*y^2=1` | 2 | `[[(2*cosh(t)-2*2*sinh(t))/8,(2*cosh(t)+2*2*sinh(t))/8],[(-2*cosh(t)-2*2*sinh(t))/8,(-2*cosh(t)+2*2*sinh(t))/8]]` |
| rectangular-hyperbola | `x*y=1` | 2 | `[[exp(t),exp(-t)],[-exp(t),-exp(-t)]]` |
| negative-product | `(x-3)*(y+1)=-4` | 2 | `[[3+2*exp(t),-1-2*exp(-t)],[3-2*exp(t),-1+2*exp(-t)]]` |
| rotated-parabola | `(x+y)^2=2*(x-y)` | 1 | `[[(t^2+4*t+3)/4,(-t^2+1)/4]]` |
| shifted-rotated-parabola | `(2*x-y+3)^2=4*(x+2*y-1)` | 1 | `[[(4*t^2+20*t-11)/20,(4*t^2+9)/10]]` |
| parallel-lines | `(x+y)^2=4` | 2 | `[[-t+2,t],[-t-2,t]]` |
| double-line | `(2*x-y+3)^2=0` | 1 | `[[(t-3)/2,t]]` |
| vertical-lines | `x^2=4` | 2 | `[[2,t],[-2,t]]` |
| axis-point | `x^2+9*y^2=0` | 1 | `[[0,0]]` |
| empty-ellipse | `x^2+y^2+1=0` | 0 | `[]` |
| empty-parallel | `(x+y)^2+1=0` | 0 | `[]` |
| negative-scaled-circle | `-7*((x-2)^2+(y+3)^2-25)=0` | 1 | `[[5*cos(t)+2,5*sin(t)-3]]` |
| mixed-eigen-ellipse | `3*x^2+2*x*y+5*y^2=1` | 1 | `[[(-sqrt((1+2/(2*sqrt(2)))/2)*sqrt(-1/(sqrt(2)-4))*sqrt(sqrt(2)+4)*sin(t)+sqrt((1-2/(2*sqrt(2)))/2)*cos(t))/sqrt(sqrt(2)+4),(sqrt((1+2/(2*sqrt(2)))/2)*cos(t)+sqrt((1-2/(2*sqrt(2)))/2)*sqrt(-1/(sqrt(2)-4))*sqrt(sqrt(2)+4)*sin(t))/sqrt(sqrt(2)+4)]]` |
| irrational-center-eigen | `3*x^2+2*x*y+5*y^2+7*x-2*y-1=0` | 1 | `[[(-28*sqrt((1+2/(2*sqrt(2)))/2)*sqrt(-341/(56*sqrt(2)-224))*sin(t)+28*sqrt((1-2/(2*sqrt(2)))/2)*sqrt(341/(56*sqrt(2)+224))*cos(t)-37)/28,(28*sqrt((1+2/(2*sqrt(2)))/2)*sqrt(341/(56*sqrt(2)+224))*cos(t)+28*sqrt((1-2/(2*sqrt(2)))/2)*sqrt(-341/(56*sqrt(2)-224))*sin(t)+13)/28]]` |
| symbolic-radius | `x^2+y^2=a^2` | 1 | `[[a*cos(t),a*sin(t)]]` |
| symbolic-axes | `x^2/a^2+y^2/9=1` | 1 | `[[a*cos(t),3*sin(t)]]` |

[26道原题和条件](../tests/conic-corpus.json) · [104次输出、覆盖证明与标签检查](benchmarks/conic-coverage-2026a.json) · [16次参数守卫检查](benchmarks/conic-guards-2026a.json) · [18次化简定义域检查](benchmarks/simplify-harmonic-domain-2026a.json)。

此前显式函数、直线、抛物线、一般有理曲线、尖点、笛卡尔叶形线和伯努利双纽线保留回归。旧转换40次栈检查及三个扩展转换套件通过；显示包装、表达式共享和多分支检查通过。[旧转换记录](benchmarks/conic-legacy-stack-2026a.json) · [扩展转换记录](benchmarks/conic-extended-2026a.txt) · [显示记录](benchmarks/conic-display-2026a.txt)。

## 积分回归与容量

当前源码重新通过21项正式回归任务，用户A1–F6仍为36/36：34个精确积分答案、2个正确发散拒绝。独立积分清单共633条来源记录，628条精确验证、5条导数采样通过，保留重复来源及验证等级。未求出的积分不计为成功。

SH4实际链接结果：ROM2059444/2065152 B，余5708 B；静态RAM424620/442368 B，余17748 B；AC2为2463508/2559996 B，余96488 B。相对前一checkpoint，ROM增加768 B、AC2增加7112 B；CAS堆1572864 B和静态RAM不变。积分辅助方法及转换入口/新增辅助方法已检查位于AC2。仍有代码余量，不能声称已经达到设备绝对上限。

主机测试运行仓库积分及FXCG化简代码，依赖主机Giac；64 KiB栈不是SH4/MMU模拟。此checkpoint尚未安装或在CG50验收，不能据此宣称实机TLB问题全部消除，或把主机耗时当作实机提速。一般无理旋转参数式仍可能较长，大导数化简和未知参数条件是后续工作。

[完整积分列表](PASSED-INTEGRALS.md) · [用户验收](USER-ACCEPTANCE-MATRIX.md) · [全部回归及源码哈希](benchmarks/conic-verification-2026a.json) · [容量与单函数栈帧](benchmarks/resources-conic-2026a.json)。
