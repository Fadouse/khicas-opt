#!/usr/bin/env python3
"""Exact finite-domain and constant-condition specializations."""
import argparse,json,os,subprocess,hashlib
from fractions import Fraction
from pathlib import Path
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
conditions=[
 ('when(cos((-sqrt(2*pi))^2)=1,7,1/0)','7'),
 ('when(cos((-sqrt(2*pi))^2)=1,7,1/0,99)','7'),
 ('piecewise(cos((-sqrt(2*pi))^2)=1,7,1/0)','7'),
 ('when(cos((-sqrt(pi))^2)=-1,5,1/0)','5'),
 ('when(cos((-sqrt(pi))^2)=1,1/0,9)','9'),
 ('when(cos(1/10000)=1,0,1)','1')]
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 def run(e,expected,method):
  r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=10)
  if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
  else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(str(expected))),(e,r.stdout,r.stderr)
  rows.append(dict(input=e,stack=stack,result=r.stdout.strip(),exact=True,method=method))
 for e,answer in conditions:run(e,answer,'Exact principal-root square and integer-pi cosine identities. Only the selected branch is evaluated, including four-argument when; a small nonzero rational angle is not equal to zero by floating tolerance.')
 for scale,shift,lo,hi,num in [(1,0,0,0,1),(2,3,-2,4,3),(-3,-1,1,8,2),(1,2,0,31,1)]:
  u=f'({scale}*k+x+({shift}))';f=f'sum({num}/({u}*({u}+({scale}))),k,{lo},{hi})'
  for outer in [False,True]:
   e='simplify('+f+')' if outer else f
   points={Fraction(-shift-scale*j) for j in range(lo,hi+2)}
   for point in sorted(points):run('eval(subst('+e+',x='+str(point)+'))','undef','Union of original adjacent affine denominator zeros, checked exhaustively for this finite sum. Canceled interior poles are not assigned continuation values.')
   for point in [min(points)-Fraction(1,2),max(points)+Fraction(1,2),min(points)+Fraction(1,3)]:
    expected=sum(Fraction(num,1)/(scale*j+point+shift)/(scale*(j+1)+point+shift) for j in range(lo,hi+1))
    run('eval(subst('+e+',x='+str(point)+'))',str(expected),'Exact rational specialization of the original finite summands, including allowed nonintegers between holes. General telescoping identity is proved separately; no floating numerical check is used.')
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n');print(len(rows),'passed')
