#ifndef KHICAS_EQUATION_NORMALIZE_H
#define KHICAS_EQUATION_NORMALIZE_H
#include "sym2poly.h"
#include "misc.h"
#include "usual.h"
namespace giac {
  inline bool equation_rational(const gen & g) {
    if (g.type == _INT_ || g.type == _ZINT)
      return true;
    if (g.type != _FRAC)
      return false;
    const gen &a = g._FRACptr->num, &b = g._FRACptr->den;
    return (a.type == _INT_ || a.type == _ZINT) && (b.type == _INT_ || b.type == _ZINT) &&
           !is_zero(b);
  }
  // Bounds describe P=N/D: numerator_bits bounds log2 of the sum of absolute
  // integer coefficients of N; denominator_bits bounds log2(abs(D)). Using the
  // coefficient sum also covers collisions during multiplication and powers.
  struct equation_polynomial_budget {
    unsigned terms, degree, numerator_bits, denominator_bits;
    equation_polynomial_budget() : terms(1), degree(0), numerator_bits(0), denominator_bits(0) {}
    bool fits() const {
      return terms && terms <= 256 && degree <= 64 && numerator_bits <= 2048 &&
             denominator_bits <= 2048 && terms * numerator_bits + denominator_bits <= 65536;
    }
  };
  inline unsigned equation_integer_bits(const gen & g) {
    if (is_zero(g) || is_one(g) || is_minus_one(g))
      return 0;
    if (g.type == _ZINT) {
      size_t n = mpz_sizeinbase(*g._ZINTptr, 2);
      return n > 2048 ? 2049 : unsigned(n);
    }
    unsigned n = g.val < 0 ? 0u - unsigned(g.val) : unsigned(g.val), bits = 0;
    while (n) {
      ++bits;
      n >>= 1;
    }
    return bits;
  }
  inline bool equation_rational_budget(const gen & g, equation_polynomial_budget & out) {
    if (!equation_rational(g))
      return false;
    out = equation_polynomial_budget();
    if (g.type == _FRAC) {
      out.numerator_bits = equation_integer_bits(g._FRACptr->num);
      out.denominator_bits = equation_integer_bits(g._FRACptr->den);
    } else
      out.numerator_bits = equation_integer_bits(g);
    return out.fits();
  }
  inline bool equation_combine_budget(equation_polynomial_budget & a,
                                      const equation_polynomial_budget & b, bool product) {
    if (product) {
      a.terms *= b.terms;
      a.degree += b.degree;
      a.numerator_bits += b.numerator_bits;
    } else {
      a.terms += b.terms;
      if (b.degree > a.degree)
        a.degree = b.degree;
      unsigned left = a.numerator_bits + b.denominator_bits,
               right = b.numerator_bits + a.denominator_bits;
      a.numerator_bits = (left > right ? left : right) + 1;
    }
    a.denominator_bits += b.denominator_bits;
    return a.fits();
  }
  // Reject before sym2r can expand excessive degree or large rational coefficients.
  // Variable denominators and transcendental nodes retain their original domains.
  inline bool equation_polynomial_bound(const gen & g, unsigned & budget, unsigned depth,
                                        equation_polynomial_budget & out) {
    if (!budget || depth > 24)
      return false;
    --budget;
    if (g.type == _IDNT) {
      out = equation_polynomial_budget();
      out.degree = 1;
      return true;
    }
    if (g.type == _INT_ || g.type == _ZINT || g.type == _FRAC)
      return equation_rational_budget(g, out);
    if (g.type != _SYMB)
      return false;
    const gen & f = g._SYMBptr->feuille;
    if (g.is_symb_of_sommet(at_inv)) {
      if (is_zero(f) || !equation_rational_budget(f, out))
        return false;
      unsigned bits = out.numerator_bits;
      out.numerator_bits = out.denominator_bits;
      out.denominator_bits = bits;
      return true;
    }
    if (g.is_symb_of_sommet(at_neg))
      return equation_polynomial_bound(f, budget, depth + 1, out);
    if (f.type != _VECT)
      return false;
    const vecteur & v = *f._VECTptr;
    if (g.is_symb_of_sommet(at_division)) {
      equation_polynomial_budget divisor;
      if (v.size() != 2 || is_zero(v[1]) || !equation_rational_budget(v[1], divisor) ||
          !equation_polynomial_bound(v[0], budget, depth + 1, out))
        return false;
      unsigned bits = divisor.numerator_bits;
      divisor.numerator_bits = divisor.denominator_bits;
      divisor.denominator_bits = bits;
      return equation_combine_budget(out, divisor, true);
    }
    if (g.is_symb_of_sommet(at_pow)) {
      if (v.size() != 2 || v[1].type != _INT_ || v[1].val < 0 || v[1].val > 32)
        return false;
      equation_polynomial_budget base;
      if (!equation_polynomial_bound(v[0], budget, depth + 1, base))
        return false;
      out = equation_polynomial_budget();
      for (int k = 0; k < v[1].val; ++k)
        if (!equation_combine_budget(out, base, true))
          return false;
      return true;
    }
    bool product = g.is_symb_of_sommet(at_prod);
    if (!product && !g.is_symb_of_sommet(at_plus))
      return false;
    if (v.empty())
      return false;
    if (!equation_polynomial_bound(v[0], budget, depth + 1, out))
      return false;
    for (unsigned i = 1; i < v.size(); ++i) {
      equation_polynomial_budget next;
      if (!equation_polynomial_bound(v[i], budget, depth + 1, next) ||
          !equation_combine_budget(out, next, product))
        return false;
    }
    return true;
  }
  inline unsigned equation_polynomial_terms(const gen & g, unsigned & budget, unsigned depth) {
    equation_polynomial_budget bound;
    return equation_polynomial_bound(g, budget, depth, bound) ? bound.terms : 0;
  }
  inline gen equation_numeric_factor(gen & g) {
    if (equation_rational(g)) {
      gen c = g;
      g = 1;
      return c;
    }
    if (g.is_symb_of_sommet(at_neg)) {
      g = gen(g._SYMBptr->feuille);
      return -equation_numeric_factor(g);
    }
    if (g.is_symb_of_sommet(at_division) && g._SYMBptr->feuille.type == _VECT) {
      const vecteur v = *g._SYMBptr->feuille._VECTptr;
      if (v.size() == 2 && equation_rational(v[1]) && !is_zero(v[1])) {
        g = gen(v[0]);
        return equation_numeric_factor(g) / v[1];
      }
    }
    if (!g.is_symb_of_sommet(at_prod) || g._SYMBptr->feuille.type != _VECT)
      return 1;
    const vecteur v = *g._SYMBptr->feuille._VECTptr;
    gen scalar = 1, result = 1;
    for (unsigned i = 0; i < v.size(); ++i) {
      gen term = v[i];
      scalar = scalar * equation_numeric_factor(term);
      result = result * term;
    }
    g = result;
    return scalar;
  }
  // Remove only a nonzero rational scalar from an equality. In particular,
  // x*y=0 keeps both components, and rational-function domains are untouched.
  inline bool equation_primitive(const gen & equation, gen & result, GIAC_CONTEXT) {
    if (!equation.is_symb_of_sommet(at_equal) || equation._SYMBptr->feuille.type != _VECT ||
        equation._SYMBptr->feuille._VECTptr->size() != 2)
      return false;
    const vecteur & sides = *equation._SYMBptr->feuille._VECTptr;
    unsigned budget = 512;
    equation_polynomial_budget left, right;
    if (!equation_polynomial_bound(sides[0], budget, 0, left) ||
        !equation_polynomial_bound(sides[1], budget, 0, right) ||
        !equation_combine_budget(left, right, false))
      return false;
    gen residual = sides[0] - sides[1];
    vecteur vars = lidnt(residual);
    if (vars.empty() || vars.size() > 8)
      return false;
    fraction f = sym2r(residual, vars, contextptr);
    if (f.num.type != _POLY || (f.den.type != _INT_ && f.den.type != _ZINT) || is_zero(f.den))
      return false;
    const polynome & p = *f.num._POLYptr;
    if (p.coord.empty() || p.coord.size() > 256)
      return false;
    vecteur coefficients;
    for (unsigned i = 0; i < p.coord.size(); ++i) {
      const gen & c = p.coord[i].value;
      if (c.type != _INT_ && c.type != _ZINT)
        return false;
      coefficients.push_back(c);
    }
    gen content = _lgcd(gen(coefficients), contextptr);
    if (is_zero(content))
      return false;
    if (is_strictly_positive(-p.coord.front().value, contextptr))
      content = -content;
    if (is_one(content / f.den))
      return false;
    gen expanded = r2e(f.num / content, vars, contextptr), compact = residual;
    gen scalar = equation_numeric_factor(compact) / (content / f.den);
    result = symb_equal(
        is_one(scalar) && taille(compact, 1024) < taille(expanded, 1024) ? compact : expanded, 0);
    return true;
  }
} // namespace giac
#endif
