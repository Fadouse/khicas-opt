#!/usr/bin/env python3
"""Build curve conversion and labels with actual FXCG simplification."""
import argparse,subprocess
from pathlib import Path
from integration_build import ROOT,compiler_options,build
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--simplify-build-dir',type=Path);p.add_argument('--probe-source',type=Path,default=ROOT/'tests/conic_probe.cc');a=p.parse_args();d=a.build_dir;d.mkdir(parents=True,exist_ok=True)
for name in ['equation_normalize.h','parametric_display.h']:(d/name).write_bytes((ROOT/name).read_bytes())
# Older host usual.h lacks the new externs. The display header supplies the
# same external pointer linkage declared in the production usual.h.
(d/'kconvert.cc').write_text((ROOT/'kconvert.cc').read_text().replace('#include "giacPCH.h"','#include "giacPCH.h"\n#include "parametric_display.h"',1))
simplify_dir=a.simplify_build_dir or d/'simplify'
if not a.simplify_build_dir:
    # Include the actual derivative, usual-function and determinant entries.
    # A fresh default build must not silently substitute the host versions.
    build(simplify_dir,target_simplify=True,target_derive=True)
flags,libs=compiler_options();flags+=['-I'+str(d)]
# The current simplifier refers to the registered Li2 function as an atom.
# Use the exact production registration rather than a dummy host placeholder.
(d/'dilogarithm.h').write_bytes((ROOT/'dilogarithm.h').read_bytes())
(d/'special.cc').write_text('#include "giacPCH.h"\n#include "dilogarithm.h"\n')
extra=[str(simplify_dir/name) for name in ('derivative.cc','matrix.cc') if (simplify_dir/name).exists()]
subprocess.run(flags+[str(d/'kconvert.cc'),str(a.probe_source),str(simplify_dir/'simplify.cc'),str(simplify_dir/'normalize.cc'),str(d/'special.cc')]+extra+libs+['-pthread','-o',str(d/'probe')],check=True)
print(d/'probe')
