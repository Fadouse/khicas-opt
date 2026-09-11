#!/usr/bin/env python3
"""Root-authored related families: exact open-region proofs and real contacts."""
import argparse,json,subprocess,os,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
def gs(e):return str(e).replace('**','^').replace('Abs(','abs(').replace('log(','ln(')
def run(expression,env):
 r=subprocess.run([a.probe,expression],env=env,capture_output=True,text=True,timeout=10)
 assert len(r.stdout.encode())<=65536
 return r
cases=[]
for Q in [x,2*x-1,(x-1)**2,(x-1)**3,(x-1)**4,x*(x-1)]:
 for H in [2+s.sin(x),s.Rational(1,2)+s.sin(x)]:
  cases.append(dict(kind='variable',Q=Q,H=H,input=f'diff(abs({gs(Q)})^({gs(H)}),x)'))
for Q in [x,1-2*x,x*x,-x*x,x**3,-x**4,(x-1)**2,(x-1)**3,x*(x-1)]:
 for power in [s.Rational(1,2),s.Rational(3,2)]:
  cases.append(dict(kind='positive',Q=Q,p=power,input=f'diff((({gs(Q)})+abs({gs(Q)}))^({gs(power)}),x)'))
# Include both orientations and a translated zero in global primitives.
for A,B,a0,b0 in [(1,-1,1,0),(-1,1,1,0),(2,-3,1,0),(-2,3,-1,0),(1,-1,2,1),(1,-1,-2,-1)]:
 phase=a0*x+b0
 cases.append(dict(kind='integral',A=A,B=B,a=a0,b=b0,input=f'integrate(exp({gs(phase)})*abs({A}*exp({gs(phase)})+({B}))/(1+exp(2*({gs(phase)}))),x)'))
finite=[('(cos(x)+x*sin(x))/(x*(x+cos(x)))',0,1,'+infinity'),('(cos(x)+x*sin(x))/(x*(x+cos(x)))',1,2,'ln(2)+ln(cos(1)+1)-ln(cos(2)+2)'),('(cos(x)+x*sin(x))/(x*(x+cos(x)))',-1,1,'undef'),('(cos(x)+x*sin(x))/(x*(x+cos(x)))',2,1,'-ln(2)-ln(cos(1)+1)+ln(cos(2)+2)'),('1/x',-1,1,'undef'),('1/x^2',-1,1,'+infinity'),('cos(x)/sin(x)',-1,1,'undef')]
for f,lo,hi,expected in finite:cases.append(dict(kind='finite',expected=expected,input=f'integrate({f},x,{lo},{hi})'))
for index,c in enumerate(cases):
 for stack in ('normal','64'):
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  for outer in (False,True):
   expr='simplify('+c['input']+')' if outer else c['input'];row=dict(case=index,kind=c['kind'],input=expr,outer=outer,stack=stack)
   try:
    r=run(expr,env);row.update(exit=r.returncode,result=r.stdout.strip())
    if c['kind']=='finite':
     expected=c['expected'];assert r.returncode==(3 if expected=='undef' else 0),row
     if expected in ('undef','+infinity'):assert r.stdout.strip()==expected,row
     else:assert equal(parse(r.stdout.strip()),parse(expected)),row
     row['proof']='Known ordinary integral: explicit singularities are not Cauchy principal values; closed regular endpoints checked exactly.'
    else:
     assert r.returncode==0 and 'integrate(' not in r.stdout and 'diff(' not in r.stdout,row
     F=parse(r.stdout.strip());checks=[]
     if c['kind']=='integral':
      t=s.Symbol('t',positive=True);A,B,a0,b0=[c[k] for k in ('A','B','a','b')];t0=-s.Rational(B,A)
      Ft=s.expand_power_exp(F.subs(x,(s.log(t)-b0)/a0));branches=[]
      for side in (-1,1):
       branch=Ft
       for atom in branch.atoms(s.sign):
        # The exact affine sign argument has only the one positive root.
        arg=s.simplify(atom.args[0]);scale=s.cancel(arg/(A*t+B));assert scale.is_positive is True or scale.is_negative is True
        branch=branch.xreplace({atom:s.sign(scale*A)*side})
       assert equal(s.diff(branch,t),s.sign(A)*side*(A*t+B)/(a0*(1+t*t)))
       branches.append(branch)
      value=s.limit(branches[0],t,t0,dir='-');assert equal(value,s.limit(branches[1],t,t0,dir='+'))
      assert equal(Ft.subs(t,t0),value)
      row['proof']='Exact exponential substitution; both derivative branches and their shared value at the sole included zero.'
     else:
      Q=c['Q'];roots=s.solve(Q,x)
      # Polynomial sign regions are proved by real roots and a symbolic
      # positive coordinate on each interval, not just numerical samples.
      bounds=[-s.oo]+sorted(roots)+[s.oo]
      t=s.Symbol('t',positive=True)
      for lo,hi in zip(bounds,bounds[1:]):
       z=hi-t if lo==-s.oo else lo+t if hi==s.oo else (lo+hi*t)/(1+t)
       q=s.factor(Q.subs(x,z));sign=s.sign(q)
       assert sign in (-1,1),(Q,z,q)
       b=F.subs(x,z)
       # Exact substitutions determine guard truth on this open interval.
       for node in list(s.preorder_traversal(b)):
        if node.func==s.Piecewise:
         b=s.simplify(b);break
       if c['kind']=='variable':
        H=c['H'];target=s.Abs(Q)**H*(s.diff(H,x)*s.log(s.Abs(Q))+H*s.diff(Q,x)/Q)
       else:
        target=0 if sign<0 else c['p']*(2*Q)**(c['p']-1)*2*s.diff(Q,x)
       target=s.sympify(target).subs(x,z)
       assert equal(b,target),(Q,z,b,target)
      for root in roots:
       m=1
       while s.diff(Q,x,m).subs(x,root)==0:m+=1
       if c['kind']=='variable':
        h0=c['H'].subs(x,root)
        if h0.is_positive is not True:continue # excluded from positive-exponent contact theorem
        expected='0' if (m*h0-1).is_positive else 'undef'
       else:
        flat=m%2==0 and s.diff(Q,x,m).subs(x,root)<0
        expected='0' if flat or m*c['p']>1 else 'undef'
       rr=run(f'eval(subst({expr},x,{gs(root)}))',env)
       assert rr.returncode==(3 if expected=='undef' else 0) and rr.stdout.strip()==expected,(root,expected,rr.returncode,rr.stdout)
       checks.append(dict(point=str(root),order=m,expected=expected))
      row['proof']='Exact differentiation on complete polynomial sign intervals. At a zero of order m, use the original O(|h|^(m*p)) quotient; a negative even-order radicand is locally on the constant-zero region.'
     row['contacts']=checks
    row['passed']=True
   except Exception as e:row.update(passed=False,error=str(e))
   rows.append(row);a.report.write_text(json.dumps(dict(scope='Full repository entry paths; independent algebra and original contact theorems. Host normal/guarded 64 KiB, not SH4 execution.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
   print(index,c['kind'],stack,outer,row['passed'],row.get('error','')[:140],flush=True)
assert all(r['passed'] for r in rows)
