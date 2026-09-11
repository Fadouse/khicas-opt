#!/usr/bin/env python3
"""Prove acceptance and safe deferral of the bounded radical substitution.

The extracted code is the production helper. SymPy independently checks its
primitive; rejection cases must leave the generic integrator in control.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
import sympy as sp
from integration_build import ROOT, compiler_options, function
from mixed_reference import parse, equal

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build-dir', type=Path, required=True)
p.add_argument('--report', type=Path, required=True)
a = p.parse_args()
a.build_dir.mkdir(parents=True, exist_ok=True)
source = (ROOT / 'yintg.cc').read_text()
header = (ROOT / 'equation_normalize.h').read_text()
(a.build_dir / 'equation_normalize.h').write_text(header)
out = '#include "giacPCH.h"\n#include "equation_normalize.h"\n#include <iostream>\nnamespace giac {\n'
for name in ('integration_rational', 'integration_syntax', 'integration_square_root',
             'integration_finite_poly_add', 'integration_finite_poly_product',
             'integration_finite_poly_terms', 'integration_polynomial_radical',
             'integrate_affine_radical'):
    signature = ('  static gen ' if name == 'integration_syntax' else '  static bool ') + name + '('
    # The integration dispatcher has an earlier forward declaration.
    start = source.rindex(signature) if name == 'integrate_affine_radical' else source.index(signature)
    out += function(source[start:], signature)
out += '\n}\nint main(int argc,char **argv){giac::context c;giac::gen x(giac::identificateur("x")),g(std::string(argv[1]),&c),r;bool ok=giac::integrate_affine_radical(g,x,r,&c);std::cout<<(ok?"ACCEPT "+r.print(&c):"DEFER")<<"\\n";}\n'
path = a.build_dir / 'probe.cc'
path.write_text(out)
flags, libs = compiler_options()
exe = a.build_dir / 'probe'
subprocess.run(flags + [str(path)] + libs + ['-o', str(exe)], check=True)
# Nonempty interval, a nonvanishing original denominator, polynomial
# divisibility, and a bounded real affine radicand are independent contracts.
cases = [
    ('x/(sqrt(1+x)+sqrt(1-x))', True, 'conjugate zero is artificial'),
    ('(4*(3*x+4)-9*(2-x))/(2*sqrt(3*x+4)+3*sqrt(2-x))', True, 'weighted positive roots'),
    ('(4*(3*x+4)-9*(2-x))/(-2*sqrt(3*x+4)-3*sqrt(2-x))', True, 'weighted negative roots'),
    ('(x^8+1)*sqrt(2*x+3)', True, 'maximum accepted degree'),
    ('(x^3+1)/sqrt(3-2*x)', True, 'negative slope'),
    ('sqrt(2)', True, 'constant positive radicand'),
    ('sqrt(x)', True, 'zero translation constant monomial'),
    ('x^3/sqrt(-2*x)', True, 'zero translation and negative slope'),
    ('x/(sqrt(1+x)-sqrt(1-x))', False, 'original denominator has zero'),
    ('x/(sqrt(x)+sqrt(2*x))', False, 'common zero remains excluded'),
    ('1/(sqrt(x)+sqrt(-x-1))', False, 'empty real domain'),
    ('1/(sqrt(x)+sqrt(-x))', False, 'no real interior'),
    ('1/(sqrt(x+1)+sqrt(1-x))', False, 'conjugate divisor does not divide numerator'),
    ('(x^9+1)*sqrt(x+1)', False, 'degree exceeds bounded algorithm'),
    ('sqrt(x^2+1)', False, 'non-affine radicand'),
    ('sqrt(a*x+1)', False, 'unknown parameter sign'),
    ('y*sqrt(x+1)', False, 'additional variable'),
    ('sqrt(-2)', False, 'complex radicand'),
    ('sin(x)*sqrt(x+1)', False, 'non-polynomial numerator'),
    ('sqrt(x+1)*sqrt(x+2)', False, 'two direct radical factors'),
    ('1/(x*sqrt(x+1))', False, 'rational coefficient has a pole'),
    ('(2^4096)*sqrt(x+1)', False, 'coefficient bit budget'),
]
rows = []
x = sp.Symbol('x', real=True)
for expression, accept, reason in cases:
    run = subprocess.run([str(exe), expression], capture_output=True, text=True, timeout=10)
    row = dict(input=expression, expected_accept=accept, reason=reason,
               exit=run.returncode, result=run.stdout.strip())
    try:
        assert run.returncode == 0
        if accept:
            assert row['result'].startswith('ACCEPT ')
            F = parse(row['result'][7:])
            assert equal(sp.diff(F, x), parse(expression))
        else:
            assert row['result'] == 'DEFER'
        row['passed'] = True
    except Exception as error:
        row.update(passed=False, error=str(error))
    rows.append(row)
a.report.write_text(json.dumps(dict(
    scope='Isolated production rule acceptance/deferral with independent exact differentiation; full integration paths are tested separately.',
    source_sha256={name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
                   for name in ('yintg.cc', 'equation_normalize.h')}, runs=rows), indent=2) + '\n')
print(len(rows), sum(row['passed'] for row in rows))
assert all(row['passed'] for row in rows), [row for row in rows if not row['passed']]
