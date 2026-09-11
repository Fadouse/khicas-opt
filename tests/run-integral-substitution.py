#!/usr/bin/env python3
"""Exercise actual symbolic-integral substitution with checked vector access.

Three arguments denote a primitive evaluated at one endpoint; only the
four-argument form has two bounds that can be compared for equality.
"""

from repository import source_path
import os
import shlex
import subprocess
import tempfile
from pathlib import Path
from integration_build import ROOT, function

source = r"""
#include "giacPCH.h"
#include <cassert>
#include <iostream>
namespace giac {
static gen &checked_index(vecteur &v,unsigned i){assert(i<v.size());return v[i];}
"""
# Giac uses its own vector wrapper, so libstdc++ assertions do not cover
# its indexing. Instrument the fourth-element accesses without changing
# the branch conditions or substitution logic.
source += function(
    (source_path("ksubst.cc")).read_text(), "  static gen subst_integrate("
).replace("v[3]", "checked_index(v,3)")
source += r"""
}
int main(){
  using namespace giac;
  context ctx; const context *contextptr=&ctx;
  gen x(identificateur("x")), a(identificateur("a"));
  // Substitute a parameter, so the integration variable remains unchanged.
  gen integrand=symbolic(at_ln,symbolic(at_sin,x))+a;
  for (int count=2;count<=4;++count){
    vecteur args=makevecteur(integrand,x);
    if(count>=3)args.push_back(1);
    if(count==4)args.push_back(2);
    gen input=symbolic(at_integrate,gen(args,_SEQ__VECT));
    gen output=subst_integrate(input,a,7,true,1,contextptr);
    assert(output.is_symb_of_sommet(at_integrate));
    assert(output._SYMBptr->feuille.type==_VECT);
    const vecteur &result=*output._SYMBptr->feuille._VECTptr;
    assert(result.size()==unsigned(count));
    assert(result[0]==symbolic(at_ln,symbolic(at_sin,x))+7);
    assert(result[1]==x);
    if(count>=3)assert(result[2]==1);
    if(count==4)assert(result[3]==2);
  }
  gen equal=symbolic(at_integrate,gen(makevecteur(integrand,x,1,a),_SEQ__VECT));
  assert(subst_integrate(equal,a,1,true,1,contextptr)==0);
  std::cout<<"PASS: symbolic integral substitution preserves 2/3/4 argument forms and collapses equal definite bounds\n";
}
"""
with tempfile.TemporaryDirectory(prefix="khicas-integral-substitution-") as tmp:
    path = Path(tmp)
    (path / "test.cc").write_text(source)
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O1",
        "-g",
        "-D_GLIBCXX_ASSERTIONS",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ]
    flags += shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    subprocess.run(
        flags + [str(path / "test.cc")] + libs + ["-o", str(path / "test")], check=True
    )
    subprocess.run([str(path / "test")], check=True, timeout=30)
