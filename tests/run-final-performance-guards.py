#!/usr/bin/env python3
"""Root-authored regression guards for the final performance changes."""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
import sympy as sp
from conic_reference import check
from mixed_reference import parse, equal, x

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',type=Path,required=True)
p.add_argument('--conic-probe',type=Path,required=True)
p.add_argument('--baseline-conic',type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args();rows=[]
def run(probe,expression,stack,outer=False):
    env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
    if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
    if outer:env['KHICAS_OUTER_SIMPLIFY']='1'
    r=subprocess.run([str(probe),expression],env=env,capture_output=True,text=True,timeout=12)
    assert r.returncode==0,(expression,r.returncode,r.stderr)
    return r.stdout.strip(),r.stderr

conics=[
    ('zero-A','2*x*y+3*y^2+4*x-6*y-1=0','hyperbola',2),
    ('zero-C','3*x^2+2*x*y+4*x-6*y-1=0','hyperbola',2),
    ('negative-pivot','-3*x^2-2*x*y-5*y^2=-1','ellipse',1),
    ('crossing-swap','2*x*y+3*y^2=0','crossing-lines',2),
    ('crossing-shear','3*x^2+2*x*y=0','crossing-lines',2),
    ('rotated-point','3*x^2+2*x*y+5*y^2=0','point',1),
    ('rotated-empty','-3*x^2-2*x*y-5*y^2=1','empty',0),
]
for id,equation,kind,branches in conics:
    c=dict(id=id,equation=equation,kind=kind,branches=branches,parameter='t')
    for outer in [False,True]:
        for stack in ['normal','64']:
            result,err=run(a.conic_probe,'('+equation+',[x,y],t)',stack,outer)
            fields=dict(line.split(' ',1) for line in result.splitlines())
            certificate=check(c,fields['RAW'])
            if branches:
                assert int(fields['NODES'].split()[1])<=256
                assert fields['VIEW'].count('x(t)=')==branches and fields['VIEW'].count('y(t)=')==branches
            rows.append(dict(id=id,stack=stack,outer=outer,result=result,verification=certificate,status='exact'))

# Unknown a may cross zero. The old eigenvector fallback must not be lost.
# This is a preservation check, not a claim that these long charts fit the UI.
for rhs in ['0','1']:
    expr='(a*x^2+x*y-a*y^2='+rhs+',[x,y],t)'
    for stack in ['normal','64']:
        before,_=run(a.baseline_conic,expr,stack)
        after,_=run(a.conic_probe,expr,stack)
        assert before==after
        rows.append(dict(id='unknown-pivot-'+rhs,stack=stack,result=after,status='unchanged-fallback',display_limit='May exceed 256 nodes; unchanged limitation, not counted as a displayed success.'))

for n in [3,4,5,7]:
    expr='3*sin(x)*cos(x)/(sin(x)^'+str(n)+'+cos(x)^'+str(n)+')'
    expected=parse(expr)
    for stack in ['normal','64']:
        for equation in [False,True]:
            result,err=run(a.probe,'simplify('+('r=' if equation else '')+expr+')',stack)
            if equation:assert result.startswith('r=');result=result.split('=',1)[1]
            actual=parse(result)
            assert equal(actual,expected)
            # Treat sine/cosine as independent atoms to check that no zeros
            # were introduced/removed by a trig denominator rewrite.
            u,v=sp.symbols('u v');old=expected.xreplace({sp.sin(x):u,sp.cos(x):v});new=actual.xreplace({sp.sin(x):u,sp.cos(x):v})
            ratio=sp.cancel(sp.denom(sp.cancel(new))/sp.denom(sp.cancel(old)))
            assert not ratio.free_symbols and ratio!=0
            assert sp.count_ops(actual)<=sp.count_ops(expected)
            rows.append(dict(id='trig-denominator-'+str(n),stack=stack,equation=equation,result=result,status='exact',verification='rational identity, same denominator zero set, no expression growth'))

for phase in ['x','x^2+pi/4','-3*x+pi/7','x^3+pi/8']:
    u=parse(phase);expected=-sp.diff(u,x)*sp.log(2*sp.Abs(sp.sin(u/2)))
    expr='diff(im(Li2(exp(i*('+phase+')))),x)'
    for stack in ['normal','64']:
        for outer in [False,True]:
            result,err=run(a.probe,'simplify('+expr+')' if outer else expr,stack)
            actual=parse(result)
            assert equal(sp.expand_log(actual,force=True),sp.expand_log(expected,force=True))
            assert 'new PARI stack' not in err
            rows.append(dict(id='clausen-'+phase,stack=stack,outer=outer,result=result,status='exact',verification='real phase derivative on sine-nonzero intervals; no eager-abs PARI warnings'))
        for point in [-2,0,2]:
            if phase=='x' and point==0:continue
            result,err=run(a.probe,'evalf(subst('+expr+',x='+str(point)+'))',stack)
            value=complex(parse(result).evalf());reference=complex(expected.subs(x,point).evalf(40))
            assert abs(value-reference)<2e-11*(1+abs(reference)),(phase,point,result,reference)
            rows.append(dict(id='clausen-substitute-'+phase,stack=stack,point=point,result=result,status='numeric',absolute_error=abs(value-reference)))

root=Path(__file__).resolve().parents[1]
a.report.write_text(json.dumps({'scope':'Independent guard proofs and selected numeric substitutions. Host execution is not CG50 MMU emulation. Unknown-pivot rows prove output preservation only.','source_sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in ['kconvert.cc','ksubst.cc','yderive.cc','dilogarithm.h']},'runs':rows},indent=2)+'\n')
print('PASS',len(rows),'final optimization guards')
