"""Root-authored global identities for the twelfth unseen batch."""
import sympy as s
from mixed_reference import x,local,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 if ident=='PC12-S1':local['y']=s.Symbol('y',real=True)
 a=parse(printed)
 if ident=='PC12-I1':
  expected=parse(case['expected']['expression']);assert equal(a,expected)
  assert equal(s.diff(a,x),x*s.atan(x)/(1+x*x)**2)
  method='Exact integration-by-parts primitive and rational derivative; 1+x²>0 gives whole-axis continuity, including zero. Limits of the odd primitive are ±pi/8.'
 elif ident=='PC12-I2':
  u=s.Symbol('u',positive=True);f=s.expand_log(a.subs(x,u*u),force=True)
  expected=4*u*s.log(u)/(1+u)-4*s.log(1+u)
  assert equal(f,expected) and equal(s.diff(f,u),4*s.log(u)/(1+u)**2)
  assert s.limit(f,u,0)==0 and s.limit(f/u**2,u,0)==-s.oo
  assert s.limit(f,u,s.oo)==0
  method='Exact positive sqrt substitution and log|x|=log x only on original x>0. Endpoint extension has quotient F(u²)/u² ->-infinity, not a finite derivative. Integrable |log u| at zero and log u/u² at infinity prove absolute convergence.'
 elif ident=='PC12-D1':
  assert a.has(s.Piecewise),'Odd pi contacts must be excluded'
  for k in range(-3,4):assert a.subs(x,k*s.pi)==(s.Symbol('undef') if k%2 else 0)
  regular=a
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  assert equal(regular,2*s.acos(s.cos(x))*s.sin(x)/s.sqrt(1-s.cos(x)**2))
  method='For every integer k, f(2k*pi+h)=h² on |h|<pi, so derivative is 2h including zero. At odd contacts f= (pi-|h|)², and original quotients tend to opposite ±2pi. Explicit guards distinguish all even and odd periods; regular chain rule is exact elsewhere.'
 elif ident=='PC12-D2':
  assert equal(a,1/(2*(1+x*x)))
  assert a.subs(x,0)==s.Rational(1,2)
  method='s=sqrt(1+x²)>|x|, t=s+x>0, t\'=t/s, and 1+t²=2st. Cancellation is valid globally, including negative x; derivative 1/(2s²).'
 elif ident=='PC12-S1':
  y=local['y'];assert equal(a,s.Abs(x+y)+s.Abs(x-y)-2*s.Abs(x))
  method='All radicands are real squares. A=|x+y|+|x-y|>=0 has A²=2(x²+y²)+2|x²-y²|=4max(x²,y²), proving exact max formula, continuity and no domain exclusions.'
 elif ident=='PC12-R1':
  assert len(printed)<65536 and len(a.atoms(s.log))==16 and s.count_ops(a)<600
  expected=sum(s.log((s.sqrt(1+(x+k)**2)+x+k)/(s.sqrt(1+(x+k-1)**2)+x+k-1)) for k in range(1,17))
  assert a==expected
  method='Exact retained 16-term sum; A(t)=sqrt(1+t²)+t>0 globally since A(t)(sqrt(1+t²)-t)=1. Each log quotient is a difference of real logs, so finite telescoping proves the compact target; A is increasing and L\'=1/sqrt(1+t²), proving positivity and both zero limits.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
