#!/usr/bin/env python3
"""Run all integral corpora on one frozen build; other acceptance scripts use its probe."""
import sys,runpy,hashlib,json,argparse
from pathlib import Path
sys.path.insert(0,str(Path.cwd()/'tests'))
import integration_build as ib
root=ib.ROOT
names=['yintg.cc','zintgab.cc','ysym2poly.cc','ksubst.cc','integration_guard.h','equation_normalize.h','integration_probe.cc','dilogarithm.h','static_lexer.h','static_lexer_.h','static_extern.h','usual.h']
def hashes():return {n:hashlib.sha256((root/('tests' if n=='integration_probe.cc' else '')/n).read_bytes()).hexdigest() for n in names}
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--summary',type=Path,required=True);options=p.parse_args()
frozen=hashes();builddir=options.build_dir;target=ib.build(builddir/'target',target_simplify=True);validator=ib.build_validation_probe(builddir/'validator')
def cached_build(directory,ref='current',target_simplify=False):
 assert ref=='current' and target_simplify and hashes()==frozen
 return target
ib.build=cached_build
ib.build_validation_probe=lambda directory:validator
jobs=[]
for corpus,report in [('calculus-corpus','calculus-cycle8'),('generalization-corpus','generalization-cycle8'),('generalization-cycle2','cycle2-cycle8'),('generalization-cycle3','cycle3-cycle8'),('generalization-cycle4','cycle4-cycle8'),('user-extra-integrals','user-extra-cycle8')]:
 jobs.append(('run-calculus.py',['--target-simplify','--corpus','tests/'+corpus+'.json','--report','docs/benchmarks/'+report+'-2026a.json']))
for corpus,report in [('generalization-cycle5','cycle5-cycle8-stack'),('generalization-cycle6','cycle6-cycle8-stack'),('generalization-cycle7','cycle7-cycle8-stack'),('generalization-cycle8','cycle8-stack'),('user-challenge-integrals','user-challenge-cycle8'),('basic-finite-integrals','basic-finite-cycle8'),('cycle7-tail-mellin','cycle7-tail-mellin-cycle8-stack'),('cycle8-gamma-log','cycle8-gamma-log-stack')]:
 jobs.append(('run-user-challenge.py',['--corpus','tests/'+corpus+'.json','--report','docs/benchmarks/'+report+'-2026a.json']))
jobs.append(('run-user-integrals.py',['--target-simplify','--report','docs/benchmarks/user-eight-cycle8-2026a.json']))
jobs.append(('run-simplify-safety.py',['--probe',str(target),'--report','docs/benchmarks/simplify-safety-cycle8-2026a.json']))
jobs.append(('run-simplify-resources.py',['--build-dir',str(builddir/'target'),'--report','docs/benchmarks/simplify-resources-cycle8-2026a.json']))
for _,args in jobs:
 for i,arg in enumerate(args):
  if arg.startswith('docs/benchmarks/') and arg.endswith('-2026a.json'):
   args[i]=arg.replace('-2026a.json','-dilog-2026a.json')
jobs += [('run-user-challenge.py',['--corpus','tests/user-matrix-next.json','--report','docs/benchmarks/user-matrix-next-dilog-2026a.json']),('run-user-challenge.py',['--corpus','tests/user-reported-five-gaps.json','--report','docs/benchmarks/user-reported-five-gaps-dilog-2026a.json'])]
jobs.append(('run-user-challenge.py',['--corpus','tests/dilogarithm-corpus.json','--report','docs/benchmarks/dilogarithm-corpus-2026a.json']))
summary=[]
for script,args in jobs:
 assert hashes()==frozen,'Production changed during final suite'
 sys.argv=[str(root/'tests'/script)]+args
 print('RUN',script,*args,flush=True)
 code=0
 try:runpy.run_path(sys.argv[0],run_name='__main__')
 except SystemExit as e:code=e.code or 0
 summary.append({'script':script,'args':args,'exit':code})
 options.summary.write_text(json.dumps({'sources':frozen,'jobs':summary},indent=2)+'\n')
assert hashes()==frozen
assert all(not r['exit'] for r in summary),summary
