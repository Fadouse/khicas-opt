"""Independent real-domain proofs for PC21; first-run history stays immutable."""
import re,subprocess
import sympy as s
from mixed_reference import parse as base_parse,local,x,equal
from polar_cycle20_reference import lazy
A=s.Symbol('a',real=True);B=s.Symbol('b',real=True)
R=s.Symbol('r',real=True);T=s.Symbol('theta',real=True)

def parse(text):
 overrides={'a':A,'b':B,'r':R,'theta':T,'max':s.Max,'min':s.Min,'EllipticF':s.elliptic_f,'Gamma':s.gamma}
 old=dict(local)
 try:
  local.update(overrides);return base_parse(text)
 finally:local.clear();local.update(old)

def strip(text):
 text=text.strip()
 while text.startswith('(') and text.endswith(')'):
  depth=0;end=None
  for i,c in enumerate(text):
   depth+=(c=='(')-(c==')')
   if depth==0:end=i;break
  if end!=len(text)-1:break
  text=text[1:-1].strip()
 return text

def polar_tree(text):
 text=strip(text);depth=0;q=colon=None
 for i,c in enumerate(text):
  depth+=(c=='(')-(c==')')
  if depth==0 and c=='?':q=i
  if depth==0 and c==':' and q is not None:colon=i;break
 if q is not None:return ('when',polar_tree(text[:q]),polar_tree(text[q+1:colon]),polar_tree(text[colon+1:]))
 depth=0
 for i,c in enumerate(text):
  depth+=(c=='(')-(c==')')
  if depth==0 and c=='=':return s.Eq(parse(text[:i]),parse(text[i+1:]),evaluate=False)
 return parse(text)

def verify(case,printed):
 ident=case['id'];assert not any(v in printed for v in ('integrate(','diff(','rootof('))
 z=s.Symbol('z',positive=True)
 if ident=='PC21-P1':
  raw=next(v[4:] for v in printed.splitlines() if v.startswith('RAW '));tree=polar_tree(raw)
  assert tree[0]=='when' and tree[1]==s.Eq(A,0,evaluate=False) and tree[2]==s.Eq(R,0,evaluate=False)
  gate=tree[3];assert gate[0]=='when' and gate[2]==s.Symbol('undef')
  assert equal(gate[1].lhs-gate[1].rhs,A*(R*s.sin(T)-A))
  chart=gate[3];assert chart[0]=='when' and chart[1]==s.Eq(R,0,evaluate=False) and chart[2]==s.Eq(R,0,evaluate=False)
  assert equal(chart[3].lhs-chart[3].rhs,R-A*s.sin(T))
  t=s.Symbol('t',real=True);xx=A*t/(1+t*t);yy=A*t*t/(1+t*t)
  assert s.factor(xx*xx+yy*yy-A*yy)==0
  assert s.factor(yy-A)==-A/(t*t+1)
  # Converse: t=x/(a-y), valid away from (0,a); substitution into
  # the parametrization gives (x,y) modulo the circle equation.
  X,Y=s.symbols('X Y',real=True);u=X/(A-Y)
  for expr in (A*u/(1+u*u)-X,A*u*u/(1+u*u)-Y):
   numerator=s.fraction(s.cancel(expr))[0]
   assert s.rem(numerator,X*X+Y*Y-A*Y,X)==0
  return dict(exact=True,method='Exact circle identity and inverse t=x/(a-y). On the circle a-y=0 is exactly the missing finite-parameter point (0,a). At a=0 the map is identically the origin; r=0 remains included at every angle. All statements use the specified r>=0 polar convention.')
 value=parse(printed)
 if ident=='PC21-I1':
  for sub,target in [(-z,s.Rational(1,2)+z),(z/(1+z),z*z/(1+z)**2-z/(1+z)+s.Rational(1,2)),(1+z,z+s.Rational(1,2))]:assert equal(value.subs(A,sub),target)
  assert value.subs(A,0)==s.Rational(1,2) and value.subs(A,1)==s.Rational(1,2)
  method='Split the original absolute-value integral at a if and only if 0<a<1. Three symbolic substitutions exhaust all open parameter regimes; endpoint values and the affine/quadratic first derivatives match at 0 and 1.'
 elif ident=='PC21-I2':
  assert value==s.elliptic_f(s.asin(x),-1)
  assert equal(s.diff(value,x),1/(s.sqrt(1-x*x)*s.sqrt(1+x*x)))
  method='u=asin(x), cos(u)>0 on the full original domain (-1,1), gives Legendre F with parameter m=-1. Endpoints have finite extensions +/-sqrt(pi)*Gamma(1/4)/(4Gamma(3/4)), but unbounded one-sided slopes; they are not integrand domain points.'
 elif ident=='PC21-D1':
  assert value.func==s.Piecewise
  (contact,cond),(regular,otherwise)=value.args
  assert isinstance(cond,s.Equality) and equal(cond.lhs-cond.rhs,x*x-A*x) and otherwise==True
  assert contact.func==s.Piecewise and contact.args[0][0]==2*x and contact.args[1][0]==s.Symbol('undef')
  assert equal(contact.args[0][1].lhs-contact.args[0][1].rhs,2*x-A)
  assert regular.func==s.Piecewise and regular.args[0][0]==2*x and regular.args[1][0]==A
  assert isinstance(regular.args[0][1],s.StrictGreaterThan) and equal(regular.args[0][1].lhs-regular.args[0][1].rhs,x*x-A*x)
  assert lazy(value,{x:0,A:0})==0
  for sign in (-1,1):
   assert lazy(value,{x:0,A:sign*z})==s.Symbol('undef')
   assert lazy(value,{x:sign*z,A:sign*z})==s.Symbol('undef')
  method='Away from x(x-a)=0, use the strict active branch. At contact, max of two C1 branches is differentiable exactly when their slopes agree. Equations x(x-a)=0 and 2x=a imply a=x=0, where the original function is x²; every other contact is a cusp.'
 elif ident=='PC21-D2':
  assert value.has(s.Piecewise),'A real derivative must not acquire values outside the original real domain'
  assert lazy(value,{x:0})==s.Symbol('undef')
  assert lazy(value,{x:1})==s.Symbol('undef')
  assert lazy(value,{x:1-z})==s.Symbol('undef')
  assert equal(lazy(value,{x:1+z}),(1+3*z)/(2*s.sqrt(z)))
  method='Original real domain is {0} union [1,infinity). The isolated point 0 admits no derivative; at 1 the original right quotient diverges. For x>1, sqrt(x²(x-1))=x*sqrt(x-1), yielding (3x-2)/(2sqrt(x-1)). The symbolic x=1-z and x=1+z proofs exhaust the remaining real axis.'
 elif ident=='PC21-S1':
  target=(A-B+s.Abs(A-B))/2
  if value==s.Max(A,s.Min(x,B))-s.Min(B,s.Max(x,A)):
   return dict(exact=True,normalized=False,method='Original expression retained; explicit simplify must eliminate x.')
  assert equal(value,target) and not value.has(x)
  method='If a<=b both clamps agree on x<a, a<=x<=b, and x>b. If a>b the first clamp is always a and the second always b. Thus the difference is max(a-b,0), including equality, for every real x.'
 else:raise AssertionError(ident)
 return dict(exact=True,normalized=True,method=method)

