#!/usr/bin/env python3
"""Positive log wedges and globally nonvanishing trigonometric atan charts."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,local,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[];local['y']=s.Symbol('y',real=True)
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 def run(e,undefined=False):
  r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=12)
  assert r.returncode==(3 if undefined else 0),(e,r.returncode,r.stdout,r.stderr)
  return r.stdout.strip()
 for shift in [-2,0,3]:
  u=f'(x+({shift}))';f=f'ln(sqrt({u}^2-y^2)/({u}-y))-(ln({u}+y)-ln({u}-y))/2'
  result=run('simplify('+f+')');assert parse(result)==parse(f)
  assert run('eval(subst(simplify('+f+'),[x,y],['+str(-shift)+',0]))',True)=='undef'
  rows.append(dict(input=f,stack=stack,result=result,exact=True,method='Original separate real logs require u+y>0,u-y>0. On exactly u>|y|, sqrt((u+y)(u-y))/(u-y)>0 and the identity is zero; retained nodes exclude both boundary rays. Simultaneous specialization verifies the excluded vertex.'))
 for k in [2,3,5]:
  d=s.sqrt(k*k-1);f=f'(x-2*atan(sin(x)/({k}+sqrt({k*k-1})+cos(x))))/sqrt({k*k-1})'
  result=run('simplify('+f+')');assert equal(parse(result),parse(f))
  for sign in [-1,1]:
   point=f'{sign}*acos(-{k}+sqrt({k*k-1}))'
   value=parse(run('evalf(subst(simplify('+f+'),x='+point+'))'));assert value.is_real and value.is_finite
  rows.append(dict(input=f,stack=stack,result=result,exact=True,method='c=k+sqrt(k²-1)>1, so c+cos x>=c-1>0. Exact retained global atan chart; its derivative is 1/(k+cos x) by c²+1=2kc. Additional finite evaluations at both conjugate-zero candidates check actual specialization, not the identity proof.'))
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n');print(len(rows),'passed')
