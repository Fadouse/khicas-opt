#!/usr/bin/env python3
"""Build actual integration, yderive, FXCG simplify, and conversion probes."""
import argparse,subprocess,sys
from pathlib import Path
from integration_build import ROOT,build
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir',type=Path,required=True)
a=p.parse_args();directory=a.build_dir.resolve();directory.mkdir(parents=True,exist_ok=True)
probe=build(directory/'simplify',target_simplify=True,target_derive=True)
subprocess.run([sys.executable,str(ROOT/'tests/build-conic-probe.py'),'--build-dir',str(directory),'--simplify-build-dir',str(directory/'simplify')],check=True)
print('Integral/derivative probe:',probe)
print('Conversion/display probe:',directory/'probe')
