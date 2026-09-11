#!/usr/bin/env python3
"""Global inverse-sine chart identities and inherited finite-product domains."""
import argparse,subprocess,os,json,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',required=True);a=p.parse_args();rows=[]
cases=[]
for wave in ['sin(x)','cos(x)','sin(2*x+1)','cos(x^2)']:
 for scale in ['1','-1','1/2','-2/3']:
  t=f'({scale})*{wave}';e=f'diff(asin(2*({t})/(1+({t})^2)),x)';T=parse(t)
  cases.append((e,2*s.diff(T,x)/(1+T*T),{},'Principal range asin(2t/(1+t²))=2atan(t), |t|<=1 everywhere.'))
for wave,points in [('sin(x)',{'pi/2':'0','-pi/2':'0',0:'2'}),('cos(x)',{0:'0','pi':'0','pi/2':'-2'})]:
 t=wave;cases.append((f'diff(asin(2*{t}/(1+{t}^2)),x)',2*s.diff(parse(t),x)/(1+parse(t)**2),points,'Original contact quotient is O(h), so the finite derivative is zero.'))
for n in [2,5,12]:
 for c in [1,2]:
  e=f'product((({c}*x)^2-k^2)/(({c}*x)^2-(k+1)^2),k,1,{n})'
  target=((c*x)**2-1)/((c*x)**2-(n+1)**2)
  points={str(s.Rational(sign*j,c)):'undef' for j in range(2,n+1) for sign in [-1,1]};points.update({'0':str(s.Rational(1,(n+1)**2))})
  cases.append((e,target,points,'Finite exact factor telescope retains every canceled ±j/c hole; origin allowed.'))
for e,target,points,method in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   expression=f'simplify({e})' if outer else e;env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   r=subprocess.run([a.probe,expression],capture_output=True,text=True,env=env,timeout=10);row=dict(input=expression,stack=stack,exit=r.returncode,result=r.stdout.strip())
   try:
    assert r.returncode==0 and len(r.stdout)<65536
    assert equal(parse(r.stdout.strip()),target)
    for point,expected in points.items():
     q=subprocess.run([a.probe,f'eval(subst({expression},x={point}))'],capture_output=True,text=True,env=env,timeout=10)
     if expected=='undef':assert q.returncode==3 and q.stdout.strip()=='undef',(point,q.stdout)
     else:assert q.returncode==0 and equal(parse(q.stdout.strip()),parse(expected)),(point,q.stdout)
    row.update(passed=True,proof=method,actual_points=points)
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row)
Path(a.report).write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
