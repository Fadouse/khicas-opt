#!/usr/bin/env python3
"""Build the reviewable passed-integral list from saved, verified reports."""
import argparse,collections,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SNAPSHOT='checkpoint/polar-stability-cycle5-2026a'
GROUPS=[
 ('极坐标优化第五轮新增积分', 'polar-cycle5-integrals', 'polar-cycle5-integrals-polar5-2026a', 'stack'),
 ('极坐标优化第四轮新增积分', 'polar-cycle4-integrals', 'polar-cycle4-integrals-polar5-2026a', 'stack'),
 ('极坐标优化第三轮新增积分', 'polar-cycle3-integrals', 'polar-cycle3-integrals-polar5-2026a', 'stack'),
 ('极坐标优化第二轮新增积分', 'polar-cycle2-integrals', 'polar-cycle2-integrals-polar5-2026a', 'stack'),
 ('极坐标优化首轮新增积分', 'polar-cycle1-integrals', 'polar-cycle1-integrals-polar5-2026a', 'stack'),
 ('用户原始 8 题', 'user-integrals', 'user-eight-cycle8-polar5-2026a', 'user'),
 ('用户追加 2 题', 'user-extra-integrals', 'user-extra-cycle8-polar5-2026a', 'corpus'),
 ('用户追加 5 题', 'user-challenge-integrals', 'user-challenge-cycle8-polar5-2026a', 'stack'),
 ('MIT / Princeton 题库', 'calculus-corpus', 'calculus-cycle8-polar5-2026a', 'corpus'),
 ('独立泛化题库', 'generalization-corpus', 'generalization-cycle8-polar5-2026a', 'corpus'),
 ('第二轮泛化题库', 'generalization-cycle2', 'cycle2-cycle8-polar5-2026a', 'corpus'),
 ('第三轮泛化题库', 'generalization-cycle3', 'cycle3-cycle8-polar5-2026a', 'corpus'),
 ('第四轮泛化题库', 'generalization-cycle4', 'cycle4-cycle8-polar5-2026a', 'corpus'),
 ('第五轮泛化题库', 'generalization-cycle5', 'cycle5-cycle8-stack-polar5-2026a', 'stack'),
 ('第六轮泛化题库', 'generalization-cycle6', 'cycle6-cycle8-stack-polar5-2026a', 'stack'),
 ('第七轮泛化题库', 'generalization-cycle7', 'cycle7-cycle8-stack-polar5-2026a', 'stack'),
 ('第八轮泛化题库', 'generalization-cycle8', 'cycle8-stack-polar5-2026a', 'stack'),
 ('基础有限区间积分', 'basic-finite-integrals', 'basic-finite-cycle8-polar5-2026a', 'stack'),
 ('误差函数与分母对数变体', 'cycle7-tail-mellin', 'cycle7-tail-mellin-cycle8-stack-polar5-2026a', 'stack'),
 ('Gamma 对数矩变体', 'cycle8-gamma-log', 'cycle8-gamma-log-stack-polar5-2026a', 'stack'),
 ('用户 A1–F6 全模式通过项', 'user-acceptance-passed-dilog', 'user-acceptance-passed-polar5-2026a', 'stack'),
 ('用户前五道未解题', 'user-reported-five-gaps', 'user-reported-five-gaps-polar5-2026a', 'stack'),
 ('验收错题结构变体', 'user-matrix-next', 'user-matrix-next-polar5-2026a', 'stack'),
 ('Li2 换元与端点变体', 'dilogarithm-corpus', 'dilogarithm-corpus-polar5-2026a', 'stack'),
 ('三角对数与主值分支', 'trig-log-corpus', 'trig-log-corpus-polar5-2026a', 'stack'),
 ('混合首轮新增积分', 'mixed-round1-integrals', 'mixed1-integrals-polar5-2026a', 'stack'),
 ('混合第二轮新增积分', 'mixed-round2-integrals', 'mixed2-integrals-polar5-2026a', 'stack'),
]

LABELS={'exact':'精确验证','sampled':'导数采样通过','numeric_constant':'数值常量检查通过'}
def code(value):
 return '`'+str(value).replace('|','&#124;').replace('`','&#96;').replace('\n',' ')+'`'
