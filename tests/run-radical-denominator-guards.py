#!/usr/bin/env python3
"""Preserve additive radical denominators, their real domains and true poles."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as s
from mixed_reference import parse,x,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
cases=[]
for Q,points in [('1-x^2',['-1','0','1']),('1+x^2',['0','sqrt(3)','-sqrt(3)']),('x+2',['-2','-1','2']),('2-x',['2','1','-2'])]:
 for c in (1,2):
  for sign in ('+','-'):
   for n in (1,2,3):cases.append((f'1/({c}{sign}sqrt({Q}))^{n}',points))
for den,points in [('sqrt(1+x)+sqrt(1-x)',['-1','0','1']),('sqrt(2)+sqrt(x)',['0','2']),('sqrt(2)-sqrt(x)',['0','2']),('(1+sqrt(1+x^2))*(2-sqrt(1+x^2))',['0','sqrt(3)'])]:
 for n in (1,2):cases.append((f'1/({den})^{n}',points))
rows=[]
for idx,(expression,points) in enumerate(cases):
 original=parse(expression)
 for stack in ('normal','64'):
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  for repeat in (1,2):
   command='simplify('*repeat+expression+')'*repeat
   row=dict(case=idx,input=command,stack=stack)
   try:
    r=subprocess.run([a.probe,command],env=env,capture_output=True,text=True,timeout=10)
    row.update(exit=r.returncode,result=r.stdout.strip());assert r.returncode==0,(r.returncode,r.stderr)
    # Structural equality, not rational cancellation: unchanged radical
    # and inverse nodes carry the original domain on the entire real set.
    assert parse(r.stdout.strip())==original,(r.stdout,original)
    checks=[]
    for pt in points:
     ref=s.simplify(original.subs(x,parse(pt)))
     q=subprocess.run([a.probe,f'eval(subst({command},x={pt}))'],env=env,capture_output=True,text=True,timeout=10)
     if ref.has(s.zoo,s.oo,-s.oo,s.nan):assert q.stdout.strip() in ('undef','infinity','-infinity')
     else:assert q.returncode==0 and equal(parse(q.stdout.strip()),ref),(pt,q.stdout,ref)
     checks.append(dict(point=pt,expected=str(ref),actual=q.stdout.strip(),exit=q.returncode))
    row.update(passed=True,actual_point_checks=checks)
   except Exception as e:row.update(passed=False,error=str(e))
   rows.append(row)
a.report.write_text(json.dumps(dict(scope='56 radical denominator families, normal/64 KiB main-thread guarded stack and once/twice simplify. Exact structural retention proves the same radical/denominator domain; actual substitutions separately test conjugate zeros, true poles and root endpoints. Retention is not claimed as canonical simplification.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),source_sha256={n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in ('ksubst.cc','zusual.cc')},runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
