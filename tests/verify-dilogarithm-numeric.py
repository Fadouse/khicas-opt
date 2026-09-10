#!/usr/bin/env python3
"""Verify the numerical core extracted from the actual production header."""
import argparse, hashlib, json, random, subprocess, tempfile
from pathlib import Path
import mpmath as mp
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path,required=True);a=p.parse_args()
h=ROOT/'dilogarithm.h';s=h.read_text();end=s.rfind('#if',0,s.index('static gen d_Li2('))
source='#include <complex>\n#include <cmath>\n#include <iostream>\n#include <iomanip>\n'+s[s.index('typedef std::complex'):end]+'''
int main(){double a,b;std::cout<<std::setprecision(17);while(std::cin>>a>>b){std::complex<double> v=khicas_dilog_numeric(std::complex<double>(a,b));std::cout<<v.real()<<" "<<v.imag()<<"\\n";}}
'''
random.seed(2026);mp.mp.dps=70
points=[complex(x,y) for x in [-10,-1,-.5,-.001,0,.25,.5,.75,1,1.1,2,100] for y in [0,-1,1,-1e-12,1e-12]]
points += [complex(random.uniform(-10,10),random.uniform(-10,10)) for _ in range(300)]
points += [complex(x) for x in [1e-300,-1e-300,1e300,-1e300]]
points += [complex(x,y) for x in [-1.79e308,1.79e308] for y in [-1.79e308,1.79e308]]
points += [complex(2,y) for y in [1e-4,-1e-4,1e-8,-1e-8,1e-12,-1e-12]]
with tempfile.TemporaryDirectory(prefix='khicas-dilog-core-') as t:
 d=Path(t);(d/'core.cc').write_text(source)
 subprocess.run(['c++','-O2',str(d/'core.cc'),'-o',str(d/'core')],check=True,capture_output=True)
 result=subprocess.run([str(d/'core')],input=''.join(f'{z.real:.17g} {z.imag:.17g}\n' for z in points),capture_output=True,text=True,check=True)
rows=[]
for z,line in zip(points,result.stdout.splitlines()):
 r,i=map(float,line.split());value=mp.mpc(r,i);reference=mp.polylog(2,mp.mpc(z.real,z.imag))
 error=abs(value-reference)/(1+abs(reference));assert error<mp.mpf('8e-15'),(z,value,reference,error)
 if max(abs(z.real),abs(z.imag))<1e-100 and z:assert abs(value/reference-1)<mp.mpf('8e-15')
 rows.append({'input':[z.real,z.imag],'actual':[r,i],'reference':[str(reference.real),str(reference.imag)],'scaled_error':str(error)})
assert len(rows)==len(points)
# Agent L7: analytic boundary constants, independently compared on both sides.
for sign in [-1,1]:
 value=mp.polylog(2,mp.mpc(2,sign*mp.mpf('1e-50')))
 limit=mp.pi**2/4+sign*mp.j*mp.pi*mp.log(2)
 assert abs(value-limit)<mp.mpf('1e-48')
a.report.write_text(json.dumps({'scope':'Production double numerical core (extracted unchanged), compared against mpmath 70 digits. Not an arbitrary-precision Li2 implementation or SH4 timing.','source_sha256':{'dilogarithm.h':hashlib.sha256(h.read_bytes()).hexdigest()},'max_scaled_error':max(float(r['scaled_error']) for r in rows),'branch_limit_L7':'both sides independently verified; literal real cut uses negative imaginary value','runs':rows},indent=2)+'\n')
print('PASS',len(rows),'numeric points; max scaled error',max(float(r['scaled_error']) for r in rows))