def actual_checks(case,expression,probe,env):
 items={
 'PC21-I1':[('[a]','[-2]','5/2'),('[a]','[0]','1/2'),('[a]','[1/2]','1/4'),('[a]','[1]','1/2'),('[a]','[2]','3/2')],
 'PC21-I2':[('x','0','0'),('x','1','sqrt(pi)*Gamma(1/4)/(4*Gamma(3/4))'),('x','-1','-sqrt(pi)*Gamma(1/4)/(4*Gamma(3/4))')],
 'PC21-D1':[('[x,a]','[0,0]','0'),('[x,a]','[2,0]','4'),('[x,a]','[0,2]','undef'),('[x,a]','[2,2]','undef'),('[x,a]','[1,2]','2'),('[x,a]','[-1,-2]','-2'),('[x,a]','[-2,-2]','undef')],
 'PC21-D2':[('x',q,'undef') for q in ['-1','0','1/2','2/3','1']]+[('x','2','2'),('x','5','13/4')],
 'PC21-S1':[('[x,a,b]',str([xx,aa,bb]),str(max(aa-bb,0))) for xx in [-3,0,2,5] for aa,bb in [(1,3),(3,1),(2,2)]],
 }
 rows=[]
 for variables,point,expected in items.get(case['id'],[]):
  r=subprocess.run([str(probe),f'eval(subst({expression},{variables},{point}))'],env=env,capture_output=True,text=True,timeout=10)
  row=dict(point=point,expected=expected,exit=r.returncode,result=r.stdout.strip())
  assert r.returncode==(3 if expected=='undef' else 0),row
  if expected=='undef':assert row['result']=='undef',row
  else:
   actual,target=parse(row['result']),parse(expected)
   if case['id']=='PC21-I2':
    # Euler reflection at 1/4: Gamma(1/4)Gamma(3/4)=pi*sqrt(2).
    reflection={s.gamma(s.Rational(3,4)):s.pi*s.sqrt(2)/s.gamma(s.Rational(1,4))}
    actual=actual.xreplace(reflection);target=target.xreplace(reflection)
   assert equal(actual,target),row
  rows.append(row)
 return rows
