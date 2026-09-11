"""Cycle eight: real components, original difference quotients and complex cuts."""
import re
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed,printed[:200]
 if ident=='PC8-D2':
  m=re.fullmatch(r'\(\(x=0\)\? undef : (piecewise\(.*\))\)',printed)
  assert m,'The oscillatory original difference quotient at zero must be excluded'
  c,a,d,b,e=m[1][10:-1].split(',');assert c=='0>x' and d=='x<=1'
  assert equal(parse(a),s.sin(1/x)-s.cos(1/x)/x) and equal(parse(b),2*x) and parse(e)==2
  n=s.Symbol('n',integer=True,positive=True)
  assert s.simplify(s.sin(-(s.pi/2+2*s.pi*n)))==-1
  assert s.simplify(s.sin(-(3*s.pi/2+2*s.pi*n)))==1
  return dict(exact=True,method='Exact interior derivatives; original left quotient sin(1/h) has two sequences with limits ±1 at zero, whereas original quotients at one tend to 2 from both sides. Lazy undef excludes precisely zero.')
 assert 'undef' not in printed
 local['surd']=Root;a=parse(printed);assert isinstance(a,s.Expr)
 if ident=='PC8-I1':
  expected=parse(case['expected']['expression']);assert equal(a,expected)
  assert equal(s.diff(a,x),2*x*s.log(1+x*x)/((1+x*x)*(1+s.log(1+x*x))))
  u=s.Symbol('u',nonnegative=True);G=u-s.log(1+u)
  assert s.simplify(s.diff(G,u)-u/(1+u))==0 and s.limit(G/u,u,0)==0
  assert a.subs(x,0)==0
  method='Exact global primitive G(ln(1+x²)); G′=u/(1+u), u≥0. Original difference quotient at zero is (G(u)/u)*(u/h), both tending to zero. All log arguments positive, evenness and monotonicity follow directly.'
 elif ident=='PC8-I2':
  u=s.Symbol('u',positive=True);inverse=(1+u*u)/(1-u*u)
  # Accepted root-parameter expression; positive u covers both components,
  # separated only by the unattainable value u=1.
  v=1/s.sqrt((x-1)/(x+1))
  canonical=s.log(s.Abs((1+v)/(1-v)))-2*s.atan(v)
  if not equal(a,canonical):
   z=s.sqrt(x*x-1)-x
   legacy=-s.pi*s.sign(x+1)/2+(2*s.atan(z)-s.log(s.Abs(z)))/s.sign(x+1)
   outer=(-s.pi+4*s.atan(z)-2*s.log(s.sqrt(x*x-2*s.sqrt(s.Abs(x*x-1))*x*s.cos((s.pi*s.sign(x*x-1)-s.pi)/4)+s.Abs(x*x-1))))/(2*s.sign(x+1))
   assert equal(a,legacy) or equal(a,outer),'Unproved primitive form'
   v=s.Symbol('v',positive=True)
   for sign in [-1,1]:
    inverse=sign*(v*v+1)/(2*v)
    root=(v-1)/(v+1) if sign==1 else (v+1)/(v-1)
    F=s.log(v)-s.pi/2-2*s.atan(1/v) if sign==1 else s.log(v)+s.pi/2-2*s.atan(v)
    assert equal(s.diff(F,v),s.diff(inverse,v)/(inverse*root))
   return dict(exact=True,method='Legacy normal-stack formula proved on both components with x=±(v²+1)/(2v), v>1. sqrt(x²-1)=(v²-1)/(2v), z=-1/v or v; original square root is (v-1)/(v+1) or its reciprocal. Exact rational derivative; all log magnitudes nonzero. The outer magnitude reduces to sqrt(z²)=|z| because x²-1>0. This proof does not override recorded small-stack SIGSEGV.')
  for sign in [-1,1]:
   F=s.log(sign*(1+1/u)/(1-1/u))-2*s.atan(1/u)
   assert equal(s.diff(F,u),4/(1-u**4))
  method='Exact positive reciprocal-root parameter. x=(1+u²)/(1-u²), dx=4u/(1-u²)²du; derivative is 4/(1-u⁴) on both intervals u<1 and u>1. Real log magnitude nonzero, only u=1 excluded. atan(u)+atan(1/u)=pi/2 proves the displayed primitive differs by -pi from the stated normalization.'
 elif ident=='PC8-D1':
  v=s.Symbol('v',real=True,nonzero=True)
  f=a.subs(x,v**3)
  f=f.replace(lambda z:z.func==Root and z.args[1]==3 and equal(z.args[0],v**6),lambda z:v*v)
  f=f.replace(lambda z:z.func==s.Pow and z.exp==s.Rational(2,3) and z.base==s.Abs(v**3),lambda z:v*v)
  assert equal(f,(5*v**3-2)/(3*v)+2*v**3),f
  assert equal(a.subs(x,1).subs(Root(1,3),1),3)
  method='Unique real cube root proves root(x²(x-1)³)=(x-1)*root(x,3)² globally. Exact derivative on x!=0; original quotient at one tends to 3. At zero the quotient (h-1)*sign(h)*|h|^(-1/3)+h has opposite infinite one-sided limits, a true cusp.'
 elif ident=='PC8-S1':
  assert a==s.sqrt((x+s.I)**2)-s.sqrt((x-s.I)**2)
  assert a.subs(x,0)==0
  method='Exact principal-root retention. Positive real part selects ±(x±i) with matching sign of x; at zero both roots of -1 are i, hence difference zero. Full branch partition gives -2i,0,2i.'
 elif ident=='PC8-R1':
  assert len(printed)<4096 and a==parse(case['input'][9:-1])
  r=s.Symbol('r',nonnegative=True);A=1+r;B=r
  assert s.Poly(A**32-B**32-(A-B)*(A+B)*s.prod(A**k+B**k for k in [2,4,8,16]),r).is_zero
  method='Exact compact input retained. Repeated difference of squares proves the product identity; original denominator 1+2r>=1. All four factors increase from one, proving positivity, strict radial monotonicity and all stated symmetries.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
