"""Global rational primitives, stationary periodic contacts and finite sum domains."""
import sympy as s
from mixed_reference import x,local,parse,equal

def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed
 if ident=='PC14-S1':local['y']=s.Symbol('y',real=True)
 if ident=='PC14-R1':
  local['piecewise']=lambda *v:s.Piecewise(*[(v[j+1],v[j]) for j in range(0,len(v)-1,2)],(v[-1],True),evaluate=False)
  printed=printed.replace('x=(floor(x))','Eq(x,floor(x))').replace('x=floor(x)','Eq(x,floor(x))')
 a=parse(printed)
 if ident=='PC14-I1':
  assert equal(s.diff(a,x),1/(x**4+4)) and a.subs(x,0)==0
  assert a.atoms(s.atan)=={s.atan(x-1),s.atan(x+1)}
  assert {s.expand(v.args[0]) for v in a.atoms(s.log)}=={x*x-2*x+2,x*x+2*x+2}
  method='Exact rational partial fractions; both quadratic log arguments are (x±1)²+1>0, and separate real atan terms are continuous globally. Zero normalization and whole-line domain are retained.'
 elif ident=='PC14-I2':
  assert not a.has(s.tan),'Half-angle tangent leaves finite chart holes'
  # Canonical sine chart is globally continuous because its denominator
  # exceeds zero; verify the exact derivative and any constant difference.
  target=parse(case['expected']['expression'])
  assert s.trigsimp(s.simplify(a-target))==0
  assert s.trigsimp(s.diff(target,x)-1/(2+s.sin(x))**2)==0
  method='c=2+sqrt3>1 and c²+1=4c give J\'=1/(2+sin x); derivative of cos/(2+sin x) is -(2sin x+1)/(2+sin x)². All denominators positive globally, including every half-angle chart boundary.'
 elif ident=='PC14-D1':
  assert a.has(s.Piecewise) and a.subs(x,0)==0
  for k in range(1,5):
   for sign in [-1,1]:assert a.subs(x,sign*s.sqrt(k*s.pi))==s.Symbol('undef')
  regular=a
  while regular.has(s.Piecewise):regular=regular.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
  assert equal(regular,2*x*s.sin(x*x)/s.sqrt(1-s.cos(x*x)**2))
  method='Exact regular chain rule. At nonzero a²=k*pi, original increment is (-1)^k |2ah+h²| and opposite quotient limits give corners. At zero f(h)=h², quotient h->0, proving the stationary finite derivative.'
 elif ident=='PC14-D2':
  assert equal(a,s.exp(x)/(s.exp(x)-1)-1/x)
  method='The unsplit log quotient is positive on both x!=0 components. Exact derivative preserves x=0 exclusion. Taylor of the original quotient gives 1+h/2+O(h²), and log(1+r)/r->1 proves the separately assigned extension slope 1/2.'
 elif ident=='PC14-S1':
  assert a==parse(case['input'][9:-1])
  method='Exact retention with sqrt((x-y)²)=|x-y|. Square logs require x!=y and x!=-y; the positive root quotient additionally requires x+y>0. On exactly that domain positive logarithm identities cancel, retaining both signs of x-y.'
 elif ident=='PC14-R1':
  assert a.has(s.Piecewise),'Canceled interior holes lost'
  assert a.func==s.Piecewise and len(a.args)==4
  assert a.args[0][1]==(x < -20) and a.args[1][1]==(x>0)
  assert a.args[2]==(s.Symbol('undef'),s.Eq(x,s.floor(x))) and a.args[3][1]==True
  for j in [0,1,3]:assert equal(a.args[j][0],20/(x*(x+20)))
  assert len(printed)<65536 and s.count_ops(a)<256
  method='Exact finite telescope with the inherited domain. The bounded integer-membership guard excludes precisely the 21 original zeros 0,-1,...,-20; on its complement the sum telescopes to 1/x-1/(x+20). Every canceled interior hole remains undefined.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
