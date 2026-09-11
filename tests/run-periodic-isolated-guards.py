#!/usr/bin/env python3
"""Periodic absolute contacts and actual isolated piecewise values."""
import argparse,json,os,subprocess
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
checks=[
 ('ln(2+x^2)*abs(sin(2*x+1))','-1/2','undef'),
 ('ln(1+(x-2)^2)*abs(sin(x-2))',2,'0'),
 ('ln(1+(x-2)^2)*abs(sin(x-2))','2+pi','undef'),
 ('x*abs(sin(x))',0,'0'),('x*abs(sin(x))','pi','undef'),
 ('(x-1)^2*abs(cos(2*x))','pi/4','undef'),
 ('piecewise(x<0,atan(x),x==0,0,x/(1+x))',0,'1'),
 ('piecewise(x<0,atan(x),x==0,2,x/(1+x))',0,'undef'),
 ('piecewise(x<=0,x,x==0,2,x)',0,'1'),
 ('piecewise(x<0,x,x==0,0,x<=1,x,2*x-1)',1,'undef'),
 ('piecewise(x<0,x^2*sin(1/x),x==0,1,x^2)',0,'undef'),
 ('piecewise(x<0,x^2*sin(1/x),x==0,0,x^2)',0,'0'),
 ('piecewise(x<0,x^2*sin(1/x),x==0,0,x)',0,'undef')]

for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for f,point,expected in checks:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],env=env,capture_output=True,text=True,timeout=12)
   if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result=r.stdout.strip(),exact=True,method='At a simple sine/cosine zero, analytic amplitude zero makes the original quotient tend to zero; nonzero amplitude gives opposite slopes. Piecewise derivatives require both adjacent quotients relative to the actual selected value; shadowed equality branches do not change it. Oscillatory x² sin(1/x)=O(x²) has removable slope zero only if the selected point value is zero.'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
