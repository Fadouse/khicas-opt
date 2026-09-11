#!/usr/bin/env python3
"""Verify original difference-quotient cases of bounded oscillatory joins."""
import argparse,subprocess,os,json
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
checks=[
 ('piecewise(x<2,(x-2)*cos(1/(x-2)),0)',2,'undef'),
 ('piecewise(x<2,(x-2)^2*sin(1/(x-2)),(x-2)^2)',2,'0'),
 ('piecewise(x<=2,(x-2)^2*sin(1/(x-2)),(x-2)^2)',2,'undef'),
 ('piecewise(x<0,x*sin(1/x^2),x^2)',0,'undef'),
 ('piecewise(x<0,x^2*cos(1/x^2),0)',0,'0'),
 ('piecewise(x<0,x^2*sin(1/x)+x,x)',0,'1'),
 ('piecewise(x<0,sin(1/x),0)',0,'undef'),
 ('surd((x-2)^3*(x+1)^2,3)+x^2',2,'4+9^(1/3)'),
 ('surd((x-2)^5*(x+1)^2,5)',2,'9^(1/5)')]
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for f,point,expected in checks:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],env=env,capture_output=True,text=True,timeout=12)
   if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result=r.stdout.strip(),exact=True,method='Original difference quotient: analytic amplitude double zero times bounded oscillation is o(h), simple zero oscillates, nonzero amplitude is discontinuous. Rational phase poles supply subsequences with sine/cosine values ±1. Exact odd-power extraction gives the root endpoints.'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
