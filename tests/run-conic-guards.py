#!/usr/bin/env python3
"""Do not silently divide away parameter factors which may be zero."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
cases=[('(x^2+y^2=1,[x,y],x)',None),('(0=0,[x,y],t)',None),('(a*(x*y)=0,[x,y],t)',None),('(a*(x^2+y^2-1)=0,[x,y],t)',None),('(a^2*(x^2+y^2-1)=0,[x,y],t)',None),('((a^2+b^2)*(x^2+y^2-1)=0,[x,y],t)',None),('((a-b)^2*(x^2+y^2-1)=0,[x,y],t)','[assume(a>0),assume(b>0)]'),('(a*(x^2+y^2-1)=0,[x,y],t)','a:=0')]
rows=[]
for expression,setup in cases:
 for stack in ['normal','64']:
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  r=subprocess.run([str(a.probe),expression]+([setup] if setup else []),capture_output=True,text=True,env=env,timeout=5)
  row=dict(input=expression,setup=setup,stack=stack,exit=r.returncode,result=r.stdout,stderr=r.stderr)
  row['pass']=r.returncode==3 and 'RAW undef' in r.stdout and ('Parameter factor may vanish' in r.stderr or 'curve' in r.stderr or 'coordinate' in r.stderr)
  rows.append(row)
a.report.write_text(json.dumps({'scope':'Actual conversion declines whole-plane/collision inputs and unproved scalar factors; squared or summed-square factors cannot be assumed nonzero. Exceptions are caught by the host harness and reported as errors, not process crashes.','source_sha256':{'kconvert.cc':hashlib.sha256((ROOT/'kconvert.cc').read_bytes()).hexdigest()},'runs':rows},indent=2)+'\n')
for r in rows:
 if not r['pass']:print('FAIL',r)
assert all(r['pass'] for r in rows)
print('PASS',len(rows),'condition/error checks')
