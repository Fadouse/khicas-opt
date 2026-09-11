"""Independent branch and original-difference-quotient proofs, cycle 19."""
import subprocess
import sympy as s
from mixed_reference import parse, x, equal


def verify(case, printed):
    ident=case['id']
    assert not any(op in printed for op in ('integrate(', 'diff(', 'rootof('))
    if ident=='PC19-P1':
        raw=next(line[4:] for line in printed.splitlines() if line.startswith('RAW '))
        lhs,rhs=raw.split('=');a=parse(lhs)-parse(rhs)
        r,theta=s.symbols('r theta')
        target=r*s.sin(theta)*(r*r-4*r*s.cos(theta)+3)
        assert equal(s.expand_trig(a),s.expand_trig(target))
        return dict(exact=True,method='Polynomial/trigonometric identity without division. The full r=0 component, arbitrary nonnegative r on sin(theta)=0, both circle branches and their tangencies are retained.')
    a=parse(printed);t=s.Symbol('t',positive=True)
    if ident=='PC19-I1':
        F=a.subs(x,s.log(t)); branches=[]
        for sign in (-1,1):
            b=F
            for atom in b.atoms(s.sign):
                assert equal(atom.args[0],t-1)
                b=b.xreplace({atom:s.Integer(sign)})
            assert equal(s.diff(b,t),sign*(t-1)/(1+t*t))
            branches.append(b)
        left=s.limit(branches[0],t,1,dir='-');right=s.limit(branches[1],t,1,dir='+')
        assert equal(left,right),'Primitive branches need one common constant at the included zero'
        assert equal(a.subs(x,0),left),'Assigned primitive value differs from the common limit'
        assert s.limit((branches[0]-left)/(t-1),t,1,dir='-')==0
        assert s.limit((branches[1]-right)/(t-1),t,1,dir='+')==0
        method='Set t=exp(x)>0. Exact derivatives on both sign components, equal branch limits and assigned value at t=1. Since (exp(h)-1)/h->1, both original x difference quotients tend to zero. Denominator 1+t² has no real zeros.'
    elif ident=='PC19-I2':
        # x=cosh(u), u>=0 establishes BOTH the root domain and positive
        # real logarithm argument. The negative root interval is excluded.
        u=s.Symbol('u',positive=True)
        F=a.subs(x,s.cosh(u))
        F=F.xreplace({s.sqrt(s.cosh(u)**2-1):s.sinh(u)})
        F=s.expand_log(F,force=True).subs(s.log(s.sinh(u)+s.cosh(u)),u)
        assert s.simplify(s.trigsimp(s.diff(F,u)-u*s.sinh(u)/s.cosh(u)**2))==0
        assert a.subs(x,1).is_finite is True
        method='x=cosh(u), u>=0 proves the complete real domain [1,infinity). Differentiate the transformed primitive exactly. Its slope in u is O(u²), hence its change is O(u³); x-1>=u²/2 gives right derivative zero at x=1.'
    elif ident=='PC19-D1':
        assert a.has(s.Piecewise),'Missing included zero derivative'
        for sign in (-1,1):
            b=s.simplify(a.subs(x,sign*t))
            target=t**(2+s.sin(sign*t))*(s.cos(sign*t)*s.log(t)+(2+s.sin(sign*t))/(sign*t))
            assert equal(b,target)
        assert s.simplify(a.subs(x,0))==0,'Original function is differentiable at zero'
        method='Exact formula on both nonzero half-lines. At zero, f(h)/h=sign(h)|h|^(1+sin(h)); for |h|<1/2 its magnitude is bounded by sqrt(|h|), proving the original derivative zero.'
    elif ident=='PC19-D2':
        assert equal(s.simplify(a.subs(x,-t)),0),'Negative half-line is a constant function'
        assert equal(s.simplify(a.subs(x,t)),1/s.sqrt(2*t))
        assert a.subs(x,0)==s.Symbol('undef')
        method='Original sqrt(x+sqrt(x²)) is zero for x<=0 and sqrt(2x) for x>0. The left original quotient is zero, the right is sqrt(2/h)->infinity: derivative at zero is undefined.'
    elif ident=='PC19-S1':
        if a.has(s.atan):
            y=s.Symbol('y');assert equal(a,s.atan(x)+s.atan(y)-s.atan((x+y)/(1-x*y)))
            return dict(exact=True,normalized=False,method='Direct arithmetic precursor retains the exact original expression and domain; explicit simplify still must normalize its branches.')
        assert a.has(s.Piecewise)
        y=s.Symbol('y');d=1-x*y
        (bad,condition),(body,otherwise)=a.args
        assert bad==s.Symbol('undef') and otherwise==True and isinstance(condition,s.Equality)
        assert equal(condition.lhs-condition.rhs,d)
        (zero,positive),(negative,otherwise)=body.args
        assert zero==0 and otherwise==True and isinstance(positive,s.StrictGreaterThan)
        assert equal(positive.lhs-positive.rhs,d) and equal(negative,s.pi*s.sign(x+y))
        method='For A=atan(x)+atan(y) in (-pi,pi), cos(A) has sign 1-xy and sin(A) sign x+y. Thus the principal atan subtraction is zero for xy<1 and pi*sign(x+y) for xy>1. The entire excluded hyperbola xy=1 survives.'
    else:raise AssertionError(ident)
    return dict(exact=True,normalized=True,method=method)


def actual_checks(case,expression,probe,env):
    cases={
      'PC19-I1':[('x','0','finite')],
      'PC19-I2':[('x','1','0')],
      'PC19-D1':[('x','0','0')],
      'PC19-D2':[('x','-2','0'),('x','-1','0'),('x','0','undef'),('x','2','1/2')],
      'PC19-S1':[('[x,y]','[0,0]','0'),('[x,y]','[1,2]','pi'),('[x,y]','[-1,-2]','-pi'),('[x,y]','[1,1]','undef'),('[x,y]','[-1,-1]','undef')],
    }
    rows=[]
    for variables,point,expected in cases.get(case['id'],[]):
        run=subprocess.run([str(probe),f'eval(subst({expression},{variables},{point}))'],env=env,capture_output=True,text=True,timeout=10)
        row=dict(point=point,expected=expected,exit=run.returncode,result=run.stdout.strip())
        if expected=='undef':assert run.returncode==3 and row['result']=='undef',row
        else:
            assert run.returncode==0,row
            value=parse(row['result'])
            if expected=='finite':assert value.is_finite is True,row
            elif case['id']=='PC19-S1' and not expression.startswith('simplify('):
                # Exact branch identity is established above; this is only
                # an independent diagnostic of actual point evaluation.
                assert abs(s.N(value-parse(expected),60))<s.Float('1e-50'),row
            else:assert equal(value,parse(expected)),row
        rows.append(row)
    return rows
