#!/usr/bin/env python3
"""Independent real-root identities and lazy conditional boundary checks."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import sympy as s
from mixed_reference import parse,x,local,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
rows=[]
def run(e,stack):
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 r=subprocess.run([a.probe,e],capture_output=True,text=True,env=env,timeout=12)
 assert r.returncode==0,(e,r.stdout,r.stderr)
 return r.stdout.strip()
checks=[('when(x==0,0,1/x)','((x=0)? 0 : 1/x)'),('piecewise(x==0,0,1/x)','piecewise(x=0,0,1/x)'),('when(x==0,0,1/x,7)','7'),('when(0==0,3,1/0)','3'),('piecewise(1==0,1/0,2==2,7,1/0)','7'),('x==0','false'),('when(x==x,7,1/0)','7')]
for f in ['piecewise(x==0,0,1/x)','piecewise(x<0,-x,x==0,0,1/x)']:
 for point,expected in [(0,'0'),(2,'1/2'),(-2,'2' if '<' in f else '-1/2')]:
  checks.append(('eval(subst('+f+',x='+str(point)+'))',expected))
for stack in ['normal','64']:
 for e,expected in checks:
  out=run(e,stack);assert out==expected,(e,out,expected)
  rows.append(dict(input=e,stack=stack,result=out,exact=True))
 root=s.Function('RealRoot');local['surd']=root;u=s.Symbol('u',real=True)
 for n,c,d,A,B,K in [(3,1,0,1,1,1),(5,3,-1,2,3,2),(7,1,0,-1,2,1),(3,1,0,1,0,1)]:
  arg=c*x+d;f=f'{K}/({A}*surd({s.sstr(arg)}, {n})+{B})'
  for outer in [False,True]:
   e='integrate('+f+',x)';e='simplify('+e+')' if outer else e
   out=run(e,stack);actual=parse(out).xreplace({root(arg,n):u})
   assert not actual.has(x),out
   # On either side of the only real pole, differentiate log magnitude
   # with its local sign. x=(u**n-d)/c is a real bijection for odd n.
   target=s.Rational(K*n,c)*u**(n-1)/(A*u+B)
   for sign in [-1,1]:
    branch=actual.replace(s.Abs,lambda v:sign*v)
    assert equal(s.diff(branch,u),target),(out,branch,target)
   rows.append(dict(input=e,stack=stack,result=out,exact=True,method='Exact real-root substitution and both local logarithm signs'))
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),scope='Actual scalar probe; condition substitution is lazy, odd-root integrals independently differentiated in a real bijective variable.',runs=rows),indent=2)+'\n')
print('PASS',len(rows),'boundary and root checks')
