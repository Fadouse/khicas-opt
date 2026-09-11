#!/usr/bin/env python3
"""Run new conversion paths on a guarded 64KiB host computation stack."""
from pathlib import Path
import argparse,hashlib,json,os,shlex,subprocess,tempfile
from integration_build import function,special_source
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path)
p.add_argument('--target-simplify',action='store_true',help='Compile actual FXCG simplification and test conversion both directly and with outer simplify')
p.add_argument('--simplify-candidate',type=Path,help='Pending full ksubst source for the target simplification module')
args=p.parse_args()
cases=[
('cart','(x^3+y^3=3*x*y,[x,y],t)'),
('cart','(-27*x^3+81*x*y-27*y^3=0,[x,y],t)'),
('cart','(x^3+y^3=0,[x,y],t)'),
('cart','(x^3+y^3=3*a*x*y,[x,y],t)','assume(a>0)'),
('cart','((x^2+y^2)^2=2*(x^2-y^2),[x,y],t)'),
('cart','((x^2+y^2)^2=-2*(x^2-y^2),[x,y],t)'),
('cart','((x^2+y^2)^2=0,[x,y],t)'),
('cart','((x^2+y^2)^2/4=x^2-y^2,[x,y],t)'),
('cart','(x+y^5+y=0,[x,y],t)'),
('cart','(x^3+y^3+x^2+y^2=0,[x,y],t)'),
('cart','(x*(x^3+y^3-3*x*y)=0,[x,y],t)'),
('polar','(r^3=sin(theta),[r,theta],t)'),
('polar','(r^5=1-2*cos(theta),[r,theta],t)'),
('polar','(r^3/2+sin(theta)=0,[r,theta],t)'),
('polar','(r^9=sin(theta),[r,theta],t)'),
('polar','(r^3=-8,[r,theta],t)'),
('cartpolar','((x^2+y^2)^2=2*(x^2-y^2),[x,y],[r,theta])'),
('cartpolar','(x^3+y^3=3*x*y,[x,y],[r,theta])'),
('cartpolar','(x^3+y^3=3*a*x*y,[x,y],[r,theta])'),
('cartpolar','((x^2+y^2)^2/4=x^2-y^2,[x,y],[r,theta])')]
report=[]
with tempfile.TemporaryDirectory(prefix='khicas-equation-stack-') as tmp:
 d=Path(tmp);(d/'kconvert.cc').write_bytes((ROOT/'kconvert.cc').read_bytes());(d/'equation_normalize.h').write_bytes((ROOT/'equation_normalize.h').read_bytes())
 flags=[os.environ.get('CXX','c++'),'-std=c++11','-O2','-DHAVE_CONFIG_H','-DGIAC_GENERIC_CONSTANTS','-Wno-deprecated-declarations','-I',os.environ.get('GIAC_INCLUDE','/usr/include/giac')]+shlex.split(os.environ.get('CXXFLAGS',''))
 libs=shlex.split(os.environ.get('LDFLAGS',''))+['-lgiac','-pthread']
 libs += shlex.split(os.environ.get('GIAC_NUMERIC_LIBS', '-lgmp -lmpfr'))
 sources=[str(d/'kconvert.cc'),str(ROOT/'tests/equation_conversion_stack.cc')]
 if args.target_simplify:
  s=(args.simplify_candidate or ROOT/'ksubst.cc').read_text()
  text='#include "giacPCH.h"\n#include "equation_normalize.h"\n#define FXCG\n#define NO_STDEXCEPT\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
  text+='gen ataninv2atan(const gen &,GIAC_CONTEXT);\ngen cklin(const gen &,GIAC_CONTEXT);\n'
  text+=function((ROOT/'zprog.cc').read_text(),'  gen symb_prog3(')
  text+=function(s,'  gen tsimplify_noexpln(')
  if '  static unsigned simplify_special_terms(' in s:
   text+=function(s,'  static bool simplify_preflight(')+function(s,'  static gen simplify_shallow_leaf(')+function(s,'  static unsigned simplify_special_terms(')
   text+=function(s,'  static gen simplify_special_core(')
  text+=function(s,'  gen simplify(const gen & e_orig,GIAC_CONTEXT)')
  text+=function(s,'  gen _simplify(')+'}\n'
  norm='#include "giacPCH.h"\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
  s=(ROOT/'ysym2poly.cc').read_text()
  for sig in ('  static bool sort_func(', '  static vecteur sort1(', '  gen ratnormal(const gen & e,GIAC_CONTEXT)', '  gen recursive_ratnormal(const gen & e,GIAC_CONTEXT)'):
   norm+=function(s,sig)
  norm+='}\n'
  (d/'simplify.cc').write_text(text);(d/'normalize.cc').write_text(norm)
  sources += [str(d/'simplify.cc'),str(d/'normalize.cc'),str(special_source(d))]
 subprocess.run(flags+sources+libs+['-o',str(d/'probe')],check=True)
 for outer,case in [(outer,case) for outer in ([False,True] if args.target_simplify else [False]) for case in cases]:
  env=os.environ.copy();env.pop('KHICAS_OUTER_SIMPLIFY',None)
  if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
  try:
   r=subprocess.run([str(d/'probe'),*case],capture_output=True,text=True,timeout=10,env=env)
   row=dict(case=case,outer_simplify=outer,returncode=r.returncode,output=r.stdout,output_length=len(r.stdout.rstrip('\n')),diagnostics=r.stderr)
  except subprocess.TimeoutExpired:row=dict(case=case,outer_simplify=outer,status='timeout')
  report.append(row)
  print(('PASS' if row.get('returncode')==0 else 'FAIL'),'outer' if outer else 'direct',case[0],case[1])
if args.report:args.report.write_text(json.dumps(dict(stack_bytes=65536,guard_bytes=4096,target_simplify=args.target_simplify,scope='repository conversion source with host Giac dependencies and parser',source_sha256={name:hashlib.sha256((args.simplify_candidate if name=='ksubst.cc' and args.simplify_candidate else ROOT/name).read_bytes()).hexdigest() for name in ('kconvert.cc','equation_normalize.h','ksubst.cc','ysym2poly.cc','zprog.cc')},cases=report),indent=2)+'\n')
raise SystemExit(0 if all(r.get('returncode')==0 for r in report) else 1)
