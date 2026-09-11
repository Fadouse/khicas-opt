#!/usr/bin/env python3
"""Direct PC21 rule bounds and independent double-precision RF checks."""
import argparse,hashlib,json,subprocess,math
from pathlib import Path
import mpmath as mp
from integration_build import ROOT,function,compiler_options
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
for name in ('equation_normalize.h','elliptic_first.h'):(a.build_dir/name).write_bytes((ROOT/name).read_bytes())
src='#include "giacPCH.h"\n#include "equation_normalize.h"\n#include "elliptic_first.h"\n#include <iostream>\n#include <iomanip>\nnamespace giac {\n'
for file,sig in [('yintg.cc','  static bool integrate_elliptic_quartic('),('yderive.cc','  static bool derive_minmax_contact('),('yderive.cc','  static bool derive_squared_affine_radical('),('ksubst.cc','  static bool simplify_minmax_clamp(')]:src+=function((ROOT/file).read_text(),sig)
src+='''}
int main(int argc,char **argv){
 std::string mode=argv[1];
 if(mode=="rf") {double v;bool ok=giac::elliptic_first_rf(std::stod(argv[2]),std::stod(argv[3]),std::stod(argv[4]),v);std::cout<<std::setprecision(17)<<(ok?"ACCEPT ":"DEFER ");if(ok)std::cout<<v;std::cout<<"\\n";return 0;}
 giac::context c;if(argc>3)giac::complex_mode(true,&c);
 giac::gen x(giac::identificateur("x")),g(std::string(argv[2]),&c),r;
 // The elliptic dispatcher receives evaluated reciprocal syntax.
 if(mode=="elliptic")g=g.eval(1,&c);
 bool ok=mode=="minmax"?giac::derive_minmax_contact(g,*x._IDNTptr,r,&c):mode=="radical"?giac::derive_squared_affine_radical(g,*x._IDNTptr,r,&c):mode=="clamp"?giac::simplify_minmax_clamp(g,r,&c):giac::integrate_elliptic_quartic(g,x,r,&c);
 std::cout<<(ok?"ACCEPT":"DEFER")<<"\\n";
}
'''
path=a.build_dir/'probe.cc';path.write_text(src);exe=a.build_dir/'probe';flags,libs=compiler_options();subprocess.run(flags+[str(path)]+libs+['-o',str(exe)],check=True)
# The factored shifted fourth power costs 17 syntactic terms before
# normalization, exceeding the rule's 16-term preflight. Its expanded
# five-term polynomial is accepted; the full integrator is tested separately.
groups={
'elliptic': [('1/sqrt(1-x^4)',True),('1/sqrt(2-3*(x-1)^4)',False),('1/sqrt(-3*x^4+12*x^3-18*x^2+12*x-1)',True),('1/sqrt(1+x^2-x^4)',True),('1/sqrt(1+x^4)',False),('1/sqrt(x^4-1)',False),('1/sqrt(-x^4)',False),('1/sqrt(a-x^4)',False),('1/sqrt(1+x-x^4)',False),('1/sqrt(1-x^8)',False)],
'minmax': [('max(x^2,a*x)',True),('min((x-1)^2,0)',True),('max(x^3,-x^3)',True),('max(x^8,0)',True),('max(x^9,0)',False),('max(x,1/x)',False),('max(sin(x),x)',False),('max(i*x,x)',False),('max(x,a,b)',False)],
'radical': [('sqrt(x^2*(x-1))',True),('sqrt(x^2*(x+1))',True),('sqrt(x^3)',True),('sqrt((2*x-1)^2*(3-2*x))',True),('sqrt(x^2)',False),('sqrt(x^4*(x-1))',False),('sqrt(x^2*(x^2+1))',False),('sqrt(x^2*(a*x+1))',False),('sqrt(sin(x)^2*(x-1))',False)],
'clamp': [('max(a,min(x,b))-min(b,max(x,a))',True),('min(b,x)',False),('max(a,min(x,b))+min(b,max(x,a))',False),('max(a,min(x,b))-min(b,max(x,c))',False),('max(a,min(sin(x),b))-min(b,max(sin(x),a))',False)]}
rows=[]
for mode,cases in groups.items():
 for expression,accept in cases+[(cases[0][0],False)]:
  complex_mode=(expression==cases[0][0] and not accept)
  r=subprocess.run([str(exe),mode,expression]+(['complex'] if complex_mode else []),capture_output=True,text=True,timeout=10)
  rows.append(dict(kind=mode,input=expression,complex_mode=complex_mode,expected_accept=accept,result=r.stdout.strip(),exit=r.returncode,passed=r.returncode==0 and r.stdout.strip()==('ACCEPT' if accept else 'DEFER')))
mp.mp.dps=70
for triple in [(1,1,1),(0,1,1),(0,1e-6,1),(1e-12,1e-6,1),(1,2,3),(3,2,1),(0,1,1000),(1e-100,1,1e100)]:
 for scale in [1e-100,1,1e100]:
  args=[v*scale for v in triple];r=subprocess.run([str(exe),'rf']+[str(v) for v in args],capture_output=True,text=True,timeout=10)
  row=dict(kind='rf',arguments=args,result=r.stdout.strip(),exit=r.returncode)
  try:
   assert r.returncode==0 and r.stdout.startswith('ACCEPT ')
   actual=mp.mpf(r.stdout.split()[1]);expected=mp.elliprf(*map(mp.mpf,args))
   assert abs(actual-expected)<=mp.mpf('2e-14')*abs(expected),(actual,expected)
   row.update(passed=True,expected=str(expected))
  except Exception as ex:row.update(passed=False,error=str(ex))
  rows.append(row)
for args in [(-1,1,1),(0,0,1),(0,0,0),(math.inf,1,1),(math.nan,1,1)]:
 r=subprocess.run([str(exe),'rf']+[str(v) for v in args],capture_output=True,text=True,timeout=10)
 rows.append(dict(kind='rf-reject',arguments=list(map(str,args)),result=r.stdout.strip(),exit=r.returncode,passed=r.returncode==0 and r.stdout.startswith('DEFER')))
a.report.write_text(json.dumps(dict(scope='Extracted production helpers: acceptance/defer is not a guarantee about legacy fallback. RF uses actual production double arithmetic independently of gen tagged-number storage; 70-digit independent references and invalid-domain rejection.',source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ['yintg.cc','yderive.cc','ksubst.cc','elliptic_first.h','equation_normalize.h']},runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
