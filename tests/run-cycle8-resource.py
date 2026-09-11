#!/usr/bin/env python3
"""Bounded polynomial pullback / exponential Beta rules, host normal + 64 KiB."""
import argparse,subprocess,tempfile
from pathlib import Path
from integration_build import ROOT,function,compiler_options,special_source
p=argparse.ArgumentParser();p.add_argument('--helpers',type=Path);p.add_argument('--simplify-source',type=Path);a=p.parse_args()
s=(ROOT/'yintg.cc').read_text();t='#include "giacPCH.h"\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
for sig in ('  void decompose_prod(', '  gen extract_cst(', '  static bool integration_rational(', '  static gen integration_syntax(', '  static gen integration_coefficient(', '  static bool small_polynomial(', '  static bool small_sparse_polynomial(', '  static bool integration_one_plus(', '  static bool integration_outer_power('):t+=function(s,sig)
helpers=a.helpers.read_text() if a.helpers else s
for sig in ('  static bool integration_resource_rational(', '  static bool integrate_composed_binomial(', '  static bool integrate_exponential_beta('):t+=function(helpers,sig)
t+='''bool resource_rule(const gen &input,const gen &x,gen &r,bool definite,GIAC_CONTEXT){
 if(taille(input,129)>128)return false;gen e=integration_syntax(input,contextptr),c=integration_syntax(integration_coefficient(e,x,contextptr),contextptr);
 if(!integration_rational(c))return false;
 bool ok=definite?integrate_exponential_beta(e,x,0,plus_inf,r,contextptr):integrate_composed_binomial(e,x,r,contextptr);
 if(ok)r=c*r;return ok;
}\n}\n'''
flags,libs=compiler_options()
with tempfile.TemporaryDirectory(prefix='khicas-c8-resource-') as tmp:
 d=Path(tmp);(d/'rule.cc').write_text(t)
 subprocess.run(flags+[str(d/'rule.cc'),str(ROOT/'tests/cycle8_resource_integrals.cc')]+libs+['-pthread','-o',str(d/'test')],check=True)
 subprocess.run([str(d/'test')],check=True,timeout=60)
 if a.simplify_source:
  s=a.simplify_source.read_text();t='#include "giacPCH.h"\n#include "equation_normalize.h"\n#define FXCG\n#define NO_STDEXCEPT\nnamespace giac {\nextern const unary_function_ptr * const at_Li2;\n'
  t+='gen ataninv2atan(const gen &,GIAC_CONTEXT);\ngen cklin(const gen &,GIAC_CONTEXT);\n'+function((ROOT/'zprog.cc').read_text(),'  gen symb_prog3(')
  for sig in ('  gen tsimplify_noexpln(', '  static bool simplify_preflight(', '  static gen simplify_shallow_leaf(', '  static unsigned simplify_special_terms(', '  static gen simplify_special_core(', '  gen simplify(const gen & e_orig,GIAC_CONTEXT)', '  gen _simplify('):t+=function(s,sig)
  (d/'simplify.cc').write_text(t+'}\n');(d/'equation_normalize.h').write_bytes((ROOT/'equation_normalize.h').read_bytes())
  subprocess.run(flags+['-I',str(d),str(special_source(d)),str(d/'simplify.cc'),str(ROOT/'tests/cycle8_resource_simplify.cc')]+libs+['-pthread','-o',str(d/'simplify-test')],check=True)
  subprocess.run([str(d/'simplify-test')],check=True,timeout=60)
