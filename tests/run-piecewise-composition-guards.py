#!/usr/bin/env python3
"""Check smooth joins, true corners and rational real-root compositions."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal,local
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();rows=[]
def run(e,stack,undefined=False):
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 r=subprocess.run([a.probe,e],env=env,capture_output=True,text=True,timeout=12)
 assert r.returncode==(3 if undefined else 0),(e,r.stdout,r.stderr)
 return r.stdout.strip()
checks=[('piecewise(x<0,-x,x)',0,'undef'),('piecewise(x<=0,0,1)',0,'undef'),('piecewise(x<0,x^2,x^2)',0,'0'),('piecewise(x<1,x^2,2*x-1)',1,'2'),('piecewise(x<0,ln(1-x),-x)',0,'-1'),('piecewise(x<0,1/(1-x),1+x)',0,'1')]
Root=s.Function('RealRoot');local['surd']=Root;u=s.Symbol('u',real=True)
for stack in ['normal','64']:
 for f,point,expected in checks:
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   out=run('eval(subst('+e+',x='+str(point)+'))',stack,expected=='undef')
   assert out=='undef' if expected=='undef' else equal(parse(out),parse(expected))
   rows.append(dict(input=e,stack=stack,point=point,result=out,exact=True,method='Both branches analytic through the join: equal values and derivatives for smooth cases; unequal finite one-sided derivatives or values for corner/jump cases.'))
 for inner,n,m,k,power in [('(x-2)/(x+1)',5,6,2,2),('(2*x+1)/(3-x)',3,4,2,1),('x^2-1',3,4,1,1)]:
  arg=parse(inner);root=f'surd({inner},{n})';f=f'{root}^{m}/(2+{root}^{k})^{power}'
  expected=s.diff(u**m/(2+u**k)**power,u)/(n*u**(n-1))*s.diff(arg,x)
  for outer in [False,True]:
   e='diff('+f+',x)';e='simplify('+e+')' if outer else e
   out=run(e,stack);actual=parse(out).xreplace({Root(arg,n):u})
   if not equal(actual,expected):
    assert inner=='x^2-1'
    numerator=s.fraction(s.cancel(actual-expected))[0]
    assert s.rem(s.Poly(numerator,u),s.Poly(u**n-arg,u)).is_zero,(e,out)
    assert equal(s.fraction(actual)[1],3*(x*x+7)**2)
    # The displayed denominator is positive; also u³=x²-1>=-1 gives
    # u>=-1, so the original denominator u+2 cannot vanish either.
   for point in s.solve(arg,x):
    value=run('eval(subst('+e+',x='+s.sstr(point)+'))',stack)
    assert parse(value)==0,(e,point,value)
   rows.append(dict(input=e,stack=stack,result=out,exact=True,method='Exact rational chain rule after root-power cancellation. Inner rational functions are smooth at their zeros; m>n gives zero derivative there. Original rational poles are excluded.'))
 for slope,shift,radius,A,B in [(2,-1,3,1,1),(-1,3,5,2,3)]:
  v=slope*x+shift;root=s.sqrt(v*v+radius*radius);t=s.log((v+root)/radius)
  f=1/(root*(A+B*t*t));expected=s.atan(t*s.sqrt(s.Rational(B,A)))/(slope*s.sqrt(A*B))
  raw=s.sstr(f).replace('**','^').replace('log(','ln(')
  for outer in [False,True]:
   e='integrate('+raw+',x)';e='simplify('+e+')' if outer else e
   out=run(e,stack);assert equal(parse(out),expected),(e,out,expected)
   assert equal(s.diff(t,x),slope/root)
   rows.append(dict(input=e,stack=stack,result=out,exact=True,method='Exact logarithmic derivative; positive radicand gap radius² proves global real log argument and positive quadratic denominator. Includes negative inner slope.'))
a.report.write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n');print('PASS',len(rows),'piecewise/rational-root checks')
