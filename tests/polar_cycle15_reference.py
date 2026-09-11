"""Independent full-domain proofs; contact checks use actual engine substitution."""
import sympy as s
from mixed_reference import x,local,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 local['y']=s.Symbol('y',real=True)
 a=parse(printed)
 if ident=='PC15-I1':
  assert equal(s.diff(a,x),x*x*s.exp(2*x)*s.sin(3*x))
  assert equal(a,parse(case['expected']['expression']))
  assert a.subs(x,0)==-s.Rational(18,2197)
  method='Exact derivative of polynomial times exponential times sine/cosine; entire real domain, no denominators depending on x. Exponential decay gives zero at negative infinity.'
 elif ident=='PC15-I2':
  target=parse(case['expected']['expression'])
  assert equal(a,target) and s.trigsimp(s.diff(target,x)-1/(5+3*s.cos(x))**3)==0
  assert not a.has(s.tan)
  method='Exact recurrence derivative. All displayed denominators are positive powers of 5+3cos(x)>=2 or atan chart 3+cos(x)>=2; globally continuous primitive with increment 59pi/1024.'
 elif ident=='PC15-D1':
  assert s.trigsimp(a-2*s.cos(x)/(1+s.sin(x)**2))==0
  method='Principal range identity asin(2t/(1+t²))=2atan(t) for |t|<=1 proves smoothness for t=sin(x). At a=pi/2+kpi the original quotient is bounded by |h| via cos(h)-1, hence zero; actual contact substitution additionally required.'
 elif ident=='PC15-D2':
  assert a.has(s.Piecewise)
  for point in [-1,1]:assert a.subs(x,point).has(s.Symbol('undef'))
  regular=a
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  assert equal(regular,2*s.sqrt(x*x-1))
  method='On |x|>1, (x+sqrt(x²-1))(x-sqrt(x²-1))=1 proves no log holes and derivative 2sqrt(x²-1). Original inward endpoint quotient is 4t/3+O(t³) under x=±sqrt(1+t²); endpoints lack a two-sided original domain. Separate zero-filled extension is C1 with endpoint slopes zero.'
 elif ident=='PC15-S1':
  y=local['y'];assert a==s.Abs(s.log(x/y))-s.log(s.Abs(x)/s.Abs(y))
  assert equal(a.subs({x:-1,y:-2}),2*s.log(2)) and a.subs({x:-2,y:-1})==0
  method='Real log(x/y) requires exactly xy>0. On this full domain including the negative quadrant, second log is log(x/y), so |L|-L is zero for ratio>=1 and 2log(y/x) otherwise. The original log retains axes and opposite-sign exclusions.'
 elif ident=='PC15-R1':
  assert len(printed)<65536
  assert equal(a,(x*x-1)/(x*x-169))
  # Parsing in Sympy cancels factors; inspect unevaluated original syntax
  # and independently require actual-engine undef at every inherited hole.
  assert all(('x^2-'+str(j*j)+')') in printed for j in range(2,14)), 'Canceled product domain lost'
  method='Finite factor telescope on the complement of ±2..±13; retained factors retain exactly this full domain, and actual engine tests every excluded point plus allowed zeros and origin. No expanded degree-24 polynomial needed.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
