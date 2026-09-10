#!/usr/bin/env python3
"""Preserve regular points of inverse-angle sums through FXCG simplify."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as sp
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
x=sp.Symbol('x',real=True);local={'x':x,'i':sp.I,'sign':sp.sign,'ln':sp.log}
expressions=['atan(x)+atan(2*x)','atan(x)+atan(2*x)+atan(3*x)+atan(4*x)','atan(x)+atan(2*x)+atan(3*x+1)','atan(x)^2+atan(2*x)','3*atan(x)-2*atan(2*x)','(1+x)*(atan(x)+atan(2*x))/(1+x)','atan(x)+atan(1/(x-1))','atan(x)+atan(2*x)+sin(x)^2+cos(x)^2']
rows=[]
for e in expressions:
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(a.probe),'simplify('+e+')'],env=env,capture_output=True,text=True,timeout=5)
  row=dict(input=e,stack=stack,exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
  assert r.returncode==0,row
  got=sp.sympify(row['result'].replace('^','**'),locals=local);want=sp.sympify(e.replace('^','**'),locals=local)
  assert sp.simplify(got-want)==0,row
  # Exact values at former artificial poles, including zero and shifted zero.
  for point in [0,sp.Rational(-1,3),sp.Rational(1,3),2]:
   assert sp.simplify(got.subs(x,point)-want.subs(x,point))==0,row
  row['pass']=True;rows.append(row)
# Preserve the ordinary real reciprocal identity, with its existing x!=0.
for expression,expected in [('simplify(atan(x)+atan(1/x))','pi*sign(x)/2'),('[assume(x,complex),simplify(atan(x)+atan(1/x))][1]','atan(x)+atan(1/x)')]:
 r=subprocess.run([str(a.probe),expression],capture_output=True,text=True,timeout=5)
 assert r.returncode==0 and sp.simplify(sp.sympify(r.stdout.replace('^','**'),locals=local)-sp.sympify(expected,locals=local))==0,(expression,r.stdout)
 rows.append(dict(input=expression,result=r.stdout.strip(),exit=r.returncode,**{'pass':True}))
a.report.write_text(json.dumps({'scope':'Actual repository FXCG simplify and independent symbolic equality, including regular points newly made undefined by the old inverse-angle rewrite. Real reciprocal identity and complex guard separately checked.','source_sha256':{'ksubst.cc':hashlib.sha256((ROOT/'ksubst.cc').read_bytes()).hexdigest()},'runs':rows},indent=2)+'\n')
print('PASS',len(rows),'angle/domain checks')
