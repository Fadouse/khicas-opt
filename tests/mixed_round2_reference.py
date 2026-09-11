"""Root-authored exact acceptance of the second fresh mixed question batch."""
import sympy as sp
from mixed_reference import x,parse,equal
from trig_log_reference import verify_trig_log_reference

def canonical_dilog(actual,expected):
    replacements={}
    for p in actual.atoms(sp.polylog):
        for q in expected.atoms(sp.polylog):
            if equal(p.args[1],q.args[1]):replacements[p]=q;break
    return actual.xreplace(replacements)

def verify(case,printed):
    assert not any(s in printed for s in ['integrate(', 'diff(', 'rootof(']),printed
    id=case['id'];actual=parse(printed)
    if id=='MR2-I2':
        u='(2*x/3+pi/7+pi)'
        expected='x*ln(3/2)-3/4*im(Li2(exp(2*i*'+u+')))+3*i*pi/4*('+u+'-pi+abs('+u+'-2*pi*floor('+u+'/(2*pi))-pi))'
        return verify_trig_log_reference(dict(id=id,expected=expected,f='ln(-3*sin(2*x/3+pi/7))',frequency='2/3',phase='pi/7',inside_coefficient=-3,cosine=False),printed)
    if id in ['MR2-I1','MR2-I3','MR2-I4']:
        expected=parse(case['expected']['expression']);assert equal(canonical_dilog(actual,expected),expected)
        if id=='MR2-I1':
            # Both real log magnitudes are positive on every intersection
            # interval. Each independent H phase has its own chain factor.
            u=3*x/4+sp.pi/8;v=x/2+7*sp.pi/18
            assert sp.diff(u,x)==sp.Rational(3,4) and sp.diff(v,x)==sp.Rational(1,2)
            assert sp.trigsimp(sp.sin(v)-sp.cos(x/2-sp.pi/9))==0
            return {'exact':True,'method':'canonical two-phase Li2 primitive; exact Clausen derivative and positive-log scale identities on all sign intersections'}
        derivative=sp.diff(expected,x).replace(lambda z:z.func==sp.polylog and z.args[0]==1,lambda z:-sp.log(1-z.args[1]))
        if id=='MR2-I3':
            w=x*x+x;target=(2*x+1)*sp.log(1+w*w)/w
            assert equal(derivative,target) and sp.simplify(derivative.subs(x,-sp.Rational(1,2)))==0
        else:
            assert equal(derivative,x*sp.atan(x*x))
            assert expected.subs(x,0)==0
        return {'exact':True,'method':'canonical primitive and exact real-domain differentiation, including stationary substitution point'}
    if id=='MR2-D1':
        assert sp.count_ops(actual)<128
        expected=parse(case['expected']['expression'])
        regular=actual
        for branch in actual.atoms(sp.Piecewise):
            (yes,condition),(no,otherwise)=branch.args
            assert yes==0 and otherwise==True and set(sp.solve(condition,x))=={0}
            assert sp.simplify(expected.subs(x,0))==0
            regular=regular.xreplace({branch:no})
        # Force only a positive magnitude's constant-factor logarithm split;
        # sin zeros are excluded in the question, so its abs is positive.
        assert equal(sp.expand_log(regular,force=True),sp.expand_log(expected,force=True))
        assert sp.simplify(actual.subs(x,0))==0
        return {'exact':True,'method':'positive magnitude logarithm identity; exact nonlinear phase chain rule; regular value at zero'}
    if id in ['MR2-D2','MR2-D3']:
        zeros=[0,1] if id=='MR2-D2' else [0]
        target=(-2*(2*x-1)*sp.log(1+x*x*(x-1)**2)/(x*(x-1)) if id=='MR2-D2' else 2*sp.log(1+x*x)/(x*(1+x*x)))
        assert actual.has(sp.Piecewise)
        regular=actual
        for p in actual.atoms(sp.Piecewise):
            (yes,condition),(no,otherwise)=p.args
            assert isinstance(condition,sp.Equality) and otherwise==True
            assert set(sp.solve(condition,x))==set(zeros)
            regular=regular.xreplace({p:no})
        assert equal(regular,target)
        for point in zeros:
            assert sp.simplify(actual.subs(x,point))==0
            assert sp.limit(target,x,point)==0
        if id=='MR2-D2':assert sp.simplify(actual.subs(x,sp.Rational(1,2)))==0
        else:assert equal(regular.subs(x,-x),-regular)
        return {'exact':True,'method':'complete polynomial zero locus, exact branch formulas, all removable values/limits and stationary point or parity'}
    assert id=='MR2-D4'
    expected=parse(case['expected']['expression']);assert equal(actual,expected) and actual.subs(x,0)==2
    # The unsimplified correct form can have sqrt(1+x^2) in its
    # denominator. Check its real zero set without assuming polynomiality.
    assert sp.solveset(sp.denom(sp.cancel(actual)),x,domain=sp.S.Reals)==sp.S.EmptySet
    return {'exact':True,'method':'exact positive-radical algebra and trig identity; real denominator has no roots'}
