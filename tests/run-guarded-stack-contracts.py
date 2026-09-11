#!/usr/bin/env python3
"""Verify actual stack overflow is still fatal and ordinary teardown runs."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
from integration_build import compiler_options,ROOT
p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args();a.build_dir.mkdir(parents=True,exist_ok=True)
src=a.build_dir/'probe.cc'
src.write_text(r'''
#include "guarded_probe_stack.h"
#include <cstdio>
struct Marker {const char *label; ~Marker(){std::puts(label);}};
static Marker global={"GLOBAL_DESTRUCTOR"};
__attribute__((noinline)) static int recurse(int n){
 volatile char buffer[8192];for(int i=0;i<8192;++i)buffer[i]=char(n);
 int result=n?recurse(n-1):0;
 return result+buffer[n%8192];
}
static int compute(int argc,char **argv){Marker local={"LOCAL_DESTRUCTOR"};recurse(std::atoi(argv[1]));return 0;}
int main(int argc,char **argv){return guarded_probe::run(argc,argv,compute);}
''')
flags,libs=compiler_options();exe=a.build_dir/'probe'
subprocess.run(flags+['-O0','-I'+str(ROOT/'tests'),str(src),'-o',str(exe)],check=True)
rows=[]
for stack,depth,expected in [(None,64,0),('64',2,0),('64',64,-11),('128',2,0),('15',2,7),('bad',2,7)]:
 env=dict(os.environ);env.pop('KHICAS_TEST_STACK_KIB',None)
 if stack is not None:env['KHICAS_TEST_STACK_KIB']=stack
 r=subprocess.run([str(exe),str(depth)],capture_output=True,text=True,env=env,timeout=5)
 ok=r.returncode==expected
 if expected==0:ok=ok and r.stdout.splitlines()==['LOCAL_DESTRUCTOR','GLOBAL_DESTRUCTOR']
 rows.append(dict(stack=stack,depth=depth,expected_exit=expected,exit=r.returncode,result=r.stdout,passed=ok))
a.report.write_text(json.dumps(dict(scope='Host stack harness only: guarded overflow, normal local/global destructors and invalid input. SIGSEGV is required for deliberate overflow, never counted as successful computation.',source_sha256=hashlib.sha256((ROOT/'tests/guarded_probe_stack.h').read_bytes()).hexdigest(),runs=rows),indent=2)+'\n')
print(len(rows),sum(r['passed'] for r in rows));assert all(r['passed'] for r in rows),rows
