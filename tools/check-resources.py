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
definite_helpers=re.findall(r'zintgab\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
derivative_helpers=re.findall(r'yderive\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
simplify_helpers=re.findall(r'ksubst\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
conditional_helpers=re.findall(r'zprog\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
matrix_helpers=re.findall(r'zvecteur\.o\(\.text\.\*(\w+)\*\)',(d/'prizm.ld').read_text())
symbols=(d/'khicasen.elf.symbols').read_text(); moved={}
conversion_names=['_'+name for name in ('cart2param','cart2polar','param2cart','param2polar','polar2cart','polar2param')] if 'kconvert.o(.text.*)' in (d/'prizm.ld').read_text() else []
conversion_helpers=['curve_conic_param','curve_sign'] if 'static bool curve_conic_param(' in (d/'kconvert.cc').read_text() else []
for name in selectors+definite_helpers+conversion_names+conversion_helpers+derivative_helpers+simplify_helpers+conditional_helpers+matrix_helpers:
    hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::'+name+r'\(',symbols,re.M)
    assert hits, f'Missing helper: {name}'
    for address,section,size in hits:
        assert section=='.rominram' and regions['r8c2'][0]<=int(address,16)<sum(regions['r8c2']), name
    moved[name]=sum(int(size,16) for _,_,size in hits)
# These bounded rules intentionally remain in ROM to balance the two
# fixed code regions. Check the linked result, not just linker selectors.
rom_rules={}
for name,source in [('integrate_parameter_quadratic','yintg.cc'),('simplify_conjugate_roots','ksubst.cc')]:
    if 'static bool '+name+'(' not in (d/source).read_text():continue
    hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::'+name+r'\(',symbols,re.M)
    assert len(hits)==1 and regions['rom'][0]<=int(hits[0][0],16)<sum(regions['rom']),name
    rom_rules[name]={'address':hits[0][0],'code_bytes':int(hits[0][2],16),'source':source}
shared_walkers={}
if 'inline bool logarithmic_span_entire(' in (d/'logarithmic_span.h').read_text():
    hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::logarithmic_span_entire\(',symbols,re.M)
    assert len(hits)==1,'COMDAT must retain exactly one shared proof walker'
    shared_walkers['logarithmic_span_entire']={'copies':len(hits),'address':hits[0][0],'code_bytes':int(hits[0][2],16)}
for name in ('determinant_atoms','determinant_polynomial_bound','determinant_fraction'):
    if not re.search(r'inline (?:bool|vecteur) '+name+r'\(', (d/'determinant_small.h').read_text()):continue
    hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::'+name+r'\(',symbols,re.M)
    assert len(hits)==1 and hits[0][1]=='.rominram','Shared fraction walker must be linked once in AC2: '+name
    shared_walkers[name]={'copies':1,'address':hits[0][0],'code_bytes':int(hits[0][2],16)}
# The calculator links zusual, so host extraction and provenance must use
# this implementation rather than the parallel, unlinked kusual copy.
assert 'zusual.o' in (d/'Makefile').read_text()
linked_usual={}
for name in ('sqrt','acos','_abs'):
 hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::'+name+r'\(giac::gen const&',symbols,re.M)
 assert len(hits)==1 and hits[0][1]=='.rominram',name
 linked_usual[name]={'address':hits[0][0],'code_bytes':int(hits[0][2],16),'source':'zusual.cc'}
assert 'zvecteur.o' in (d/'Makefile').read_text()
det_hits=re.findall(r'^([0-9a-f]+)\s+.*?\bF\s+(\S+)\s+([0-9a-f]+)\s+giac::_det\(giac::gen const&',symbols,re.M)
assert len(det_hits)==1, 'The actual determinant entry must be linked once'
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
        'rom_rule_helpers':rom_rules,
        'shared_proof_walkers':shared_walkers,
        'linked_usual_functions':linked_usual,
        'linked_determinant_entry':{'address':det_hits[0][0],'section':det_hits[0][1],'code_bytes':int(det_hits[0][2],16),'source':'zvecteur.cc'},
        'moved_helpers':{name:moved[name] for name in selectors},
        'moved_conversion_entries':{name:moved[name] for name in conversion_names},
        'moved_conversion_helpers':{name:moved[name] for name in conversion_helpers},
        'moved_matrix_helpers':{name:moved[name] for name in matrix_helpers},
        'matrix_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in matrix_helpers)],
        'moved_definite_helpers':{name:moved[name] for name in definite_helpers},
        'definite_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in definite_helpers)],
        'moved_derivative_helpers':{name:moved[name] for name in derivative_helpers},
        'conversion_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in conversion_helpers)],
        'moved_conditional_helpers':{name:moved[name] for name in conditional_helpers},
        'moved_simplify_helpers':{name:moved[name] for name in simplify_helpers},
        'derivative_frames':[r for r in frames if 'derive' in r['function']],
        'checkpoint21_frames':[r for r in frames if any(name+'(' in r['function'] for name in ('_integrate_','intgab','intgab_r','integrate_affine_trig_square','integrate_affine_trig_square_interval','integrate_quadratic_affine_root','integrate_elliptic_quartic','elliptic_first_rf','_EllipticF','derive_squared_affine_radical','derive_minmax_contact','simplify_minmax_clamp','curve_quadratic_image'))],
        'largest_single_frames':sorted(frames,key=lambda v:v['bytes'],reverse=True)[:30],
        'integration_helper_frames':[r for r in frames if any(name+'(' in r['function'] for name in selectors)],
        'sha256':{name:hashlib.sha256((d/name).read_bytes()).hexdigest() for name in ('khicas50.g3a','khicas50.ac2','khicasen.elf','prizm.ld','main.cc','kglobal.cc','static_lexer_.h','static_lexer.h','static_extern.h','usual.h','dilogarithm.h','yderive.cc','zmaple.cc','yintg.cc','zintgab.cc','ksubst.cc','equation_normalize.h','parametric_display.h','kconvert.cc','zprog.cc','kusual.cc','zusual.cc','conditional_eval.h','input_lexer.cc','input_lexer.ll','zvecteur.cc','determinant_small.h','logarithmic_span.h','elliptic_first.h','ksymbolic.cc') if (d/name).exists()}}
a.report.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
for name,row in report['regions'].items():
    print(f"{name}: {row['used_bytes']} / {row['capacity_bytes']} bytes; {row['remaining_bytes']} free")
print(f'PASS: {len(selectors)} integration helpers and {len(conversion_names)} conversion entries placed in AC2; CAS heap configuration unchanged')
