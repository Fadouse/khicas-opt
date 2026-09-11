#!/usr/bin/env python3
"""Independent changed-rule variants, including stationary logarithmic chains."""
import argparse,json,os,subprocess
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
cases=[
 ('(1-x^2)*ln((sqrt(x^4+3*x^2+1)+x)/(1+x^2))/((1+x^2)*sqrt(x^4+3*x^2+1))','ln((sqrt(x^4+3*x^2+1)+x)/(1+x^2))^2/2'),
 ('2*ln(x+sqrt(x^2+9))/sqrt(x^2+9)','ln(x+sqrt(x^2+9))^2'),
 ('sqrt((x-2)/(5-x))/(x+1)','2*atan(sqrt((x-2)/(5-x)))-sqrt(2)*atan(sqrt(2)*sqrt((x-2)/(5-x)))'),
 ('sqrt((2*x+1)/(3-x))/(x+4)','2*atan(sqrt((2*x+1)/(3-x))/sqrt(2))*sqrt(2)-2*atan(sqrt((2*x+1)/(3-x)))')]
# Verify expected answers independently before comparing any engine result.
u=s.Symbol('u',positive=True)
for idx,(f,F) in enumerate(cases):
 if idx<2:assert equal(s.diff(parse(F),x),parse(f))
 else:
  inverse=(5*u*u+2)/(1+u*u) if idx==2 else (3*u*u-1)/(2+u*u)
  transformed=s.simplify(parse(F).subs(x,inverse))
  target=s.simplify(parse(f).subs(x,inverse)*s.diff(inverse,u))
  assert equal(s.diff(transformed,u),target)
# The last two primitives follow x=(f*u²-b)/(a-d*u²),
# dx=2(ad-bd_num)*u/(a-d*u²)² du, then two positive quadratic partial fractions.
for stack in ['normal','64']:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 for idx,(f,F) in enumerate(cases):
  for outer in [False,True]:
   e='integrate('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,e],capture_output=True,text=True,env=env,timeout=12)
   assert r.returncode==0,(e,r.stdout,r.stderr)
   result=parse(r.stdout.strip());expected=parse(F)
   assert equal(result,expected),(e,result,expected)
   rows.append(dict(input=e,stack=stack,result=r.stdout.strip(),exact=True,method='Exact primitive identity and independent logarithmic-chain/positive-ratio substitution. No denominator of the derivative of the substitution appears in the returned primitive.'))
 for f,point in [('piecewise(x<2,0,sqrt(x-2))',2),('piecewise(x<=3,2-sqrt(3-x),2)',3)]:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   r=subprocess.run([a.probe,'eval(subst('+e+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=12)
   assert r.returncode==3 and r.stdout.strip()=='undef',(e,r.stdout,r.stderr)
   rows.append(dict(input=e,stack=stack,point=point,result='undef',exact=True,method='Original square-root branch has infinite one-sided difference quotient; a finite derivative does not exist.'))
a.report.write_text(json.dumps(dict(runs=rows),indent=2)+'\n');print(len(rows),'passed')
