#!/usr/bin/env python3
"""Independent rational derivative proofs for global trig-power primitives."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[];cache={};t=s.Symbol('t',real=True);z=s.Symbol('z',real=True)
cases=[(2,1,'sin',n) for n in [1,2,3,4,8]]+[(3,-2,'cos',n) for n in [2,3,6]]
for A,B,w,n in cases:
 wave=s.sin(x) if w=='sin' else s.cos(x);f=f'1/({A}+({B})*{w}(x))^{n}'
 for outer in [False,True]:
  e=f'integrate({f},x)';e='simplify('+e+')' if outer else e
  for stack in ['normal','64']:
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=12);assert r.returncode==0,(e,r.stdout,r.stderr)
   printed=r.stdout.strip()
   if printed not in cache:
    actual=parse(printed);assert not actual.has(s.tan) and 'integrate(' not in printed
    delta=s.diff(actual,x)-(A+B*wave)**(-n)
    rational=delta.subs({s.sin(x):2*t/(1+t*t),s.cos(x):(1-t*t)/(1+t*t)},simultaneous=True)
    assert s.cancel(rational)==0,(e,printed)
    # Each variable denominator is a nonzero affine polynomial in the
    # selected sine/cosine. This proves continuity at the chart point the
    # independent rational substitution omits, and on the whole real line.
    for power in actual.atoms(s.Pow):
     if power.exp.is_negative and power.base.has(x):
      base=power.base.subs(wave,z);P=s.Poly(base,z);assert P.degree()<=1 and not base.has(x)
      lo=s.simplify(base.subs(z,-1));hi=s.simplify(base.subs(z,1))
      assert (lo.is_positive and hi.is_positive) or (lo.is_negative and hi.is_negative),(e,base)
    cache[printed]=True
   rows.append(dict(input=e,stack=stack,result=printed,exact=True,method='Independent exact differentiation followed by rational unit-circle substitution proves the derivative identity. Every displayed variable denominator is affine in the selected sine/cosine and has same strict sign at ±1, proving global continuity and no half-angle chart holes.'))
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n');print(len(rows),'passed')
