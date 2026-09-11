#!/usr/bin/env python3
"""Verify the positive-quadratic / affine-root substitution and its domain gate."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
import mpmath as mp
import sympy as s
from integration_build import ROOT, compiler_options, function
from mixed_reference import parse, equal, x
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',required=True)
p.add_argument('--build-dir',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
source=(ROOT/'yintg.cc').read_text()
(a.build_dir/'equation_normalize.h').write_text((ROOT/'equation_normalize.h').read_text())
(a.build_dir/'guarded_probe_stack.h').write_text((ROOT/'tests/guarded_probe_stack.h').read_text())
out='#include "giacPCH.h"\n#include "equation_normalize.h"\n#include "guarded_probe_stack.h"\n#include <iostream>\nnamespace giac {\n'
out+=function(source,'  void decompose_prod(')+function(source,'  gen extract_cst(')
for name,typ in [('integration_syntax','gen'),('integration_coefficient','gen'),('integration_power','bool'),('integration_square_root','bool'),('integration_resource_rational','bool'),('integrate_quadratic_affine_root','bool')]:
 sig='  static '+typ+' '+name+'('
 out+=function(source[source.rindex(sig):],sig)
out+='''\n}\nstatic int probe(int argc,char **argv){
 giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[1]),&c),lo(std::string(argv[2]),&c),hi(std::string(argv[3]),&c),r;
 if(argc>4 && std::string(argv[4])=="complex")giac::complex_mode(true,&c);
 if(argc>4 && std::string(argv[4])=="degree")giac::angle_radian(false,&c);
 g=giac::integration_syntax(g,&c);giac::gen scale=giac::integration_coefficient(g,x,&c);
 bool ok=giac::integration_resource_rational(scale) && giac::integrate_quadratic_affine_root(g,x,lo,hi,r,&c);
 if(ok)r=scale*r;
 std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";return 0;
}\nint main(int argc,char **argv){return guarded_probe::run(argc,argv,probe);}\n'''
(a.build_dir/'probe.cc').write_text(out)
flags,libs=compiler_options();contract=a.build_dir/'probe'
subprocess.run(flags+[str(a.build_dir/'probe.cc')]+libs+['-o',str(contract)],check=True)
# Exact family proof: differentiate the log and the two atan terms. Keeping
# h²=4v-w² explicit makes the proof rational, independent of the CAS output.
u,v,w,h=s.symbols('u v w h',positive=True)
dlog=((2*u+w)/(u*u+w*u+v)-(2*u-w)/(u*u-w*u+v))/(4*v*w)
datan=(2*h/(h*h+(2*u+w)**2)+2*h/(h*h+(2*u-w)**2))/(2*v*h)
proof=s.factor((dlog+datan).subs(h*h,4*v-w*w)-1/((u*u+v)**2-w*w*u*u))
assert proof==0
mp.mp.dps=55
rows=[]
def gs(e):return str(e).replace('**','^').replace('log(','ln(').replace('oo','+infinity')
def run(exe,args,stack):
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
 return subprocess.run([str(exe)]+args,env=env,capture_output=True,text=True,timeout=10)
def save():
 a.report.write_text(json.dumps(dict(scope='Exact rational primitive identity; exact production outputs against the derived family; independent 55-digit quadrature cross-check; domain/size/mode deferral contracts. Guarded host stack is not CG50 emulation.',proof=str(proof),source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','equation_normalize.h')},probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
# Q=A*x²+B*x+C, L=k*x+b. Exercise both slopes, coefficients,
# translations, endpoint square-root singularities and reversed intervals.
for A,B,C,k,b,c in [(1,0,1,1,1,1),(2,3,4,2,3,3),(3,-2,2,-2,3,-2),(1,2,2,3,-2,s.Rational(2,3)),(1,0,1,1,-3,1)]:
 A,B,C,k,b,c=map(s.sympify,(A,B,C,k,b,c));Q=A*x*x+B*x+C;L=k*x+b;f=c/(Q*s.sqrt(L))
 pp=B*k/A-2*b;qq=b*b-B*b*k/A+C*k*k/A;vv=s.sqrt(qq);ww=s.sqrt(2*vv-pp);hh=s.sqrt(2*vv+pp)
 def F(U):
  if U==s.oo:return s.pi/(2*vv*hh)
  if U==0:return s.Integer(0)
  return s.log((U*U+ww*U+vv)/(U*U-ww*U+vv))/(4*vv*ww)+(s.atan((2*U+ww)/hh)+s.atan((2*U-ww)/hh))/(2*vv*hh)
 for U,V in [(s.Integer(1),s.oo),(s.Integer(0),s.oo),(s.Integer(1),s.Integer(2)),(s.Integer(2),s.Integer(1))]:
  lo=(U*U-b)/k;hi=(s.oo if k>0 else -s.oo) if V==s.oo else (V*V-b)/k
  expected=2*k*c/A*(F(V)-F(U));expression=f'integrate({gs(f)},x,{gs(lo)},{gs(hi)})'.replace('-+infinity','-infinity')
  # Quadrature after the exact monotone substitution has no radical endpoint
  # singularity. It tests the independent rational integrand, not F'.
  integrand=s.lambdify(u,2*k*c/A/(u**4+pp*u*u+qq),'mpmath')
  numeric=mp.quad(integrand,[int(U),mp.inf if V==s.oo else int(V)])
  assert abs(mp.mpf(str(s.N(expected,50)))-numeric)<mp.mpf('1e-45')
  for stack in ('normal','64'):
   for outer in (False,True):
    e='simplify('+expression+')' if outer else expression
    row=dict(kind='definite',input=e,stack=stack,expected=str(expected),quadrature=str(numeric))
    try:
     r=run(a.probe,[e],stack);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
     assert r.returncode==0 and 'Warning' not in r.stderr and 'integrate(' not in r.stdout,row
     assert equal(parse(r.stdout.strip()),expected),row
     row['passed']=True
    except Exception as ex:row.update(passed=False,error=str(ex))
    rows.append(row);save();print(row['kind'],stack,row['passed'],flush=True)
contracts=[
 ('1/((x^2+1)*sqrt(x+1))','0','+infinity',True,'regular tail',''),
 ('1/((x^2+1)*sqrt(x+1))','-1','0',True,'integrable radical endpoint',''),
 ('1/((x^2+1)*sqrt(1-x))','-infinity','1',True,'negative slope',''),
 ('1/((x^2+1)*sqrt(x+1))','undef','undef',True,'primitive',''),
 ('1/((x^2-1)*sqrt(x+2))','0','+infinity',False,'interior pole',''),
 ('1/((x-1)^2*sqrt(x+2))','0','+infinity',False,'repeated pole',''),
 ('1/((x^2+1)*sqrt(x+1))','-2','0',False,'outside real domain',''),
 ('1/((x^2+1)*sqrt(x+1))','-infinity','0',False,'wrong infinity direction',''),
 ('1/((x^2+1)*sqrt(x^2+1))','0','1',False,'non-affine radical',''),
 ('1/((x^2+a)*sqrt(x+1))','0','1',False,'unproved parameter sign',''),
 ('x/((x^2+1)*sqrt(x+1))','0','1',False,'nonconstant numerator',''),
 ('1/((x^2+1)*sqrt(x+1))','0','1',False,'complex mode','complex'),
 ('1/((x^2+1)*sqrt(x+1))','0','1',False,'angle units','degree'),
 ('1/((x^3+1)*sqrt(x+1))','0','1',False,'higher degree',''),
 ('1/((x^2+1)*sqrt(x+1))','a','1',False,'unproved endpoint',''),
 ('1/((x^2+1)*sqrt(x+1))','0','+infinity',True,'real mode explicit','real'),
]
for e,lo,hi,accept,reason,mode in contracts:
 for stack in ('normal','64'):
  row=dict(kind='contract',input=e,lo=lo,hi=hi,stack=stack,reason=reason,accept=accept)
  try:
   r=run(contract,[e,lo,hi,mode],stack);row.update(exit=r.returncode,result=r.stdout.strip(),stderr=r.stderr)
   assert r.returncode==0 and r.stdout.startswith('ACCEPT ' if accept else 'DEFER'),row
   if accept and lo=='undef':
    # For the concrete primitive, exact substitution x=u²-1, u>0
    # makes principal sqrt(x+1)=u before differentiation.
    Factual=parse(r.stdout[7:].strip()).subs(x,u*u-1)
    assert s.simplify(s.diff(Factual,u)-2/(u**4-2*u*u+2))==0
   row['passed']=True
  except Exception as ex:row.update(passed=False,error=str(ex))
  rows.append(row);save();print(reason,stack,row['passed'],flush=True)
assert all(r['passed'] for r in rows),[r for r in rows if not r['passed']]
