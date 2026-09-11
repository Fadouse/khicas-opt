#!/usr/bin/env python3
"""Bounded closed trig-constant algebra and real/complex branch regressions."""
import argparse,json,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,function,compiler_options,special_source
p=argparse.ArgumentParser();p.add_argument('--source',type=Path);args=p.parse_args()
s=(args.source or ROOT/'ksubst.cc').read_text()
text='#include "giacPCH.h"\n#include "equation_normalize.h"\n#define FXCG\n#define NO_STDEXCEPT\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
text+='unsigned closed_trig_mask_calls=0;\ngen ataninv2atan(const gen &,GIAC_CONTEXT);\ngen cklin(const gen &,GIAC_CONTEXT);\n'
text+=function((ROOT/'zprog.cc').read_text(),'  gen symb_prog3(')
for sig in ('  gen tsimplify_noexpln(', '  static bool simplify_preflight(', '  static gen simplify_shallow_leaf(', '  static unsigned simplify_special_terms(', '  static gen simplify_special_core(', '  gen simplify(const gen & e_orig,GIAC_CONTEXT)', '  gen _simplify('):
 f=function(s,sig)
 if sig.startswith('  gen simplify('):
  marker='gen masked=quotesubst(e_orig,closed,names,contextptr);'
  assert marker in f,'closed trig-constant guard missing from this source'
  f=f.replace(marker,'++closed_trig_mask_calls;'+marker,1)
 text+=f
text+='gen closed_trig_baseline(const gen &g,GIAC_CONTEXT){return simplify_special_core(g,contextptr); }\n}\n'
norm='#include "giacPCH.h"\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n';s=(ROOT/'ysym2poly.cc').read_text()
for sig in ('  static bool sort_func(', '  static vecteur sort1(', '  gen ratnormal(const gen & e,GIAC_CONTEXT)', '  gen recursive_ratnormal(const gen & e,GIAC_CONTEXT)'):norm+=function(s,sig)
norm+='}\n'
inputs=[c['runs'][0]['stdout'] for c in json.loads((ROOT/'docs/benchmarks/cycle6-mellin-extremes-2026a.json').read_text())['cases']]
with tempfile.TemporaryDirectory(prefix='khicas-closed-trig-') as tmp:
 d=Path(tmp);(d/'simplify.cc').write_text(text);(d/'normal.cc').write_text(norm);(d/'equation_normalize.h').write_bytes((ROOT/'equation_normalize.h').read_bytes());flags,libs=compiler_options()
 subprocess.run(flags+['-I',str(d),str(special_source(d)),str(d/'simplify.cc'),str(d/'normal.cc'),str(ROOT/'tests/closed_trig_constants.cc')]+libs+['-pthread','-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')]+inputs,check=True,timeout=60)
