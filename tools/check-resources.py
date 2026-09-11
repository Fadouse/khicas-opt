#!/usr/bin/env python3
"""Check actual CG50 build regions and report compiler stack frames (not runtime peaks)."""
import argparse, hashlib, json, re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir',type=Path,default=ROOT/'.build/optimized')
p.add_argument('--report',type=Path,required=True)
a=p.parse_args(); d=a.build_dir
mapping=(d/'khicasen.map').read_text()
regions={name:(int(origin,16),int(size,16)) for name,origin,size in
         re.findall(r'^(rom|ram|r8c2)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)',mapping,re.M)}
sections={name:(int(origin,16),int(size,16)) for name,origin,size in
          re.findall(r'^(\.\w+)\s+(0x[0-9a-f]+)\s+(0x[0-9a-f]+)',mapping,re.M)}
used={'rom':(d/'khicasen.bin').stat().st_size,
      'r8c2':(d/'khicas50.ac2').stat().st_size,
      'ram':sum(sections['.bss'])-regions['ram'][0]}
assert used['r8c2']==sections['.rominram'][1], 'AC2 binary/map mismatch'
assert sections['.rominram'][0]==regions['r8c2'][0]
for name in regions:
    assert 0<used[name]<=regions[name][1], f'{name} exceeds linked region'
# Verify every selected helper actually moved, including compiler clones.
selectors=re.findall(r'yintg\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
derivative_helpers=re.findall(r'yderive\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
symbols=(d/'khicasen.elf.symbols').read_text(); moved={}
conversion_names=['_'+name for name in ('cart2param','cart2polar','param2cart','param2polar','polar2cart','polar2param')] if 'kconvert.o(.text.*)' in (d/'prizm.ld').read_text() else []
conversion_helpers=['curve_conic_param','curve_sign'] if 'static bool curve_conic_param(' in (d/'kconvert.cc').read_text() else []
for name in selectors+conversion_names+conversion_helpers+derivative_helpers:
    hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::'+name+r'\(',symbols,re.M)
    assert hits, f'Missing helper: {name}'
    for address,section,size in hits:
        assert section=='.rominram' and regions['r8c2'][0]<=int(address,16)<sum(regions['r8c2']), name
    moved[name]=sum(int(size,16) for _,_,size in hits)
frames=[]
for path in d.glob('*.su'):
    for line in path.read_text().splitlines():
        parts=line.rsplit('\t',2)
        if len(parts)==3 and parts[1].isdigit():
            frames.append({'function':parts[0],'bytes':int(parts[1]),'kind':parts[2]})
heap_sizes={int(v,16) for v in re.findall(r'ram3M.end=ram3M.start\+(0x[0-9a-f]+);',(d/'main.cc').read_text())}
assert heap_sizes=={0x180000}, 'Review CAS heap change separately'
report={'scope':'SH4 linker/binary capacity and single compiler stack frames; no CG50 runtime measurements',
        'regions':{name:{'origin':hex(origin),'used_bytes':used[name],'capacity_bytes':size,'remaining_bytes':size-used[name]} for name,(origin,size) in regions.items()},
        'configured_CAS_heap_bytes':0x180000,
        'moved_helpers':{name:moved[name] for name in selectors},
        'moved_conversion_entries':{name:moved[name] for name in conversion_names},
        'moved_conversion_helpers':{name:moved[name] for name in conversion_helpers},
        'moved_derivative_helpers':{name:moved[name] for name in derivative_helpers},
        'conversion_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in conversion_helpers)],
        'derivative_frames':[r for r in frames if 'derive' in r['function']],
        'largest_single_frames':sorted(frames,key=lambda v:v['bytes'],reverse=True)[:30],
        'integration_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in selectors)],
        'sha256':{name:hashlib.sha256((d/name).read_bytes()).hexdigest() for name in ('khicas50.g3a','khicas50.ac2','khicasen.elf','prizm.ld','main.cc','kglobal.cc','static_lexer_.h','static_lexer.h','static_extern.h','usual.h','dilogarithm.h','yderive.cc','zmaple.cc','yintg.cc','ksubst.cc','equation_normalize.h','parametric_display.h','kconvert.cc','zprog.cc','input_lexer.cc','input_lexer.ll')}}
a.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
for name,row in report['regions'].items():
    print(f"{name}: {row['used_bytes']} / {row['capacity_bytes']} bytes; {row['remaining_bytes']} free")
print(f'PASS: {len(selectors)} integration helpers and {len(conversion_names)} conversion entries placed in AC2; CAS heap configuration unchanged')
