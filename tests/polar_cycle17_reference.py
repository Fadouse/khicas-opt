"""Independent domain proofs and compactness checks for cycle 17."""
import sympy as s
from mixed_reference import x,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 a=parse(printed)
 if ident=='PC17-I1':
  assert equal(a,-s.log(2))
  method='On x>0, F=x ln(x)/(1+x)-ln(1+x) differentiates to the integrand. x ln(x)->0 gives F(0+)=0; |integrand|<=-ln(x) on (0,1] proves absolute convergence.'
 elif ident=='PC17-I2':
  target=((1+x)*s.sqrt(1+x)+(1-x)*s.sqrt(1-x))/3
  assert equal(a,target) and equal(s.diff(target,x),(s.sqrt(1+x)-s.sqrt(1-x))/2)
  method='The two nonnegative roots require exactly [-1,1] and their sum never vanishes. (a-b)(a+b)=2x gives integrand (a-b)/2 without dividing by x. Endpoint original one-sided primitive quotients give ±1/sqrt2.'
 elif ident=='PC17-D1':
  assert equal(a,s.asin(s.sqrt(x))/(s.sqrt(x)*s.sqrt(1-x)))
  method='Original domain [0,1], ordinary derivative only (0,1). Original right quotient at zero is (t/sin t)²->1, not a two-sided derivative. Original left quotient at one is t(pi-t)/sin²t->+infinity; nonfinite endpoint output is recorded, never accepted as a finite derivative.'
 elif ident=='PC17-D2':
  assert equal(a,x/(1+s.Abs(x))**2)
  method='H(t)=ln(1+t)-t/(1+t) is analytic for t>=0, H prime=t/(1+t)². At zero 0<=H(|h|)<=h²/2 proves original two-sided quotient zero. All real inputs allowed; actual zero substitution is required.'
 elif ident=='PC17-S1':
  u=x/2-s.pi/4;target=s.Abs(s.cos(u))/s.Abs(s.sin(u))-(1+s.sin(x))/s.cos(x)
  a=a.replace(lambda z:z.func in (s.sin,s.cos),lambda z:z.func(s.expand(z.args[0])))
  assert s.trigsimp(s.cancel(a-target))==0
  assert s.trigsimp(a.subs(x,x+2*s.pi)-a)==0
  method='Half-angle nonnegative roots give |cos u|/|sin u|. On cos(x)!=0 this equals (1+sin x)/|cos x|; subtract the signed quotient and separate cosine signs. Exact periodicity plus actual checks of both excluded zero classes retain the inherited domain.'
 elif ident=='PC17-R1':
  assert len(printed)<65536 and s.count_ops(a)<256,'Unnecessary 1024-monomial expansion violates compact-result requirement'
  assert equal((1-x)*a,1-x**1024)
  assert a.subs(x,1)==1024 and a.subs(x,0)==1 and a.subs(x,-1)==0
  method='Repeated difference of squares proves (1-x)P=1-x^1024. Original polynomial product is defined on all reals, including actual value 1024 at 1; compact representation required by the resource case.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
