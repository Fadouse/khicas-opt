"""Build repository integration/normalization against the host Giac ABI."""
import os, shlex, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = 'checkpoint/equations'

def source(ref, name):
    if ref == 'current':
        return (ROOT / name).read_text()
    git_ref = BASE if ref == 'baseline' else ref
    return subprocess.check_output(['git', 'show', f'{git_ref}:{name}'], cwd=ROOT, text=True)

def function(s, signature):
    start = s.index(signature)
    end = s.index('{', start) + 1
    depth = 1
    while depth:
        depth += (s[end] == '{') - (s[end] == '}')
        end += 1
    return s[start:end] + '\n'

def compiler_options():
    flags = [os.environ.get('CXX', 'c++'), '-std=c++11', '-O1', '-g',
             '-DHAVE_CONFIG_H', '-DGIAC_GENERIC_CONSTANTS', '-Wno-deprecated-declarations',
             '-I', os.environ.get('GIAC_INCLUDE', '/usr/include/giac')]
    flags += shlex.split(os.environ.get('CXXFLAGS', ''))
    libs = shlex.split(os.environ.get('LDFLAGS', '')) + ['-lgiac']
    libs += shlex.split(os.environ.get('GIAC_NUMERIC_LIBS', '-lgmp -lmpfr'))
    return flags, libs

def build_validation_probe(directory):
    """Validate a printed result using only host Giac mathematical routines.

    The instrumentation source is shared, but no repository integration,
    normalization, or FXCG simplification implementation is linked here.
    Callers pass the computed result, never the original integration request.
    """
    directory.mkdir(parents=True, exist_ok=True)
    flags, libs = compiler_options()
    exe = directory / 'host-validator'
    subprocess.run(flags + [str(ROOT / 'tests/integration_probe.cc')] + libs
                   + ['-pthread', '-o', str(exe)], check=True, timeout=60)
    return exe

def build(directory, ref='current', target_simplify=False):
    directory.mkdir(parents=True, exist_ok=True)
    text = source(ref, 'yintg.cc').replace(
        '  // Left redimension p to degree n, i.e. size n+1',
        '  bool is_potential(const vecteur &,const vecteur &,gen &,GIAC_CONTEXT);\n'
        '  // Left redimension p to degree n, i.e. size n+1')
    (directory / 'yintg.cc').write_text(text)
    (directory / 'zintgab.cc').write_text(source(ref, 'zintgab.cc'))
    if '#include "integration_guard.h"' in text:
        (directory / 'integration_guard.h').write_text(source(ref, 'integration_guard.h'))
    if '#include "dilogarithm.h"' in text:
        (directory / 'dilogarithm.h').write_text(source(ref, 'dilogarithm.h'))
    syms = source(ref, 'ysym2poly.cc')
    normalized = '#include "giacPCH.h"\nnamespace giac {\n'
    for sig in ('  static bool sort_func(', '  static vecteur sort1(',
                '  gen ratnormal(const gen & e,GIAC_CONTEXT)',
                '  gen recursive_ratnormal(const gen & e,GIAC_CONTEXT)'):
        normalized += function(syms, sig)
    (directory / 'normalize.cc').write_text(normalized + '}\n')
    extra=[]
    if target_simplify:
        # Exercise the real FXCG-only simplification branch without changing
        # the host ABI headers. Other helper/library dependencies remain host.
        s=source(ref, 'ksubst.cc')
        header=''
        if '#include "equation_normalize.h"' in s:
            (directory/'equation_normalize.h').write_text(source(ref,'equation_normalize.h'))
            header='#include "equation_normalize.h"\n'
        simplified='#include "giacPCH.h"\n'+header+'#define FXCG\n#define NO_STDEXCEPT\nnamespace giac {\n'
        simplified+='gen ataninv2atan(const gen &,GIAC_CONTEXT);\ngen cklin(const gen &,GIAC_CONTEXT);\n'
        simplified+=function(source(ref, 'zprog.cc'), '  gen symb_prog3(')
        if '  static unsigned simplify_special_terms(' in s:
            if '  static bool simplify_preflight(' in s:
                simplified+=function(s,'  static bool simplify_preflight(')+function(s,'  static gen simplify_shallow_leaf(')
            simplified+=function(s, '  static unsigned simplify_special_terms(')
            simplified+=function(s, '  static gen simplify_special_core(')
        for sig in ('  gen tsimplify_noexpln(',
                    '  gen simplify(const gen & e_orig,GIAC_CONTEXT)', '  gen _simplify('):
            simplified+=function(s, sig)
        (directory / 'simplify.cc').write_text(simplified+'}\n')
        extra=[str(directory / 'simplify.cc')]
    flags, libs = compiler_options()
    exe = directory / 'probe'
    subprocess.run(flags + ['-DKHICAS_TEST_INTEGRATION_LIMITS', str(directory / 'yintg.cc'),
        str(directory / 'zintgab.cc'), str(directory / 'normalize.cc'), str(ROOT / 'tests/integration_probe.cc')]
        + extra + libs + ['-pthread', '-o', str(exe)], check=True, timeout=180)
    return exe