def generate():
 groups=[];totals=collections.Counter();seen=[];source_hashes=None
 for title,corpus_name,report_name,kind in GROUPS:
  cp=ROOT/'tests'/f'{corpus_name}.json';rp=ROOT/'docs/benchmarks'/f'{report_name}.json'
  data=json.loads(cp.read_text());cases=data if isinstance(data,list) else data['cases']
  report=json.loads(rp.read_text())
  if 'corpus_sha256' in report:assert hashlib.sha256(cp.read_bytes()).hexdigest()==report['corpus_sha256'],corpus_name
  if source_hashes is None:source_hashes=report['source_sha256']
  for name,digest in report['source_sha256'].items():
   if name in source_hashes:assert source_hashes[name]==digest,(report_name,name)
  if kind=='user':
   rows={str(r['id']):r for r in report['cases']}
  elif kind=='corpus':rows={str(r['id']):r for r in report['runs']['current']}
  else:
   rows={}
   for r in report['runs']:rows.setdefault(str(r['id']),[]).append(r)
  assert set(rows)=={str(c['id']) for c in cases},corpus_name
  output=[]
  for c in cases:
   r=rows[str(c['id'])]
   if kind=='user':
    assert all(t['status']=='exact' for t in r['runs']);assert r['stack_64KiB']['exit']==0
    assert r['stack_64KiB']['result']==r['runs'][0]['result'];status='exact'
   elif kind=='stack':
    assert len(r)==4 and all(t['status']=='exact' for t in r);status='exact'
   else:status=r['status'];assert status in LABELS,(corpus_name,c['id'],status)
   totals[status]+=1
   expression=c.get('input') or 'integrate('+c['f']+',x'+('' if 'bounds' not in c else ','+','.join(c['bounds']))+')'
   reference=c.get('reference',c.get('expected'))
   assert reference is not None
   seen.append(''.join(expression.split()))
   output.append('| '+ ' | '.join((code(c['id']),code(expression),code(reference),LABELS[status]))+' |')
  groups.append((title,corpus_name,report_name,output))
 text=['# 已通过的积分题目列表','',
  f'验证源码：`{SNAPSHOT}`。',
  f'正式题库与用户题目共 **{sum(totals.values())} 条通过记录**：**{totals["exact"]} 条精确验证、{totals["sampled"]} 条导数采样通过、{totals["numeric_constant"]} 条数值常量检查通过**。',
  f'保留各题库的编号和重复项；仅去除输入空白后有 {len(set(seen))} 种输入文本，这不是数学意义的去重。','',
  '“核对参考结果”来自已验证题库，用于阅读和比对，并不保证与计算器实际打印的写法相同。',
  '不定积分省略积分常数；实根、对数和反三角函数须遵守原题定义域。采样检查不能代替完整符号证明。',
  '主机使用仓库积分、归一化和 FXCG 化简入口；这些通过记录不代表 CG50 实机耗时或实机全部通过。','',
  '本表列出完整的正式积分题库；其他算法参数变体、拒绝非法输入、方程转换和崩溃保护回归另见 [当前验收报告](USER-ACCEPTANCE-MATRIX.md)。',
  '安全保留未求出的积分不计入本表。后续新题只有完成验证后才应追加。','']
 for title,cn,rn,rows in groups:
  text.extend([f'## {title}（{len(rows)} 条）','',f'[原题与定义域](../tests/{cn}.json) · [实际输出和验证记录](benchmarks/{rn}.json)','',
   '| 编号 | 可输入的题目 | 核对参考结果 | 验证 |','| --- | --- | --- | --- |',*rows,''])
 text.extend(['---','', '由 `python3 tests/write-passed-integrals.py` 根据已保存报告生成。用 `--check` 检查文件与报告一致。',''])
 return '\n'.join(text)
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
 expected=generate();target=ROOT/'docs/PASSED-INTEGRALS.md'
 if args.check:assert target.read_text()==expected,'Regenerate docs/PASSED-INTEGRALS.md'
 else:target.write_text(expected)
 print('PASS: passed-integral Markdown matches all corpus reports')
