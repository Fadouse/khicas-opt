"""Cycle six: exact substitutions and required derivative-domain boundaries."""
import re
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')
def verify(case,printed):
 ident=case['id'];assert 'integrate(' not in printed and 'diff(' not in printed,printed[:200]
 if ident=='PC6-D2':
  match=re.fullmatch(r'\(\(x=1\)\? undef : (piecewise\(.*\))\)',printed)
  assert match,'True corner x=1 must be rejected explicitly'
  fields=match[1][10:-1].split(',');assert len(fields)==7
  c1,a,c2,b,c3,c,d=fields
  assert c1=='(-2)>x' and c2=='x<=0' and c3=='x<=1'
  assert equal(parse(a),(x+2)/s.sqrt(1+(x+2)**2))
  assert equal(parse(b),4*(x+2)/(2+(x+2)**2)**2)
  assert equal(parse(c),s.Rational(2,9)+2*x) and parse(d)==4
  return dict(exact=True,method='Exact four interior derivatives; original one-sided difference quotients agree at -2 (0) and 0 (2/9), but differ at 1 (20/9 versus 4). Lazy undef guard rejects precisely the true corner.')
 assert 'undef' not in printed
 local['surd']=Root;a=parse(printed);assert isinstance(a,s.Expr)
 if ident=='PC6-R1':
  assert len(printed)<4096 and a==parse(case['input'][9:-1])
  return dict(exact=True,method='Exact compact input retention. A,B,C>=1 imply P=A^4*B^3/C^2>0 and Q=1+P^6>1; sqrt(Q)/Q=1/sqrt(Q). Permutation/reversal symmetry and both stated line restrictions follow by direct substitution, without expansion.')
 if ident=='PC6-I1':
  u=s.Symbol('u',real=True);f=a.xreplace({Root(x-2,3):u}).subs(x,u**3+2)
  expected=3*u-s.Rational(9,2)*s.atan(u)+3*u/(2*(1+u*u))
  assert equal(f,expected) and equal(s.diff(f,u),3*u**4/(1+u*u)**2)
  assert f.subs(u,0)==0 and s.limit(f/u**3,u,0)==0
  method='Real cube-root substitution on both signs; exact rational derivative and difference quotient zero at the moving root. All denominators positive, global real atan.'
 elif ident=='PC6-I2':
  t=s.log((x+s.sqrt(x*x+4))/2)
  assert equal(a,s.atan(t))
  assert equal(s.diff(t,x),1/s.sqrt(x*x+4))
  assert a.subs(x,0)==0
  method='Exact logarithmic derivative t\'=1/sqrt(x²+4); sqrt(x²+4)>|x| proves positive log argument globally. atan chain rule gives integrand everywhere, including derivative 1/2 at zero.'
 elif ident=='PC6-D1':
  u=s.Symbol('u',real=True);r=(x-2)/(x+1)
  f=a.xreplace({Root(r,5):u}).subs(x,(2+u**5)/(1-u**5))
  expected=6*u*(3+u*u)/(5*(x+1)**2*(1+u*u)**3)
  assert equal(f,expected.subs(x,(2+u**5)/(1-u**5)))
  assert a.subs(x,2).subs(Root(0,5),0)==0,'Moving-root endpoint undefined'
  method='Exact real-root rational-field identity, with inner pole x=-1 retained. Original difference quotient at x=2 is v/((3+h)*(1+v²)^2), tending to zero; u=1 is unattainable for finite x.'
 elif ident=='PC6-S1':
  canonical=s.log(s.sqrt((s.log(x)+s.I*s.pi)**2))-s.log(s.log(x)+s.I*s.pi)
  assert equal(a,canonical)
  assert s.simplify(s.expand_complex(a.subs(x,1)))==0
  method='Principal-root and outer-log syntax retained. For a=ln(x)<0 root=-w and Arg(-w)=Arg(w)-pi; otherwise root=w, including sqrt(-pi²)=i*pi at x=1. No zero log arguments; lower-side jump -i*pi is preserved.'
 else:raise AssertionError(ident)
 return dict(exact=True,method=method)
