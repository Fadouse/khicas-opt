#!/usr/bin/env python3
"""Exercise bounded real integration rules directly, including rejection paths."""
from pathlib import Path
import os, shlex, subprocess, tempfile
from integration_build import ROOT, function
s=(ROOT/'yintg.cc').read_text()
text='#include "giacPCH.h"\n#include "'+str(ROOT/'dilogarithm.h')+'"\nnamespace giac {\ngen linear_integrate_nostep(const gen &,const gen &,gen &,int,GIAC_CONTEXT);\n'
for sig in ('  void decompose_prod(', '  gen extract_cst(',
            '  static bool small_polynomial(', '  static bool small_sparse_polynomial(',
            '  static bool integration_rational(', '  static gen integration_syntax(',
            '  static bool integration_power(',
            '  static bool integration_one_plus(', '  static gen integration_coefficient(',
            '  static bool integration_monomial(', '  static bool integrate_binomial_chain(',
            '  static bool integration_quadratic(', '  static bool integrate_reciprocal_quartic(',
            '  static bool integrate_quartic_trig(', '  static bool integrate_high_frequency_trig(', '  static bool integration_resource_rational(', '  static bool integrate_composed_binomial(', '  static bool integration_dilog_form(', '  static bool integrate_dilog_primitive(', '  static bool integrate_compact_primitive(',
            '  static bool integrate_sine_dirichlet(', '  static bool integrate_logistic_moment(',
            '  static bool integrate_weighted_reflection(', '  static bool integrate_decay_transform(',
            '  static bool integration_square_root(', '  static bool integrate_atan_square(',
            '  static bool integrate_log_trig(', '  static bool integrate_thermal_moment(',
            '  static gen integration_beta_psi(', '  static void integration_beta_partitions(',
            '  static unsigned integration_beta_terms(', '  static gen integration_cumulant_moment(', '  static gen integration_beta_moment(',
            '  static bool integration_outer_power(', '  static bool integration_mellin_monomial(',
            '  static bool integrate_mellin_log(', '  static bool integration_beta_weight(',
            '  static gen integration_beta_joint(',
            '  static bool integrate_beta_log(',
            '  static bool integration_harmonic_real(', '  static bool integration_harmonic_product(',
            '  static bool integration_harmonic_terms(', '  static bool integration_gaussian_quadratic(',
            '  static bool integrate_gaussian_erf(', '  static bool integrate_laplace_difference(',
            '  static bool integrate_inverse_gaussian(',
            '  static bool integrate_log_zeta(', '  static bool integrate_hyperbolic_log(',
            '  static bool integrate_atan_log_moment(', '  static bool integration_beta_affine_weight(','  static bool integrate_beta_affine_log(','  static gen integration_acos_circle_value(','  static bool integration_arc_binomial(','  static bool integrate_acos_monomial_pullback(',
            '  static bool integration_affine_cosine(',
            '  static bool integrate_acos_circle(', '  static bool integrate_atan_frullani(',
            '  static bool integrate_loglog_mellin(', '  static bool integration_log_slope(',
            '  static bool integrate_loglog_frullani(', '  static bool integrate_arc_rational_circle(', '  static bool integration_period_real_bound(',
            '  static bool integrate_reciprocal_trig_period(', '  static bool integrate_mellin_two_binomials(', '  static bool integration_exp_affine(', '  static bool integrate_exp_difference(',
            '  static bool integration_chain_add(','  static bool integration_chain_terms(','  static bool integration_chain_endpoint(','  static bool integrate_erf_chain(','  static bool integration_rectangle_term(','  static bool integrate_atan_rectangle_pair(','  static bool integrate_gaussian_erf_exp(',
            '  static bool integrate_reciprocal_cosh(',
            '  static bool integrate_gaussian_atan_moment(',
            '  static bool integrate_exponential_log_moment(',
            '  static int integration_quarter_sigma(',
            '  static bool integrate_log_trig_quarters(',
            '  static bool integrate_log_product_zeta(',
            '  static bool integrate_log_trig_sum(',
            '  static bool integrate_atan_log_measure(',
            '  static gen integration_mixed_mellin_value(',
            '  static bool integrate_mixed_mellin_log(',
            '  static bool integrate_oscillatory_cancellation(',
            '  static bool integrate_erf_tail_product(',
            '  static gen integration_cauchy_log_moment(',
            '  static bool integration_log_affine_ratio(',
            '  static bool integrate_mobius_cauchy_log(',
            '  static bool integrate_log_sine_cosine_sum(',
            '  static bool integrate_gamma_log_moment(',
            '  static bool integrate_atan_log_cauchy(',
            '  static bool integration_trig_unit_log(',
            '  static bool integrate_opposite_trig_logs(',
            '  static bool integration_scaled_chain_terms(',
            '  static bool integrate_monomial_gaussian_erf(',
            '  static bool integrate_atan_cauchy_power(',
            '  static bool integrate_exponential_beta(', '  static bool integrate_complementary_ratio(', '  static bool integrate_cauchy_fourier(', '  static bool integrate_unit_log_arc(', '  static bool integrate_positive_cosine_kernel(', '  static bool integrate_dilog_definite(', '  static bool integrate_compact_definite(',
            '  static bool integrate_real_root(', '  static bool integrate_residue_kernel(',
            '  bool integration_rational_tail(', '  static bool integration_finite_poly_add(',
            '  static bool integration_finite_poly_product(',
            '  static bool integration_finite_poly_terms(',
            '  static bool integrate_finite_polynomial(',
            '  static bool integration_finite_elementary_bound(',
            '  static bool integrate_finite_elementary(',
            '  static bool integrate_parameter_kernel(',
            '  static bool integrate_real_definite('):
    text+=function(s,sig).replace('  static ', '  ',1)
text+='}\n'
with tempfile.TemporaryDirectory(prefix='khicas-real-definite-') as tmp:
    p=Path(tmp);(p/'rules.cc').write_text(text)
    flags=[os.environ.get('CXX','c++'),'-std=c++11','-O2','-DHAVE_CONFIG_H','-DGIAC_GENERIC_CONSTANTS','-Wno-deprecated-declarations','-I',os.environ.get('GIAC_INCLUDE','/usr/include/giac')]+shlex.split(os.environ.get('CXXFLAGS',''))
    libs=shlex.split(os.environ.get('LDFLAGS',''))+['-lgiac']+shlex.split(os.environ.get('GIAC_NUMERIC_LIBS','-lgmp -lmpfr'))
    for test in ('real_definite.cc','compact_integrals.cc','cycle1_integrals.cc','cycle2_integrals.cc','user_extra_integrals.cc',
                 'cycle3_laplace_integrals.cc','user_matrix_rules.cc','user_matrix_next_rules.cc','parameter_kernel_rules.cc','dilogarithm_rules.cc'):
        subprocess.run(flags+[str(p/'rules.cc'),str(ROOT/'tests'/test)]+libs+['-pthread','-o',str(p/'test')],check=True)
        subprocess.run([str(p/'test')],check=True,timeout=60)
