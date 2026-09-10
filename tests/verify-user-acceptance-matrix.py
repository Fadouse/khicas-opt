#!/usr/bin/env python3
"""Independent symbolic audit of printed results; preserve domain/resource gaps."""
import argparse,json,signal
from pathlib import Path
import sympy as sp
from integration_build import ROOT
p=argparse.ArgumentParser();p.add_argument('--input',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
a=p.parse_args();data=json.loads(a.input.read_text());cases={c['id']:c for c in json.loads((ROOT/'tests/user-acceptance-matrix.json').read_text())['cases']}
x,b,r=sp.symbols('x b r',real=True);aa,s=sp.symbols('a s',positive=True);n=sp.Symbol('n',integer=True,nonnegative=True)
locals={'x':x,'a':aa,'b':b,'r':r,'s':s,'n':n,'i':sp.I,'ln':sp.log,'abs':sp.Abs,'sign':sp.sign,'Psi':lambda z,k=0:sp.polygamma(k,z),'Gamma':sp.gamma,'FresnelS':sp.fresnels,'Si':sp.Si,'Ei':sp.Ei,'Li2':lambda z:sp.polylog(2,z),'infinity':sp.oo}
def parse(t):return sp.sympify(t.replace('^','**'),locals=locals)
def timeout(sig,frame):raise TimeoutError('independent symbolic check exceeded 8 seconds')
signal.signal(signal.SIGALRM,timeout)
cache={};rows=[]
for row in data['runs']:
 c=cases[row['id']];entry=dict(row);key=(row['id'],row.get('result',''))
 if row['mode']=='without-assumptions':
  entry['acceptance']='condition-audit-only';rows.append(entry);continue
 if c.get('behavior')=='divergent':
  entry['acceptance']='divergence-rejected' if row.get('exit')==3 and row.get('result')=='undef' else 'incorrect-finite-answer' if row.get('exit')==0 else 'resource-or-execution-failure'
  entry['explicit_divergence_message']='Divergent improper integral' in row.get('stderr','')
  rows.append(entry);continue
 if row.get('exit')!=0:
  entry['acceptance']='unresolved' if row.get('exit')==2 else 'resource-or-execution-failure';rows.append(entry);continue
 if key not in cache:
  signal.alarm(8)
  try:
   actual=parse(row['result']);expected=parse(c['expected'])
   if 'bounds' in c:
    delta=actual-expected
    if c['id']=='C1':delta=sp.expand_log(delta.subs(b,sp.Symbol('positive_b',positive=True)),force=False)
    if c['id']=='D6':
     # a>|b| entails a-b>0 and a^2-b^2>0. These are domain facts,
     # not signs guessed from a numerical parameter substitution.
     for atom in delta.atoms(sp.sign):
      ratio=sp.simplify(atom.args[0]/(aa-b))
      if ratio.is_number and ratio.is_real and ratio!=0:delta=delta.xreplace({atom:sp.sign(ratio)})
     delta=sp.factor(delta)
    ok=sp.simplify(delta)==0;method='exact symbolic value identity under the stated real assumptions'
   else:
    f=parse(c['f']);delta=sp.expand_func(sp.diff(actual,x))-f;rd=sp.expand_func(sp.diff(expected,x))-f
    # Ordinary derivatives are checked on open domain intervals. A1's zero
    # is separately checked by the vanishing difference quotient below.
    delta=delta.replace(sp.DiracDelta,lambda *args:sp.S.Zero)
    rd=rd.replace(sp.DiracDelta,lambda *args:sp.S.Zero)
    u=sp.Symbol('u',positive=True)
    subs=[u,-u]
    if c['id'] in ('A3','F6'):subs=[sp.exp(u),sp.exp(-u)]
    if c['id']=='A6':subs=[1+u,-1-u,1/(1+u),-1/(1+u)]
    if c['id']=='F5':subs=[u/(1+u)]
    ok=all(sp.simplify(d.subs(x,v).rewrite(sp.exp))==0 for d in (delta,rd) for v in subs)
    if c['id']=='A1':ok=ok and actual.subs(x,0)==0 and all(sp.limit(actual.subs(x,v)/v,u,0,dir='+')==0 for v in (u,-u))
    method='exact ordinary derivatives of actual and reference on parameterized real domain intervals'
   if c.get('real_values'):
    ok=ok and all(sp.simplify(sp.im(actual.subs(x,parse(point))))==0 for point in c['samples'])
    method+='; actual values real at domain checkpoints'
   cache[key]={'math_pass':bool(ok),'method':method}
  except (TimeoutError,Exception) as error:cache[key]={'math_pass':False,'check_error':str(error)}
  finally:signal.alarm(0)
 entry['verification']=cache[key]
 entry['acceptance']='exact' if cache[key]['math_pass'] else 'mismatch-or-unverified'
 if entry['acceptance']=='exact' and c.get('coverage_limit'):entry['acceptance']='parameter-domain-gap'
 rows.append(entry);print(entry['id'],entry['mode'],entry['stack'],entry['acceptance'],flush=True)
report={'scope':'Independent SymPy comparison of printed answers. Ordinary derivative intervals exclude poles. Successful numeric examples do not certify symbolic parameter domains. Resource failures remain failures even if an answer was printed.','input_report':str(a.input),'runs':rows,'cases':[]}
for id,c in cases.items():
 runs=[q for q in rows if q['id']==id and q['mode']!='without-assumptions'];statuses=sorted({q['acceptance'] for q in runs})
 report['cases'].append({'id':id,'statuses':statuses,'all_four_pass':len(runs)==4 and statuses in (['exact'],['divergence-rejected']),'coverage_limit':c.get('coverage_limit')})
a.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
