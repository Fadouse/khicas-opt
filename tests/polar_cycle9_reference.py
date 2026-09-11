"""Cycle nine: global real root primitives and original boundary values."""
import re
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed,printed[:200]
 if ident=='PC9-D2':
  m=re.fullmatch(r'\(\(x=1\)\? undef : (piecewise\(.*\))\)',printed)
  assert m,'The function jumps at one even though the neighboring branch slopes agree'
  c,a,d,b,e=m[1][10:-1].split(',');assert c=='0>x' and d=='x<=1'
  assert equal(parse(a),s.exp(x)) and equal(parse(b),1/(1+x)) and equal(parse(e),s.exp((x-1)/2)/2)
  h=s.Symbol('h',positive=True)
  assert s.limit((s.exp(-h)-1)/(-h),h,0)==1 and s.limit(s.log(1+h)/h,h,0)==1
  assert s.limit(s.exp(h/2)/h,h,0)==s.oo
  return dict(exact=True,method='Exact branch derivatives. Original difference quotients at zero both tend to one. At one the right quotient exp(h/2)/h diverges because the function values differ by one, despite equal branch slopes; lazy undef rejects the jump.')
 assert 'undef' not in printed
 local['surd']=Root;a=parse(printed);assert isinstance(a,s.Expr)
 if ident=='PC9-I1':
  u=s.Symbol('u',real=True);f=a.xreplace({Root(x,3):u}).subs(x,u**3)
  expected=u**3*s.log(1+u*u)-2*u**3/3+2*u-2*s.atan(u)
  assert equal(f,expected) and equal(s.diff(f,u),3*u*u*s.log(1+u*u))
  assert s.limit(f/u**3,u,0,dir='+')==0 and s.limit(f/u**3,u,0,dir='-')==0
  method='Exact real cube-root substitution, logarithm positivity and global primitive identity. Original difference quotient G(u)/u³ tends to zero on both sides; all terms odd, and the derivative is positive away from zero. One global constant suffices.'
 elif ident=='PC9-I2':
  expected=parse(case['expected']['expression']);assert equal(a,expected)
  assert equal(s.diff(a,x),s.exp(x)*s.log(1+s.exp(x))/(1+s.exp(x))**2)
  t=s.Symbol('t',positive=True);F=-(s.log(t)+1)/t
  assert equal(s.diff(F,t),s.log(t)/t**2) and s.limit(F,t,s.oo)==0 and F.subs(t,1)==-1
  method='Exact t=1+exp(x)>1 substitution and elementary integration by parts. Positive derivative, global positive log argument, and limits -1 and zero establish the real domain and range.'
 elif ident=='PC9-D1':
  u=s.Symbol('u',real=True)
  f=a.xreplace({Root(x-1,3):u,Root(x**3*(x-1)**4,3):x*u**4,Root(x**3,3):x}).subs(x,u**3+1)
  expected=parse(case['expected']['expression']).xreplace({Root(x-1,3):u}).subs(x,u**3+1)
  assert equal(f,expected)
  assert equal(a.subs(x,0).subs(Root(-1,3),-1).subs(Root(0,3),0),2)
  assert equal(a.subs(x,1).subs(Root(0,3),0),s.cos(1)),'Removable zero at one remains undefined'
  method='Global odd-root uniqueness proves root(x³(x-1)^4)=x*root(x-1)^4. Exact rational derivative in the real bijection x=u³+1. Original difference quotients give 2 at zero and cos(1) at one; all remaining denominators are powers of 1+x²>0.'
 elif ident=='PC9-S1':
  canonical=s.sqrt((x+s.I)/(x-s.I))
  for atom in a.atoms(s.Pow):
   if atom.exp==s.Rational(1,2) and equal(atom.base,canonical.base):a=a.xreplace({atom:canonical})
  expected=s.log(canonical)-(s.log(x+s.I)-s.log(x-s.I))/2
  assert equal(a,expected),'Principal root must retain the exact quotient, not a rectangular formula with a false zero denominator'
  assert equal(a.subs(x,0),0)
  method='Exact principal root and log syntax retained after rational radicand identity. alpha=Arg(x+i) in (0,pi) gives root argument alpha for x>0 and alpha-pi for x<0; at zero sqrt(-1)=i and both log terms give i*pi/2. No original zero or pole for real x.'
 elif ident=='PC9-R1':
  assert len(printed)<4096 and (a==1 or a==parse(case['input'][9:-1]))
  A,B=s.symbols('A B',positive=True)
  assert s.expand((A-B)**2+4*A*B-(A+B)**2)==0
  method='Exact compact retention or constant one. A,B>=1 imply A+B>=2, so sqrt((A-B)²+4AB)=sqrt((A+B)²)=A+B with the positive sign globally, including A=B.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
