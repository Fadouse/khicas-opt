#!/usr/bin/env python3
"""Affine trig charts must not gain half-angle poles during simplification."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as sp
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
x=sp.Symbol('x',real=True);aa,b=sp.symbols('a b',real=True)
local={'x':x,'a':aa,'b':b,'i':sp.I,'ln':sp.log};rows=[]
expressions=['sin(x)+cos(x)','3*sin(2*x)+5*cos(2*x)+7','sqrt(3)*sin(x)-sqrt(2)*cos(x)','(sin(x)+cos(x))/2','a*sin(x)+b*cos(x)','sin(x)^2+cos(x)^2','2*sin(x)*cos(x)','(sin(x)+cos(x))^2','(1+sin(x))*(1+cos(x))']
for expression in expressions:
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(a.probe),'simplify('+expression+')'],capture_output=True,text=True,env=env,timeout=5)
  row=dict(input=expression,stack=stack,exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
  assert r.returncode==0,row
  got=sp.sympify(row['result'].replace('^','**'),locals=local);want=sp.sympify(expression.replace('^','**'),locals=local)
  assert sp.trigsimp(got-want)==0,row
  for point in [0,sp.pi,-sp.pi,sp.pi/2,-sp.pi/2,3*sp.pi]:assert sp.simplify(got.subs(x,point)-want.subs(x,point))==0,row
  row['pass']=True;rows.append(row)
a.report.write_text(json.dumps({'scope':'Actual FXCG simplify, exact independent identities and exact values at potential half-angle poles. Nonlinear trig identities retained as regressions.','source_sha256':{'ksubst.cc':hashlib.sha256((ROOT/'ksubst.cc').read_bytes()).hexdigest()},'runs':rows},indent=2)+'\n');print('PASS',len(rows),'harmonic/domain checks')
