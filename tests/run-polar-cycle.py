#!/usr/bin/env python3
"""Run one immutable question batch in four modes, with independent proofs."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from polar_cycle_reference import verify
p=argparse.ArgumentParser()
p.add_argument('--probe',type=Path,required=True)
p.add_argument('--corpus',type=Path,default=Path('tests/polar-cycle1-questions.json'))
p.add_argument('--report',type=Path,required=True)
p.add_argument('--saved-runs',type=Path)
a=p.parse_args();cases=json.loads(a.corpus.read_text())['cases'];rows=[];cache={}
saved=json.loads(a.saved_runs.read_text())['runs'] if a.saved_runs else []
report=dict(scope='Actual repository integration/yderive/FXCG simplify on host dependencies; guarded 64 KiB stack, not SH4/MMU emulation.',corpus_sha256=hashlib.sha256(a.corpus.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),runs=rows)
if not saved:report['source_sha256']={n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in ['yintg.cc','zintgab.cc','ksubst.cc','yderive.cc','dilogarithm.h','kconvert.cc','equation_normalize.h','kusual.cc','zprog.cc','conditional_eval.h']}
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   expression=c['input'];expression=expression[9:-1] if expression.startswith('simplify(') else expression
   if outer:expression='simplify('+expression+')'
   row=dict(id=c['id'],outer=outer,stack=stack,input=expression)
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   try:
    if saved:
     row.update(next(r for r in saved if r['id']==c['id'] and r['outer']==outer and r['stack']==stack))
    else:
     r=subprocess.run([str(a.probe),expression],capture_output=True,text=True,env=env,timeout=12)
     row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert row['exit']==0,(row['exit'],row['result'][:400])
    key=(c['id'],row['result'])
    if key not in cache:cache[key]=verify(c,row['result'])
    row['verification']=cache[key];row['pass']=True
    points={'PC1-D1':[-1,1],'PC2-D1':[0],'PC2-D2':[0,1],'PC3-I1':[0],'PC3-D2':[0]}.get(c['id'],[])
    if points:
     for point in points:
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=12)
      assert r.returncode==0 and r.stdout.strip()=='0',(point,r.stdout,r.stderr)
     row['actual_endpoint_substitution']={str(point):0 for point in points}
    values={'PC4-I2':{2:'0'},'PC4-D1':{0:'0',1:'1',-1:'-1'},'PC4-D2':{-1:'-1',1:'2',2:'2'},'PC4-S1':{0:'2*i*pi'}}.get(c['id'],{})
    values.update({'PC5-I1':{0:'sqrt(3)*pi/6'},'PC5-I2':{-1:'-pi/4',1:'-pi/4'},'PC5-D1':{0:'0'},'PC5-D2':{-1:'-1/2',0:'-1',1:'-1/4',2:'7/4'},'PC5-S1':{1:'0'}}.get(c['id'],{}))
    values.update({'PC6-I1':{2:'0'},'PC6-I2':{0:'0'},'PC6-D1':{2:'0'},'PC6-D2':{-2:'0',0:'2/9',1:'undef',2:'4'},'PC6-S1':{1:'0'}}.get(c['id'],{}))
    values.update({'PC7-I1':{0:'0'},'PC7-I2':{'1/2':'pi/2-sqrt(2)*atan(sqrt(2))'},'PC7-D1':{-1:'cos(1)',1:'cos(1)',0:'1'},'PC7-D2':{-1:'-2',0:'0',1:'undef',2:'0'},'PC7-S1':{'-2*pi':'2*i*pi','-pi':'2*i*pi','pi':'0','2*pi':'-2*i*pi'}}.get(c['id'],{}))
    values.update({'PC8-I1':{0:'0'},'PC8-D1':{1:'3',0:'undef'},'PC8-D2':{0:'undef',1:'2',2:'2'},'PC8-S1':{-1:'-2*i',0:'0',1:'2*i'}}.get(c['id'],{}))
    if values:
     from mixed_reference import parse,equal
     for point,expected in values.items():
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=12)
      if expected=='undef':assert r.returncode==3 and r.stdout.strip()=='undef',(point,r.stdout,r.stderr)
      else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(point,r.stdout,r.stderr)
     row['actual_boundary_substitution']=values
   except Exception as e:row.update(error=str(e),**{'pass':False})
   rows.append(row);a.report.write_text(json.dumps(report,indent=2)+'\n')
   print(c['id'],outer,stack,row['pass'],row.get('error','')[:200],flush=True)
report['cases']=[dict(id=c['id'],all_four_pass=all(r['pass'] for r in rows if r['id']==c['id'])) for c in cases]
a.report.write_text(json.dumps(report,indent=2)+'\n')
assert all(r['pass'] for r in rows)
