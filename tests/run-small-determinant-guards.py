#!/usr/bin/env python3
"""Independent determinant identities, complete pole sets and fixed-size tests."""
import argparse
import hashlib
import json
import os
import random
import subprocess
from pathlib import Path
import sympy as s
from mixed_reference import parse, x, equal

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--probe',required=True)
p.add_argument('--report',type=Path,required=True)
a=p.parse_args()
y,z=s.symbols('y z')
variables=(x,y,z)

def giac(e):return str(e).replace('**','^')
def matrix_text(matrix):return '['+','.join('['+','.join(giac(v) for v in row)+']' for row in matrix)+']'

cases=[]
for n in (2,3,4):
 for powers in (False,True):
  u=[x,y,z,x+y][:n];v=[z,x,y,x-y][:n]
  if powers:u=[q**4 for q in u];v=[q**4 for q in v]
  for diagonal in ([1]*n,list(range(1,n+1)),[0]+list(range(2,n+1)),[0,0]+list(range(3,n+1))):
   for D in (s.Integer(1),(x+y+z)**2,x-1):
    M=[[(diagonal[i] if i==j else 0)+u[i]*v[j]/D for j in range(n)] for i in range(n)]
    # Column multilinearity: terms with >=2 update columns vanish,
    # including singular diagonal backgrounds; no inverse is assumed.
    target=s.prod(diagonal)+sum(u[i]*v[i]*s.prod(diagonal[k] for k in range(n) if k!=i) for i in range(n))/D
    cases.append(dict(input='det('+matrix_text(M)+')',target=target,den=D,
                      proof='Column multilinearity for a general diagonal background plus a rank-one update; valid even with one or several zero diagonal entries.'))
# Genuine generic matrices: the independent reference expands by SymPy's
# determinant, not by reproducing the production subset recurrence.
rng=random.Random(20260918)
for n in (2,3,4):
 for D in (s.Integer(1),x+y+z):
  N=s.Matrix(n,n,lambda i,j:sum(rng.randint(-3,3)*v for v in (s.Integer(1),x,y,z)))
  M=[[N[i,j]/D for j in range(n)] for i in range(n)]
  cases.append(dict(input='det('+matrix_text(M)+')',target=N.det(method='berkowitz')/D**n,den=D,
                    proof='Independent Berkowitz determinant of a seeded full polynomial matrix; no assumption about rank, pivot signs or nonzero diagonal.'))
# Trace cancellation must not erase poles in off-diagonal matrix entries.
for n in (2,3,4):
 D=x+y+z
 M=[[s.Integer(i==j) for j in range(n)] for i in range(n)];M[0][1]=1/D
 cases.append(dict(input='det('+matrix_text(M)+')',target=s.Integer(1),den=D,
                   proof='Unit triangular matrix on its complete original domain; determinant 1 does not define an originally undefined off-diagonal entry.'))
rows=[];cache={}
for index,c in enumerate(cases):
 for outer in (False,True):
  for stack in ('normal','64'):
   expression='simplify('+c['input']+')' if outer else c['input']
   env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
   if stack=='64':env['KHICAS_TEST_STACK_KIB']='64'
   row=dict(case=index,input=expression,stack=stack)
   try:
    run=subprocess.run([a.probe,expression],capture_output=True,text=True,env=env,timeout=10)
    row.update(exit=run.returncode,result=run.stdout.strip(),stderr=run.stderr)
    assert run.returncode==0 and len(run.stdout.encode())<=65536,row
    assert 'det(' not in run.stdout,'The bounded test requires a computed determinant'
    key=(index,run.stdout)
    if key not in cache:
     actual=parse(run.stdout.strip())
     if c['den']!=1:
      assert actual.func==s.Piecewise,'Missing original matrix pole guard'
      (value,condition),(regular,otherwise)=actual.args
      assert value==s.Symbol('undef') and otherwise==True and isinstance(condition,s.Equality)
      lhs=s.Poly(condition.lhs-condition.rhs,*variables).sqf_part().monic()
      rhs=s.Poly(c['den'],*variables).sqf_part().monic()
      assert lhs==rhs,'Guard must have exactly the original excluded zero set'
      actual=regular
     assert s.cancel(actual-c['target'])==0,(actual,c['target'])
     cache[key]=True
    # Actual program substitution verifies lazy handling of forbidden
    # planes. Different allowed points include singular matrices too.
    points=[(0,0,0),(1,-1,0),(1,1,-2),(1,0,0),(2,1,1)]
    checked=[]
    for point in points:
     sub=dict(zip(variables,point));excluded=c['den'].subs(sub)==0
     request=f'eval(subst({expression},[x,y,z],{list(point)}))'
     r=subprocess.run([a.probe,request],capture_output=True,text=True,env=env,timeout=10)
     if excluded:assert r.returncode==3 and r.stdout.strip()=='undef',(point,r.stdout)
     else:assert r.returncode==0 and equal(parse(r.stdout.strip()),c['target'].subs(sub)),(point,r.stdout)
     checked.append(dict(point=point,excluded=bool(excluded)))
    row.update(passed=True,proof=c['proof'],actual_points=checked)
   except Exception as error:row.update(passed=False,error=str(error))
   rows.append(row)
   if not row['passed']:print('FAIL',index,outer,stack,row.get('error','')[:160],flush=True)
   a.report.write_text(json.dumps(dict(scope='Actual determinant entry and bounded algorithm; independent identities and exact complete pole-set comparison, normal and guarded host stacks.',probe_sha256=hashlib.sha256(Path(a.probe).read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows)
