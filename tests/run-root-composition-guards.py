#!/usr/bin/env python3
"""Exact rational composition derivatives and principal-root cut values."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal,local
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
Root=s.Function('RealRoot');local['surd']=Root;u=s.Symbol('u',real=True)
def run(e,stack):
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=12)
 assert r.returncode==0,(e,r.stdout,r.stderr)
 return r.stdout.strip()
for stack in ['normal','64']:
 for n,m,k,power,A,B,c,d,C in [(3,3,1,1,2,1,1,0,1),(3,5,2,2,1,2,2,1,3),(5,6,2,2,2,3,3,-1,2),(3,4,3,1,1,1,1,0,1),(5,5,3,1,2,-1,2,-3,1)]:
  root='surd('+s.sstr(c*x+d)+','+str(n)+')'
  f=f'{C}*{root}^{m}/({A}+{B}*{root}^{k})^{power}'
  expected=c*s.diff(C*u**m/(A+B*u**k)**power,u)/(n*u**(n-1))
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   out=run(e,stack);v=parse(out).xreplace({Root(c*x+d,n):u}).subs(x,(u**n-d)/c)
   assert equal(v,expected),(e,out,v,expected)
   point=s.Rational(-d,c);value=s.Rational(C*c,A**power) if m==n else s.Integer(0)
   actual=run('eval(subst('+e+',x='+s.sstr(point)+'))',stack)
   assert equal(parse(actual),value),(e,point,actual,value)
   rows.append(dict(input=e,stack=stack,result=out,endpoint=str(point),endpoint_value=actual,exact=True,method='Exact rational derivative in odd-root variable; at moving zero F(h)/h tends to C*c/A^p if m=n and 0 if m>n. Original denominator zeros remain excluded.'))
 for sign in [-1,1]:
  e=f'sqrt((ln(x)+{sign}*i*pi)^2)-ln(x)-{sign}*i*pi'
  for outer in [False,True]:
   expr='simplify('+e+')' if outer else e;out=run(expr,stack)
   assert equal(parse(out),parse(e))
   endpoint=run('eval(subst('+expr+',x=1))',stack)
   assert equal(parse(endpoint),s.Integer(0) if sign==1 else 2*s.I*s.pi)
   rows.append(dict(input=expr,stack=stack,result=out,endpoint_value=endpoint,exact=True,method='Principal-root syntax preserved; sqrt(-pi²)=i*pi at x=1, with both upper/lower imaginary offsets checked.'))
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print('PASS',len(rows),'root-composition and cut checks')
