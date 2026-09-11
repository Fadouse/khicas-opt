"""Independent complete-domain proofs for cycle 20 (P1 is historical)."""
import re,subprocess
import sympy as s
from mixed_reference import parse as base_parse,local,x,equal

def parse(text):
    # Literal Giac piecewise uses comma-separated condition/value pairs.
    # Protect each nested piecewise before the shared ternary parser runs.
    values={}
    def replace(text):
        while 'piecewise(' in text:
            start=text.rfind('piecewise(');pos=start+10;depth=0;parts=[];last=pos
            for j in range(pos,len(text)):
                c=text[j]
                if c=='(':depth+=1
                elif c==')':
                    if depth==0:
                        parts.append(text[last:j]);end=j+1;break
                    depth-=1
                elif c==',' and depth==0:parts.append(text[last:j]);last=j+1
            pairs=[]
            for k in range(0,len(parts)-1,2):
                bits=re.split(r'(?<![<>!])={1,2}',parts[k],maxsplit=1)
                condition=s.Eq(base_parse(bits[0]),base_parse(bits[1]),evaluate=False) if len(bits)==2 else base_parse(parts[k])
                pairs.append((base_parse(parts[k+1]),condition))
            name='pc20_piece_'+str(len(values));values[name]=s.Piecewise(*pairs,(base_parse(parts[-1]),True),evaluate=False);local[name]=values[name]
            text=text[:start]+name+text[end:]
        return text
    try:return base_parse(replace(text))
    finally:
        for name in values:local.pop(name,None)

def lazy(e,sub):
    if e.func==s.Piecewise:
        for value,condition in e.args:
            c=s.simplify(condition.subs(sub)) if condition!=True else s.true
            if c==s.true:return lazy(value,sub)
            assert c==s.false,('Undecided reference branch',c)
        raise AssertionError('No selected branch')
    return s.simplify(e.subs(sub))

def regular(e):
    while e.has(s.Piecewise):e=e.replace(lambda z:z.func==s.Piecewise,lambda z:z.args[-1][0])
    return e

