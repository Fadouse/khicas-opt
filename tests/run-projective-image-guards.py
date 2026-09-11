#!/usr/bin/env python3
"""Finite-parameter image checks for rational circles, hyperbola and parabola."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import sympy as s
from polar_cycle21_reference import parse,polar_tree,A,R,T
from mixed_reference import equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--polar-probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
X,Y=s.symbols('X Y',real=True);u=s.Symbol('t',real=True)
cases=[
 ('vertical','[a*t/(1+t^2),a*t^2/(1+t^2)]',X*X+Y*Y-A*Y,(0,A),A),
 ('shifted-parameter','[(a-2)*t/(1+t^2),(a-2)*t^2/(1+t^2)]',X*X+Y*Y-(A-2)*Y,(0,A-2),A-2),
 ('horizontal','[a*t^2/(1+t^2),a*t/(1+t^2)]',X*X+Y*Y-A*X,(A,0),A),
 ('circle','[a*(1-t^2)/(1+t^2),2*a*t/(1+t^2)]',X*X+Y*Y-A*A,(-A,0),A),
 ('hyperbola','[(1+t^2)/(1-t^2),2*t/(1-t^2)]',X*X-Y*Y-1,(-1,0),None),
 ('parabola','[t,t^2]',Y-X*X,None,None)]
rows=[]
def gs(e):return str(e).replace('**','^')
for name,pair,equation,hole,scale in cases:
 # Independent full-image certificates: circle inverses are t=x/(a-y),
 # t=y/(a-x), or t=y/(a+x), after using the stated scale. Hyperbola
 # inverse t=y/(x+1); parabola inverse t=x. Denominator zeros on the
 # conic are exactly the stated omitted points, and the scaled a=0
 # parametrizations collapse to the origin. These give surjectivity,
 # rather than relying on the finite numeric checks alone.
 for stack in ('normal','64'):
  for outer in (False,True):
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
   expression=f'({pair},t,[r,theta])';row=dict(case=name,input=expression,stack=stack,outer=outer)
   try:
    r=subprocess.run([a.polar_probe,'param',expression],env=env,capture_output=True,text=True,timeout=10)
    row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr);assert r.returncode==0,row
    raw=next(v[4:] for v in r.stdout.splitlines() if v.startswith('RAW '));assert len(raw)<4096
    checked=[]
    # Values include both signs, degeneration and shifted degeneration.
    for aa in ([-2,0,2] if scale is not None else [1]):
     for rr,tt in [(0,0),(0,s.pi/2),(1,0),(1,s.pi/2),(2,0),(2,s.pi/2),(2,s.pi),(2,3*s.pi/2),(s.sqrt(2),s.pi/4)]:
      xx=s.simplify(rr*s.cos(tt));yy=s.simplify(rr*s.sin(tt));sub={A:aa,X:xx,Y:yy}
      degenerate=scale is not None and scale.subs(A,aa)==0
      expected=(xx==0 and yy==0) if degenerate else s.simplify(equation.subs(sub))==0
      if expected and not degenerate and hole is not None:expected=not (s.sympify(hole[0]).subs(A,aa)==xx and s.sympify(hole[1]).subs(A,aa)==yy)
      req=f'eval(subst({raw},[a,r,theta],[{aa},{gs(rr)},{gs(tt)}]))'
      q=subprocess.run([a.probe,req],env=env,capture_output=True,text=True,timeout=10)
      actual=q.stdout.strip();assert q.returncode in (0,3),(req,q.returncode,q.stdout,q.stderr)
      # A forbidden point may be explicitly undefined or false. Neither
      # may masquerade as a true equation. Unexpected symbolic answers fail.
      if actual=='undef':truth=False
      else:
       relation=polar_tree(actual)
       assert isinstance(relation,s.Equality),(req,actual)
       # The equation is the product output. Compare its substituted
       # residual exactly; Giac's generic boolean == uses representation
       # equality, which is not an oracle for mathematical equations.
       truth=equal(relation.lhs,relation.rhs)
      assert truth==bool(expected),(name,aa,rr,tt,actual,expected)
      checked.append(dict(a=aa,r=str(rr),theta=str(tt),expected=bool(expected),actual=actual))
    row.update(passed=True,points=checked)
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row);a.report.write_text(json.dumps(dict(scope='Actual conversion output and actual lazy substitution and independently proved equation residuals, with independent explicit inverse-map certificates for complete finite-parameter conic images. Covers the specified examples; not a proof for arbitrary rational maps or previously canceled input holes.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),polar_probe_sha256=hashlib.sha256(Path(a.polar_probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
   print(name,stack,outer,row['passed'],row.get('error','')[:180],flush=True)
assert all(r['passed'] for r in rows)
