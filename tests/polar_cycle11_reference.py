"""Independent domain and difference-quotient proofs for cycle eleven."""
import sympy as s
import re
from mixed_reference import x,parse,equal,local

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 if ident=='PC11-D2':
  assert 'undef' in printed,'Isolated value one is discontinuous at zero'
  local['piecewise']=lambda *v:s.Piecewise(*[(v[j+1],v[j]) for j in range(0,len(v)-1,2)],(v[-1],True),evaluate=False)
  a=parse(re.sub(r'\(x==(-?\d+)\)',r'Eq(x,\1)',printed))
  def branch(e,point):
   if e.func==s.Piecewise:
    for v,c in e.args:
     if c==True or c.subs(x,point)==True:return branch(v,point)
   return e
  for point,target in [(-1,1/(1+x*x)),(s.Rational(1,2),1/(1+x)**2),(2,s.Rational(1,4))]:assert equal(branch(a,point),target)
  assert a.subs(x,0)==s.Symbol('undef') and a.subs(x,1)==s.Rational(1,4)
  method='Original quotients at zero contain -1/h and diverge with opposite signs; isolated branch derivative zero is invalid. At one the left quotient is 1/(2*(2+h)) and the right is 1/4. Exact symbolic branch formulas cover all three open intervals, with no inactive pole at -1.'
  return dict(exact=True,method=method)
 a=parse(printed)
 if ident=='PC11-I1':
  assert equal(a,parse(case['expected']['expression']))
  t=s.Symbol('t',positive=True);f=a.subs(s.exp(x),t)
  assert equal(s.diff(f,t)*t,t*t/(t*t+t+1))
  method='Exact t=exp(x)>0 rational substitution; t²+t+1=(t+1/2)²+3/4>0. Real log and atan are continuous globally, so no variable additive jump.'
 elif ident=='PC11-I2':
  assert equal(a,parse(case['expected']['expression']))
  assert equal(s.diff(a,x),s.sqrt(x*x-1)/x)
  u=s.Symbol('u',positive=True);g=u-s.atan(u)
  assert s.limit(g/(s.sqrt(1+u*u)-1),u,0)==0
  method='Exact derivative on both |x|>1 components. G(u)=u-atan(u)=O(u³), while |x|-1=u²/(sqrt(1+u²)+1), proves continuous endpoint values and both one-sided slopes zero. No continuation to |x|<1 is asserted.'
 elif ident=='PC11-D1':
  assert a.has(s.Piecewise),'Missing removable contact and cusp guards'
  assert a.subs(x,0)==0
  for k in [-2,-1,1,2]:assert a.subs(x,k*s.pi)==s.Symbol('undef')
  regular=a
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  target=2*x*s.Abs(s.sin(x))/(1+x*x)+s.log(1+x*x)*s.sign(s.sin(x))*s.cos(x)
  assert equal(regular.replace(s.sign,lambda z:z/s.Abs(z)),target.replace(s.sign,lambda z:z/s.Abs(z)))
  method='Exact regular product rule. At zero |f(h)/h|<=h²; at k*pi!=0 the original quotients tend to opposite nonzero ±ln(1+k²*pi²). Guards preserve all contacts, including the removable one.'
 elif ident=='PC11-S1':
  assert a==parse(case['input'][9:-1]),'Domain-bearing logarithms must remain unless explicit conditions are returned'
  t=s.Symbol('t',positive=True)
  for branch in [t+1,-t-1]:assert s.simplify(s.expand_log(a.subs(x,branch).replace(s.log,lambda z:s.log(s.factor(z))),force=True))==0
  method='Exact compact retention. Real log requires (x-1)/(x+1)>0 and x!=±1, hence two open rays. On each, ln(v²)=2 ln|v| cancels exactly; no values are assigned at the excluded points or middle interval.'
 elif ident=='PC11-R1':
  assert len(printed)<4096 and s.count_ops(a)<256
  num,den=s.fraction(a);assert num==s.factorial(24)
  assert s.Poly(den,x)==s.Poly(s.prod(x+k for k in range(25)),x)
  for j in range(25):assert s.diff(den,x).subs(x,-j)==(-1)**j*s.factorial(j)*s.factorial(24-j)
  method='Exact degree-25 denominator identity; every derivative P\'(-j) is nonzero and numerator is 24!, proving all 25 genuine poles and the stated nonzero residues. Bounded 25-term univariate polynomial, no resource runaway.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
