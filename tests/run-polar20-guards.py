#!/usr/bin/env python3
"""Independent related families and boundary tests for checkpoint 20."""
import argparse,json,subprocess,os,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,equal
from polar_cycle20_reference import parse,regular
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
y,z=s.symbols('y z');ap,bp=s.symbols('a b')
def gs(e):return str(e).replace('**','^').replace('Abs(','abs(').replace('log(','ln(')
def call(e,env):
 r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=10)
 assert len(r.stdout.encode())<=65536
 return r

def specialize(e,sub):
 if e.func!=s.Piecewise:return e.subs(sub)
 pairs=[]
 for value,condition in e.args:
  c=s.simplify(condition.subs(sub)) if condition!=True else s.true
  if c==s.false:continue
  v=specialize(value,sub);pairs.append((v,c))
  if c==s.true:break
 return s.Piecewise(*pairs,evaluate=False)

cases=[]
for Q in [x*x+2*ap*x+1,ap*x*x+x+1,ap*x*x+bp*x+1,(x+ap)**2+bp,ap*x*x+ap*x+ap]:
 cases.append(dict(kind='quadratic',Q=Q,input=f'integrate(1/({gs(Q)}),x)'))
for aa,bb,R,C,L in [(1,0,y*y,1,0),(-2,1,y*y,1,0),(2,-1,y*y+z*z,-2,x*x),(1,2,2*y*y,3,x),(1,0,y**4,1,0)]:
 Q=aa*x+bb;N=Q*Q+R;L=s.sympify(L)
 cases.append(dict(kind='norm',Q=Q,R=R,C=C,L=L,input=f'diff({C}*(sqrt({gs(N)})-abs({gs(Q)}))+({gs(L)}),x)'))
for phase,A,B in [(x,s.Integer(1),s.Integer(0)),(2*x+1,x*x,s.Integer(0)),(x,x,s.Integer(0)),(1/x,x*x,s.Integer(0)),(2/(x-1)+s.Rational(1,3),(x-1)**3,x*x),((2*x+1)/(x-1),x*x-1,s.Integer(1))]:
 cases.append(dict(kind='floor',phase=phase,A=A,B=B,input=f'diff(({gs(A)})*floor({gs(phase)})+({gs(B)}),x)'))
for c in (0,1):
 for m in (1,2,3,4):
  h=x-c;phase=2/h+s.Rational(1,3);A=h**m;assigned=2 if m==1 else 0
  cases.append(dict(kind='accumulation',phase=phase,A=A,B=s.Integer(0),c=c,m=m,assigned=assigned,input=f'diff(piecewise(x=={c},{assigned},({gs(A)})*floor({gs(phase)})),x)'))
for f,target in [('sin(2*pi*floor(x))','0'),('cos(2*pi*floor(x))','0'),('sin(pi*floor(x))','0'),('cos(2*pi*(x-floor(x)))','-2*pi*sin(2*pi*x)'),('sin(2*pi*floor(1/x)+x)','cos(x)')]:
 cases.append(dict(kind='periodic',input=f'diff({f},x)',expected=target,pole='1/x' in f))
# Denesting must preserve the full original domain, not only values where
# both sides already happen to be real. Literal square and shifted forms.
for U,V in [(x,y*y),(x*x+1,y*y),(2*x,y*y),(x,(y-1)**2),(x,4*y*y),
            (x,y),(x,4*y*y-3*x*x),(x,y*y-1),(x*x+1,y)]:
 cases.append(dict(kind='roots',U=U,V=V,input=f'sqrt(({gs(U)})+sqrt(({gs(U*U-V)})))+sqrt(({gs(U)})-sqrt(({gs(U*U-V)})))'))

