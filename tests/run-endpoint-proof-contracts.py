#!/usr/bin/env python3
"""Check mathematical endpoint acceptance AND deliberate fallback conditions."""
import argparse,json,subprocess,hashlib
from pathlib import Path
from integration_build import ROOT,function,compiler_options
from mixed_reference import parse,equal
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
source=(ROOT/'yintg.cc').read_text();out='#include "giacPCH.h"\n#include <iostream>\nnamespace giac {\n'
for signature in ['  static bool integration_rational(','  static bool integration_affine_atom(','  static bool integration_log_endpoint(']:out+=function(source,signature)
out+='}\nint main(int argc,char **argv){giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[1]),&c),p(std::string(argv[2]),&c),r;bool ok=giac::integration_log_endpoint(giac::eval(g,1,&c),x,giac::eval(p,1,&c),std::atoi(argv[3]),r,&c);std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";}\n'
path=a.build_dir/'probe.cc';path.write_text(out);flags,libs=compiler_options();exe=a.build_dir/'probe';subprocess.run(flags+[str(path)]+libs+['-o',str(exe)],check=True)
cases=[('x*ln(x)',0,1,'0'),('ln(x)-ln(x)/(1+x)-ln(1+x)',0,1,'0'),('-ln(x)',0,1,'+infinity'),('ln(x)',0,1,'-infinity'),('ln(x)+1/x',0,1,None),('ln(x)-1/x',0,1,None),('ln(x)-ln(2*x)',0,1,'-ln(2)'),('ln(abs(x))-ln(abs(-3*x))',0,-1,'-ln(3)'),('ln(abs(x))-ln(abs(-3*x))',0,1,'-ln(3)'),('ln(-x)-ln(x)',0,1,None),('ln(x)*ln(1+x)',0,1,None),('x*ln(-x)',0,-1,'0'),('x*ln(-x)',0,1,None),('x*ln(x^2)',0,1,None),('x*ln(a*x)',0,1,None),('sin(1/x)*ln(x)',0,1,None),('x^2*ln(x)/(1+x^2)',0,1,'0'),('ln(x)/(x-1)',0,1,'+infinity'),('ln(x)/ln(2)',0,1,None),('ln(x)',1,1,'0'),('ln(x)',-1,1,None),('ln(x)',0,0,None),('ln(x)+ln(x+1)/x',0,1,None)]
# Different affine zeros/scales and both real approach directions. The
# excluded endpoint is never evaluated as ln(0) before collecting terms.
for zero in (-2, 0, 3):
 for scale in (-5, 2):
  for direction in (-1, 1):
   cases.append((f'ln(abs(x-({zero})))-ln(abs({scale}*(x-({zero}))))', zero, direction, f'-ln({abs(scale)})'))
cases += [('ln(-2*abs(x))',0,1,None),
          ('ln(abs(x)*abs(x+1))',0,1,None),
          ('ln(abs(x))-ln(abs(x-1))',0,1,'-infinity'),
          ('ln(abs(x))-ln(abs(x-1))',0,-1,'-infinity'),
          ('ln(x)-ln(2*x)+1/x',0,1,None)]
rows=[]
for expression,point,direction,expected in cases:
 r=subprocess.run([str(exe),expression,str(point),str(direction)],capture_output=True,text=True,timeout=5);row=dict(input=expression,point=point,direction=direction,expected=expected,exit=r.returncode,result=r.stdout.strip())
 try:
  assert r.returncode==0
  if expected is None:assert r.stdout.strip()=='DEFER'
  elif 'infinity' in expected:assert r.stdout.strip() in ['ACCEPT '+expected,'ACCEPT '+expected.lstrip('+')]
  else:assert r.stdout.startswith('ACCEPT ') and equal(parse(r.stdout[7:].strip()),parse(expected))
  row['passed']=True
 except Exception as ex:row.update(passed=False,error=str(ex))
 rows.append(row)
a.report.write_text(json.dumps(dict(scope='Direct contract checks of actual endpoint proof helper; DEFER must preserve the generic fallback, not return a guessed finite value.',source_sha256=hashlib.sha256(source.encode()).hexdigest(),runs=rows),indent=2)+'\n');print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
