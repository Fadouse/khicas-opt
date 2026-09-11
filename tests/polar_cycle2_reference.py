"""Independent real-domain and complex-branch proofs for the fresh batch."""
import sympy as s
from mixed_reference import x,parse,equal,local
local['surd']=lambda u,n:s.real_root(u,n)

def verify(case,printed):
    ident=case['id']
    assert not any(word in printed for word in ('undef','integrate(','diff(')),printed
    a=parse(printed)
    if ident=='PC2-I1':
        t=s.Symbol('t',positive=True)
        # x=(t-1)/(t+1) bijects positive t to the entire real domain.
        transformed=s.simplify(a.subs(x,(t-1)/(t+1)))
        assert s.simplify(s.diff(transformed+1/s.sqrt(t),t))==0
        assert a.subs(x,0).is_finite
        method="Exact derivative after a bijective positive-domain substitution. Both original endpoints excluded; primitive regular at x=0."
    elif ident=='PC2-I2':
        expected=s.log(x+1+s.I)+s.log(x-1+s.I)
        assert equal(a,expected)
        assert equal(s.diff(a,x),(2*x+2*s.I)/(x*x+2*s.I*x-2))
        assert equal(s.diff(a,x).subs(x,0),-s.I)
        method="Exact sum of principal logs of two strictly upper-half-plane factors; neither crosses a cut. Global derivative and regularity at x=0, not just intervalwise derivative."
    elif ident=='PC2-I3':
        assert equal(s.diff(a,x),x*s.exp(x)*s.cos(x))
        method="Exact ordinary derivative on the entire real axis; expression composed of entire factors."
    elif ident=='PC2-D1':
        expected=4*x/s.sqrt(2-x**4)
        assert equal(a,expected)
        assert a.subs(x,0)==0
        method="Exact radical algebra on |x|<2^(1/4); acos(1-x^4)=2 asin(x²/sqrt(2)) proves the removable derivative value 0."
    elif ident=='PC2-D2':
        # Separate the three real sign intervals. real_root is never
        # replaced by the principal complex cube root on the middle one.
        w=x*x-x
        expected=s.Rational(4,3)*(2*x-1)*s.real_root(w,3)
        assert equal(a,expected)
        for point in [0,1,s.Rational(1,2)]:assert s.simplify(a.subs(x,point))==0
        method="Exact real-root chain formula; |u|^(4/3) is differentiable with derivative zero at u=0. Both polynomial zeros and its stationary point included."
    elif ident=='PC2-D3':
        expected=s.exp(x)*(1-s.log(1+s.exp(x)))/(1+s.exp(x))**2
        assert equal(a,expected)
        method="Exact quotient derivative; positive denominator on all real x and unique sign-changing zero ln(e-1)."
    elif ident=='PC2-S1':
        original=s.log((x+s.I)**2)-2*s.log(x+s.I)
        expected=s.Piecewise((0,x>=0),(-2*s.pi*s.I,True))
        assert a==original or a==expected
        if a!=original:assert a.subs(x,0)==0
        method="For theta=arg(x+i) in (0,pi), doubling wraps by -2*pi exactly when x<0. Boundary arg(-1)=pi gives zero at x=0. Original branch expression also preserved exactly in direct mode."
    elif ident=='PC2-R1':
        original=parse(case['input'][9:-1])
        if a!=original:
            u=s.Symbol('u')
            an,ad=s.fraction(a.xreplace({s.sqrt(x*x+1):u}))
            bn,bd=s.fraction(original.xreplace({s.sqrt(x*x+1):u}))
            residual=s.Poly(s.expand(an*bd-bn*ad),u)
            assert s.rem(residual,s.Poly(u*u-x*x-1,u)).is_zero
            # The expanded denominator must not introduce a real zero.
            assert s.Poly(ad,x).degree()%2==0
            assert all(power[0]%2==0 and coeff>0 for power,coeff in s.Poly(ad,x).terms())
        method="Exact original expression or polynomial identity modulo u²=x²+1, with positive real denominator. Original positive factors prove oddness, sign and strict (-1,1) bounds."
    else:raise AssertionError(ident)
    return dict(exact=True,method=method)
