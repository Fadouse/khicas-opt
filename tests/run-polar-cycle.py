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
p.add_argument('--polar-probe',type=Path)
a=p.parse_args();cases=json.loads(a.corpus.read_text())['cases'];rows=[];cache={}
saved=json.loads(a.saved_runs.read_text())['runs'] if a.saved_runs else []
report=dict(scope='Actual repository integration/yderive/FXCG simplify on host dependencies; guarded 64 KiB stack, not SH4/MMU emulation.',corpus_sha256=hashlib.sha256(a.corpus.read_bytes()).hexdigest(),probe_sha256=hashlib.sha256(a.probe.read_bytes()).hexdigest(),runs=rows)
if a.polar_probe:report['polar_probe_sha256']=hashlib.sha256(a.polar_probe.read_bytes()).hexdigest()
if not saved:report['instrumentation_sha256']={n:hashlib.sha256(Path('tests',n).read_bytes()).hexdigest() for n in ['integration_probe.cc','guarded_probe_stack.h']}
if not saved:report['source_sha256']={n:hashlib.sha256(Path(n).read_bytes()).hexdigest() for n in ['yintg.cc','zintgab.cc','ksubst.cc','yderive.cc','dilogarithm.h','kconvert.cc','equation_normalize.h','kusual.cc','zusual.cc','zprog.cc','conditional_eval.h','zvecteur.cc','determinant_small.h','logarithmic_span.h','elliptic_first.h','ksymbolic.cc']}
for c in cases:
 for outer in [False,True]:
  for stack in ['normal','64']:
   expression=c['input'];expression=expression[9:-1] if expression.startswith('simplify(') else expression
   if outer:expression='simplify('+expression+')'
   row=dict(id=c['id'],outer=outer,stack=stack,input=expression)
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   env.pop('KHICAS_OUTER_SIMPLIFY',None)
   command=[str(a.probe),expression]
   if c['type']=='polar':
    assert a.polar_probe is not None
    conversion='param2polar' if c['input'].startswith('param2polar(') else 'cart2polar'
    command=[str(a.polar_probe),'param' if conversion=='param2polar' else 'cart',c['input'][len(conversion):]]
    if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
   try:
    if saved:
     row.update(next(r for r in saved if r['id']==c['id'] and r['outer']==outer and r['stack']==stack))
    else:
     r=subprocess.run(command,capture_output=True,text=True,env=env,timeout=10 if c['id'] in ('PC12-R1','PC13-R1','PC14-R1','PC15-R1','PC16-R1','PC17-R1','PC18-R1') or c['id'].startswith(('PC18-','PC19-','PC20-')) else 12)
     row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    assert row['exit']==0,(row['exit'],row['result'][:400])
    if c['id'].startswith(('PC18-','PC19-','PC20-')):assert len(row['result'].encode())<=65536,'Output budget exceeded'
    key=(c['id'],row['result'])
    if key not in cache:cache[key]=verify(c,row['result'])
    row['verification']=cache[key];row['pass']=True
    if c['id'] in ('PC19-S1','PC20-S1','PC21-S1') and outer:assert row['verification'].get('normalized',False),'Explicit simplify must produce the proved guarded normal form'
    if c['id'].startswith('PC18-'):
     from polar_cycle18_reference import actual_checks
     row['actual_domain_checks']=actual_checks(c,expression,a.probe,env)
    if c['id'].startswith('PC19-'):
     from polar_cycle19_reference import actual_checks
     row['actual_domain_checks']=actual_checks(c,expression,a.probe,env)
    if c['id'].startswith('PC21-'):
     from polar_cycle21_reference import actual_checks
     row['actual_domain_checks']=actual_checks(c,expression,a.probe,env)
    if c['id'].startswith('PC20-'):
     from polar_cycle20_reference import actual_checks
     row['actual_domain_checks']=actual_checks(c,expression,a.probe,env)
    points={'PC1-D1':[-1,1],'PC2-D1':[0],'PC2-D2':[0,1],'PC3-I1':[0],'PC3-D2':[0]}.get(c['id'],[])
    if points:
     for point in points:
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=10 if c['id'] in ('PC12-R1','PC13-R1','PC14-R1','PC15-R1','PC16-R1','PC17-R1','PC18-R1') or c['id'].startswith(('PC18-','PC19-','PC20-')) else 12)
      assert r.returncode==0 and r.stdout.strip()=='0',(point,r.stdout,r.stderr)
     row['actual_endpoint_substitution']={str(point):0 for point in points}
    values={'PC4-I2':{2:'0'},'PC4-D1':{0:'0',1:'1',-1:'-1'},'PC4-D2':{-1:'-1',1:'2',2:'2'},'PC4-S1':{0:'2*i*pi'}}.get(c['id'],{})
    values.update({'PC5-I1':{0:'sqrt(3)*pi/6'},'PC5-I2':{-1:'-pi/4',1:'-pi/4'},'PC5-D1':{0:'0'},'PC5-D2':{-1:'-1/2',0:'-1',1:'-1/4',2:'7/4'},'PC5-S1':{1:'0'}}.get(c['id'],{}))
    values.update({'PC6-I1':{2:'0'},'PC6-I2':{0:'0'},'PC6-D1':{2:'0'},'PC6-D2':{-2:'0',0:'2/9',1:'undef',2:'4'},'PC6-S1':{1:'0'}}.get(c['id'],{}))
    values.update({'PC7-I1':{0:'0'},'PC7-I2':{'1/2':'pi/2-sqrt(2)*atan(sqrt(2))'},'PC7-D1':{-1:'cos(1)',1:'cos(1)',0:'1'},'PC7-D2':{-1:'-2',0:'0',1:'undef',2:'0'},'PC7-S1':{'-2*pi':'2*i*pi','-pi':'2*i*pi','pi':'0','2*pi':'-2*i*pi'}}.get(c['id'],{}))
    values.update({'PC8-I1':{0:'0'},'PC8-D1':{1:'3',0:'undef'},'PC8-D2':{0:'undef',1:'2',2:'2'},'PC8-S1':{-1:'-2*i',0:'0',1:'2*i'}}.get(c['id'],{}))
    values.update({'PC9-I1':{0:'0'},'PC9-I2':{0:'-(ln(2)+1)/2'},'PC9-D1':{0:'2',1:'cos(1)'},'PC9-D2':{-1:'exp(-1)',0:'1',1:'undef',2:'exp(1/2)/2'},'PC9-S1':{0:'0'}}.get(c['id'],{}))
    values.update({'PC10-I2':{1:'0'},'PC10-D1':{0:'0',1:'undef'},'PC10-D2':{-1:'1/2',0:'0',1:'3/4',2:'9/4'},'PC10-S1':{-1:'2*i*pi',1:'0'}}.get(c['id'],{}))
    values.update({'PC11-I2':{-1:'0',1:'0'},'PC11-D1':{0:'0','pi':'undef','-2*pi':'undef'},'PC11-D2':{-1:'1/2',0:'undef','1/2':'4/9',1:'1/4',2:'1/4'}}.get(c['id'],{}))
    values.update({'PC12-I1':{0:'0'},'PC12-I2':{1:'-4*ln(2)'},'PC12-D1':{0:'0','2*pi':'0','-2*pi':'0','pi':'undef','-pi':'undef'},'PC12-D2':{0:'1/2',-1:'1/4'}}.get(c['id'],{}))
    values.update({'PC13-D1':{0:'0','pi':'0','pi/2':'undef','-pi/2':'undef'},'PC13-D2':{0:'undef'}}.get(c['id'],{}))
    values.update({'PC14-D1':{0:'0','sqrt(pi)':'undef','-sqrt(2*pi)':'undef'},'PC14-D2':{0:'undef'},'PC14-R1':{**{-j:'undef' for j in range(21)},1:'20/21'}}.get(c['id'],{}))
    values.update({'PC15-I1':{0:'-18/2197'},'PC15-I2':{0:'0','pi':'59*pi/2048','-pi':'-59*pi/2048'},'PC15-D1':{0:'2','pi':'-2','pi/2':'0','-pi/2':'0'},'PC15-D2':{-1:'undef',1:'undef'},'PC15-R1':{**{sign*j:'undef' for j in range(2,13) for sign in [-1,1]},13:'infinity',-13:'infinity',0:'1/169',1:'0',-1:'0'}}.get(c['id'],{}))
    values.update({'PC16-I1':{1:'-2'},'PC16-D1':{0:'0',-1:'0',1:'exp(-1)'},'PC16-D2':{**{j:'0' for j in range(-3,4)},'1/2':'0','-1/2':'0'},'PC16-R1':{0:'2432902008176640000',-10:'2432902008176640000'}}.get(c['id'],{}))
    values.update({'PC17-I2':{0:'2/3',-1:'2*sqrt(2)/3',1:'2*sqrt(2)/3'},'PC17-D1':{0:'undef',1:'infinity','1/2':'pi/2'},'PC17-D2':{0:'0',-1:'-1/4',1:'1/4'},'PC17-S1':{0:'0','pi':'2','2*pi':'0','pi/2':'undef','3*pi/2':'undef'},'PC17-R1':{0:'1',1:'1024',-1:'0'}}.get(c['id'],{}))
    if values:
     from mixed_reference import parse,equal
     for point,expected in values.items():
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',x='+str(point)+'))'],capture_output=True,text=True,env=env,timeout=10 if c['id'] in ('PC12-R1','PC13-R1','PC14-R1','PC15-R1','PC16-R1','PC17-R1','PC18-R1') or c['id'].startswith(('PC18-','PC19-','PC20-')) else 12)
      if expected in ('undef','infinity'):assert r.returncode==(3 if expected=='undef' else 0) and r.stdout.strip()==expected,(point,r.stdout,r.stderr)
      else:assert r.returncode==0 and equal(parse(r.stdout.strip()),parse(expected)),(point,r.stdout,r.stderr)
     row['actual_boundary_substitution']=values
    if c['id']=='PC16-S1':
     samples=[('pi/2','pi/2','undef'),('pi/4','pi/4','undef'),('0','0','0'),('pi/4','-pi/4','0'),('pi','pi','0')]
     for px,py,expected in samples:
      r=subprocess.run([str(a.probe),'eval(subst('+expression+',[x,y],['+px+','+py+']))'],capture_output=True,text=True,env=env,timeout=10)
      assert r.returncode==(3 if expected=='undef' else 0) and r.stdout.strip()==expected,(px,py,r.returncode,r.stdout)
     row['actual_two_variable_samples']=samples
    if c['id']=='PC10-R1':
     r=subprocess.run([str(a.probe),'eval(subst('+expression+',[x,y],[0,0]))'],capture_output=True,text=True,env=env,timeout=10 if c['id'] in ('PC12-R1','PC13-R1','PC14-R1','PC15-R1','PC16-R1','PC17-R1','PC18-R1') or c['id'].startswith(('PC18-','PC19-','PC20-')) else 12)
     assert r.returncode==3 and r.stdout.strip()=='undef',(r.stdout,r.stderr)
     row['actual_origin_exclusion']='undef'
   except Exception as e:row.update(error=str(e),**{'pass':False})
   rows.append(row);a.report.write_text(json.dumps(report,indent=2)+'\n')
   print(c['id'],outer,stack,row['pass'],row.get('error','')[:200],flush=True)
report['cases']=[dict(id=c['id'],all_four_pass=all(r['pass'] for r in rows if r['id']==c['id'])) for c in cases]
a.report.write_text(json.dumps(report,indent=2)+'\n')
assert all(r['pass'] for r in rows)
