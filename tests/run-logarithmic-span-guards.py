#!/usr/bin/env python3
"""Independent exact logarithmic-span identities and rejection contracts."""
import argparse,json,subprocess,hashlib,os
from pathlib import Path
import sympy as s
from integration_build import ROOT,compiler_options,function
from mixed_reference import x,parse,equal
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
for name in ('logarithmic_span.h','equation_normalize.h'):(a.build_dir/name).write_bytes((ROOT/name).read_bytes())
out='#include "giacPCH.h"\n#include "logarithmic_span.h"\n#include <iostream>\nnamespace giac {\n'+function((ROOT/'yintg.cc').read_text(),'  static gen integration_syntax(')+'}\n'
out+='int main(int argc,char **argv){giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[1]),&c),r;g=giac::integration_syntax(giac::eval(g,1,&c),&c);bool ok=giac::integrate_logarithmic_span(g,x,r,&c);std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";}\n'
src=a.build_dir/'probe.cc';src.write_text(out);flags,libs=compiler_options();exe=a.build_dir/'probe';subprocess.run(flags+[str(src)]+libs+['-o',str(exe)],check=True)
def giac(e):return str(e).replace('**','^').replace('log(','ln(')
cases=[]
for fs in [[x+s.cos(x)],[2*s.exp(x)+s.cos(x)+s.sin(x)],[s.sin(x)+2],[x+s.exp(x)],[s.exp(s.sin(x))+2],[x,x+s.cos(x)],[x-1,s.exp(x)+s.cos(x)],[s.sin(x),x+2],[x,s.sin(x),s.exp(x)+1]]:
 for constant in (-2,s.Rational(1,2)):
  weights=[s.Rational((-1)**j*(j+1),3) for j in range(len(fs))]
  D=s.prod(fs);N=s.expand(constant*D+sum(weights[j]*s.diff(fs[j],x)*s.prod(fs[k] for k in range(len(fs)) if k!=j) for j in range(len(fs))))
  expression='('+giac(N)+')/('+giac(D)+')'
  cases.append((expression,constant+sum(weights[j]*s.diff(fs[j],x)/fs[j] for j in range(len(fs))),D))
rows=[]
for index,(expression,reference,D) in enumerate(cases):
 # Check that the production rule handles this whole family before the
 # full integrator gets any opportunity to use another algorithm.
 r=subprocess.run([str(exe),expression],capture_output=True,text=True,timeout=10)
 assert r.returncode==0 and r.stdout.startswith('ACCEPT '),(expression,r.stdout,r.stderr)
 for stack in ('normal','64'):
  env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
  if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
  for outer in (False,True):
   command='integrate('+expression+',x)';command='simplify('+command+')' if outer else command
   row=dict(case=index,input=command,stack=stack)
   try:
    r=subprocess.run([a.probe,command],env=env,capture_output=True,text=True,timeout=10)
    row.update(exit=r.returncode,result=r.stdout.strip());assert r.returncode==0 and 'integrate(' not in r.stdout
    F=parse(r.stdout.strip());regular=F
    if F.func==s.Piecewise:
     (value,condition),(regular,otherwise)=F.args
     assert value==s.Symbol('undef') and otherwise==True
     assert equal(condition.lhs-condition.rhs,D) or equal(condition.lhs-condition.rhs,-D)
    else:
     logs=[node for node in s.preorder_traversal(F) if node.func==s.log and node.args[0].func==s.Abs]
     assert D.is_positive or D.is_negative or len(logs)==1 and equal(logs[0].args[0].args[0],D),('Original pole condition missing',F,D)
    # On each connected original domain, d log|D_j| = D_j'/D_j.
    for node in list(s.preorder_traversal(regular)):
     if node.func==s.log and node.args[0].func==s.Abs:regular=regular.xreplace({node:s.log(node.args[0].args[0])})
    assert s.simplify(s.diff(regular,x)-reference)==0
    checks=[]
    for pt in (-2,0,1,2):
     if s.simplify(D.subs(x,pt))!=0:continue
     q=subprocess.run([a.probe,f'eval(subst({command},x={pt}))'],env=env,capture_output=True,text=True,timeout=10)
     assert q.stdout.strip() in ('undef','infinity','-infinity'),(pt,q.stdout)
     checks.append(pt)
    row.update(passed=True,excluded_points=checks)
   except Exception as e:row.update(passed=False,error=str(e))
   rows.append(row)
contracts=[]
for expression,reason in [('sin(x)/(x+cos(x))','not in the proposed span'),('cos(x)/sqrt(sin(x))','radical branch'),('cos(x)/ln(sin(x))','nested log domain'),('1/(x+tan(x))','hidden tangent poles'),('cos(x)/(y+sin(x))','additional parameter'),('cos(x)/(x^9+sin(x))','degree bound'),('cos(x)/(x*(x+1)*(x+2)*sin(x))','factor count'),('cos(x)/sin(x)^(-1)','inverse of nonentire expression')]:
 r=subprocess.run([str(exe),expression],capture_output=True,text=True,timeout=10)
 contracts.append(dict(input=expression,reason=reason,exit=r.returncode,result=r.stdout.strip(),passed=r.returncode==0 and r.stdout.strip()=='DEFER'))
a.report.write_text(json.dumps(dict(scope='18 independent affine combinations of logarithmic derivatives, four full modes plus isolated dispatch; exact derivative identities on complete original-domain components and original denominator exclusions. Rejections do not claim native fallback safety.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','yderive.cc','logarithmic_span.h')},runs=rows,contracts=contracts),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows),'contracts',len(contracts),sum(r['passed'] for r in contracts));assert all(r['passed'] for r in rows+contracts),[r for r in rows+contracts if not r['passed']]
