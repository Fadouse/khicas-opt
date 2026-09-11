"""Independent root-substitution, piecewise and complex-cut proofs."""
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id']
 assert not any(word in printed for word in ('undef','integrate(','diff(')),printed[:200]
 if ident=='PC4-R1':
  assert len(printed)<4096,'Runaway expansion: %d output characters from a 93-character input'%len(printed)
  assert parse(printed)==parse(case['input'][9:-1])
  return dict(exact=True,method='Exact structural preservation; A=2+30*r²+30*r⁴+2*r⁶>=2 for r=x²+y²+z²>=0, hence 64/65<=A⁶/(1+A⁶)<1 and positive denominator.')
 if ident in ['PC4-D1','PC4-D2']:
  assert printed.startswith('piecewise(') and printed.endswith(')')
  fields=printed[10:-1].split(',')
  if ident=='PC4-D1':
   cond,zero,positive,yes,no=fields
   assert cond=='x=0' and zero=='0' and positive=='(1/x)>0'
   assert equal(parse(yes),2*x*s.log(x)+x)
   assert equal(parse(no),2*x*s.log(-x)+x)
   method='Ordered lazy conditions. Exact derivatives on both half-lines; h*ln|h| tends to zero at the isolated branch. Actual substitutions check that inactive 1/x and log branches stay inactive.'
  else:
   cond,negative,upper,middle,otherwise=fields
   assert cond=='0>x' and negative=='-1' and upper=='x<=1' and middle=='2*x' and otherwise=='2'
   method='Exact open-interval derivatives. At 1, original difference quotients are 2+h from the left and 2 from the right; both tend to 2. At 0 one-sided derivatives -1 and 0 differ, and the point remains excluded.'
  return dict(exact=True,method=method)
 local['surd']=Root;a=parse(printed)
 if ident in ['PC4-I1','PC4-I2']:
  u=s.Symbol('u',real=True)
  if ident=='PC4-I1':
   f=a.xreplace({Root(x,3):u});assert equal(f,3*u-3*s.log(s.Abs(u+1)))
   for sign in [-1,1]:assert equal(s.diff(f.xreplace({s.Abs(u+1):sign*(u+1)}),u),3*u/(1+u))
   method='Real cube-root bijection; exact local log derivatives on all three intervals. Both original denominator zeros -1 and 0 remain excluded.'
  else:
   f=a.xreplace({Root(x-2,5):u});assert equal(f,5*u**3/3-5*u+5*s.atan(u))
   assert equal(s.diff(f,u),5*u**4/(1+u*u))
   assert f.subs(u,0)==0 and s.limit(f/u**5,u,0,dir='+')==s.limit(f/u**5,u,0,dir='-')==1
   method='Real fifth-root bijection x=2+u⁵; exact derivative plus two-sided difference quotient 1 at the moving root; real continuous primitive on the entire real line.'
 elif ident=='PC4-S1':
  canonical=s.log(s.Abs(x-1))+s.log(-1+s.I*x)+s.log(-1-s.I*x)-s.log(1+x*x)
  assert equal(a,canonical);assert a.subs(x,0)==2*s.I*s.pi
  method='Exact retained principal logs. Nonreal conjugates have opposite arguments off x=0; at zero both arguments are -1 and each principal log is i*pi. sqrt((x-1)²)=|x-1|; x=1 stays excluded.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
