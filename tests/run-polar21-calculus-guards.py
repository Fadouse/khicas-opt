#!/usr/bin/env python3
"""Broader real-domain and numerical checks for PC21 calculus changes."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import sympy as s
import mpmath as mp
from polar_cycle21_reference import parse,lazy,x,A,B
from mixed_reference import equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
rows=[];mp.mp.dps=55

def gs(e):return str(e).replace('**','^').replace('Abs(','abs(').replace('Max(','max(').replace('Min(','min(')
def run(e,stack):
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 return subprocess.run([a.probe,e],capture_output=True,text=True,env=env,timeout=10)
def save():
 a.report.write_text(json.dumps(dict(scope='Independent symbolic derivatives, original contact/domain rules, and high-precision elliptic references; normal/64 KiB host tests. Numerical agreement is not a device-accuracy claim.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
def check(e,proof,kind,expected_undef=False):
 for stack in ('normal','64'):
  row=dict(kind=kind,input=e,stack=stack)
  try:
   r=run(e,stack);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
   assert r.returncode==(3 if expected_undef else 0),row
   if expected_undef:assert row['result']=='undef'
   else:proof(parse(row['result']))
   row['passed']=True
  except Exception as ex:row.update(passed=False,error=str(ex))
  rows.append(row);save()
  if not row['passed']:print('FAIL',row,flush=True)
def same(expected):
 def verify(actual):assert equal(actual,expected),(actual,expected)
 return verify
# Elliptic F, parameter m (not modulus). Cross period boundaries, signs,
# near-zero amplitudes, parameters near 1 and a large negative parameter.
for m in [-1000,-2,-1,0,s.Rational(1,2),s.Rational(999999,1000000)]:
 for phi in [-10,-s.pi/2,-s.Rational(1,100000),0,s.Rational(1,3),s.pi/2,4,10]:
  pm=mp.mpf(str(s.N(m,50)));pp=mp.mpf(str(s.N(phi,50)));ref=mp.ellipf(pp,pm)
  def close(v,ref=ref):
   assert not v.has(s.elliptic_f)
   val=mp.mpf(str(s.N(v,45)))
   assert abs(val-ref)<=mp.mpf('3e-11')*(1+abs(ref)),(val,ref)
  check(f'evalf(EllipticF({gs(phi)},{gs(m)}))',close,'elliptic-numeric')
# Rational shifts/scales of the whole admissible quartic family.
for scale,alpha,beta,center in [(1,1,-1,0),(2,2,-1,1),(3,s.Rational(3,2),-s.Rational(1,3),-2),(1,3,-2,s.Rational(1,2))]:
 scale,alpha,beta,center=map(s.sympify,(scale,alpha,beta,center));u=x-center
 Q=scale*(1-alpha*u*u)*(1-beta*u*u)
 expected=s.elliptic_f(s.asin(s.sqrt(alpha)*u),beta/alpha)/s.sqrt(scale*alpha)
 for outer in (False,True):
  e=f'integrate(1/sqrt({gs(Q)}),x)';e='simplify('+e+')' if outer else e
  check(e,same(expected),'elliptic-quartic')
# Contact theorem for max/min of two C1 polynomials: equal slopes are
# necessary and sufficient at equality, including tangencies and merged roots.
for fun in (s.Max,s.Min):
 for f,g in [(x*x,A*x),((x-1)**2,0),(x**3,-x**3),(x*x,(x-1)**2),(x**4,x*x)]:
  f=s.sympify(f);g=s.sympify(g);e=f'simplify(diff({gs(fun(f,g,evaluate=False))},x))'
  for aa in [-2,0,2]:
   for xx in [-2,0,1,2]:
    sub={x:s.Integer(xx),A:s.Integer(aa)};v1=f.subs(sub);v2=g.subs(sub);d1=s.diff(f,x).subs(sub);d2=s.diff(g,x).subs(sub)
    bad=v1==v2 and d1!=d2
    expected=d1 if (v1==v2 or (v1>v2 if fun==s.Max else v1<v2)) else d2
    check(f'eval(subst({e},[x,a],[{xx},{aa}]))',same(expected),'minmax-contact',bad)
# H²*L: check an isolated H=0 point outside the half-line, an interior
# cusp, the common-root 3/2 contact, both slopes and a nontrivial affine H.
for H,L in [(x,x-1),(x,x+1),(x,x),(2*x-1,3-2*x),(x+2,-x-3)]:
 H=s.sympify(H);L=s.sympify(L);e=f'simplify(diff(sqrt({gs(H*H*L)}),x))'
 for xx in [-4,-3,-2,-1,0,s.Rational(1,2),1,s.Rational(3,2),2,3]:
  hh=H.subs(x,xx);ll=L.subs(x,xx)
  bad=ll<0 or (ll==0 and hh!=0) or (ll>0 and hh==0)
  expected=s.Integer(0) if ll==0 else s.diff(H,x)*s.sign(hh)*s.sqrt(ll)+s.Abs(hh)*s.diff(L,x)/(2*s.sqrt(ll))
  check(f'eval(subst({e},x,{gs(xx)}))',same(expected),'squared-affine-domain',bad)
# Clamp order identities, including polynomial thresholds and outer scale.
for left,right,variable,c in [(A,B,x,1),(A*A,B+1,2*x-3,-2),(x*x,x*x+1,A,3),(A,A,B,1)]:
 e=c*(s.Max(left,s.Min(variable,right,evaluate=False),evaluate=False)-s.Min(right,s.Max(variable,left,evaluate=False),evaluate=False))
 check('simplify('+gs(e)+')',same(c*(left-right+s.Abs(left-right))/2),'clamp-order')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows)
