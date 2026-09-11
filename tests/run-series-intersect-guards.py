#!/usr/bin/env python3
"""Exhaust complete short-list domains for the MRV intersection index contract."""

from repository import source_path
import argparse, hashlib, json, subprocess
from pathlib import Path
from integration_build import ROOT, function, compiler_options

p = argparse.ArgumentParser()
p.add_argument("--build-dir", type=Path, required=True)
p.add_argument("--report", type=Path, required=True)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)
s = (source_path("zseries.cc")).read_text()
body = r"""
#include "giacPCH.h"
#include <iostream>
#include <vector>
namespace giac {
FUNCTION
}
int main(){using namespace giac;
 std::vector<vecteur> lists(1,vecteur());
 for(int n=1;n<=4;++n){int count=1;for(int k=0;k<n;++k)count*=3;
  for(int k=0;k<count;++k){int q=k;vecteur v;for(int j=0;j<n;++j){v.push_back(q%3-1);q/=3;}lists.push_back(v);}}
 unsigned checks=0,failures=0;
 for(const auto &a:lists)for(const auto &b:lists){
  int expected_i=-1,expected_j=-1;
  for(unsigned i=0;i<a.size() && expected_i<0;++i)for(unsigned j=0;j<b.size();++j)
   if(a[i]==b[j]){expected_i=i;expected_j=j;break;}
  int i=-1,j=-1;bool found=intersect(a,b,i,j);++checks;
  if(found!=(expected_i>=0) || (found && (i!=expected_i || j!=expected_j)))++failures;
 }
 std::cout<<checks<<" "<<failures<<"\n";return failures?1:0;
}
""".replace("FUNCTION", function(s, "  bool intersect("))
p = a.build_dir / "probe.cc"
p.write_text(body)
f, l = compiler_options()
exe = a.build_dir / "probe"
subprocess.run(f + [str(p)] + l + ["-o", str(exe)], check=True)
r = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
checks, failures = map(int, r.stdout.split())
a.report.write_text(
    json.dumps(
        dict(
            source="zseries.cc",
            source_sha256=hashlib.sha256(s.encode()).hexdigest(),
            checks=checks,
            failures=failures,
            exit=r.returncode,
            proof="Every ordered pair of lists over {-1,0,1}, lengths 0..4 including duplicates. A match must return the first valid indices in both lists; this is the contract needed before MRV indexes the vectors.",
        ),
        indent=2,
    )
    + "\n"
)
print(checks, failures)
assert r.returncode == 0