def verify(case,printed):
    ident=case['id'];assert not any(t in printed for t in ('integrate(','diff(','rootof('))
    if ident.startswith('PC20-P'):
        raw=next(line[4:] for line in printed.splitlines() if line.startswith('RAW '));left,right=raw.split('=');r,t=s.symbols('r theta')
        if ident=='PC20-P1':
            assert left=='r' and equal(parse(right),s.sqrt(2)*s.sqrt(s.cos(2*t)))
            return dict(exact=True,novel=False,method='Historical lemniscate mechanism. Reduced real-radius chart includes cos(2theta)=0 endpoints, hence the origin and both lobes. It represents the Cartesian zero set, not every redundant polar pair for the origin.')
        assert equal(parse(left)-parse(right),r*s.sin(t)-s.exp(r*s.cos(t)))
        return dict(exact=True,novel=True,method='Direct residual identity, with no division. For r>0 and sin(theta)>0, r*sin(theta)*exp(-r*cos(theta)) increases monotonically if cos<=0; if cos>0 it has maximum tan(theta)/e at r=1/cos(theta). This proves all zero/one/two-radius branches and the included double tangency at theta=atan(e). r=0 is excluded by residual -1.')
    a=parse(printed);y=s.Symbol('y');parameter=s.Symbol('a');t=s.Symbol('t',positive=True)
    if ident=='PC20-I1':
        assert a.has(s.Piecewise),'Generic primitive misses a=+/-1 and real pole regimes'
        # All three parameter regimes are verified algebraically; the
        # boundary cases must be selected before evaluating invalid roots.
        q=x*x+2*parameter*x+1
        leaves=[]
        def visit(e):
            if e.func==s.Piecewise:
                for v,_ in e.args:visit(v)
            else:leaves.append(e)
        visit(a)
        types=set()
        for f in leaves:
            if f==s.Symbol('undef'):continue
            if f.has(s.atan):assert equal(s.diff(f,x),1/q);types.add('atan')
            elif f.has(s.log):
                for node in list(s.preorder_traversal(f)):
                    if node.func==s.log and node.args[0].func==s.Abs:f=f.xreplace({node:s.log(node.args[0].args[0])})
                assert equal(s.diff(f,x),1/q);types.add('log')
            else:
                for v in (-1,1):assert equal(s.diff(f.subs(parameter,v),x),1/q.subs(parameter,v))
                types.add('repeated')
        assert types=={'atan','log','repeated'}
        for v in (-1,1):assert lazy(a,{parameter:v,x:0}).is_finite is True
        method='Complete the square Q=(x+a)^2+1-a². Positive, zero and negative 1-a² give respectively the atan, repeated-pole rational and real ln|ratio| branches. Exact derivative identities and explicit degeneracy selection; original poles are excluded by Q=0.'
    elif ident=='PC20-I2':
        target=2*s.sqrt(1+x)*s.asin(x)+4*s.sqrt(1-x)
        constant=s.simplify((a-target).subs(x,0));assert not constant.has(x) and equal(a,target+constant)
        # Endpoint proof uses the ORIGINAL primitive under x=cos(t).
        transformed=2*s.sqrt(2)*s.cos(t/2)*(s.pi/2-t)+4*s.sqrt(2)*s.sin(t/2)
        assert s.limit((transformed-s.pi*s.sqrt(2))/(s.cos(t)-1),t,0,dir='+')==s.pi/(2*s.sqrt(2))
        assert a.subs(x,1).is_finite is True
        method='Integration by parts on the exact original domain (-1,1]. Under x=cos(t), compute the original endpoint difference quotient, obtaining pi/(2sqrt2). The finite primitive limit at excluded x=-1 is only an extension value; no finite original derivative there is claimed. One additive constant is accepted.'
    elif ident=='PC20-D1':
        assert lazy(a,{x:0})==1,'Original quotient at accumulation point tends to 1, not 0 or 2'
        assert equal(regular(a),2*x*s.floor(1/x))
        n=s.Symbol('n',integer=True,nonzero=True)
        assert lazy(a,{x:1/n})==s.Symbol('undef'),'All reciprocal-integer jumps must be excluded'
        method='Write floor(1/h)=1/h-q, 0<=q<1: original quotient 1-h*q tends to 1 on both sides. At 1/n the original left quotient tends to 2 and the right contains -1/(n²*h), hence diverges. Between these points floor is locally constant; zero is included despite accumulating discontinuities.'
    elif ident=='PC20-D2':
        assert lazy(a,{x:0,y:0})==0,'At y=0 the entire original function is zero'
        for sign in (-1,1):assert lazy(a,{x:0,y:sign*t})==s.Symbol('undef')
        assert equal(regular(a),x/s.sqrt(x*x+y*y)-s.sign(x))
        method='For y=0 sqrt(x²)-|x| vanishes identically. For fixed y!=0, the original quotient at x=0 is h/(sqrt(h²+y²)+|y|)-sign(h), giving +1 and -1. Else differentiate on x!=0. Thus only x=0,y!=0 is excluded.'
    elif ident=='PC20-S1':
        source=s.sqrt(x+s.sqrt(x*x-y*y))+s.sqrt(x-s.sqrt(x*x-y*y))
        if equal(a,source):return dict(exact=True,normalized=False,method='Direct precursor retains all original radicals. Original real domain is x>=|y|; explicit simplify must denest with the domain guard.')
        assert a.func==s.Piecewise
        (bad,condition),(value,otherwise)=a.args
        assert bad==s.Symbol('undef') and otherwise==True and isinstance(condition,s.StrictLessThan)
        assert equal(condition.lhs,x) and equal(condition.rhs,s.Abs(y))
        assert equal(value,s.sqrt(2*(x+s.Abs(y))))
        method='The original three-root domain is exactly x>=|y|. Nonnegative outer roots A,B satisfy AB=sqrt(y²)=|y|, hence A+B=sqrt(2*(x+|y|)). The guard excludes both the negative cone and all newly real points of the simplified radical outside the original cone; boundary rays and origin are included.'
    else:raise AssertionError(ident)
    return dict(exact=True,normalized=True,method=method)


def actual_checks(case,expression,probe,env):
    items={
      'PC20-I1':[('[x,a]','[0,1]','finite'),('[x,a]','[0,-1]','finite'),('[x,a]','[-1,1]','undef'),('[x,a]','[1,-1]','undef'),('[x,a]','[-2+sqrt(3),2]','undef')],
      'PC20-I2':[('x','1','finite')],
      'PC20-D1':[('x','0','1')]+[('x',str(s.Rational(1,n)),'undef') for n in [-7,-2,-1,1,2,7]]+[('x','2/3','4/3'),('x','-2/3','8/3')],
      'PC20-D2':[('[x,y]','[0,0]','0'),('[x,y]','[2,0]','0'),('[x,y]','[-2,0]','0'),('[x,y]','[0,1]','undef'),('[x,y]','[0,-1]','undef')],
      'PC20-S1':[('[x,y]','[0,0]','0'),('[x,y]','[1,1]','2'),('[x,y]','[1,-1]','2'),('[x,y]','[2,0]','2')]+([('[x,y]','[0,1]','undef'),('[x,y]','[-2,1]','undef')] if expression.startswith('simplify(') else []),
    }
    rows=[]
    for variables,point,expected in items.get(case['id'],[]):
        r=subprocess.run([str(probe),f'eval(subst({expression},{variables},{point}))'],env=env,capture_output=True,text=True,timeout=10)
        row=dict(point=point,expected=expected,exit=r.returncode,result=r.stdout.strip())
        assert r.returncode==(3 if expected=='undef' else 0),row
        if expected=='undef':assert row['result']=='undef',row
        elif expected=='finite':assert parse(row['result']).is_finite is True,row
        else:assert equal(parse(row['result']),parse(expected)),row
        rows.append(row)
    return rows
