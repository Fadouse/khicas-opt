#!/usr/bin/env python3
"""Whole-interval and pole checks for affine weights over sin/cos squared."""
import argparse,json,subprocess,os,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import parse,equal,x
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
def gs(e):return str(e).replace('**','^').replace('Abs(','abs(').replace('log(','ln(')
cases=[]
for wave in (s.sin,s.cos):
 for P,k,b in [(x,1,s.Integer(0)),(3*x-2,-2,s.pi/3),(-2*x+1,s.Rational(3,2),-s.pi/2)]:
  u=k*x+b;w=wave(u);F=(-P*s.cos(u)/(k*w) if wave==s.sin else P*s.sin(u)/(k*w))+s.diff(P,x)*s.log(s.Abs(w))/(k*k)
  f=P/(w*w);cases.append(dict(kind='primitive',input=f'integrate({gs(f)},x)',f=f,F=F))
  intervals=[(s.pi/4,s.pi/2),(5*s.pi/4,3*s.pi/2)] if wave==s.sin else [(s.Integer(0),s.pi/4),(s.pi,5*s.pi/4)]
  for L,H in intervals:
   lo=(L-b)/k;hi=(H-b)/k
   expected=s.simplify(F.subs(x,hi)-F.subs(x,lo))
   cases.append(dict(kind='finite',input=f'integrate({gs(f)},x,{gs(lo)},{gs(hi)})',expected=expected))
   if P==x:cases.append(dict(kind='finite',input=f'integrate({gs(f)},x,{gs(hi)},{gs(lo)})',expected=-expected))
for e,value in [('x/sin(x)^2,x,-pi/4,pi/4','undef'),('x/sin(x)^2,x,0,pi/4','+infinity'),('-x/sin(x)^2,x,0,pi/4','-infinity'),('x/cos(x)^2,x,pi/4,3*pi/4','+infinity'),('(x-pi/2)/cos(x)^2,x,pi/4,3*pi/4','undef'),('(x-1)/sin(x-1)^2,x,0,2','undef')]:
 cases.append(dict(kind='pole',input='integrate('+e+')',expected=value))
rows=[]
for index,c in enumerate(cases):
 for stack in ('normal','64'):
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  for outer in (False,True):
   e='simplify('+c['input']+')' if outer else c['input'];row=dict(case=index,kind=c['kind'],stack=stack,outer=outer,input=e)
   try:
    r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=10)
    row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert r.returncode==(3 if c.get('expected')=='undef' else 0),row
    if c['kind']=='pole':assert row['result'].lstrip('+')==c['expected'].lstrip('+'),row
    else:
     assert not any(t in r.stdout for t in ('integrate(','rootof('))
     assert 'Warning' not in r.stderr,'A proved regular interval should not enter PARI/range search'
     f=parse(row['result'])
     if c['kind']=='finite':assert equal(f,c['expected']),(f,c['expected'])
     else:
      assert equal(f,c['F']),(f,c['F'])
      # d ln|wave| = wave'/wave on each nonzero real interval.
      d=f.replace(lambda z:z.func==s.log and z.args[0].func==s.Abs,lambda z:s.log(z.args[0].args[0]))
      assert s.simplify(s.trigsimp(s.diff(d,x)-c['f'],method='fu'))==0
    row['passed']=True
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row);a.report.write_text(json.dumps(dict(scope='Exact original primitive identities and finite integrals; ordinary improper pole rejection. Normal and guarded 64 KiB host. Warning-free checks apply to proved regular intervals, not unimplemented inputs.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
   print(index,c['kind'],stack,outer,row['passed'],row.get('error','')[:180],flush=True)
assert all(r['passed'] for r in rows)
