#!/usr/bin/env python3
"""Bounded MIT / Princeton calculus benchmark: actual old/new repository integrator.

An unresolved result/timeout is recorded, not counted as a correct answer.
Use --strict to fail unless all selected problems have exact validation.
Host RSS includes the host library and is not calculator RAM usage.
"""
import argparse, hashlib, json, re, subprocess, tempfile, time
from collections import Counter
from pathlib import Path
from integration_build import ROOT, BASE, build, build_validation_probe

def output_text(value):
    return value.decode(errors='replace') if isinstance(value, bytes) else value or ''

def checked_status(process, computed=False):
    checks = re.findall(r'^CHECK (\w+)', process.stderr, re.M)
    status = checks[-1] if checks else {2:'unevaluated', 3:'undefined'}.get(
        process.returncode, 'computed' if computed and process.returncode==0 else 'error')
    return 'error' if status=='exact' and process.returncode else status

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compare', action='store_true')
parser.add_argument('--baseline-ref', default=BASE, help='Git reference used with --compare')
parser.add_argument('--corpus', type=Path, default=ROOT/'tests/calculus-corpus.json',
                    help='Input corpus JSON (sources and cases)')
parser.add_argument('--timeout', type=float, default=10)
parser.add_argument('--report', type=Path, required=True)
parser.add_argument('--strict', action='store_true')
parser.add_argument('--require-solved', action='store_true',
                    help='Fail on unresolved/error results; retain exact versus sampled validation labels')
parser.add_argument('--target-simplify', action='store_true', help='Include repository FXCG simplification entry points')
parser.add_argument('--only', help='Comma-separated problem IDs')
args = parser.parse_args()
data = json.loads(args.corpus.read_text())
cases = data['cases']
if args.only:
    selected = args.only.split(',')
    cases = [c for c in cases if c['id'] in selected]
    assert len(cases) == len(selected), 'Unknown or duplicate problem ID'
report = {'baseline': args.baseline_ref, 'timeout_seconds': args.timeout,
          'scope': 'actual yintg, zintgab and normalization, host Giac dependencies; not CG50 timings',
          'validation_probe_sha256':hashlib.sha256((ROOT/'tests/integration_probe.cc').read_bytes()).hexdigest(),
          'source_sha256': {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
                            for name in ('yintg.cc','zintgab.cc','ysym2poly.cc','integration_guard.h','dilogarithm.h')},
          'corpus_file': str(args.corpus),
          'corpus_sha256': hashlib.sha256(args.corpus.read_bytes()).hexdigest(),
          'sources': data['sources'], 'runs': {},
          'validation_scope': 'same probe process; host Giac outer simplify and repository normalization'}
if args.target_simplify:
    report['scope']='actual yintg, zintgab, normalization and FXCG simplify entry points; other dependencies host Giac; not CG50 timings'
    report['source_sha256']['ksubst.cc']=hashlib.sha256((ROOT/'ksubst.cc').read_bytes()).hexdigest()
    if (ROOT/'equation_normalize.h').exists():report['source_sha256']['equation_normalize.h']=hashlib.sha256((ROOT/'equation_normalize.h').read_bytes()).hexdigest()
    report['validation_scope']='separate host Giac probe validates the printed target result; no repository integration, normalization or FXCG simplify linked'
    report['validation_timeout_seconds']=args.timeout
with tempfile.TemporaryDirectory(prefix='khicas-calculus-') as tmp:
    validator = build_validation_probe(Path(tmp)/'validation') if args.target_simplify else None
    for ref in (('baseline', 'current') if args.compare else ('current',)):
        exe = build(Path(tmp)/ref, 'current' if ref=='current' else args.baseline_ref, target_simplify=args.target_simplify)
        rows = []
        for case in cases:
            tail = ','.join(['x'] + case.get('bounds', []))
            expr = f"integrate({case['f']},{tail})"
            mode = 'definite' if 'bounds' in case else 'indefinite'
            verification = [case['expected'], mode, case['f']]
            verification += case.get('samples', ['-2', '-1', '1/3', '1', '2'])
            command = [str(exe), expr] + ([] if validator else verification)
            start = time.monotonic()
            try:
                p = subprocess.run(command, capture_output=True, text=True, timeout=args.timeout)
                integration_wall = time.monotonic()-start
                status = checked_status(p, computed=bool(validator))
                row = {'id':case['id'], 'status':status, 'exit':p.returncode,
                       'result':p.stdout.strip()[:4096], 'result_characters':len(p.stdout.strip()),
                       'result_sha256':hashlib.sha256(p.stdout.strip().encode()).hexdigest(),
                       'stderr':p.stderr[-2500:]}
                if validator:
                    row['integration_status']=status
                    row['integration_wall_seconds']=integration_wall
                for label, key, convert in [('SECONDS', 'integration_seconds', float),
                                           ('PARSER_CALLS', 'parser_calls', int),
                                           ('MAX_RSS_KB', 'host_max_rss_kb', int)]:
                    match = re.search(r'^'+label+r' ([\d.e+-]+)', p.stderr, re.M)
                    if match: row[key] = convert(match[1])
                if validator and status=='computed':
                    validation_start=time.monotonic()
                    try:
                        validated=subprocess.run([str(validator),p.stdout.strip()]+verification,
                                                 capture_output=True,text=True,timeout=args.timeout)
                        validation={'exit':validated.returncode,'status':checked_status(validated),
                                    'result':validated.stdout.strip()[:4096],
                                    'stderr':validated.stderr[-2500:]}
                        measured=re.search(r'^SECONDS ([\d.e+-]+)',validated.stderr,re.M)
                        if measured:validation['result_parse_eval_seconds']=float(measured[1])
                    except subprocess.TimeoutExpired as error:
                        validation={'status':'validation_timeout',
                                    'result':output_text(error.stdout)[:2500],
                                    'stderr':output_text(error.stderr)[-2500:]}
                    validation['wall_seconds']=time.monotonic()-validation_start
                    row['validation']=validation
                    row['status']=validation['status']
            except subprocess.TimeoutExpired as error:
                stderr=output_text(error.stderr)
                status='validation_timeout' if not validator and re.search(r'^SECONDS ',stderr,re.M) else 'integration_timeout'
                row = {'id':case['id'], 'status':status,
                       'result':output_text(error.stdout)[:2500], 'stderr':stderr[-2500:]}
                if validator:
                    row['integration_status']=status
                    row['integration_wall_seconds']=time.monotonic()-start
            row['wall_seconds'] = time.monotonic()-start
            rows.append(row)
            print(ref, row['id'], row['status'], f"{row['wall_seconds']:.3f}s", flush=True)
            report['runs'][ref] = rows
            args.report.write_text(json.dumps(report, indent=2, ensure_ascii=False)+'\n')
        print(ref, dict(Counter(row['status'] for row in rows)), flush=True)
if args.strict and any(row['status'] != 'exact' for row in report['runs']['current']):
    raise SystemExit(1)
if args.require_solved and any(row['status'] not in ('exact', 'sampled', 'numeric_constant')
                               for row in report['runs']['current']):
    raise SystemExit(1)
