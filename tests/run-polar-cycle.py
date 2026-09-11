#!/usr/bin/env python3
"""Run one immutable question batch in four modes, with independent proofs."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from polar_cycle_reference import verify
p=argparse.ArgumentParser()
p.add_argument('--probe',type=Path,required=True)
p.add_argument('--corpus',type=Path,default=Path('tests/polar-cycle1-questions.json'))
p.add_argument('--report',type=Path,required=True)
p.add_argument('--saved-runs',type=Path)
a=p.parse_args();cases=json.loads(a.corpus.read_text())['cases'];rows=[];cache={}
saved=json.loads(a.saved_runs.read_text())['runs'] if a.saved_runs else []
report=dict(scope='Actual repository integration/yderive/FXCG simplify on host dependencies; guarded 64 KiB stack, not SH4/MMU emulation.',corpus_sha256=hashlib.sha256(a.corpus.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),runs=rows)
if not saved:report['source_sha256']={n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in ['yintg.cc','zintgab.cc','ksubst.cc','yderive.cc','dilogarithm.h','kconvert.cc','equation_normalize.h']}
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   expression=c['input'];expression=expression[9:-1] if expression.startswith('simplify(') else expression
   if outer:expression='simplify('+expression+')'
   row=dict(id=c['id'],outer=outer,stack=stack,input=expression)
   try:
    if saved:
     row.update(next(r for r in saved if r['id']==c['id'] and r['outer']==outer and r['stack']==stack))
    else:
     env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
     if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
     r=subprocess.run([str(a.probe),expression],capture_output=True,text=True,env=env,timeout=12)
     row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert row['exit']==0,(row['exit'],row['result'][:400])
    key=(c['id'],row['result'])
    if key not in cache:cache[key]=verify(c,row['result'])
    row['verification']=cache[key];row['pass']=True
    if c['id']=='PC1-D1' and not saved:
     for point in [-1,1]:
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=12)
      assert r.returncode==0 and r.stdout.strip()=='0',(point,r.stdout,r.stderr)
     row['actual_endpoint_substitution']='Both -1 and 1 evaluate to 0'
   except Exception as e:row.update(error=str(e),**{'pass':False})
   rows.append(row);a.report.write_text(json.dumps(report,indent=2)+'\n')
   print(c['id'],outer,stack,row['pass'],row.get('error','')[:200],flush=True)
report['cases']=[dict(id=c['id'],all_four_pass=all(r['pass'] for r in rows if r['id']==c['id'])) for c in cases]
a.report.write_text(json.dumps(report,indent=2)+'\n')
assert all(r['pass'] for r in rows)
