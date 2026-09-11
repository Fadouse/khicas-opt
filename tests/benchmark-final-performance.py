#!/usr/bin/env python3
"""Alternating fresh-process timings; no extrapolation to calculator speed."""
import argparse,hashlib,json,os,re,statistics,subprocess
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
for name in ['before','after','before-conic','after-conic']:
    p.add_argument('--'+name,type=Path,required=True)
p.add_argument('--report',type=Path,required=True)
p.add_argument('--repetitions',type=int,default=11)
a=p.parse_args();root=Path(__file__).resolve().parents[1];cases=[];rows=[];summary=[]
for n in [1,2]:cases+=json.loads((root/f'tests/mixed-round{n}-questions.json').read_text())['cases']
cases.append(dict(id='trig-rational',type='simplify',input='simplify(3*sin(x)*cos(x)/(sin(x)^3+cos(x)^3))'))
env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None);env.pop('KHICAS_OUTER_SIMPLIFY',None)
paths={'before':a.before,'after':a.after,'before-conic':a.before_conic,'after-conic':a.after_conic}
for c in cases:
    conic=c['type']=='conic';expression=c['input']
    if conic:expression='('+c['equation']+',[x,y],'+c['parameter']+')'
    for n in range(a.repetitions):
        for version in (['before','after'] if n%2==0 else ['after','before']):
            r=subprocess.run([str(paths[version+('-conic' if conic else '')]),expression],capture_output=True,text=True,timeout=12,env=dict(env,KHICAS_OUTER_SIMPLIFY='1') if conic else env)
            timing=re.search(r'^SECONDS ([\d.e+-]+)',r.stderr,re.M);rss=re.search(r'^MAX_RSS_KB (\d+)',r.stderr,re.M)
            assert r.returncode==0 and timing,(c['id'],r.returncode,r.stderr)
            row=dict(id=c['id'],version=version,iteration=n,seconds=float(timing[1]),result_chars=len(r.stdout.strip()),pari_warning_count=r.stderr.count('new PARI stack'))
            if rss:row['rss_kb']=int(rss[1])
            if conic:row['label_nodes']=int(re.search(r'^NODES \d+ (\d+)',r.stdout,re.M)[1])
            rows.append(row)
    medians={v:statistics.median(r['seconds'] for r in rows if r['id']==c['id'] and r['version']==v) for v in ['before','after']}
    summary.append(dict(id=c['id'],before_seconds=medians['before'],after_seconds=medians['after'],speed_ratio=medians['before']/medians['after']))
a.report.write_text(json.dumps({'scope':'Alternating fresh host processes per version/case; parser and evaluation included. All mixed cases accepted on baseline and final cores. Ratios are case-specific host measurements, not CG50 or global speedups. RSS includes host libraries and process inheritance, not calculator heap. The trig-rational timing is a standalone scalar check, not the previously crashing conversion pipeline.','before_commit':'89a17b8','repetitions':a.repetitions,'probe_sha256':{n:hashlib.sha256(q.read_bytes()).hexdigest() for n,q in paths.items()},'source_sha256':{n:hashlib.sha256((root/n).read_bytes()).hexdigest() for n in ['kconvert.cc','ksubst.cc','yderive.cc','dilogarithm.h']},'summary':summary,'runs':rows},indent=2)+'\n')
for row in summary:print(row)
