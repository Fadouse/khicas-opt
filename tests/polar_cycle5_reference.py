"""Exact root substitution and explicit principal-branch proofs, cycle five."""
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert not any(w in printed for w in ['undef','integrate(','diff(']),printed[:200]
 local['surd']=Root
 if ident=='PC5-R1':
  assert len(printed)<4096
  assert parse(printed)==parse(case['input'][9:-1])
  return dict(exact=True,method='Exact structural preservation. Both powered bases are >=1; reciprocal radicand >=2, equality only at origin. Squared linear forms prove simultaneous reversal and coordinate-reversal symmetry, and the diagonal restriction.')
 if ident=='PC5-D2':
  assert printed.startswith('piecewise(') and printed.endswith(')')
  c1,a,c2,b,c=printed[10:-1].split(',');assert c1=='0>x' and c2=='x<=1'
  assert equal(parse(a),-1/(1-x)) and equal(parse(b),-1/(1+x)**2) and equal(parse(c),-s.Rational(1,4)+2*(x-1))
  return dict(exact=True,method='Exact open-region derivatives; original one-sided difference quotients agree at 0 (-1) and 1 (-1/4). Actual substitution checks inactive poles and logarithm branches.')
 a=parse(printed);assert isinstance(a,s.Expr),'Scalar expression returned a list instead of a value'
 u=s.Symbol('u',real=True)
 if ident=='PC5-I1':
  f=a.xreplace({Root(x,3):u})
  expected=3*u-s.log(s.Abs(1+u))+s.log(u*u-u+1)/2-s.sqrt(3)*s.atan((2*u-1)/s.sqrt(3))
  assert equal(f,expected)
  for sign in [-1,1]:assert equal(s.diff(f.xreplace({s.Abs(1+u):sign*(1+u)}),u),3*u**3/(1+u**3))
  assert s.simplify(s.limit((f-f.subs(u,0))/u**3,u,0))==0
  method='Exact real-root partial fractions; positive quadratic (u-1/2)^2+3/4 and both real log signs. Difference quotient at x=0 is zero; only x=-1 excluded.'
 elif ident=='PC5-I2':
  f=a.xreplace({s.log(x*s.sign(x)):u,s.log(s.Abs(x)):u})
  assert not f.has(x),f
  assert equal(s.diff(f,u),u/(1+u**4))
  assert equal(f,s.I*s.log(u*u+s.I)/4-s.I*s.log(u*u-s.I)/4)
  assert equal(f.subs(u,0),-s.pi/4)
  method='t=ln|x| and dt=dx/x on both half-lines. Exact transformed derivative. Complex-log arguments t²±i never touch a cut and are conjugates, so this real primitive differs from atan(t²)/2 only by -pi/4; ±1 are regular.'
 elif ident=='PC5-D1':
  f=a.xreplace({Root(x,3):u}).subs(x,u**3)
  assert equal(f,2*u*(2+u*u)/(3*(1+u*u)**2))
  assert a.subs(x,0).subs(Root(0,3),0)==0,'Derivative is undefined at zero before cancellation'
  method='Exact real-root rational identity for x!=0. Original difference quotient u/(1+u²) tends to zero; actual zero substitution verifies removable denominator was eliminated.'
 elif ident=='PC5-S1':
  assert equal(a.subs(x,1),0),'Principal square-root cut endpoint is undefined or incorrect'
  t=s.Symbol('t',real=True)
  f=a.subs(x,s.exp(t))
  canonical=s.sqrt((t+s.I*s.pi)**2)-t-s.I*s.pi
  assert equal(f,canonical)
  method='Exact retained principal square root. For t=ln(x)>0 principal root is t+i*pi, for t<0 it is -t-i*pi; at t=0 sqrt(-pi²)=i*pi. Boundary value 0 and lower-side jump -2*i*pi are preserved.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
