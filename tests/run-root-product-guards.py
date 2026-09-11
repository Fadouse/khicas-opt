#!/usr/bin/env python3
"""Original-domain endpoints for analytic amplitudes multiplying real roots."""
import argparse,json,os,subprocess
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
checks=[
 ('(exp(x)-1)*surd(x*(x-1),3)',0,'0'),('(exp(x)-1)*surd(x*(x-1),3)',1,'undef'),
 ('ln(1+x)*surd(x*(x-2),5)',0,'0'),('ln(1+x)*surd(x*(x-2),5)',2,'undef'),
 ('(x-1)*sqrt((x-1)*(x+2))',1,'0'),('(x-1)*sqrt((x-1)*(x+2))',-2,'undef'),
 ('(exp(x)-1)*surd(x^2,3)',0,'0'),('2*sqrt(x^2)',0,'undef'),('x*sqrt(x^2)',0,'0'),
 ('sin(x)*surd(x,3)',0,'0'),('atan(x)*surd(x,3)',0,'0'),('cos(x)*surd(x,3)',0,'undef')]
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for f,point,expected in checks:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],env=env,capture_output=True,text=True,timeout=12)
   if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result=r.stdout.strip(),exact=True,method='Analytic amplitude with a zero gives O(h)*O(|h|^alpha)=o(h), alpha>0. Nonzero amplitude retains the odd-root cusp, absolute-value corner, or infinite one-sided square-root slope. Real domains and radian trig convention are retained.'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
