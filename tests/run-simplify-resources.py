#!/usr/bin/env python3
"""Isolated FXCG simplify stack failures and bounded output regressions."""
import argparse,hashlib,json,os,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,build,compiler_options,special_source
p=argparse.ArgumentParser();p.add_argument('--report',type=Path,required=True);p.add_argument('--build-dir',type=Path)
a=p.parse_args()
report={'scope':'Actual repository FXCG simplify and NO_STDEXCEPT. Host Giac dependencies, not SH4 emulation. Isolated raw nesting is constructed and compared on main stack.',
 'ksubst_sha256':hashlib.sha256((ROOT/'ksubst.cc').read_bytes()).hexdigest(),'runs':[]}
with tempfile.TemporaryDirectory(prefix='khicas-simplify-resources-') as tmp:
 d=a.build_dir or Path(tmp)/'target'
 exe=d/'probe' if a.build_dir else build(d,target_simplify=True)
 flags,libs=compiler_options();isolated=Path(tmp)/'isolated'
 subprocess.run(flags+[str(d/'simplify.cc'),str(ROOT/'tests/simplify-resource-boundary.cc'),str(special_source(Path(tmp)))]+libs+['-pthread','-o',str(isolated)],check=True)
 for eager in (False,True):
  env=dict(os.environ)
  if eager:env['LD_BIND_NOW']='1'
  r=subprocess.run([str(isolated)],capture_output=True,text=True,errors="replace",env=env,timeout=30)
  report['runs'].append({'id':'isolated-boundary','eager_binding':eager,'exit':r.returncode,'output':r.stdout,'stderr':r.stderr})
  a.report.write_text(json.dumps(report,indent=2)+'\n');assert r.returncode==0,report['runs'][-1];print(r.stdout.strip(),flush=True)
 cases=json.loads((ROOT/'tests/simplify-recursion-stress.json').read_text())['cases']
 cases += [{'id':'dilog-compact-'+str(i),'expression':e,'expected':e} for i,e in enumerate(['(1+Li2(x))^32','(1+Li2(x))^63','(Li2(x)+Li2(-x))^32'])]
 cases += [{'id':'compact-'+str(i),'expression':e,'expected':e} for i,e in enumerate(['(1+x)^1024/x','(1+x)^(-1024)','sqrt((x+1)^2048+1)','(1+sin(x))^128','(x+i)^2048+(x-i)^2048'])]
 cases += [{'id':'multivariate-'+str(i),'expression':e,'expected':e} for i,e in enumerate(['((1+x^2+y^2)^5+(1-x^2-y^2)^5)^5','(1+x+y+z+t)^12','1/(1+(x+y)^12)'])]
 cases += [{'id':'small-polynomial-cancellation','expression':'(x+y)^2-x^2-2*x*y-y^2','expected':'0'}]
 for case in cases:
  output=None
  for stack in ('normal','64'):
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']=stack
   r=subprocess.run([str(exe),'simplify('+case['expression']+')'],capture_output=True,text=True,env=env,timeout=15)
   expected=subprocess.run([str(exe),case['expected']],capture_output=True,text=True,timeout=15)
   row={'id':case['id'],'stack':stack,'input':case['expression'],'exit':r.returncode,'result':r.stdout.strip(),'stderr':r.stderr}
   row['pass']=r.returncode==0 and expected.returncode==0 and r.stdout==expected.stdout and (output is None or r.stdout==output)
   report['runs'].append(row);a.report.write_text(json.dumps(report,indent=2)+'\n');assert row['pass'],row
   output=r.stdout
 print('PASS:',2*len(cases),'complete-pipeline nested/large-power calls',flush=True)
