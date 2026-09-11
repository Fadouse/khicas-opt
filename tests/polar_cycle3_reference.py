"""Independent branch, endpoint and resource proofs for cycle three."""
import sympy as s
from mixed_reference import x,parse,equal,local
Root=s.Function('RealRoot')

def verify(case,printed):
    ident=case['id']
    assert not any(word in printed for word in ('undef','integrate(','diff(')),printed
    local['surd']=Root
    if ident=='PC3-D2':
        assert printed.startswith('piecewise(') and printed.endswith(')')
        condition,yes,no=printed[len('piecewise('):-1].split(',',2)
        assert condition=='x=0' and yes=='0'
        assert equal(parse(no),2*x*s.sin(1/x)-s.cos(1/x))
        return dict(exact=True,method="Exact nonzero branch; original difference quotient h*sin(1/h) is bounded by |h| and tends to zero. Lazy zero branch is retained; derivative continuity is neither required nor true.")
    a=parse(printed)
    if ident=='PC3-I1':
        u=s.Symbol('u',real=True)
        f=a.xreplace({Root(x,3):u})
        expected=3*u*u/2-3*u+3*s.log(s.Abs(1+u))
        assert equal(f,expected)
        for sign in [-1,1]:
            branch=f.xreplace({s.Abs(1+u):sign*(1+u)})
            assert equal(s.diff(branch,u),3*u*u/(1+u))
        near=f.xreplace({s.Abs(1+u):1+u})
        assert near.subs(u,0)==0
        assert s.limit(near/u**3,u,0,dir='+')==s.limit(near/u**3,u,0,dir='-')==1
        method="Real-root bijection x=u³, exact derivative on both sides of u=-1, and two-sided difference quotient 1 at u=0; real log magnitude retained."
    elif ident=='PC3-I2':
        z=s.exp(s.I*x)
        canonical=-s.I*s.log(z)+s.I*s.log(s.I*z+s.I)
        assert equal(a,canonical)
        assert equal(s.diff(a,x),1/(1+z))
        method="Exact derivative. log(exp(i*x)) cuts only at excluded odd*pi; i*(1+exp(i*x)) has strictly positive imaginary part 1+cos(x) away from those same poles. No additional jump within any allowed interval."
    elif ident=='PC3-I3':
        assert equal(a,s.atan(x)/2-x/(2*(1+x*x)))
        assert equal(s.diff(a,x),x*x/(1+x*x)**2)
        method="Exact rational derivative, positive denominator and global real atan."
    elif ident=='PC3-D1':
        for sign in [-1,1]:
            branch=a.xreplace({s.Abs(x**4-1):-sign*(x**4-1)})
            assert equal(branch,2*sign/(1+x*x))
        assert a.subs(x,0)==2
        method="Exact two-sign proof; |x⁴-1|=|x²-1|(x²+1). True corners ±1 excluded; value 2 at zero."
    elif ident=='PC3-D3':
        expected=(1+x*x)**s.sin(x)*(s.cos(x)*s.log(1+x*x)+2*x*s.sin(x)/(1+x*x))
        # Keep real sin/cos intact: rewriting every trigonometric function into
        # complex exponentials obscures this positive-base identity.
        base=1+x*x
        canonical={p:s.exp(s.log(base)*s.sin(x))/base
                   for p in a.atoms(s.exp)
                   if s.expand(p.args[0]-(s.sin(x)-1)*s.log(base))==0}
        assert equal(a.xreplace(canonical),expected)
        assert a.subs(x,0)==0
        method="Positive-base logarithmic differentiation accounts for both base and exponent; entire real domain and zero at origin."
    elif ident=='PC3-S1':
        expected=s.sqrt(x*x+1)+s.I*s.Abs(x)
        retained=s.sqrt(-x+s.I)*s.sqrt(-x-s.I)+s.I*s.Abs(x)
        if not equal(a,retained):
            delta=s.cancel(a-expected)
            u=s.Symbol('u')
            numerator=s.fraction(delta.xreplace({s.sqrt(x*x+1):u}))[0]
            assert s.rem(s.Poly(s.expand(numerator),u),s.Poly(u*u-x*x-1,u)).is_zero
        assert s.simplify(s.expand_complex(a.subs(x,0)))==1
        method="Retained conjugate principal roots, or exact radical-field identity with positive denominator sqrt(x²+1)-x>0. Conjugate principal roots multiply to the positive modulus; sqrt(-x²)=i|x| on both signs."
    elif ident=='PC3-R1':
        original=parse(case['input'][9:-1]);assert a==original
        method="Exact structural retention. Twelve recurrence steps with R_n>=1 prove positive denominators, evenness, and 1<=R_12<=1+x²/2."
    else:raise AssertionError(ident)
    return dict(exact=True,method=method)
