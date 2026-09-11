"""Independent interval proofs for the cycle 18 questions."""
import subprocess
import sympy as s
from mixed_reference import parse, x, equal


def verify(case, printed):
    ident = case['id']
    assert not any(op in printed for op in ('integrate(', 'diff('))
    a = parse(printed)
    t = s.Symbol('t', positive=True)
    if ident in ('PC18-I1', 'PC18-I2'):
        F = a.subs(x, t*t)
        # Complete root range: t>=0, t!=1. The open components are
        # independently verified with the appropriate sign, not sampled.
        for sign in (-1, 1):
            branch = F
            for atom in branch.atoms(s.sign):
                assert equal(atom.args[0], 1-t) or equal(atom.args[0], t-1)
                branch = branch.xreplace({atom: s.Integer(sign if equal(atom.args[0], 1-t) else -sign)})
            for atom in branch.atoms(s.Abs):
                assert equal(atom.args[0], 1-t) or equal(atom.args[0], t-1)
                branch = branch.xreplace({atom: sign*(1-t)})
            target = 2*t*t/(t*t-1)**2 if ident=='PC18-I1' else 2*t*s.log(sign*(1-t))
            assert s.simplify(s.expand_log(s.diff(branch,t)-target,force=True))==0
        assert a.subs(x,0).is_finite is True
        method = ('Set t=sqrt(x). Verify both t<1 and t>1 signs by exact differentiation. '
                  'The complete real domain is [0,1) union (1,infinity). At zero the original '
                  'primitive difference quotient is O(sqrt(h)); independent additive constants '
                  'do not affect this proof. The missing point 1 is separately evaluated.')
    elif ident=='PC18-D1':
        u=s.Symbol('u',real=True)
        b=s.simplify(a.subs(x,s.exp(u))*s.exp(u))
        target=2*u*s.Abs(u-1)+u*u*s.sign(u-1)
        for sign in (-1,1):
            left=b.replace(lambda z:z.func==s.Abs,lambda z:sign*(u-1))
            right=target.replace(lambda z:z.func==s.Abs,lambda z:sign*(u-1)).subs(s.sign(u-1),sign)
            assert equal(left,right)
        method=('On x>0 substitute u=ln(x). Both signs around u=1 give the exact derivative. '
                'At x=1 the original quotient is h*(ln(1+h)/h)^2*|ln(1+h)-1| ->0. '
                'At x=e the two original quotients tend to +/-1/e, so it is a true cusp.')
    elif ident=='PC18-D2':
        assert equal(a,x/(1+s.sqrt(1-x*x)))
        method=('Original domain [-1,1], ordinary derivative on (-1,1), including zero. '
                'With H(s)=ln(1+s)-s, H(s)/s² -> -1/2. The original inward endpoint '
                'quotients therefore tend to -1 and +1; neither is a two-sided derivative.')
    elif ident=='PC18-S1':
        b=a.subs(x,t*t)
        for atom in b.atoms(s.log):
            arg=s.cancel(atom.args[0])
            assert equal(arg,t+1) or equal(arg,1/(t+1))
            b=b.xreplace({atom:s.log(arg)})
        assert s.expand_log(b,force=True)==0
        # A bare zero would erase the inherited hole. A retained exact
        # expression is recorded as retained, not as zero normalization.
        assert a.has(s.log) or a.has(s.Piecewise), 'Unrestricted zero loses x=1'
        method=('On the original domain x>=0, x!=1, factor x-1=(t-1)(t+1). '
                'Both log arguments are positive and reciprocal. The exact value is zero; '
                'retained source-domain nodes are acceptable but not claimed as normalized zero.')
    elif ident=='PC18-R1':
        y,z=s.symbols('y z')
        if a.func==s.Piecewise:
            (value,condition),(regular,otherwise)=a.args
            assert value==s.Symbol('undef') and otherwise==True
            assert isinstance(condition,s.Equality) and condition.rhs==0 and equal(condition.lhs,(x+y+z)**2)
            a=regular
        assert equal(a,1+(x**8+y**8+z**8)/(x+y+z)**2)
        assert s.count_ops(a)<256 and len(printed.encode())<=65536
        method=('Column multilinearity proves det(I+vv^T)=1+v^Tv: every term using '
                'two rank-one columns vanishes. v=(x^4,y^4,z^4)/(x+y+z); '
                'the entire excluded plane must survive actual substitution.')
    else:
        raise AssertionError(ident)
    return dict(exact=True,method=method)


def actual_checks(case, expression, probe, env):
    ident=case['id']
    points={
        'PC18-I1':{'0':'finite','1':'nonfinite'},
        'PC18-I2':{'0':'finite','1':'nonfinite'},
        'PC18-D1':{'1':'0','exp(1)':'nonfinite'},
        'PC18-D2':{'0':'0','-1':'nonfinite','1':'nonfinite'},
        'PC18-S1':{'0':'0','1':'nonfinite','4':'0'},
    }.get(ident,{})
    rows=[]
    if ident=='PC18-R1':
        substitutions=[('[x,y,z]','[0,0,0]','nonfinite'),
                       ('[x,y,z]','[1,-1,0]','nonfinite'),
                       ('[x,y,z]','[1,1,-2]','nonfinite'),
                       ('[x,y,z]','[1,0,0]','2'),
                       ('[x,y,z]','[1,1,1]','4/3')]
    else:
        substitutions=[('x',point,expected) for point,expected in points.items()]
    for variables,point,expected in substitutions:
        run=subprocess.run([str(probe),f'eval(subst({expression},{variables},{point}))'],
                           env=env,capture_output=True,text=True,timeout=10)
        value=run.stdout.strip()
        row=dict(point=point,expected=expected,exit=run.returncode,result=value)
        if expected=='nonfinite':
            assert run.returncode in (0,3) and value in ('undef','infinity','-infinity'),row
        else:
            assert run.returncode==0,row
            assert parse(value).is_finite is True if expected=='finite' else equal(parse(value),parse(expected)),row
        rows.append(row)
    return rows
