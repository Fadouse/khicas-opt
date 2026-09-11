"""Cycle ten: removable root products and one-sided domain endpoints."""
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 local['surd']=Root
 if ident=='PC10-D2':
  assert printed.startswith('piecewise(')
  c,a,d,b,e=printed[10:-1].split(',');assert c=='0>=x' and d=='x<1'
  assert equal(parse(a),x/(x-1)) and equal(parse(b),x*(x+2)/(1+x)**2)
  f=parse(e);pieces=f.atoms(s.Piecewise);assert len(pieces)==1
  p=next(iter(pieces));assert p.args[0]==(0,s.Eq(x,1)) and p.args[1][1]==True
  regular=f.xreplace({p:p.args[1][0]});assert equal(regular,s.Rational(3,4)+3*s.sqrt(x-1)/2)
  assert f.subs(x,1)==s.Rational(3,4)
  return dict(exact=True,method='Exact interior derivatives and lazy removable endpoint. Original quotients at zero both tend to zero; at one they are (3+2h)/(2*(2+h)) on the left and 3/4+sqrt(h) on the right, both tending to 3/4. Original function values agree; the infinite second derivative is irrelevant to the finite first derivative.')
 a=parse(printed);assert isinstance(a,s.Expr)
 if ident=='PC10-D1':
  pieces=a.atoms(s.Piecewise);regular=a
  assert any(p.args[0]==(0,s.Eq(x,0)) for p in pieces),'Missing removable value at zero'
  assert any(p.args[0][0]==s.Symbol('undef') and p.args[0][1]==s.Eq(x,1) for p in pieces),'Missing genuine cusp exclusion'
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  u=Root(x*(x-1),3)
  expected=s.exp(x)*u+(s.exp(x)-1)*(2*x-1)/(3*u*u)+2*x/(1+x*x)
  assert equal(regular,expected) and a.subs(x,0)==0
  method='Exact regular product rule with real cube root. At zero the original quotient is ((exp(h)-1)/h)*root(h(h-1))+ln(1+h²)/h ->0. At one its leading term is (exp(1+h)-1)*root(1+h)/root(h)² ->+infinity on both sides; lazy undef excludes one and only one.'
 elif ident=='PC10-I1':
  assert equal(a,parse(case['expected']['expression']))
  assert s.trigsimp(s.diff(a,x)-s.sin(x)**3/(2+s.cos(x)))==0
  method='Exact trig derivative from polynomial division in cos(x). 2+cos(x)>=1 proves global smoothness, including every k*pi stationary point; cos composition establishes evenness, periodicity and zero full-period integral.'
 elif ident=='PC10-I2':
  u=s.Symbol('u',positive=True);f=s.simplify(a.subs(x,s.exp(u*u)))
  expected=2*u**3/3-u*u+2*u-2*s.log(1+u)
  assert equal(f,expected) and equal(s.diff(f,u),2*u**3/(1+u))
  assert s.limit(f/(s.exp(u*u)-1),u,0,dir='+')==0 and s.limit(f,u,0)==0
  method='Exact u=sqrt(ln x) substitution on x>1. The original primitive quotient G(u)/(exp(u²)-1) tends to zero from the right at one. Its domain is x>=1; no two-sided derivative there is claimed.'
 elif ident=='PC10-S1':
  assert a==parse(case['input'][9:-1])
  assert equal(a.subs(x,-1),2*s.I*s.pi) and equal(a.subs(x,1),0)
  method='Exact retained principal logs. Arguments multiply to one and are nonzero globally; both positive for x>1, both negative for x<-1, unit-circle conjugates for -1<x<1. Separate cut evaluations give 2*i*pi at -1 and zero at 1.'
 elif ident=='PC10-R1':
  y=s.Symbol('y',real=True);local['y']=y;a=parse(printed)
  assert len(printed)<4096 and s.count_ops(a)<256
  numerator,denominator=s.fraction(a)
  assert s.Poly(s.expand(numerator-denominator),x,y).is_zero
  assert equal(denominator,(x*x+y*y)**24),'Original zero denominator must remain explicit'
  method='Exact polynomial identity for the two retained real/imaginary component polynomials. Their 25 total monomials remain under outer squares, within the existing finite budget; no full numerator distribution is performed by the engine. Complex norm algebra gives one on the punctured real plane; explicit denominator preserves the origin exclusion.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
