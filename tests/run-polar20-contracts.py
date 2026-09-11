#!/usr/bin/env python3
"""Bounded rule acceptance/rejection, independent of fallback completion."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from integration_build import ROOT,function,compiler_options

p=argparse.ArgumentParser()
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
for name in ('equation_normalize.h','conditional_eval.h'):
 (a.build_dir/name).write_bytes((ROOT/name).read_bytes())
source='#include "giacPCH.h"\n#include "conditional_eval.h"\n#include <iostream>\nnamespace giac {\n'
for file,signature in [('yintg.cc','  static bool integrate_parameter_quadratic('),('ksubst.cc','  static bool simplify_conjugate_roots('),('yderive.cc','  static bool derive_floor_phase(')]:
 source+=function((ROOT/file).read_text(),signature)
source+='''}
int main(int argc,char **argv){
 giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[2]),&c),r,d;
 std::string mode=argv[1];if(argc>3)giac::complex_mode(true,&c);
 unsigned budget=32;giac::vecteur roots;bool ok=false;
 if(mode=="constant")ok=giac::conditional_algebraic_constant(g,roots,budget,0,&c);
 else if(mode=="equal")ok=giac::conditional_exact_equal(g,giac::gen(0),&c);
 else if(mode=="roots")ok=giac::simplify_conjugate_roots(g,r,&c);
 else {g=giac::eval(g,1,&c);ok=mode=="phase"?giac::derive_floor_phase(g,x,r,d,&c):giac::integrate_parameter_quadratic(g,x,r,&c);}
 std::cout<<(ok?"ACCEPT":"DEFER")<<"\\n";
}
'''
src=a.build_dir/'probe.cc';src.write_text(source)
flags,libs=compiler_options();exe=a.build_dir/'probe'
subprocess.run(flags+[str(src)]+libs+['-o',str(exe)],check=True)
cases=[]
groups={
 'quadratic':[
  ('1/(x^2+2*a*x+1)',True),('1/(a*x^2+b*x+1)',True),
  ('1/(a*x+1)',True),('1/((x+a)^2+b)',True),
  ('1/(x^3+a)',False),('1/(x^2+sin(a)*x+1)',False),
  ('1/(a*x^2+b*x+c)',False),('1/(x^2+a^8)',False),
  ('1/(x^2+i*a*x+1)',False),('sin(x)/(x^2+a)',False)],
 'phase':[
  ('floor(x)',True),('floor(-2*x+1)',True),('x^2*floor(1/x)',True),
  ('floor((2*x+1)/(x-1))',True),('floor(2/(x-1)+1/3)',True),
  ('floor(x^2)',False),('floor(1/x^2)',False),('floor(sin(x))',False),
  ('floor(a*x)',False),('floor(x)+floor(2*x)',False),
  ('floor(1/(x^2-1))',False),('x^2',False)],
 'roots':[
  ('sqrt(x+sqrt(x^2-y^2))+sqrt(x-sqrt(x^2-y^2))',True),
  ('sqrt(2*x+sqrt(4*x^2-y^2))+sqrt(2*x-sqrt(4*x^2-y^2))',True),
  ('sqrt(x+2*sqrt(x^2-y^2))+sqrt(x-2*sqrt(x^2-y^2))',True),
  ('sqrt(x+sqrt(x^2-y^2))-sqrt(x-sqrt(x^2-y^2))',False),
  ('sqrt(x+sqrt(x^2-y^2))+sqrt(x+sqrt(x^2-y^2))',False),
  ('sqrt(x+sqrt(x^2-y^2))+sqrt(x-sqrt(x^2-y^2))+1',False),
  ('sqrt(x+sqrt(x^2-sin(y)^2))+sqrt(x-sqrt(x^2-sin(y)^2))',False),
  ('sqrt(x+sqrt(x^2-y^8))+sqrt(x-sqrt(x^2-y^8))',False)],
 'constant':[
  ('3/7',True),('sqrt(3)',True),('(2+sqrt(3))^2-7-4*sqrt(3)',True),
  ('sqrt(3)+1/sqrt(3)',True),('sqrt(2)+sqrt(3)',False),
  ('sqrt(1+sqrt(2))',False),('sqrt(-1)',False),('sqrt(x)',False),
  ('0.00000000001',False),('exp(1)',False),('sin(1)',False),
  ('(2+sqrt(3))^9',False)],
 'equal':[
  ('(-2+sqrt(3))^2+4*(-2+sqrt(3))+1',True),
  ('(-2-sqrt(3))^2+4*(-2-sqrt(3))+1',True),
  ('(1+sqrt(2))*(1-sqrt(2))+1',True),
  ('(-2+sqrt(3))^2+4*(-2+sqrt(3))+1+1/1000000',False),
  ('cos((-sqrt(2*pi))^2)-1',True),('x-x',False)]}
for mode,items in groups.items():
 for expression,accepted in items:cases.append((mode,expression,accepted,False))
for mode in ('quadratic','roots'):
 cases.append((mode,groups[mode][0][0],False,True))
rows=[]
for mode,expression,accepted,complex_mode in cases:
 r=subprocess.run([str(exe),mode,expression]+(['complex'] if complex_mode else []),capture_output=True,text=True,timeout=10)
 rows.append(dict(mode=mode,input=expression,expected_accept=accepted,complex_mode=complex_mode,exit=r.returncode,result=r.stdout.strip(),passed=r.returncode==0 and r.stdout.strip()==('ACCEPT' if accepted else 'DEFER')))
a.report.write_text(json.dumps(dict(scope='Independent direct dispatch contracts. DEFER promises only rejection by this bounded rule, not completion or correctness of the legacy fallback. Constant equality uses exact algebra, never a floating tolerance.',source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','ksubst.cc','yderive.cc','conditional_eval.h','equation_normalize.h')},runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows))
assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
