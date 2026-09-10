#!/usr/bin/env python3
"""Root acceptance of the fresh question-only agent's eight cases."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import sympy as sp
p=argparse.ArgumentParser();p.add_argument('--probe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();root=Path(__file__).resolve().parents[1];cp=root/'tests/parameter-agent-questions.json';cases=json.loads(cp.read_text())['cases']
A,B,C,S=sp.symbols('a b c s');local={'a':A,'b':B,'c':C,'s':S,'re':sp.re,'ln':sp.log,'Gamma':sp.gamma,'Li2':lambda z:sp.polylog(2,z),'i':sp.I}
def parse(t):return sp.sympify(t.replace('^','**').replace(' and ',' & '),locals=local)
contract={'PARAM-Q1':(['a'],'re(a)>0'),'PARAM-Q3':(['s','c'],'(re(c)>0) & (re(s)>0)'),'PARAM-Q4':(['a','b'],'(re(a)>0) & (re(b)>0)')}
rows=[]
for c in cases:
 for outer in [False,True]:
  e='integrate('+c['f']+',x'+(','+','.join(c['bounds']) if 'bounds' in c else '')+')';e='simplify('+e+')' if outer else e
  if c['id'] in contract:
   params,_=contract[c['id']];e='['+','.join(['assume('+v+',complex)' for v in params]+[e])+']['+str(len(params))+']'
  for stack in ['normal','64']:
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   row=dict(id=c['id'],input=e,stack_KiB=stack,outer_simplify=outer)
   try:
    r=subprocess.run([str(a.probe),e],capture_output=True,text=True,env=env,timeout=5);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
    if c.get('behavior')=='divergent':
     assert r.returncode==3 and row['result']=='undef' and 'Divergent improper integral' in r.stderr;row['status']='divergence-rejected'
    elif c['id'] in contract:
     assert r.returncode in [0,2];value=row['result'];assert value[0]=='(' and value[-1]==')';guard,branches=value[1:-1].split('?',1);yes,no=branches.rsplit(':',1)
     assert parse(guard)==parse(contract[c['id']][1]);assert sp.simplify((parse(yes)-parse(c['expected'])).rewrite(sp.exp))==0
     if c['id']=='PARAM-Q3':assert ('quote(' in no or no.strip().startswith("'integrate(")) and 'integrate(' in no
     else:assert no.strip()=='undef'
     row['status']='exact-under-explicit-complex-conditions'
    else:
     assert r.returncode==0;assert sp.simplify(parse(row['result'])-parse(c['expected']))==0;row['status']='exact'
   except Exception as error:row['status']='unresolved-or-failed';row['verification_error']=repr(error)
   rows.append(row)
a.report.write_text(json.dumps({'scope':'Root independently verifies whole-pipeline answers and complex condition predicates for eight agent-generated cases. A remaining unevaluated integral is not a pass.','corpus_sha256':hashlib.sha256(cp.read_bytes()).hexdigest(),'source_sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in ['yintg.cc','ksubst.cc','dilogarithm.h']},'runs':rows},indent=2)+'\n')
for c in cases:print(c['id'],sorted({r['status'] for r in rows if r['id']==c['id']}))
