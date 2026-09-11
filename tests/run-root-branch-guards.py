#!/usr/bin/env python3
"""Exact endpoint checks for root power cancellation and transcendental joins."""
import argparse,json,os,subprocess
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
checks=[
 ('diff(surd((x-1)^4,3),x)',1,'0'),
 ('diff(surd(x^5,3),x)',0,'0'),
 ('diff(surd(x^3*(x-1)^5,3),x)',1,'0'),
 ('diff(surd(x^3*(x-1)^4,3)/(1+x^2)+sin(x),x)',1,'cos(1)'),
 ('diff(piecewise(x<0,exp(x)-1,ln(1+x)),x)',0,'1'),
 ('diff(piecewise(x<=1,ln(1+x),ln(2)+exp(x-1)),x)',1,'undef'),
 ('diff(piecewise(x<=1,ln(1+x),ln(2)+exp(x-1)-1),x)',1,'undef'),
 ('diff(piecewise(x<=1,ln(1+x),ln(2)+(exp(x-1)-1)/2),x)',1,'1/2'),
 ('sqrt((x+i)/(x-i))',0,'i'),
 ('sqrt((x-2+i)/(x-2-i))',2,'i'),
 ('sqrt(x+i*x)',0,'0')]
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for f,point,expected in checks:
  for outer in [False,True]:
   e='simplify('+f+')' if outer else f
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],env=env,capture_output=True,text=True,timeout=12)
   if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result=r.stdout.strip(),exact=True,method='Root power exponent >1 gives a zero original difference quotient. Analytic exponential/logarithm branches require equality of both values and slopes; common log(2) cancels exactly. Principal complex roots at the cut are evaluated on the original radicand (-1 or 0).'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
