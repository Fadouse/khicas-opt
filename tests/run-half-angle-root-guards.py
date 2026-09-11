#!/usr/bin/env python3
"""Exact complete-period square-root tests, including every sign chart."""
import argparse,subprocess,os,json,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',required=True);a=p.parse_args();rows=[]
cases=[]
for wave in ['sin','cos']:
 for sign in [-1,1]:
  for frequency,shift,scale in [(1,'0',1),(2,'pi/3',3),(-3,'1',2)]:
   hi=f'2*pi/{abs(frequency)}';e=f'integrate(sqrt({scale}*(1+({sign})*{wave}({frequency}*x+({shift})))),x,0,{hi})'
   cases.append((e,4*s.sqrt(2*scale)/abs(frequency)))
for sign in [-1,1]:
 cases.append((f'integrate(sqrt(1+({sign})*sin(x)),x,2*pi,0)',-4*s.sqrt(2)))
for e,target in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   expression='simplify('+e+')' if outer else e;env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   row=dict(input=expression,stack=stack)
   try:
    r=subprocess.run([a.probe,expression],capture_output=True,text=True,env=env,timeout=10);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert r.returncode==0 and equal(parse(r.stdout.strip()),target),(r.returncode,r.stdout,r.stderr)
    row.update(passed=True,proof='Nonnegative half-angle square root is sqrt(2A)*absolute sine/cosine. Integral over a complete phase period equals 4sqrt(2A)/|frequency|, independent of phase; reversed bounds reverse sign.')
   except Exception as ex:row.update(passed=False,error=str(ex))
   rows.append(row)
Path(a.report).write_text(json.dumps(dict(probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
