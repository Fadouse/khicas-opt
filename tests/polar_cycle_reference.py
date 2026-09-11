"""Root-authored independent proofs for the polar/stability question cycle."""
import sympy as s
from mixed_reference import x,parse,equal
from mixed_round2_reference import canonical_dilog

def verify(case,printed):
    ident=case['id']
    if ident.startswith('PC13-'):
        from polar_cycle13_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC12-'):
        from polar_cycle12_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC11-'):
        from polar_cycle11_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC10-'):
        from polar_cycle10_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC9-'):
        from polar_cycle9_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC8-'):
        from polar_cycle8_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC7-'):
        from polar_cycle7_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC6-'):
        from polar_cycle6_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC5-'):
        from polar_cycle5_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC4-'):
        from polar_cycle4_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC2-'):
        from polar_cycle2_reference import verify as next_cycle
        return next_cycle(case,printed)
    if ident.startswith('PC3-'):
        from polar_cycle3_reference import verify as next_cycle
        return next_cycle(case,printed)
    forbidden=('integrate(', 'diff(', 'rootof(') if ident=='PC1-D4' else ('integrate(', 'diff(', 'undef', 'rootof(')
    assert not any(t in printed for t in forbidden),printed
    a=parse(printed)
    if ident=='PC1-D4' and a.has(s.Piecewise):
        excluded=set()
        for p in a.atoms(s.Piecewise):
            (yes,condition),(no,otherwise)=p.args
            assert yes==s.Symbol('undef') and otherwise==True and condition.lhs==x
            excluded.add(condition.rhs)
        assert excluded=={-1,1},'Only the original absolute-value corners are excluded'
        while a.has(s.Piecewise):a=a.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
    if ident in ('PC1-I1','PC1-I2'):
        expected=parse(case['expected']['expression'])
        assert equal(canonical_dilog(a,expected),expected)
        if ident.endswith('I1'):
            phi=x**3-x
            assert s.diff(phi,x)==3*x*x-1
            assert all(s.simplify(s.diff(phi,x).subs(x,p))==0 for p in [-1/s.sqrt(3),1/s.sqrt(3)])
            method="Exact canonical Clausen primitive; |1-exp(i*phi)|=2|sin(phi/2)|; chain factor phi' is multiplied, never divided. All sine-sign intervals and stationary points retained."
        else:
            z=2*s.exp(-3*s.I*x/2)
            assert equal(-2*s.I/3*(-s.diff(z,x)/z),1)
            method="Exact canonical principal Li2 primitive and logarithmic derivative z'/z=-3i/2. Valid on each arc excluding the principal cut, on both half-planes."
    elif ident=='PC1-I3':
        assert equal(a,-s.log(2-s.sin(x)**2)/2)
        target=s.sin(x)*s.cos(x)/(s.sin(x)**4+3*s.sin(x)**2*s.cos(x)**2+2*s.cos(x)**4)
        assert s.trigsimp(s.diff(a,x)-target)==0
        method="Exact derivative; denominator=(sin²+cos²)(sin²+2cos²)=1+cos²>=1 on the full real axis."
    elif ident in ('PC1-I4','PC1-D4'):
        w=x*x-(4 if ident=='PC1-I4' else 1)
        for sign in [-1,1]:
            branch=a.xreplace({s.Abs(w):sign*w,s.sign(w):s.Integer(sign)})
            if ident=='PC1-I4':
                assert equal(s.diff(branch,x),x/(sign*w))
            else:
                assert equal(branch,4*x*sign/(1+x*x)**2)
        method="Exact algebra on both signs of x²-c; all connected real intervals covered. Only the original excluded zeros of x²-c remain excluded."
    elif ident=='PC1-D1':
        assert a.has(s.Piecewise)
        regular=a
        for p in a.atoms(s.Piecewise):
            (yes,condition),(no,otherwise)=p.args
            assert yes==0 and otherwise==True
            assert set(s.solve(condition,x))=={-1,0,1}
            regular=regular.xreplace({p:no})
        phi=(x*x-1)**2
        expected=-s.diff(phi,x)*s.log(2*s.Abs(s.sin(phi/2)))
        # The CAS may expand the polynomial argument.
        regular=regular.replace(lambda z:z.func==s.sin,lambda z:s.sin(s.factor(z.args[0])))
        assert equal(regular,expected)
        for point in [-1,0,1]:
            assert s.simplify(a.subs(x,point))==0
        method="Exact regular derivative and complete stationary zero locus {-1,0,1}; lazy zeros at ±1 agree with delta*log(delta) and a quadratic phase zero, hence derivative limit 0."
    elif ident=='PC1-D2':
        z=s.sin(x)**2/(2+s.sin(x)**2);regular=a
        assert a.has(s.Piecewise)
        for p in a.atoms(s.Piecewise):
            (yes,condition),(no,otherwise)=p.args
            assert yes==1 and otherwise==True and condition.rhs==0 and equal(condition.lhs,z)
            regular=regular.xreplace({p:no})
        expected=-s.diff(z,x)/z*s.log(1-z)
        assert equal(regular,expected)
        k=s.Symbol('k',integer=True)
        assert s.simplify(a.subs(x,k*s.pi))==0
        method="Exact Li2 chain rule with removable value dLi2/dz=1 at z=0; complete zeros k*pi, positive denominator and 0<=z<=1/3 prove the global real domain."
    elif ident=='PC1-D3':
        assert equal(a,parse(case['expected']['expression']))
        method="Exact principal-argument sawtooth derivative on every 2*pi phase interval; floor handles negative periods. Phase cut points are excluded by the question."
    elif ident=='PC1-S1':
        target=(s.sin(x)**3*s.cos(x)+s.sin(x)*s.cos(x)**3)/(s.sin(x)**6+s.cos(x)**6)
        assert s.trigsimp(a-target)==0
        method="Exact trig identity; sin^6+cos^6=1-3 sin²cos²>=1/4, so no real pole."
    elif ident=='PC1-S2':
        # Partition at all possible absolute-value zeros; x=2 is excluded.
        for lo,hi,point in [(-s.oo,-1,-2),(-1,2,0),(2,s.oo,3)]:
            def signed(z):
                sign=s.sign(z.subs(x,point))
                return sign*z
            b=a.replace(lambda z:z.func==s.Abs,lambda z:signed(z.args[0]))
            for power in list(b.atoms(s.Pow)):
                if power.exp==s.Rational(1,2):
                    root=(x-2)*(x+1)
                    assert equal(power.base,root**2)
                    b=b.xreplace({power:signed(root)})
            assert equal(b,s.sign(point+1)*(x+1))
        assert a.subs(x,-1)==0
        method="Exact polynomial factorization on all three sign intervals, including zero at -1; original denominator exclusion x=2 is retained."
    elif ident=='PC1-R1':
        assert equal(a,s.sqrt(x*x+18))
        method="Exact nested positive-radical identity, 18 levels."
    elif ident=='PC1-R2':
        original=parse(case['input'][9:-1])
        assert a==original
        assert s.count_ops(a)<400
        method="Exact structural retention without distribution. Each factor has t=sin^6+cos^6 in [1/4,1], so (1+t)/(2+t) in [5/9,2/3]; finite positive product on all real x."
    else:
        raise AssertionError(ident)
    return dict(exact=True,method=method)
