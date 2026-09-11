#!/usr/bin/env python3
"""Whole new batches: actual core, guarded stacks, independent mathematics."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from mixed_reference import verify
from integration_build import source
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--conic-probe',type=Path,required=True);p.add_argument('--corpus',type=Path,default=ROOT/'tests/mixed-round1-questions.json');p.add_argument('--report',type=Path,required=True);p.add_argument('--ref',default='current');a=p.parse_args()
cases=json.loads(a.corpus.read_text())['cases'];rows=[];cache={}
report={'scope':'Repository integration, derivative engine and FXCG simplify; host dependencies remain, not CG50 hardware/MMU emulation. Independent exact proofs and explicit domain/display checks.','ref':a.ref,'corpus_sha256':hashlib.sha256(a.corpus.read_bytes()).hexdigest(),'source_sha256':{n:hashlib.sha256(source(a.ref,n).encode()).hexdigest() for n in ['yintg.cc','zintgab.cc','ksubst.cc','yderive.cc','dilogarithm.h','kconvert.cc','parametric_display.h']},'probe_sha256':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [a.probe,a.conic_probe]},'runs':rows}
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   conic=c['type']=='conic'
   expression=c['input'];expression=expression[9:-1] if expression.startswith('simplify(') else expression
   if conic:
    expression='('+c['equation']+',[x,y],'+c['parameter']+')'
    if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
   elif outer:expression='simplify('+expression+')'
   row=dict(id=c['id'],input=expression,outer_simplify=outer,stack=stack)
   try:
    r=subprocess.run([str(a.conic_probe if conic else a.probe),expression],env=env,capture_output=True,text=True,timeout=12)
    row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr);assert r.returncode==0,row
    m=re.search(r'^SECONDS ([\d.e+-]+)',r.stderr,re.M)
    if m:row['host_seconds']=float(m[1])
    result=row['result']
    if conic:
     fields=dict(line.split(' ',1) for line in result.splitlines());result=fields['RAW'];row['coordinates']=result
     if c['branches']:
      assert fields['VIEW'].count('x('+c['parameter']+')=')==c['branches'] and fields['VIEW'].count('y('+c['parameter']+')=')==c['branches']
      row['label_nodes']=int(fields['NODES'].split()[1]);assert row['label_nodes']<=256
    key=(c['id'],result)
    if key not in cache:cache[key]=verify(c,result)
    row['verification']=cache[key];row['pass']=True
   except Exception as e:row.update(error=str(e)[:4000],**{'pass':False})
   rows.append(row);a.report.write_text(json.dumps(report,indent=2)+'\n');print(c['id'],outer,stack,row['pass'],row.get('error','')[:200],flush=True)
report['cases']=[{'id':c['id'],'all_four_pass':all(r['pass'] for r in rows if r['id']==c['id'])} for c in cases]
a.report.write_text(json.dumps(report,indent=2)+'\n')
assert all(r['pass'] for r in rows)
print('PASS',len(cases),'cases;',len(rows),'exact stack/mode checks')
