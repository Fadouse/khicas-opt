"""Independent mathematics for mixed acceptance, including domain checks."""
import sympy as sp
from trig_log_reference import verify_trig_log_reference
from conic_reference import check as conic_check
x=sp.Symbol('x',real=True)
local={'x':x,'i':sp.I,'ln':sp.log,'abs':sp.Abs,'im':sp.im,'re':sp.re,'floor':sp.floor,'Li2':lambda z:sp.polylog(2,z),'Eq':sp.Eq,'Piecewise':lambda *a:sp.Piecewise(*a,evaluate=False)}
def convert_ternary(text):
    # Giac prints when as ((condition)? yes : no). Parentheses are parsed,
    # never matched by a regex across nested expressions.
    def group(start):
        out='';j=start
        while j<len(text) and text[j]!=')':
            if text[j]=='(':
                inner,j=group(j+1);out+='('+inner+')'
            else:out+=text[j];j+=1
        if '?' in out:
            depth=0;q=colon=None
            for k,ch in enumerate(out):
                depth+=(ch=='(')-(ch==')')
                if depth==0 and ch=='?':q=k
                if depth==0 and ch==':' and q is not None:colon=k;break
            if q is not None:
                condition=out[:q].strip()
                while condition.startswith('(') and condition.endswith(')'):condition=condition[1:-1]
                lhs,rhs=condition.split('=',1)
                out='Piecewise(('+out[q+1:colon]+',Eq('+lhs+','+rhs+')),('+out[colon+1:]+',True))'
        return out,j+1
    return group(0)[0]
def parse(s):return sp.sympify(convert_ternary(s).replace('^','**'),locals=local)
def equal(a,b):return sp.simplify(sp.expand_func(a-b))==0

def verify(case,printed):
    id=case['id']
    if case['type']=='conic':return conic_check(case,printed)
    assert not any(s in printed for s in ['integrate(', 'diff(', 'rootof(']),printed
    actual=parse(printed)
    if id in ['MR1-I1','MR1-I2']:
        c={'id':id,'expected':case['expected']['expression']}
        if id=='MR1-I1':c.update(f='-3*ln(abs(4*sin(-2*x/3+pi/5)))',frequency='-2/3',phase='pi/5',inside_coefficient=4,cosine=False)
        else:c.update(f='2*ln(-2*cos(3*x/2-pi/7))',frequency='3/2',phase='-pi/7',inside_coefficient=-2,cosine=True)
        return verify_trig_log_reference(c,printed)
    if id=='MR1-I3':
        # Li2(z)+Li2(-z)=Li2(z^2)/2 on the closed unit disk away from
        # singular endpoints; analytic continuation of the even power series.
        z=sp.exp(sp.I*(x+sp.pi/3));canonical=-sp.im(sp.polylog(2,z))-sp.im(sp.polylog(2,-z))
        repl={}
        for p in actual.atoms(sp.polylog):
            for q in canonical.atoms(sp.polylog):
                if sp.simplify(sp.expand_complex(p.args[1]-q.args[1]))==0:repl[p]=q;break
        assert equal(actual.xreplace(repl),canonical) or equal(actual,parse(case['expected']['expression']))
        return {'exact':True,'method':'unit-circle Li2 duplication identity; logarithm product is positive on the stated real intervals'}
    if id=='MR1-I4':
        assert equal(actual,parse(case['expected']['expression']))
        derivative=sp.diff(actual,x).replace(lambda a:a.func==sp.polylog and a.args[0]==1,lambda a:-sp.log(1-a.args[1]))
        assert equal(derivative,sp.log(1+x*x)/(x*(1+x*x)))
        return {'exact':True,'method':'exact real-domain derivative, negative Li2 argument avoids its branch cut'}
    if id=='MR1-D1':
        assert sp.count_ops(actual)<128,('unresolved derivative expansion',sp.count_ops(actual))
        u=-2*x/3+sp.pi/5;s=sp.Symbol('s',real=True);L=sp.Symbol('L',positive=True)
        def canonical(e):
            replacements={}
            for atom in e.atoms(sp.sin):
                if equal(atom.args[0],u):replacements[atom]=s
                elif equal(atom.args[0],-u):replacements[atom]=-s
            e=e.xreplace(replacements).subs(sp.Abs(s),L)
            return sp.expand_log(e,force=True)
        assert equal(canonical(actual),canonical(parse(case['expected']['expression'])))
        return {'exact':True,'method':'positive real logarithm identities, both signs of sine; bounded output'}
    if id=='MR1-D2':
        w=2*x-1;target=-4*sp.log(1+w*w)/w
        assert actual.has(sp.Piecewise),'missing removable derivative value'
        # The only possible zero is w=0; prove every guard, then verify
        # the formula on its complement and its value at the isolated point.
        regular=actual
        for p in actual.atoms(sp.Piecewise):
            (yes,condition),(no,otherwise)=p.args
            assert isinstance(condition,sp.Equality) and otherwise==True
            assert sp.solve(condition,x)==[sp.Rational(1,2)]
            regular=regular.xreplace({p:no})
        assert equal(regular,target)
        assert sp.simplify(actual.subs(x,sp.Rational(1,2)))==0
        assert sp.limit(target,x,sp.Rational(1,2))==0
        return {'exact':True,'method':'piecewise algebra, unique zero locus, and exact removable limit'}
    target=parse(case['expected']['expression']);delta=sp.trigsimp(sp.expand(actual-target))
    assert sp.simplify(delta)==0
    denominator=sp.denom(sp.cancel(actual));assert not denominator.has(sp.sin,sp.cos,sp.tan)
    if id=='MR1-D3':
        assert sp.Poly(denominator,x).intervals(eps=sp.Rational(1,100))==[]
        assert equal(actual.subs(x,1),sp.sin(2))
    if id=='MR1-D4':assert not denominator.has(x)
    return {'exact':True,'method':'exact rational/trig identity and real denominator/domain check'}
