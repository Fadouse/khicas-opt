#!/usr/bin/env python3
"""Actual polar equations: explicit radius, real branches and degeneracies."""
import argparse,json,os,subprocess
from pathlib import Path
import sympy as s
from mixed_reference import parse
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
t=s.Symbol('theta');r=s.Symbol('r');c=s.cos(t);sn=s.sin(t)
cases=[
 ('circle','cart','(x^2+y^2=4,[x,y],[r,theta])',[s.Integer(2)]),
 ('ellipse','cart','(x^2/4+y^2/9=1,[x,y],[r,theta])',[6/s.sqrt(9*c*c+4*sn*sn)]),
 ('hyperbola','cart','(x^2-y^2=1,[x,y],[r,theta])',[1/s.sqrt(c*c-sn*sn)]),
 ('lemniscate','cart','((x^2+y^2)^2=4*(x^2-y^2),[x,y],[r,theta])',[2*s.sqrt(s.cos(2*t))]),
 ('rotated-lemniscate','cart','((x^2+y^2)^2=-4*(x^2-y^2),[x,y],[r,theta])',[2*s.sqrt(-s.cos(2*t))]),
 ('shifted-circle','cart','((x-2)^2+y^2=1,[x,y],[r,theta])',[(4*c+s.sqrt(16*c*c-12))/2,(4*c-s.sqrt(16*c*c-12))/2]),
 ('origin-circle','cart','(x^2+y^2=4*x,[x,y],[r,theta])',[4*c]),
 ('line','cart','(y=x+1,[x,y],[r,theta])',[-1/(c-sn)]),
 ('empty-circle','cart','(x^2+y^2=-1,[x,y],[r,theta])',[]),
 ('point','cart','(x^2+y^2=0,[x,y],[r,theta])',[s.Integer(0)]),
 ('parametric-circle','param','([2*cos(t),2*sin(t)],t,[r,theta])',[s.Integer(2)]),
 ('origin-line','cart','(y=x,[x,y],[r,theta])',None),
 ('unknown-factor','cart','(a*(x^2+y^2)=4*a,[x,y],[r,theta])',None),
 ('degenerate-quadratic','cart','(x^2=0,[x,y],[r,theta])',None),
 ('shifted-parabola','cart','(y=x^2+1,[x,y],[r,theta])',None),
]
def radius(text):
 # Extract equations from a list without splitting function arguments.
 if text=='[]':return []
 if text.startswith('['):text=text[1:-1]
 pieces=[];depth=0;start=0
 for j,ch in enumerate(text):
  depth+=(ch=='(')-(ch==')')
  if ch==',' and depth==0:pieces.append(text[start:j]);start=j+1
 pieces.append(text[start:])
 out=[]
 for item in pieces:
  left,right=item.split('=',1);assert left=='r',item
  out.append(parse(right))
 return out
rows=[]
for name,mode,expr,expected in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   env=dict(os.environ);env.pop('KHICAS_OUTER_SIMPLIFY',None);env.pop('KHICAS_TEST_STACK_KIB',None)
   if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   run=subprocess.run([a.probe,mode,expr],env=env,capture_output=True,text=True,timeout=12)
   row=dict(id=name,outer=outer,stack=stack,exit=run.returncode,stdout=run.stdout,stderr=run.stderr)
   try:
    assert run.returncode==0
    fields=dict(line.split(' ',1) for line in run.stdout.strip().splitlines())
    assert int(fields['NODES'])<=128
    if expected is not None:
     actual=radius(fields['RAW']);assert len(actual)==len(expected)
     for aa,bb in zip(actual,expected):assert s.trigsimp(aa-bb)==0,(aa,bb)
    else:
     left,right=fields['RAW'].split('=',1);residual=parse(left)-parse(right)
     if name=='origin-line':
      assert s.trigsimp(residual.subs(r,0))==0 and s.trigsimp(residual.subs(t,s.pi/4))==0
     elif name=='unknown-factor':assert s.simplify(residual.subs(s.Symbol('a'),0))==0
     elif name=='degenerate-quadratic':assert s.trigsimp(residual.subs(t,s.pi/2))==0
     else:
      # At cos(theta)=0 the radial equation becomes linear. Preserve both
      # directions, including the point (0,1), rather than divide by cos².
      assert s.trigsimp(residual.subs({t:s.pi/2,r:1}))==0
    row['pass']=True
   except Exception as e:row.update(error=str(e),**{'pass':False})
   rows.append(row);print(name,outer,stack,row['pass'],row.get('error',''),flush=True)
   a.report.write_text(json.dumps(dict(runs=rows,proof='Centered H is pi-periodic, so the nonnegative radius represents every real Cartesian point. Definite shifted quadratics retain both roots; H never zero. Nondefinite shifted curves, origin lines, and unknown vanishing factors retain implicit equations. All square-root charts restrict radicand>=0 and denominators!=0.'),indent=2)+'\n')
assert all(row['pass'] for row in rows)
