#!/usr/bin/env python3
"""Check bounded determinant dispatch, including deliberate native fallback."""
import argparse,json,hashlib,subprocess
from pathlib import Path
from integration_build import ROOT,compiler_options
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
for name in ('determinant_small.h','equation_normalize.h'):(a.build_dir/name).write_bytes((ROOT/name).read_bytes())
src=a.build_dir/'probe.cc';src.write_text('#include "giacPCH.h"\n#include "determinant_small.h"\n#include <iostream>\nint main(int argc,char **argv){giac::context c;giac::gen g(std::string(argv[1]),&c),r;bool ok=giac::determinant_small(g,r,&c);std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";}\n')
flags,libs=compiler_options();exe=a.build_dir/'probe';subprocess.run(flags+[str(src)]+libs+['-o',str(exe)],check=True)
cases=[('[[1+x,y],[z,1]]',True,'generic polynomial'),('[[1+x^2/(x+y),x*y/(x+y)],[x*y/(x+y),1+y^2/(x+y)]]',True,'rank one'),('[[((x+y)+x^2)/(x+y),x*y/(x+y)],[x*y/(x+y),((x+y)+y^2)/(x+y)]]',True,'explicit common fraction'),('[[1,1/(x+y)],[0,1]]',True,'constant determinant preserves original matrix pole'),('[[0,x],[y,0]]',True,'zero pivots do not require division'),('[[1,2],[3,4]]',False,'native numeric path'),('[[x]]',False,'native one by one'),('[[x,y,z],[1,2,3]]',False,'non square'),('[x,y]',False,'not a matrix'),('[[[x,0],[0,1]],minor_det]',False,'option sequence'),('[[sin(x),1],[0,x]]',False,'non polynomial'),('[[sqrt(x),0],[0,1]]',False,'fractional power'),('[[1/(x/y),0],[0,1]]',False,'nested reciprocal retains inner hole'),('[[1/x,0],[0,1/y]]',False,'unrelated denominators'),('[[1/x+1/y,0],[0,1]]',False,'unrelated denominators within a sum'),('[[x^17,0],[0,1]]',False,'degree budget'),('[[x^(-17),0],[0,1]]',False,'denominator exponent budget'),('[[2^4096*x,0],[0,1]]',False,'coefficient bit budget')]
cases.append(('['+','.join('['+','.join('x' if i==j else '0' for j in range(5))+']' for i in range(5))+']',False,'dimension budget'))
rows=[]
for expression,accept,reason in cases:
 r=subprocess.run([str(exe),expression],capture_output=True,text=True,timeout=10)
 ok=r.returncode==0 and (r.stdout.startswith('ACCEPT ') if accept else r.stdout.strip()=='DEFER')
 rows.append(dict(input=expression,expected_accept=accept,reason=reason,exit=r.returncode,result=r.stdout.strip(),passed=ok))
a.report.write_text(json.dumps(dict(scope='Isolated production dispatch contracts; DEFER does not claim the native fallback is safe for arbitrary input. Full computed identities and excluded sets are tested separately.',source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('determinant_small.h','equation_normalize.h')},runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
