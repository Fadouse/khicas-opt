"""Independent proofs for logarithmic, reflection, flat and periodic functions."""
import sympy as s
from mixed_reference import x,local,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 local['y']=s.Symbol('y',real=True)
 local['piecewise']=lambda *v:s.Piecewise(*[(v[j+1],v[j]) for j in range(0,len(v)-1,2)],(v[-1],True),evaluate=False)
 a=parse(printed)
 if ident=='PC16-I1':
  target=-(s.log(x)**2+2*s.log(x)+2)/x
  assert equal(a,target) and equal(s.diff(a,x),s.log(x)**2/x**2)
  assert a.atoms(s.log)=={s.log(x)}
  method='Two integrations by parts, checked by exact differentiation. The retained real log requires precisely x>0; no log denominator adds a hole at 1. Exponential decay after x=exp(t) proves the upper limit and improper tail 2.'
 elif ident=='PC16-I2':
  assert equal(a,s.pi**2/4)
  method='Smooth whole-line integrand because denominator>=1. Reflect x->pi-x, add, then t=cos(x) gives 2I=pi*(atan(1)-atan(-1))=pi²/2. Historical duplicate retained only as regression, not new coverage.'
 elif ident=='PC16-I3':
  assert equal(a,4*s.sqrt(2))
  method='Global identity sqrt(1+sin x)=sqrt2*|cos(x/2-pi/4)|. Split the continuous integral at 3pi/2 and integrate the two signs; exact sum is 4sqrt2. All radicand zeros and both endpoints are included.'
 elif ident=='PC16-D1':
  assert a.func==s.Piecewise and len(a.args)==2 and a.args[0][0]==0 and a.args[0][1].canonical==(x<=0) and a.args[1][1]==True
  assert equal(a.args[1][0],s.exp(-1/x)/x**2)
  method='Original right quotient t*exp(-t)->0 at t=1/h->infinity, left quotient zero. Exponential dominates every polynomial, so derivative joins continuously and all subsequent derivatives at zero vanish. Lazy branch avoids the reciprocal at zero.'
 elif ident=='PC16-D2':
  t=x-s.floor(x);assert equal(a,2*t*(1-t)*(1-2*t))
  method='Between integers differentiate t²(1-t)². At integer n original quotients h(1-h)² from right and h(1+h)² from left both tend to zero. Endpoint slopes of derivative give f"=2; third derivatives differ ±12. Formula includes every integer and is periodic.'
 elif ident=='PC16-S1':
  y=local['y'];assert a.atoms(s.tan)=={s.tan(x),s.tan(y),s.tan(x+y)}
  assert equal(a,(s.tan(x)+s.tan(y))/(1-s.tan(x)*s.tan(y))-s.tan(x+y))
  numerator,den=s.fraction(s.cancel(a));assert equal(den,s.tan(x)*s.tan(y)-1) or equal(den,1-s.tan(x)*s.tan(y))
  method='Retained tangents require cos(x),cos(y),cos(x+y) nonzero. On exactly this domain 1-tan(x)tan(y)=cos(x+y)/(cos(x)cos(y)); sine addition proves zero. No domain-carrying tangent or denominator was canceled.'
 elif ident=='PC16-R1':
  assert a==s.factorial(20) and len(printed)<65536
  method='The twentieth forward difference lowers polynomial degree twenty times and multiplies leading coefficient by 20!, exactly the alternating binomial sum. No variable denominator or original-domain restriction exists.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
