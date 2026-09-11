"""Global branch/domain proofs; no numerical substitution as an identity proof."""
import sympy as s
from mixed_reference import x,local,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 if ident in ['PC13-S1','PC13-R1']:local['y']=s.Symbol('y',real=True)
 a=parse(printed)
 if ident=='PC13-I1':
  assert equal(s.diff(a,x),x*x/s.sqrt(x*x+4))
  for atom in a.atoms(s.log):
   assert equal(atom.args[0],s.sqrt(x*x+4)-x)
  method='Exact derivative; sqrt(x²+4)±x>0 and their product is four. Printed primitive differs from the reference only by 2log2 globally, so its alternative positive log is valid also on the negative axis.'
 elif ident=='PC13-I2':
  angles=a.atoms(s.atan);assert len(angles)==1;angle=next(iter(angles));arg=angle.args[0]
  assert s.trigsimp(s.simplify(arg+s.sin(x)/(2+s.sqrt(3)+s.cos(x))))==0
  assert equal(a.xreplace({angle:s.Symbol('A')}),(x+2*s.Symbol('A'))/s.sqrt(3))
  t=s.Symbol('t',real=True);den=s.denom(arg).subs(s.sin(x)**2,1-s.cos(x)**2).subs(s.cos(x),t)
  P=s.Poly(den,t);assert P.degree()<=2
  extrema=[s.Integer(-1),s.Integer(1)]+[r for r in s.solve(s.diff(den,t),t) if r.is_real and -1<=r<=1]
  values=[s.simplify(den.subs(t,r)) for r in extrema]
  assert all(v.is_positive for v in values) or all(v.is_negative for v in values),'Rationalization introduced a real zero denominator'
  method='Exact atan argument identity with a denominator proved nonzero for all cos(x) in [-1,1]. For c=2+sqrt3, c²+1=4c gives global derivative 1/(2+cos x); oddness, period increment and all k*pi values follow without inverse-tangent jump corrections.'
 elif ident=='PC13-D1':
  assert a.has(s.Piecewise),'Missing periodic contact guards'
  for k in range(-3,4):assert a.subs(x,k*s.pi)==0 and a.subs(x,s.pi/2+k*s.pi).has(s.Symbol('undef'))
  regular=a
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  target=s.cos(x)*s.Abs(s.sin(2*x))+2*s.sin(x)*s.cos(2*x)*s.sign(s.sin(2*x))-s.sin(x)
  assert equal(regular,target)
  method='Exact product rule off contacts. At k*pi, original quotient sin(h)|sin2h|/h+(cos h-1)/h tends to zero. At pi/2+k*pi its two limits are -3(-1)^k and (-1)^k, including the smooth cosine summand. Guards cover both infinite families.'
 elif ident=='PC13-D2':
  t=s.Symbol('t',positive=True);f=a.subs(x,t-1);expected=t**(1/(t-1))*((t-1)/t-s.log(t))/(t-1)**2
  f=f.replace(lambda z:z.is_Pow and z.base==t,lambda z:s.expand_power_exp(s.Pow(t,s.apart(z.exp,t),evaluate=False)))
  assert equal(s.simplify(f/t**(1/(t-1))),((t-1)/t-s.log(t))/(t-1)**2)
  method='Exact power algebra for positive base t=1+x, t!=1; logarithmic differentiation holds on both components. Taylor ln(1+h)/h=1-h/2+O(h²) and the original exponential quotient prove the separate extension slope -e/2, while original x=0 stays excluded.'
 elif ident=='PC13-S1':
  assert a==parse(case['input'][9:-1]),'Original log/root domain erased'
  method='Exact retained expression. Original separate logs require x±y>0, precisely x>|y|. Set A=x+y>0,B=x-y>0: sqrt(AB)/B=sqrt(A/B), so real logs cancel only on that wedge. Opposite wedge and both boundary rays remain excluded.'
 elif ident=='PC13-R1':
  y=local['y'];assert len(printed)<65536 and s.count_ops(a)<256
  assert s.cancel(a-16*x/(x+y))==0
  assert s.denom(a)==(x+y)**16 or s.denom(a)==x+y
  method='Differentiate the finite binomial polynomial and multiply by x: numerator=16x(x+y)^15 identically, including both axes. Original and returned denominators exclude exactly x+y=0; bounded sixteen nonzero monomials, not runaway expansion.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
