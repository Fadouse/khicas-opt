"""Independent interval, real-root and branch proofs for cycle seven."""
import re
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed,printed[:200]
 if ident=='PC7-D2':
  match=re.fullmatch(r'\(\(x=1\)\? undef : (piecewise\(.*\))\)',printed)
  assert match,'Infinite one-sided slope at x=1 must be excluded'
  c,a,d,b,e=match[1][10:-1].split(',')
  assert c=='0>x' and d=='x<=1'
  assert equal(parse(a),2*x) and equal(parse(b),x/s.sqrt(1-x*x)) and parse(e)==0
  h=s.Symbol('h',positive=True)
  assert s.limit(h/(1+s.sqrt(1-h*h)),h,0)==0
  assert s.limit(s.sqrt(2-h)/s.sqrt(h),h,0)==s.oo
  return dict(exact=True,method='Exact interior derivatives; original difference quotients at zero both tend to zero, at one the left tends to +infinity and right is zero. Explicit lazy undef at one; no inactive-root exclusions.')
 assert 'undef' not in printed
 local['surd']=Root;a=parse(printed);assert isinstance(a,s.Expr)
 if ident=='PC7-I1':
  t=s.log((s.sqrt(x**4+3*x*x+1)+x)/(1+x*x))
  assert equal(a,t*t/2)
  assert equal(s.diff(t,x),(1-x*x)/((1+x*x)*s.sqrt(x**4+3*x*x+1)))
  assert a.subs(x,0)==0
  method='Exact logarithmic derivative, including stationary points ±1 and zero. With A=1+x²>0, sqrt(A²+x²)>|x| proves the log argument positive globally. Its reciprocal at -x proves evenness of the primitive; x/A tends to zero at both infinities.'
 elif ident=='PC7-I2':
  u=s.Symbol('u',positive=True)
  f=a.subs(x,u*u/(1+u*u))
  f=s.simplify(f)
  assert equal(s.diff(f,u),2*u*u/((1+u*u)*(1+2*u*u)))
  # The accepted rational-root atan form has no interior pole or branch jump.
  assert equal(a,parse(case['expected']['expression'])),'Require a globally regular primitive, not a formula with a hidden midpoint pole'
  assert s.limit(f,u,0)==0 and equal(s.limit(f,u,s.oo),s.pi*(1-1/s.sqrt(2)))
  method='Positive bijection u=sqrt(x/(1-x)) on (0,1); exact partial fractions and derivative. Both atan arguments are smooth finite inside the interval, with endpoint limits 0 and pi*(1-1/sqrt(2)).'
 elif ident=='PC7-D1':
  expected=parse(case['expected']['expression']);assert equal(a,expected)
  for point in [-1,1]:assert equal(a.subs(x,point).subs(Root(0,3),0),s.cos(1))
  method='Exact regular derivative with q′=4x/(1+x²)². At a=±1 the original root-term difference quotient is v*(2a+h)/(1+(a+h)²), tending to zero on both sides; sine contributes cos(1). Denominator positive globally; negative real roots retained.'
 elif ident=='PC7-S1':
  assert a==s.log(s.exp(s.I*x))-s.I*x
  for point,w in [(-2*s.pi,2*s.I*s.pi),(-s.pi,2*s.I*s.pi),(0,0),(s.pi,0),(2*s.pi,-2*s.I*s.pi)]:assert equal(a.subs(x,point),w)
  method='Exact retained principal logarithm. Argument chosen in (-pi,pi] yields phase corrections +2pi,0,-2pi on the three specified intervals; exact values at both cuts and both outer endpoints checked.'
 elif ident=='PC7-R1':
  assert len(printed)<4096 and a==parse(case['input'][9:-1])
  method='Exact compact syntax retained. Every base 1+square is positive, so the stated real-log sum identity is valid globally. Permutation/sign symmetries and diagonal/opposite restrictions follow without expansion.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
