#!/usr/bin/env python3
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from conic_reference import check
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();cp=ROOT/'tests/conic-corpus.json';cases=json.loads(cp.read_text())['cases'];rows=[];cache={}
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
   expression='('+c['equation']+',[x,y],'+c['parameter']+')';args=[str(a.probe),expression]+([c['setup']] if 'setup' in c else [])
   row=dict(id=c['id'],input=expression,stack=stack,outer_simplify=outer)
   try:
    r=subprocess.run(args,env=env,capture_output=True,text=True,timeout=10);row.update(exit=r.returncode,stdout=r.stdout,stderr=r.stderr);assert r.returncode==0,row
    fields=dict(line.split(' ',1) for line in r.stdout.splitlines());raw=fields['RAW'];row['result']=raw
    key=(c['id'],raw)
    if key not in cache:cache[key]=check(c,raw)
    row['verification']=cache[key]
    if c['branches']:
     view=fields['VIEW'];assert view.count('x('+c['parameter']+')=')==c['branches'] and view.count('y('+c['parameter']+')=')==c['branches'],view
     row['label_nodes']=int(fields['NODES'].split()[1]);row['within_renderer_budget']=row['label_nodes']<256
    row['pass']=True
   except Exception as error:row.update(error=repr(error),**{'pass':False})
   rows.append(row);a.report.write_text(json.dumps({'scope':'Actual conic conversion and repository FXCG simplify. Independent exact residual and full real-chart coverage certificates, not only sample substitution. Display labels inspected; host stack is not CG50 MMU emulation.','source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ['kconvert.cc','ksubst.cc','equation_normalize.h','parametric_display.h']},'corpus_sha256':hashlib.sha256(cp.read_bytes()).hexdigest(),'runs':rows},indent=2)+'\n');print(c['id'],outer,stack,row['pass'],row.get('error','')[:220],flush=True)
assert all(r['pass'] for r in rows)
print('PASS',len(rows),'full curve coverage and label checks')
