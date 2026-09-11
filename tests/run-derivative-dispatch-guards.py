#!/usr/bin/env python3
"""Independent differentiation checks for the split recursive dispatcher."""
import argparse,json,subprocess,os,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
expressions=[]
u='x'
for k in range(8):
 u=('sin' if k%2 else 'cos')+'('+u+')';expressions.append(u)
for phase in ['x','2*x+1','x^2-1','x^3+x']:
 for op in ['exp','sinh','cosh','tanh','tan']:
  expressions.append(op+'('+phase+')')
expressions+=['(x^3+2)*(sin(x)+cos(x))','(1+x^2)/(2+cos(x))','1/((x+2)^3)','(sin(x)^2+cos(x)^3)^4','ln(abs(x+cos(x)))','ln((1+x^2)*(2+x^2))','exp(x*sin(x))','sin(x)*cos(x)*exp(x)','sin(x)^2+x^3+exp(x)+cos(x)','1/(1+exp(sin(x)))']
rows=[];cache={}
for index,e in enumerate(expressions):
 expected=s.diff(parse(e),x)
 for outer in (False,True):
  for stack in ('normal','64'):
   command='diff('+e+',x)';command='simplify('+command+')' if outer else command
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   row=dict(case=index,input=command,stack=stack)
   try:
    r=subprocess.run([a.probe,command],capture_output=True,text=True,env=env,timeout=10)
    row.update(exit=r.returncode,result=r.stdout.strip());assert r.returncode==0
    key=(index,r.stdout)
    if key not in cache:
     value=parse(r.stdout.strip())
     # For log|u| compare using its real derivative on u!=0.
     target=expected
     target=target.replace(lambda t:t.func==s.sign,lambda t:t.args[0]/s.Abs(t.args[0]))
     assert s.simplify(s.expand_trig(value-target))==0,(value,target)
     cache[key]=True
    row['passed']=True
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row)
a.report.write_text(json.dumps(dict(scope='38 expression families, direct/outer simplify and normal/64 KiB stack. Exact symbolic derivatives on each original differentiability component; dedicated historical branch/endpoint guards are separate.',source_sha256={n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in ('yderive.cc','ksubst.cc')},probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
