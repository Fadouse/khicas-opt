"""Independent SymPy proof of printed Li2 primitives on their real domains."""
import sympy as sp

def verify_dilogarithm_reference(case, printed):
    x=sp.Symbol('x',positive=True)
    local={'x':x,'ln':sp.log,'Li2':lambda z:sp.polylog(2,z)}
    def parse(t):return sp.sympify(t.replace('^','**'),locals=local)
    actual,reference=parse(printed),parse(case['expected'])
    if 'bounds' in case:
        assert sp.simplify(actual-reference)==0,(case['id'],actual,reference)
        method='exact independent symbolic endpoint value identity'
    else:
        f=parse(case['f'])
        for primitive in (actual,reference):
            assert sp.simplify(sp.expand_func(sp.diff(primitive,x))-f)==0,(case['id'],primitive)
        method='exact independent derivatives of actual and reference, principal Li2 on declared real intervals'
    return {'exit':0,'stderr':'CHECK exact: '+method,'engine':'SymPy (does not invoke target derivative metadata)'}
