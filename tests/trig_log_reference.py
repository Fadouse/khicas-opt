"""Independent proof using the Clausen identity, plus multi-interval checks."""
import sympy as sp
import mpmath as mp
x=sp.Symbol('x',real=True)
local={'x':x,'i':sp.I,'ln':sp.log,'abs':sp.Abs,'im':sp.im,'floor':sp.floor,'max':sp.Max,'Li2':lambda z:sp.polylog(2,z)}
def parse(t):return sp.sympify(t.replace('^','**'),locals=local)
def verify_trig_log_reference(case,printed):
 actual,expected=parse(printed),parse(case['expected'])
 # Normalize equivalent unit-circle arguments before comparing the formulas.
 replacements={}
 for p in actual.atoms(sp.polylog):
  for q in expected.atoms(sp.polylog):
   if sp.simplify(sp.expand_trig(sp.expand_complex(p.args[1]-q.args[1])))==0:
    replacements[p]=q;break
 delta=sp.expand(actual.xreplace(replacements)-expected)
 # Work symbolically on every half-period, k integer and v>0; no sampled
 # sign guess. Floor/abs signs must follow exact rational inequalities.
 v=sp.Symbol('v',positive=True);k=sp.Symbol('k',integer=True)
 frequency=sp.S.One if case['id']=='TL6' else parse(case['frequency'])
 phase=sp.S.Zero if case['id']=='TL6' else parse(case['phase'])+(sp.pi/2 if case['cosine'] else 0)+(sp.pi if case['inside_coefficient']<0 else 0)
 for half in range(2):
  point=((2*k+half)*sp.pi+sp.pi*v/(1+v)-phase)/frequency
  d=delta.subs(x,point)
  for atom in sorted(d.atoms(sp.floor),key=lambda t:sp.count_ops(t)):
   argument=sp.factor(atom.args[0]);index=sp.floor(sp.simplify(argument.subs(v,1)))
   assert sp.factor(argument-index).is_nonnegative and sp.factor(index+1-argument).is_positive,(case['id'],argument,index)
   d=d.xreplace({atom:index})
  for atom in sorted(d.atoms(sp.Abs),key=lambda t:sp.count_ops(t)):
   argument=sp.factor(atom.args[0])
   if argument.is_nonnegative:d=d.xreplace({atom:argument})
   elif argument.is_nonpositive:d=d.xreplace({atom:-argument})
   else:raise AssertionError((case['id'],'unproved absolute-value sign',argument))
  assert sp.simplify(sp.diff(d,v))==0,(case['id'],half,d)
 # The canonical primitive's derivative identity is proved analytically in
 # the corpus. These additional checks evaluate the ACTUAL printed formula,
 # using independent mpmath Li2 and derivatives on both signs and many cells.
 mp.mp.dps=55
 value=sp.lambdify(x,actual,modules=[{'polylog':mp.polylog},'mpmath'])
 integrand=sp.lambdify(x,parse(case['f']),modules='mpmath')
 errors=[]
 for k in range(-3,4):
  for fraction in [mp.mpf('0.23'),mp.mpf('0.67'),mp.mpf('1.23'),mp.mpf('1.67')]:
   u=(2*k+fraction)*mp.pi
   if case['id']=='TL6':point=u
   else:
    a=mp.mpf(str(parse(case['frequency']).evalf(60)));b=mp.mpf(str(parse(case['phase']).evalf(60)))
    if case['cosine']:b+=mp.pi/2
    if case['inside_coefficient']<0:b+=mp.pi
    point=(u-b)/a
   error=abs(mp.diff(value,point)-integrand(point));assert error<mp.mpf('1e-40'),(case['id'],point,error)
   errors.append(str(error))
 return {'exit':0,'stderr':'CHECK exact: canonical Clausen/Li2 identity with principal-log ramp; independent derivative samples on 28 real subinterval points','derivative_numeric_max_error':max(map(float,errors))}
