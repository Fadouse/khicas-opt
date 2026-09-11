OUTPUT_FORMAT(elf32-sh)
OUTPUT_ARCH(sh3)
ENTRY(initialize)
/* ENTRY(_main) */
 
MEMORY
{
        /* Loads code at 300000, skips g3a header */
        rom (rx) : o = 0x00300000, l = 2M-32000
        ram (rwx) : o = 0x08101000, l = 432k  /* skip 4k at start */
	/* 0x88160000 virtualized to 0x08100000 */
        ilram (rwx) : o = 0xE5200000, l = 4k
	/* RAM is available at 0x8c200000, 3M maybe more */
        r8c2 (rx) : o = 0x88200000, l = 2500K-4
}
 
SECTIONS
{
	.rominram : {
		/* Bounded integration helpers use spare AC2 code space.
		   Keep hot general integration and the CAS heap layout unchanged. */
    /* Conversion routines share the available AC2 code region. */
    kconvert.o(.text.*)
    yintg.o(.text.*integrate_atan_square*)
    yintg.o(.text.*integrate_log_zeta*)
    yintg.o(.text.*integrate_hyperbolic_log*)
    yintg.o(.text.*integrate_atan_log_moment*)
    yintg.o(.text.*integrate_acos_circle*)
    yintg.o(.text.*integrate_mellin_two_binomials*)
    yintg.o(.text.*integrate_gaussian_erf*)
    /* Cycle5 bounded helpers; gaussian_erf* also covers gaussian_erf_exp. */
    yintg.o(.text.*integrate_high_frequency_trig*)
    yintg.o(.text.*integrate_reciprocal_cosh*)
    yintg.o(.text.*integration_finite_poly_add*)
    yintg.o(.text.*integration_finite_poly_product*)
    yintg.o(.text.*integration_finite_poly_terms*)
    yintg.o(.text.*integrate_finite_polynomial*)
    yintg.o(.text.*integrate_finite_elementary*)
    yintg.o(.text.*integrate_erf_tail_product*)
    yintg.o(.text.*integration_cumulant_moment*)
    yintg.o(.text.*integrate_gamma_log_moment*)
    yintg.o(.text.*integrate_monomial_gaussian_erf*)
    yintg.o(.text.*integrate_atan_cauchy_power*)
    yintg.o(.text.*integrate_atan_log_cauchy*)
    yintg.o(.text.*integrate_opposite_trig_logs*)
    yintg.o(.text.*integration_cauchy_log_moment*)
    yintg.o(.text.*integrate_mobius_cauchy_log*)
    yintg.o(.text.*integrate_log_sine_cosine_sum*)
    yintg.o(.text.*integrate_gaussian_atan_moment*)
    yintg.o(.text.*integrate_exponential_log_moment*)
    yintg.o(.text.*integrate_log_trig_quarters*)
    yintg.o(.text.*integrate_log_product_zeta*)
    yintg.o(.text.*integrate_log_trig_sum*)
    yintg.o(.text.*integrate_atan_log_measure*)
    yintg.o(.text.*integrate_mixed_mellin_log*)
    yintg.o(.text.*integrate_oscillatory_cancellation*)
    yintg.o(.text.*integrate_beta_affine_log*)
    yintg.o(.text.*integrate_acos_monomial_pullback*)
    yintg.o(.text.*integrate_erf_chain*)
    yintg.o(.text.*integrate_atan_rectangle_pair*)
    yintg.o(.text.*integration_acos_circle_value*)
    yintg.o(.text.*integrate_exp_difference*)
    yintg.o(.text.*integrate_reciprocal_trig_period*)
    yintg.o(.text.*integrate_atan_frullani*)
    yintg.o(.text.*integrate_loglog_frullani*)
    yintg.o(.text.*integrate_arc_rational_circle*)
    yintg.o(.text.*integrate_loglog_mellin*)
    yintg.o(.text.*integration_beta_psi*)
    yintg.o(.text.*integration_beta_partitions*)
    yintg.o(.text.*integration_beta_terms*)
    yintg.o(.text.*integration_beta_moment*)
    yintg.o(.text.*integration_beta_joint*)
		yintg.o(.text.*integrate_mellin_log*)
		yintg.o(.text.*integration_beta_weight*)
		yintg.o(.text.*integrate_beta_log*)
		yintg.o(.text.*integrate_laplace_difference*)
		yintg.o(.text.*integrate_inverse_gaussian*)
		yintg.o(.text.*integrate_binomial_chain*)
		yintg.o(.text.*integrate_reciprocal_quartic*)
		yintg.o(.text.*integrate_quartic_trig*)
		yintg.o(.text.*integrate_composed_binomial*)
    yintg.o(.text.*integrate_exponential_beta*)
    yintg.o(.text.*integrate_real_quadratic*)
    yintg.o(.text.*integrate_compact_primitive*)
		yintg.o(.text.*integrate_sine_dirichlet*)
		yintg.o(.text.*integrate_logistic_moment*)
		yintg.o(.text.*integrate_weighted_reflection*)
		yintg.o(.text.*integrate_decay_transform*)
		yintg.o(.text.*integrate_log_trig*)
		yintg.o(.text.*integrate_thermal_moment*)
		yintg.o(.text.*integrate_complementary_ratio*)
    yintg.o(.text.*integrate_cauchy_fourier*)
    yintg.o(.text.*integrate_unit_log_arc*)
    yintg.o(.text.*integrate_positive_cosine_kernel*)
    yintg.o(.text.*integrate_parameter_kernel*)
    yintg.o(.text.*khicas_dilog_log*)
    yintg.o(.text.*khicas_dilog_numeric*)
    yintg.o(.text.*d_Li2*)
    yintg.o(.text.*_Li2*)
    yintg.o(.text.*integration_dilog_form*)
    yintg.o(.text.*integrate_dilog_primitive*)
    yintg.o(.text.*integrate_log_trig_primitive*)
    yderive.o(.text.*derive_real_composition*)
    yderive.o(.text.*derive_piecewise_regular*)
    ksubst.o(.text.*simplify_root_domain*)
    yderive.o(.text.*derive_root_product*)
    yderive.o(.text.*derive_piecewise_oscillation*)
    yderive.o(.text.*derive_piecewise_joints*)
    yintg.o(.text.*integrate_dilog_definite*)
    yintg.o(.text.*integrate_compact_definite*)
		yintg.o(.text.*integrate_real_root*)
		yintg.o(.text.*integrate_residue_kernel*)
		yintg.o(.text.*integrate_real_definite*)
		z*.o(.text)
		z*.o(.text.*)
		z*.o(.rodata)
		z*.o(.rodata.*)
	} > r8c2

        /* Code, in ROM */
        .text : {
		*(.pretext)
                *(.text)
                *(.text.*)
        } > rom

        /* Read-only data, in ROM */
        .rodata : {
                *(.rodata)
                *(.rodata.*)
        } > rom
       
        /* RW initialized data, VMA in RAM but LMA in ROM */
        .data : ALIGN(4) {
                _datald = LOADADDR(.data) ;
                _sdata = . ;
                *(.data)
                *(.data.*);
                _edata = . ;
        } >ram AT>rom
       
        /* Uninitialized data (fill with 0), in RAM */
        .bss ALIGN(4) : ALIGN(4) {
                _bbss = . ;
                *(.bss)
                *(.bss*)
                *(COMMON)
                _ebss = . ;

		/* Extra unused static space */
		_sextra = ALIGN(32);
		_eextra = ORIGIN(ram) + LENGTH(ram);
        } >ram

}
