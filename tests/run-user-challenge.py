#!/usr/bin/env python3
"""Integrals: target computation, outer simplify, independent checks."""
import argparse,hashlib,json,os,re,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,build,build_validation_probe
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--report',type=Path,required=True)
p.add_argument('--corpus',type=Path,default=ROOT/'tests/user-challenge-integrals.json')
args=p.parse_args()
corpus=args.corpus
cases=json.loads(corpus.read_text())['cases']
report={'scope':'Repository integration, normalization and FXCG simplification entry points; other dependencies host Giac, not a CG50 emulator or hardware timings',
        'validation':'Printed target results independently checked in a separate host-Giac process',
        'corpus_sha256':hashlib.sha256(corpus.read_bytes()).hexdigest(),
        'validation_probe_sha256':hashlib.sha256((ROOT/'tests/integration_probe.cc').read_bytes()).hexdigest(),
          'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','zintgab.cc','ysym2poly.cc','ksubst.cc','integration_guard.h','equation_normalize.h','dilogarithm.h') if (ROOT/n).exists()},
        'runs':[]}
with tempfile.TemporaryDirectory(prefix='khicas-user-challenge-') as tmp:
    d=Path(tmp);target=build(d/'target',target_simplify=True);validator=build_validation_probe(d/'validator')
    for case in cases:
        definite='bounds' in case
        integral='integrate('+case['f']+',x'+(','+','.join(case['bounds']) if definite else '')+')'
        for outer in (False,True):
            expression='simplify('+integral+')' if outer else integral
            normal_result=None
            for stack in ('normal','64'):
                env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
                if stack!='normal':env['KHICAS_TEST_STACK_KIB']=stack
                row={'id':case['id'],'input':expression,'stack_KiB':stack}
                try:
                    result=subprocess.run([str(target),expression],capture_output=True,text=True,env=env,timeout=30)
                    row.update(exit=result.returncode,result=result.stdout.strip(),stderr=result.stderr)
                    m=re.search(r'^SECONDS ([\d.e+-]+)',result.stderr,re.M)
                    if m:row['host_seconds']=float(m[1])
                    for label,key in (('PARSER_CALLS','parser_calls'),('MAX_RSS_KB','host_max_rss_kb')):
                        m=re.search(r'^'+label+r' (\d+)',result.stderr,re.M)
                        if m:row[key]=int(m[1])
                    row['result_characters']=len(row['result'])
                    assert result.returncode==0,row
                    verification=[case['expected'],'definite' if definite else 'indefinite',case['f']]
                    if not definite:verification+=case.get('samples',['1/3','1','2'])
                    if case.get('verification',{}).get('symbolic_proof')=='generic-binomial-parts':
                        from compact_reference import verify_binomial_reference
                        row['validation']=verify_binomial_reference(case,row['result'])
                    elif case.get('verification',{}).get('symbolic_proof')=='dilogarithm':
                        from dilogarithm_reference import verify_dilogarithm_reference
                        row['validation']=verify_dilogarithm_reference(case,row['result'])
                    else:
                        check=subprocess.run([str(validator),row['result']]+verification,
                                             capture_output=True,text=True,timeout=30)
                        row['validation']={'exit':check.returncode,'stderr':check.stderr}
                        assert check.returncode==0 and 'CHECK exact' in check.stderr,row
                    if stack=='normal':normal_result=row['result']
                    else:assert row['result']==normal_result,row
                    row['status']='exact'
                except (AssertionError,subprocess.TimeoutExpired) as error:
                    row['status']='failed';report['runs'].append(row)
                    row['error']=str(error)
                    args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
                    print(case['id'],'simplify' if outer else 'integrate',stack,'FAILED',flush=True)
                    continue
                report['runs'].append(row)
                args.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
                print(case['id'],'simplify' if outer else 'integrate',stack,'exact',flush=True)
failures=sum(r['status']!='exact' for r in report['runs'])
if failures:raise SystemExit(f'FAIL: {failures} of {len(report["runs"])} runs failed; see {args.report}')
print(f'PASS: all {len(cases)} exact with/without outer simplify on normal and guarded 64 KiB stacks')
