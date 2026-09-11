#!/usr/bin/env python3
"""Validate target time(expr,\"resources\") with deterministic clock/arena stubs.

This checks dispatch/evaluation semantics, not real CG50 timing or memory.
"""

from repository import source_path
import os, shlex, subprocess, tempfile
from pathlib import Path
from integration_build import ROOT, function

source = r"""
#include "giacPCH.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
static unsigned ticks=100, snapshots=0;
static bool available=true;
int RTC_GetTicks(){unsigned t=ticks;ticks+=32;return t;}
bool get_cas_memory_stats(unsigned *s){
  if (!available) return false;
  unsigned v[]={1000,400,450,2,3};
  if (snapshots++){v[0]=800;v[1]=600;v[2]=650;v[3]=3;v[4]=5;}
  for (unsigned i=0;i<5;++i) s[i]=v[i];
  return true;
}
extern "C" char *c_load_script(const char *filename){
  std::string source;
  if (std::string(filename)=="side-effect") source="n:=n+1";
  else if (std::string(filename)=="oversized") source=std::string(6144,'x');
  else {
    std::ifstream file(filename);if(!file)return 0;
    source.assign(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
  }
  char *data=static_cast<char*>(malloc(source.size()+1));
  if(data)std::memcpy(data,source.c_str(),source.size()+1);return data;
}
namespace giac {
static int clock_h=-1,clock_m=-1;
void set_time(int h,int m){clock_h=h;clock_m=m;}
"""
source += (
    "#define FXCG\n"
    + function((source_path("zprog.cc")).read_text(), "  gen _read(")
    + "#undef FXCG\n"
)
source += function((source_path("zmaple.cc")).read_text(), "  gen _time(")
source += r"""
}
int main(int argc,char **argv){
  using namespace giac;
  assert(argc==4);
  context ctx; const context *contextptr=&ctx;
  gen("n:=0",contextptr).eval(1,contextptr);
  gen expression("n:=n+1",contextptr);
  gen args(makevecteur(expression,string2gen("resources",false)),_SEQ__VECT);
  gen r=_time(args,contextptr);
  assert(r.type==_VECT && r._VECTptr->size()==7);
  const vecteur &v=*r._VECTptr;
  assert(v[0]==1 && v[1]==gen(0.25) && v[2]==1000 && v[3]==800);
  assert(v[4]==650 && v[5]==1 && v[6]==2);
  assert(gen("n",contextptr).eval(1,contextptr)==1);
  assert(snapshots==2);
  available=false;
  bool failed=false;
  try { failed=is_undef(_time(args,contextptr)); } catch (...) { failed=true; }
  assert(failed && gen("n",contextptr).eval(1,contextptr)==1);
  available=true;
  assert(_time(gen(makevecteur(-1,65),_SEQ__VECT),contextptr)==1);
  assert(clock_h==23 && clock_m==5);
  assert(_time(gen(vecteur(),_SEQ__VECT),contextptr).type==_INT_);
  assert(_time(expression,contextptr)==gen(0.25));
  assert(gen("n",contextptr).eval(1,contextptr)==2);
  assert(_read(string2gen("side-effect",false),contextptr)==3);
  assert(gen("n",contextptr).eval(1,contextptr)==3);
  assert(is_undef(_read(string2gen("/missing-khicas-script",false),contextptr)));
  for(gen arg:makevecteur(17,string2gen("",false),string2gen("oversized",false))){
    bool rejected=false;try{rejected=is_undef(_read(arg,contextptr));}catch(...){rejected=true;}
    assert(rejected);
  }
  gen benchmark=_read(string2gen(argv[1],false),contextptr);
  assert(!is_undef(benchmark));
  gen bresult=gen("cgtest(0)",contextptr).eval(1,contextptr);
  assert(bresult==string2gen("Choose a case from 1 to 15",false));
  assert(!is_undef(_read(string2gen(argv[2],false),contextptr)));
  assert(gen("ct(0)",contextptr).eval(1,contextptr)==string2gen("ct(1) to ct(5)",false));
  assert(!is_undef(_read(string2gen(argv[3],false),contextptr)));
  assert(gen("cv(0)",contextptr).eval(1,contextptr)==string2gen("cv(1) to cv(5)",false));
  std::cout<<"PASS: target read branch and benchmark parsing, single evaluation, resource fields, unavailable stats, legacy time modes\n";
}
"""
with tempfile.TemporaryDirectory(prefix="khicas-resource-time-") as tmp:
    p = Path(tmp)
    (p / "test.cc").write_text(source)
    flags = [
        os.environ.get("CXX", "c++"),
        "-std=c++11",
        "-O2",
        "-DHAVE_CONFIG_H",
        "-DGIAC_GENERIC_CONSTANTS",
        "-Wno-deprecated-declarations",
        "-I",
        os.environ.get("GIAC_INCLUDE", "/usr/include/giac"),
    ] + shlex.split(os.environ.get("CXXFLAGS", ""))
    libs = shlex.split(os.environ.get("LDFLAGS", "")) + ["-lgiac"]
    subprocess.run(
        flags + [str(p / "test.cc")] + libs + ["-o", str(p / "test")], check=True
    )
    subprocess.run(
        [
            str(p / "test"),
            str(ROOT / "tests/device/CG50TEST.xw"),
            str(ROOT / "tests/device/CG50USER.xw"),
            str(ROOT / "tests/device/CG50CURV.xw"),
        ],
        check=True,
        timeout=30,
    )
