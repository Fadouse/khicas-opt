#!/usr/bin/env python3
"""Independent families for endpoint limits, absolute compositions and radicals."""
import argparse,subprocess,os,json,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',required=True);args=p.parse_args();cases=[];rows=[];cache={}
def giac(e):return str(e).replace('**','^').replace('Abs(','abs(').replace('log(','ln(')
def add(e,kind,target,proof,points=None):cases.append(dict(input=e,kind=kind,target=target,proof=proof,points=points or {}))
# Independent integration-by-parts identity, rather than reading CAS rules.
for n in [2,3,4,6]:
 for a,b,L in [(1,0,s.Integer(1)),(2,1,s.Rational(2,3)),(-3,2,s.Integer(3))]:
  u=a*x+b;m=n-1
  value=((1-(1+L)**(-m))*s.log(L)-s.log(1+L)-sum((1-(1+L)**(1-j))/s.Integer(j-1) for j in range(2,m+1)))/m/a
  e=f'integrate(ln({giac(u)})/(1+({giac(u)}))^{n},x,{giac(-s.Rational(b,a))},{giac((L-b)/a)})'
  add(e,'value',value,'Set u=ax+b. With v=(1-(1+u)^(-m))/m, integration by parts and (1-y^(-m))/(y-1)=sum(y^(-j),j=1..m) prove the endpoint expression. u ln(u)->0 and denominator>=1 give convergence; negative a preserves oriented bounds.')
# H is analytic on t>=0; difference quotients, not individual abs derivatives,
# determine whether polynomial zeros remain differentiable.
t=s.Symbol('t',real=True)
Hs=[s.log(1+t)-t/(1+t),s.exp(t)-1-t,s.log(1+t*t),t*t/(1+t),1/(1+t)+t-1,s.log(2+t),t/(3+t),s.exp(t)]
for Q,zeros in [(x,[0]),(2*x-1,[s.Rational(1,2)]),(x*x-1,[-1,1]),(x**3,[0])]:
 for H in Hs:
  dH=s.diff(H,t);flat=dH.subs(t,0)==0;G=H.subs(t,s.Abs(Q));target=dH.subs(t,s.Abs(Q))*s.sign(Q)*s.diff(Q,x)
  points={giac(z):'0' if flat or s.diff(Q,x).subs(x,z)==0 else 'undef' for z in zeros}
  add('diff('+giac(G)+',x)','derivative',target,'Positive polynomial log arguments and denominators prove H analytic on [0,infinity). H prime(0)=0 gives H(|Q|)-H(0)=O(Q²); otherwise stationary polynomial zeros give O(h²) while simple zeros have unequal signed slopes.',points)
# Independent rational summands must not be combined into a huge denominator
# merely to prove their cusp. All denominators are positive; the negative
# right slope at zero proves a genuine cusp, while each regular derivative
# remains a short sum.
for count in (2, 4, 8):
 H=sum(1/(t+k)**8 for k in range(1,count+1))
 G=H.subs(t,s.Abs(x))
 add('diff('+giac(G)+',x)','derivative',s.diff(H,t).subs(t,s.Abs(x))*s.sign(x),
     'Every denominator is >=1 on t>=0. The right slope at zero is -8*sum(k^(-9)), strictly negative, so the original even function has a cusp. Differentiate each summand independently on both open half-lines.',{'0':'undef'})
 cases[-1]['output_limit']=2048
# Primitive derivative checks are independent of the coefficient-convolution
# implementation and apply over the entire affine-root interior.
for a,b in [(1,0),(-2,0),(1,2),(-2,3),(s.Rational(2,3),s.Rational(1,2))]:
 A=a*x+b
 for degree in [0,1,3,8]:
  C=x**degree+1
  for inverse in [False,True]:
   f=C/s.sqrt(A) if inverse else C*s.sqrt(A)
   add('integrate('+giac(f)+',x)','primitive',f,'On the whole affine half-line A>0, differentiate the printed primitive exactly. Real root limits give its finite endpoint normalization; inverse-root integrands keep their excluded endpoint.',{giac(-b/a):'0'})
for c,d in [(1,1),(2,3),(-2,-3)]:
 A=3*x+4;B=2-x;delta=c*c*A-d*d*B
 for degree in [0,1,3,8]:
  P=x**degree+1;f=delta*P/(c*s.sqrt(A)+d*s.sqrt(B));target=P*(c*s.sqrt(A)-d*s.sqrt(B))
  add('integrate('+giac(f)+',x)','primitive',target,'Complete real domain [-4/3,2]. Same-sign coefficients and distinct affine zeros make the original root sum nonzero everywhere. Conjugate identity proves equality even where delta=0; exact primitive derivative then verifies the whole interior.')
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   e='simplify('+c['input']+')' if outer else c['input'];env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   row=dict(input=e,stack=stack,kind=c['kind'])
   try:
    r=subprocess.run([args.probe,e],env=env,capture_output=True,text=True,timeout=10);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert r.returncode==0 and len(r.stdout)<c.get('output_limit',65536),(r.returncode,r.stdout,r.stderr)
    key=(c['input'],r.stdout)
    if key not in cache:
     a=parse(r.stdout.strip())
     if c['kind']=='derivative':
      while a.has(s.Piecewise):a=a.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
     if c['kind']=='primitive':a=s.diff(a,x)
     target=c['target']
     if c['kind']=='derivative':
      # On regular components sign(u)=u/|u|; actual root values are
      # checked separately so this proof cannot hide removable 0/0 holes.
      a=a.replace(lambda z:z.func==s.sign,lambda z:z.args[0]/s.Abs(z.args[0]))
      target=target.replace(lambda z:z.func==s.sign,lambda z:z.args[0]/s.Abs(z.args[0]))
     assert equal(a,target),(a,target)
     cache[key]=True
    for point,expected in c['points'].items():
     q=subprocess.run([args.probe,f'eval(subst({e},x={point}))'],env=env,capture_output=True,text=True,timeout=10)
     if expected=='undef':assert q.returncode==3 and q.stdout.strip()=='undef',(point,q.stdout)
     else:assert q.returncode==0 and equal(parse(q.stdout.strip()),parse(expected)),(point,q.stdout)
    row.update(passed=True,proof=c['proof'],actual_points=c['points'])
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row)
   Path(args.report).write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(args.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
   if not row['passed']:print('FAIL',e,stack,row.get('error','')[:180],flush=True)
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows)
