#!/usr/bin/env python3
"""Exact compact trig-power primitives, including a guarded 64 KiB stack."""
import argparse,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,function,compiler_options,special_source
p=argparse.ArgumentParser();p.add_argument('--helpers',type=Path);p.add_argument('--simplify-source',type=Path,help='Also test the actual candidate FXCG simplifier');args=p.parse_args()
s=(ROOT/'yintg.cc').read_text()
text='#include "giacPCH.h"\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
for sig in ('  void decompose_prod(', '  gen extract_cst(', '  static bool integration_rational(', '  static gen integration_syntax(', '  static gen integration_coefficient('):text+=function(s,sig)
text+=args.helpers.read_text() if args.helpers else function(s,'  static bool integrate_high_frequency_trig(')
text+='''bool high_frequency_rule(const gen &input,const gen &x,gen &r,GIAC_CONTEXT){
 if(taille(input,129)>128)return false;
 gen e=integration_syntax(input,contextptr),c=integration_coefficient(e,x,contextptr);
 if(is_undef(c) || is_inf(c))return false;
 if(!integrate_high_frequency_trig(e,x,r,contextptr))return false;
 r=c*r;return true;
}\n}\n'''
with tempfile.TemporaryDirectory(prefix='khicas-high-frequency-') as tmp:
 d=Path(tmp);(d/'rule.cc').write_text(text);flags,libs=compiler_options()
 subprocess.run(flags+[str(d/'rule.cc'),str(ROOT/'tests/high_frequency_trig_integrals.cc')]+libs+['-pthread','-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,timeout=60)

if args.simplify_source:
 s=args.simplify_source.read_text()
 text='#include "giacPCH.h"\n#include "equation_normalize.h"\n#define FXCG\n#define NO_STDEXCEPT\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
 text+='gen ataninv2atan(const gen &,GIAC_CONTEXT);\ngen cklin(const gen &,GIAC_CONTEXT);\n'
 text+=function((ROOT/'zprog.cc').read_text(),'  gen symb_prog3(')
 for sig in ('  gen tsimplify_noexpln(', '  static bool simplify_preflight(', '  static gen simplify_shallow_leaf(', '  static unsigned simplify_special_terms(', '  static gen simplify_special_core(', '  gen simplify(const gen & e_orig,GIAC_CONTEXT)', '  gen _simplify('):text+=function(s,sig)
 text+='}\n'
 with tempfile.TemporaryDirectory(prefix='khicas-high-frequency-simplify-') as tmp:
  d=Path(tmp);(d/'simplify.cc').write_text(text);(d/'equation_normalize.h').write_bytes((ROOT/'equation_normalize.h').read_bytes())
  flags,libs=compiler_options()
  subprocess.run(flags+['-I',str(d),str(special_source(d)),str(d/'simplify.cc'),str(ROOT/'tests/high_frequency_trig_simplify.cc')]+libs+['-pthread','-o',str(d/'test')],check=True)
  subprocess.run([str(d/'test')],check=True,timeout=60)
