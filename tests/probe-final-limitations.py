#!/usr/bin/env python3
"""Record unresolved boundaries honestly; these are not passing test cases."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',type=Path,required=True)
p.add_argument('--conic-probe',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args();rows=[]
cases=[
 ('stationary-branch-endpoint',a.probe,'eval(subst(diff(im(Li2(exp(i*x^2))),x),x=0))','Expected derivative 0 by the local x^2*log(abs(x)) behavior; current output is undef. The real-phase identity only promises values off sine zeros.'),
 ('explicit-symbolic-eval',a.probe,'eval(diff(im(Li2(exp(i*(x^2+pi/4)))),x))','Mathematically correct, but an explicit eval of the unassigned symbolic result re-enters the generic abs/Sturm path; host PARI warnings can return.'),
 ('unknown-pivot-display',a.conic_probe,'(a*x^2+x*y-a*y^2=1,[x,y],t)','Unknown-pivot fallback retained. Expression can exceed the 256-node display budget; not counted as complete displayed coverage.'),
]
for id,probe,expression,note in cases:
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(probe),expression],env=env,capture_output=True,text=True,timeout=12)
  rows.append(dict(id=id,input=expression,stack=stack,exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr,status='known-gap-not-passed',note=note))
root=Path(__file__).resolve().parents[1]
a.report.write_text(json.dumps({'scope':'Diagnostic record of remaining gaps; excluded from passed-integral and acceptance totals.','source_sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in ['kconvert.cc','ksubst.cc','yderive.cc','dilogarithm.h']},'runs':rows},indent=2)+'\n')
print('Recorded',len(rows),'unresolved boundary runs; not counted as passes')
