#!/usr/bin/env python3
"""Proof-dispatch acceptance and rejection, independent of full CAS fallback."""
import argparse,json,subprocess,hashlib
from pathlib import Path
from integration_build import ROOT,function,compiler_options
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
for n in ('equation_normalize.h','logarithmic_span.h'):(a.build_dir/n).write_bytes((ROOT/n).read_bytes())
out='#include "giacPCH.h"\n#include "logarithmic_span.h"\n#include <iostream>\nnamespace giac {\n'
for file,sigs in [('yintg.cc',['  static bool integration_punctured_guard(','  static bool integration_exponential_zero(']),('zintgab.cc',['  static bool intgab_continuous_finite(']),('ksubst.cc',['  static bool simplify_atan_addition('])]:
 for sig in sigs:out+=function((ROOT/file).read_text(),sig)
out+='}\nint main(int argc,char **argv){giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[2]),&c),r;std::string mode=argv[1];if(argc>3)giac::complex_mode(true,&c);unsigned budget=64;bool ok=mode=="continuous"?giac::intgab_continuous_finite(giac::eval(g,1,&c),x,budget,0,&c):mode=="puncture"?giac::integration_punctured_guard(g,x,&c):mode=="zero"?giac::integration_exponential_zero(g,x,r,&c):giac::simplify_atan_addition(g,r,&c);std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";}\n'
src=a.build_dir/'probe.cc';src.write_text(out);flags,libs=compiler_options();exe=a.build_dir/'probe';subprocess.run(flags+[str(src)]+libs+['-o',str(exe)],check=True)
cases=[]
for expr,ok in [('x^2=0',True),('sin(x)^4=0',False),('sin(x)^2=0',True),('x^4=0',True),('sin(x)^8=0',False),('x-x=0',False),('sin(x)^2+cos(x)^2-1=0',False),('exp(x)=0',True),('sin(1/x)=0',False),('tan(x)=0',False),('abs(x)=0',False),('a*x=0',False),('x^2/x=0',False),('2=0',True),('0=0',False),('x>0',False),('x=1',True),('ln(x)=0',False),('sqrt(x)=0',False)]:cases.append(('puncture',expr,ok,False))
for expr,ok in [('exp(x)-1',True),('2*exp(-3*x+1)-5',True),('-exp(2*x)+2',True),('3-2*exp(x)^(-1)',True),('exp(2*x+1)^(-1)-1',True),('3-2/exp(x)',False),('exp(x)+1',False),('exp(x)',False),('exp(x^2)-1',False),('exp(1/x)-1',False),('exp(x)-exp(2*x)',False),('a*exp(x)-1',False),('sin(exp(x))-1',False),('exp(x)+x',False)]:cases.append(('zero',expr,ok,False))
for expr,ok in [('atan(x)+atan(y)-atan((x+y)/(1-x*y))',True),('2*atan(x)+2*atan(y)-2*atan((x+y)/(1-x*y))+3',True),('atan(x^2)+atan(y+1)-atan((x^2+y+1)/(1-x^2*(y+1)))',True),('atan(x)+atan(y)-atan((x+y)/(1+x*y))',False),('atan(x)+atan(y)-atan((x*(x+y))/(x*(1-x*y)))',False),('atan(1/x)+atan(y)-atan((1/x+y)/(1-y/x))',False),('atan(sin(x))+atan(y)-atan((sin(x)+y)/(1-sin(x)*y))',False),('atan(i*x)+atan(y)-atan((i*x+y)/(1-i*x*y))',False),('atan(x)+atan(y)+atan((x+y)/(1-x*y))',False),('atan(x^8)+atan(y)-atan((x^8+y)/(1-x^8*y))',False)]:cases.append(('atan',expr,ok,False))
cases.extend([('zero','exp(x)-1',False,True),('atan','atan(x)+atan(y)-atan((x+y)/(1-x*y))',False,True)])
for expr,ok in [('tan(cos(x))',True),('tan(sin(x)/2+1/2)',True),('tan(2*sin(x))',False),('tan(x)',False),('tan(sin(x))/x^2',False),('1/x',False),('x*sin(x)^2',True),('tan(1/x)',False),('tan(cos(x))/(1+x^2)',False)]:cases.append(('continuous',expr,ok,False))
rows=[]
for mode,expression,accept,complex_mode in cases:
 r=subprocess.run([str(exe),mode,expression]+(['complex'] if complex_mode else []),capture_output=True,text=True,timeout=10)
 row=dict(mode=mode,input=expression,expected_accept=accept,complex_mode=complex_mode,exit=r.returncode,result=r.stdout.strip())
 row['passed']=r.returncode==0 and (r.stdout.startswith('ACCEPT ') if accept else r.stdout.strip()=='DEFER');rows.append(row)
a.report.write_text(json.dumps(dict(scope='Dispatch proof contracts: rejection only promises fallback, not completion by the fallback. Puncture nonidentity is proved by an exact nonzero Taylor coefficient, never numerical sampling.',source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','zintgab.cc','ksubst.cc','logarithmic_span.h')},runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
