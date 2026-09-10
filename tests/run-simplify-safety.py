#!/usr/bin/env python3
"""FXCG simplification regression; host dependencies, not an SH4 emulator."""
import argparse, hashlib, json, os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, build, function

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--report',type=Path,required=True)
p.add_argument('--probe',type=Path,help='Reuse a probe built with target_simplify=True')
args=p.parse_args()
report={'scope':'Repository integration/normalization and FXCG simplify entry points; other dependencies are host Giac. A guarded pthread stack excludes dynamic-loader startup; not CG50 memory or timing.',
        'validation_probe_sha256':hashlib.sha256((ROOT/'tests/integration_probe.cc').read_bytes()).hexdigest(),
          'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in
                         ('yintg.cc','zintgab.cc','ysym2poly.cc','ksubst.cc','integration_guard.h','equation_normalize.h','dilogarithm.h') if (ROOT/n).exists()},
        'runs':[]}
with tempfile.TemporaryDirectory(prefix='khicas-simplify-safety-') as tmp:
    d=Path(tmp)
    # Giac's custom vector is not protected by _GLIBCXX_ASSERTIONS.
    # Exercise the actual FXCG condition with checked vector reads.
    text='#include "giacPCH.h"\n#include <cassert>\n#define FXCG\nnamespace giac {\n'
    text+='static const gen &checked(const vecteur &v,unsigned i){assert(i<v.size());return v[i];}\n'
    text+=function((ROOT/'ksubst.cc').read_text(),'  gen tsimplify_noexpln(').replace('v1[0]','checked(v1,0)').replace('v1[1]','checked(v1,1)')
    text+='}\nint main(){using namespace giac;context ctx;const context *contextptr=&ctx;\n'
    text+='angle_radian(true,contextptr);\n'
    text+='for(const char *s:{"x","tan(x)","sin(x)","tan(x)+tan(2*x)","sin(x)^2+cos(x)^2"}) {gen e(s,contextptr);assert(!is_undef(tsimplify_noexpln(e,2,0,contextptr)));}\n}\n'
    (d/'bounds.cc').write_text(text)
    flags=[os.environ.get('CXX','c++'),'-std=c++11','-O1','-DHAVE_CONFIG_H','-DGIAC_GENERIC_CONSTANTS',
           '-Wno-deprecated-declarations','-I',os.environ.get('GIAC_INCLUDE','/usr/include/giac')]+shlex.split(os.environ.get('CXXFLAGS',''))
    libs=shlex.split(os.environ.get('LDFLAGS',''))+['-lgiac']
    subprocess.run(flags+[str(d/'bounds.cc')]+libs+['-o',str(d/'bounds')],check=True)
    subprocess.run([str(d/'bounds')],check=True,timeout=20)
    report['stale_trig_count_checked_access']='passed (0, 1 and 2 actual trig nodes)'
    exe=args.probe or build(d/'probe',target_simplify=True)
    cases=[
        ('reported','simplify(integrate(3*ln(sin(2*x)),x))','closed'),
        ('implicit-products','simplify(integrate(3ln(sin(2x))))','closed'),
        ('cosine-shift','simplify(integrate(ln(cos(3*x+1)),x))','closed'),
        ('absolute-log','simplify(integrate(ln(abs(sin(2*x))),x))','closed'),
        ('closed-absolute-derivative','simplify(diff(simplify(integrate(ln(abs(sin(x))),x)),x)-ln(abs(sin(x))))',0),
        ('elementary-log','simplify(diff(integrate(ln(x),x),x)-ln(x))',0),
        ('definite-log-sine','simplify(integrate(3*ln(sin(2*x)),x,0,pi/2)+3*pi*ln(2)/2)',0),
        ('weighted-definite','simplify(integrate(x*ln(sin(x)),x,0,pi)+pi^2*ln(2)/2)',0),
    ]
    for name,expr,status in cases:
        for stack in ('normal','64'):
            env=dict(os.environ)
            env.pop('KHICAS_TEST_STACK_KIB',None)
            if stack!='normal':env['KHICAS_TEST_STACK_KIB']=stack
            result=subprocess.run([str(exe),expr],capture_output=True,text=True,env=env,timeout=20)
            row={'id':name,'input':expr,'stack_KiB':stack,'exit':result.returncode,
                 'result':result.stdout.strip(),'stderr':result.stderr}
            report['runs'].append(row)
            args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
            assert result.returncode==(0 if status=='closed' else status),row
            if status==0:assert result.stdout.strip()=='0',row
            elif status=='closed':assert 'integrate(' not in result.stdout and 'Li2(' in result.stdout,row
            else:assert 'integrate(' in result.stdout,row
            print(name,stack,'PASS',flush=True)
print('PASS: target simplification bounds, closed trigonometric-log integrals and guarded computation stack')
