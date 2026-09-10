"""Independent exact residual AND full real-chart coverage certificates."""
import sympy as sp
x,y=sp.symbols('x y',real=True);A=sp.Symbol('a',positive=True)
def check(case,printed):
 t=sp.Symbol(case['parameter'],real=True);u,v=sp.symbols('U V',real=True)
 local={'x':x,'y':y,case['parameter']:t,'a':A,'ln':sp.log,'i':sp.I}
 def parse(s):return sp.sympify(s.replace('^','**'),locals=local)
 lhs,rhs=case['equation'].split('=');f=sp.expand(parse(lhs)-parse(rhs));branches=parse(printed)
 assert len(branches)==case['branches'],(case['id'],branches)
 def zero(g):return sp.simplify(sp.expand(g))==0
 def nonzero(g):return sp.simplify(g).is_zero is False
 def affine(branch,atoms):
  b=[sp.expand(q.xreplace(atoms)) for q in branch]
  assert all(not q.has(t) for q in b),(case['id'],b)
  polys=[sp.Poly(q,u,v) for q in b];assert all(p.total_degree()<=1 for p in polys)
  center=sp.Matrix([p.coeff_monomial(1) for p in polys]);M=sp.Matrix([[p.coeff_monomial(u),p.coeff_monomial(v)] for p in polys])
  assert nonzero(M.det()),(case['id'],M)
  return center,M
 def transform(center,M,shape):
  z=center+M*sp.Matrix([u,v]);g=sp.Poly(sp.expand(f.subs({x:z[0],y:z[1]},simultaneous=True)),u,v)
  target=sp.Poly(shape,u,v);monomial=next(iter(target.monoms()));scale=sp.simplify(g.coeff_monomial(monomial)/target.coeff_monomial(monomial))
  assert nonzero(scale)
  assert all(zero(q) for q in sp.Poly(g.as_expr()-scale*shape,u,v).coeffs()),(case['id'],g,shape)
 kind=case['kind']
 if kind=='ellipse':
  center,M=affine(branches[0],{sp.cos(t):u,sp.sin(t):v});transform(center,M,u*u+v*v-1)
  proof='invertible affine image of the entire unit circle, t real; inverse angle exists for every point'
 elif kind=='hyperbola':
  exponential=any(q.has(sp.exp(t)) for q in branches[0])
  atoms={sp.exp(t):u,sp.exp(-t):v} if exponential else {sp.cosh(t):u,sp.sinh(t):v}
  c,M=affine(branches[0],atoms);c1,M1=affine(branches[1],atoms)
  assert all(zero(q) for q in c1-c)
  expected=-M if exponential else M*sp.diag(-1,1)
  assert all(zero(q) for q in M1-expected),(case['id'],M,M1)
  transform(c,M,u*v-1 if exponential else u*u-v*v-1)
  proof='invertible affine image of both hyperbola sheets: '+('exp(t)>0 and the second chart changes both signs' if exponential else 'cosh(t)>=1, sinh(t) surjective, and the second chart reverses cosh direction')
 elif kind in ['parallel-lines','crossing-lines','parabola']:
  polynomials=[[sp.Poly(q,t) for q in b] for b in branches]
  for b in branches:assert zero(f.subs({x:b[0],y:b[1]},simultaneous=True)),(case['id'],b)
  if kind=='parabola':
   assert max(p.degree() for p in polynomials[0])==2
   vectors=sp.Matrix([[p.coeff_monomial(t),p.coeff_monomial(t*t)] for p in polynomials[0]])
   assert nonzero(vectors.det());proof='invertible affine image of (t,t^2), with t covering the full real line'
  else:
   centers=[];directions=[]
   for p in polynomials:
    assert max(q.degree() for q in p)==1
    centers.append(sp.Matrix([q.coeff_monomial(1) for q in p]));directions.append(sp.Matrix([q.coeff_monomial(t) for q in p]))
    assert any(nonzero(c) for c in directions[-1])
   if len(branches)==2:
    if kind=='crossing-lines':assert nonzero(sp.Matrix.hstack(*directions).det())
    else:
     assert zero(sp.Matrix.hstack(*directions).det())
     assert nonzero(sp.Matrix.hstack(directions[0],centers[1]-centers[0]).det())
    proof='two distinct complete lines contained in a nonzero quadratic exhaust its real zero set'
   else:
    nx,ny=-directions[0][1],directions[0][0];line=nx*(x-centers[0][0])+ny*(y-centers[0][1]);fpoly=sp.Poly(f,x,y);lpoly=sp.Poly(line*line,x,y)
    m=next(iter(lpoly.monoms()));scale=sp.simplify(fpoly.coeff_monomial(m)/lpoly.coeff_monomial(m))
    assert nonzero(scale) and zero(f-scale*line*line);proof='the quadratic is a nonzero multiple of the square of the returned complete line'
 else:
  poly=sp.Poly(f,x,y);a=poly.coeff_monomial(x*x);b=poly.coeff_monomial(x*y);c=poly.coeff_monomial(y*y);d=poly.coeff_monomial(x);e=poly.coeff_monomial(y);constant=poly.coeff_monomial(1);det=4*a*c-b*b
  if kind=='point':
   point=branches[0];assert all(not q.has(t) for q in point);assert zero(f.subs({x:point[0],y:point[1]}));assert sp.simplify(det).is_positive
   assert zero(sp.diff(f,x).subs({x:point[0],y:point[1]})) and zero(sp.diff(f,y).subs({x:point[0],y:point[1]}));proof='definite quadratic with zero value at its stationary point has precisely that singleton real zero set'
  else:
   assert kind=='empty'
   if nonzero(det):
    assert sp.simplify(det).is_positive;h=(b*e-2*c*d)/det;k=(b*d-2*a*e)/det
    assert sp.simplify(a*f.subs({x:h,y:k})).is_positive
   else:
    if zero(a):a,c,d,e=c,a,e,d
    assert nonzero(a) and zero(e-d*b/(2*a));assert sp.simplify(d*d-4*a*constant).is_negative
   proof='definite shifted quadratic of the wrong sign, or rank-one quadratic with negative discriminant: empty real zero set'
 return {'exact':True,'coverage':proof,'branch_count':len(branches)}
