#!/usr/bin/env python3
"""Original user inputs, exact validation and a separate 64 KiB stack regression."""
import argparse,hashlib,json,re,os,statistics,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,build,build_validation_probe

def output_text(value):
    # TimeoutExpired can expose bytes even when subprocess text=True was used.
    return value.decode(errors='replace') if isinstance(value,bytes) else value or ''

def run_probe(command,phase):
    try:
        r=subprocess.run(command,capture_output=True,text=True,timeout=30)
        checks=re.findall(r'^CHECK (\w+)',r.stderr,re.M)
        result={'exit':r.returncode,
                'status':checks[-1] if checks else {2:'unevaluated',3:'undefined'}.get(
                    r.returncode,'computed' if phase=='integration' and r.returncode==0 else 'error'),
                'result':r.stdout.strip(),'stderr':r.stderr}
        if r.returncode and result['status']=='exact':result['status']='error'
    except subprocess.TimeoutExpired as error:
        stderr=output_text(error.stderr)
        # In a combined probe SECONDS is emitted before verification starts.
        timed_out='validation' if phase=='combined' and re.search(r'^SECONDS ',stderr,re.M) else phase
        if timed_out=='combined':timed_out='integration'
        result={'status':timed_out+'_timeout','result':output_text(error.stdout).strip(),'stderr':stderr}
    measured=re.search(r'^SECONDS ([\d.e+-]+)',result['stderr'],re.M)
    if measured:result['seconds']=float(measured[1])
    return result

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--report',type=Path,required=True)
p.add_argument('--repeats',type=int,default=3)
p.add_argument('--target-simplify',action='store_true')
args=p.parse_args();assert args.repeats>0
cases=json.loads((ROOT/'tests/user-integrals.json').read_text())
report={'base_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
        'scope':'actual yintg, zintgab and normalization; other dependencies including outer simplify are host Giac; not CG50 timings',
        'validation_probe_sha256':hashlib.sha256((ROOT/'tests/integration_probe.cc').read_bytes()).hexdigest(),
          'source_sha256':{n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in ('yintg.cc','zintgab.cc','ysym2poly.cc','integration_guard.h','dilogarithm.h')},
        'timeout_seconds':30,'virtual_address_limit_MiB':768,'cases':[]}
report['stack_test']='64 KiB guarded pthread computation stack; process/library startup uses normal stack'
report['validation_scope']='same probe process; host Giac outer simplify and repository normalization'
if args.target_simplify:
    report['scope']='actual yintg, zintgab, normalization and FXCG simplify entry points; other dependencies host Giac; not CG50 timings'
    report['validation_scope']='separate host Giac probe; validates the printed target result, with no repository integration, normalization or FXCG simplify linked'
    report['validation_timeout_seconds']=30
    report['source_sha256']['ksubst.cc']=hashlib.sha256((ROOT/'ksubst.cc').read_bytes()).hexdigest()
    if (ROOT/'equation_normalize.h').exists():report['source_sha256']['equation_normalize.h']=hashlib.sha256((ROOT/'equation_normalize.h').read_bytes()).hexdigest()
failed=False
with tempfile.TemporaryDirectory(prefix='khicas-user-integrals-') as tmp:
    exe=build(Path(tmp),target_simplify=args.target_simplify)
    validator=build_validation_probe(Path(tmp)/'validation') if args.target_simplify else None
    for case in cases:
        row=dict(case);row['runs']=[]
        for _ in range(args.repeats):
            verification=[case['reference'],case['mode'],case['integrand']]+case['samples']
            if validator:
                result=run_probe([str(exe),case['input']],'integration')
                result['integration_status']=result['status']
                if result['status']=='computed':
                    validation=run_probe([str(validator),result['result']]+verification,'validation')
                    if 'seconds' in validation:
                        validation['result_parse_eval_seconds']=validation.pop('seconds')
                    result['validation']=validation
                    result['status']=validation['status']
            else:
                result=run_probe([str(exe),case['input']]+verification,'combined')
            row['runs'].append(result)
            if result['status']!='exact':failed=True;break
        ts=[r['seconds'] for r in row['runs'] if 'seconds' in r]
        if ts:row['median_host_seconds']=statistics.median(ts)
        try:
            r=subprocess.run([str(exe),case['input']],capture_output=True,text=True,timeout=30,
                             env=dict(os.environ,KHICAS_TEST_STACK_KIB='64'))
            row['stack_64KiB']={'exit':r.returncode,'result':r.stdout.strip(),'stderr':r.stderr,
                                'same_as_normal_stack':r.stdout.strip()==row['runs'][0].get('result')}
            if r.returncode or not row['stack_64KiB']['same_as_normal_stack']:failed=True
        except subprocess.TimeoutExpired as error:
            row['stack_64KiB']={'status':'integration_timeout',
                                'result':output_text(error.stdout).strip(),'stderr':output_text(error.stderr)};failed=True
        report['cases'].append(row)
        args.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
        print(case['id'],row['runs'][0]['status'],row.get('median_host_seconds'),row['stack_64KiB'].get('exit'),flush=True)
if failed:raise SystemExit(1)
print('PASS: all eight exact; all eight complete with identical results under a 64 KiB stack')