for index,c in enumerate(cases):
 for stack in ('normal','64'):
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  for outer in (False,True):
   expr='simplify('+c['input']+')' if outer else c['input'];row=dict(case=index,kind=c['kind'],input=expr,outer=outer,stack=stack);points=[]
   try:
    r=call(expr,env);row.update(exit=r.returncode,result=r.stdout.strip());assert r.returncode==0 and 'integrate(' not in r.stdout and 'diff(' not in r.stdout
    F=parse(r.stdout.strip())
    if c['kind']=='quadratic':
     for aval,bval in [(-2,0),(-1,1),(0,0),(0,1),(1,0),(1,-1),(2,2)]:
      sub={ap:aval,bp:bval};Q=s.expand(c['Q'].subs(sub))
      if Q==0:
       points.append(('[x,a,b]',f'[0,{aval},{bval}]','undef'));continue
      f=regular(specialize(F,sub))
      for node in list(s.preorder_traversal(f)):
       if node.func==s.log:
        # SymPy may pull a positive constant out of Abs after parameter
        # specialization. d log(c*|u|)=u'/u on each real nonzero interval.
        factors=s.Mul.make_args(node.args[0])
        if all(t.func==s.Abs or (not t.has(x) and t.is_positive) for t in factors):
         f=f.xreplace({node:s.log(s.Mul(*(t.args[0] if t.func==s.Abs else t for t in factors)))})
      assert equal(s.diff(f,x),1/Q),(Q,f)
      for root in s.solve(Q,x):
       if root.is_real:points.append(('[x,a,b]',f'[{gs(root)},{aval},{bval}]','undef'))
      for xx in (0,2):
       if Q.subs(x,xx)!=0:points.append(('[x,a,b]',f'[{xx},{aval},{bval}]','finite'))
     row['proof']='Exact rational/atan/log derivatives in all sampled parameter regimes, including loss of quadratic and linear terms; every real root of each specialized denominator is tested in the actual conditional evaluator.'
    elif c['kind']=='norm':
     Q,R,C,L=[c[k] for k in ('Q','R','C','L')];target=s.diff(L,x)+C*s.diff(Q,x)*(Q/s.sqrt(Q*Q+R)-s.sign(Q))
     assert equal(regular(F),target)
     root=s.solve(Q,x)[0]
     for xx in (root,root-1,root+1):points.append(('[x,y,z]',f'[{gs(xx)},0,0]',gs(s.diff(L,x).subs(x,xx))))
     for yy,zz in [(1,0),(-1,0),(1,1)]:points.append(('[x,y,z]',f'[{gs(root)},{yy},{zz}]','undef'))
     row['proof']='Nonnegative parameter sum of squares vanishes exactly on its zero locus, where the complete norm-minus-magnitude expression cancels. Else the original contact quotient has unequal +/-|Q prime| limits. Exact off-contact derivative identity.'
    elif c['kind'] in ('floor','accumulation'):
     phase,A,B=[c[k] for k in ('phase','A','B')];target=s.diff(A,x)*s.floor(phase)+s.diff(B,x)
     assert equal(regular(F),target)
     for n in (-3,-1,0,1,3):
      for xx in s.solve(phase-n,x):
       if xx.is_real is not True:continue
       smooth=A.subs(x,xx)==0 and s.diff(A,x).subs(x,xx)==0
       expected=gs(s.diff(B,x).subs(x,xx)) if smooth else 'undef'
       points.append(('x',gs(xx),expected))
     if c['kind']=='accumulation':points.append(('x',str(c['c']),'undef' if c['m']==1 else '2' if c['m']==2 else '0'))
     row['proof']='A nonconstant Mobius phase crosses its integer levels. Exact whole-branch value/slope differences A and A prime determine jumps. At the assigned simple-pole accumulation, floor(phi)=phi-q with 0<=q<1 gives derivative 2 for quadratic damping, zero for higher damping, and no derivative for linear damping.'
    elif c['kind']=='periodic':
     assert equal(regular(F),parse(c['expected']))
     for xx in (-1,0,1):points.append(('x',str(xx),'undef' if c['pole'] and xx==0 else gs(parse(c['expected']).subs(x,xx))))
     row['proof']='For integer n, sin(k*pi*n)=0 and adding 2*k*pi*n leaves sine/cosine unchanged. Compare the whole original function before differentiating floor; preserve the original reciprocal-phase hole.'
    else:
     U,V=c['U'],c['V'];value=s.refine(s.sqrt(2*(U+s.sqrt(V))),s.Q.real(y))
     if outer:
      assert F.has(s.Piecewise),'Explicit denesting needs the original real domain'
      # Prove square factors before applying the stated real assumption;
      # generic SymPy symbols intentionally remain assumption-free elsewhere.
      yr=s.Dummy('real_y',real=True)
      real_value=regular(F).subs(y,yr).replace(lambda t:t.is_Pow and t.exp==s.Rational(1,2),lambda t:s.sqrt(s.factor(t.base)))
      assert equal(real_value,value.subs(y,yr))
     for xx,yy in [(0,0),(1,0),(2,1),(2,-1),(0,1),(-2,1)]:
      u=U.subs(x,xx);v=V.subs({x:xx,y:yy})
      valid=v>=0 and u>=s.sqrt(v)
      if valid:points.append(('[x,y]',f'[{xx},{yy}]',gs(value.subs({x:xx,y:yy}))))
      elif outer:points.append(('[x,y]',f'[{xx},{yy}]','undef'))
     row['proof']='For nonnegative outer conjugate roots, product is sqrt(V); exact original domain U>=sqrt(V) with V>=0. Check included boundaries and points admitted only by the unguarded simplified expression.'
    checked=[]
    for variables,point,expected in points:
     rr=call(f'eval(subst({expr},{variables},{point}))',env);item=dict(point=point,expected=expected,exit=rr.returncode,result=rr.stdout.strip())
     assert rr.returncode==(3 if expected=='undef' else 0),item
     if expected=='undef':assert item['result']=='undef',item
     elif expected=='finite':assert parse(item['result']).is_finite is True,item
     else:assert equal(parse(item['result']),parse(expected)),item
     checked.append(item)
    row.update(passed=True,actual_checks=checked)
   except Exception as e:row.update(passed=False,error=str(e))
   rows.append(row);a.report.write_text(json.dumps(dict(scope='Complete symbolic region identities plus actual contact/pole substitution. Host normal/guarded 64 KiB, 10 second per invocation; not SH4 emulation.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
   print(index,c['kind'],stack,outer,row['passed'],row.get('error','')[:150],flush=True)
assert all(r['passed'] for r in rows)
