#!/usr/bin/env python3
"""Principal periodic inverse-trig powers and stationary phase contacts."""
import argparse,json,os,subprocess
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
checks=[
 ('acos(cos(x))^2',0,'0'),('acos(cos(x))^2','2*pi','0'),('acos(cos(x))^2','-3*pi','undef'),
 ('acos(cos(2*x+pi/3))^3','-pi/6','0'),('acos(cos(2*x+pi/3))^3','pi/3','undef'),
 ('acos(cos(-3*x))^4',0,'0'),('acos(cos(-3*x))^4','pi/3','undef'),
 ('acos(cos(x))',0,'undef'),('acos(cos(x))','pi','undef'),
 ('acos(cos(x^2))',0,'0'),('acos(cos(pi+x^2))^2',0,'0'),
 ('acos(cos(x^2))^2','sqrt(pi)','undef'),('acos(cos(x^2))^2','sqrt(2*pi)','0'),
 ('acos(cos(x))^2','pi/2','pi'),('acos(cos(x))^2','3*pi/2','-pi')]

for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for f,point,expected in checks:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],env=env,capture_output=True,text=True,timeout=12)
   if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result=r.stdout.strip(),exact=True,method='For polynomial real phase phi, stationary phi gives delta phi=O(h²), so every periodic contact contributes derivative zero. Nonstationary even contacts behave as |phi-principal-center|^m: finite zero for m>1, a cusp for m=1. Odd contacts have nonzero height pi and opposite slopes for every positive integer power.'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
