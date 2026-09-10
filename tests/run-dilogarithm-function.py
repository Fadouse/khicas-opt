#!/usr/bin/env python3
"""Real CAS registration, derivative metadata, numerical branches and pole tests."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import mpmath as mp
import sympy as sp
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
rows=[];mp.mp.dps=60
checks=[('registration','type(Li2)','func'),('zero','Li2(0)','0'),('exact-one','simplify(Li2(1)-pi^2/6)','0'),('exact-minus-one','simplify(Li2(-1)+pi^2/12)','0'),('exact-half','simplify(Li2(1/2)-pi^2/12+ln(2)^2/2)','0'),('derivative','simplify(diff(Li2(x),x)+ln(1-x)/x)','0'),('chain-derivative','simplify(diff(Li2(x^3),x)+3*ln(1-x^3)/x)','0'),('derivative-zero','subst(diff(Li2(x),x),x=0)','undef'),('vector','Li2([0,1,-1])','[0,pi*pi/6,-pi*pi/12]')]
# The printed generic derivative has a removable 0/0 at zero. The derivative
# callback knows Li2'(0)=1, but substituting into the printed formula is not
# a limit operation. Check its limit independently in the CAS too.
checks.append(('derivative-zero-limit','limit(diff(Li2(x),x),x=0)','1'))
for f,sign in [('ln(1+x)/x^2','+'),('ln(1-x)/x^2','-'),('ln(1+x^2)/x^3','+'),('ln(1-x^3)/x^5','-'),('ln(1+2*x^7)/x^8','+')]:
 checks.append(('endpoint-pole-'+f,'simplify(integrate('+f+',x,0,1))',sign+'infinity'))
checks.append(('reversed-pole','simplify(integrate(ln(1+x)/x^2,x,1,0))','-infinity'))
for id,expression,expected in checks:
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(a.probe),expression],capture_output=True,text=True,env=env,timeout=5)
  row=dict(id=id,input=expression,stack=stack,exit=r.returncode,result=r.stdout.strip(),expected=expected,stderr=r.stderr)
  row['pass']=r.returncode==(3 if expected=='undef' else 0) and row['result']==expected
  rows.append(row)
points=[complex(x,y) for x in [-10,-1,-.5,0,.25,.5,.75,1,2,100] for y in [0,-1,1,-1e-10,1e-10]]+[complex(1e-300),complex(-1e-300),complex(1e300),complex(-1e300)]
for z in points:
 expression='Li2(('+format(z.real,'.17e')+')+i*('+format(z.imag,'.17e')+'))'
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(a.probe),expression],capture_output=True,text=True,env=env,timeout=5)
  row=dict(id='numeric',input=expression,stack=stack,exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
  try:
   actual=sp.sympify(row['result'].replace('^','**'),locals={'i':sp.I,'ln':sp.log})
   real,imag=actual.as_real_imag();value=mp.mpc(str(real.evalf(30)),str(imag.evalf(30)))
   reference=mp.polylog(2,mp.mpc(z.real,z.imag));error=abs(value-reference)/(1+abs(reference))
   row.update(reference=[str(reference.real),str(reference.imag)],scaled_error=str(error))
   row['pass']=r.returncode==0 and error<mp.mpf('5e-11')
   if z and abs(z)<1e-100:row['pass']=row['pass'] and abs(value/reference-1)<mp.mpf('5e-11')
  except Exception as error:row.update(error=repr(error),**{'pass':False})
  rows.append(row)
a.report.write_text(json.dumps({'scope':'Actual host CAS function registration, derivatives and endpoint handling on normal/64 KiB stacks; 12-digit printed numerical values compared independently to mpmath. Double precision, not arbitrary precision or CG50 hardware acceptance.','source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ['yintg.cc','ksubst.cc','dilogarithm.h']},'probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'runs':rows},indent=2)+'\n')
failed=[r for r in rows if not r['pass']]
for row in failed:print('FAIL',row)
assert not failed,len(failed)
print('PASS',len(rows),'CAS function/branch/pole checks')
