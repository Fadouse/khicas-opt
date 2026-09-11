#ifndef KHICAS_CONDITIONAL_EVAL_H
#define KHICAS_CONDITIONAL_EVAL_H
#include "equation_normalize.h"
namespace giac {
// Only inspect pure, small exact constants; no user calls or bindings are
// evaluated here. This makes cos((-sqrt(2*pi))^2)=1 decidable without using
// numerical tolerances or repeating side effects from arbitrary conditions.
static bool conditional_pure_constant(const gen &g,unsigned &budget,unsigned depth){
  if(!budget || depth>8)return false;
  --budget;
  if(g.type==_INT_ || g.type==_ZINT || g==cst_pi)return true;
  if(g.type==_FRAC)return conditional_pure_constant(g._FRACptr->num,budget,depth+1) && conditional_pure_constant(g._FRACptr->den,budget,depth+1);
  if(g.type!=_SYMB)return false;
  const unary_function_ptr &op=g._SYMBptr->sommet;const gen &f=g._SYMBptr->feuille;
  if(op!=at_plus && op!=at_prod && op!=at_neg && op!=at_inv && op!=at_division && op!=at_pow && op!=at_sqrt && op!=at_sin && op!=at_cos)return false;
  if(f.type!=_VECT)return conditional_pure_constant(f,budget,depth+1);
  if(f._VECTptr->size()>8)return false;
  if(op==at_pow && (f._VECTptr->size()!=2 || (f[1]!=gen(1)/2 && (f[1].type!=_INT_ || f[1].val < -8 || f[1].val>8))))return false;
  for(unsigned j=0;j<f._VECTptr->size();++j)if(!conditional_pure_constant(f[j],budget,depth+1))return false;
  return true;
}
// One quadratic radical gives an extension of degree at most two.
// Keep constant equality exact without opening arbitrary multiradical
// normalization or treating a floating-point residual as mathematical zero.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool conditional_algebraic_constant(const gen &g,vecteur &roots,unsigned &budget,unsigned depth,GIAC_CONTEXT){
  if(!budget || depth>8)return false;--budget;
  equation_polynomial_budget bound;
  if(equation_rational(g))return equation_rational_budget(g,bound);
  if(g.type!=_SYMB)return false;
  const gen &f=g._SYMBptr->feuille;gen radicand;
  if(g.is_symb_of_sommet(at_sqrt))radicand=f;
  else if(g.is_symb_of_sommet(at_pow) && f.type==_VECT && f._VECTptr->size()==2 && f[1]==gen(1)/2)radicand=f[0];
  else {
    if(g.is_symb_of_sommet(at_neg) || g.is_symb_of_sommet(at_inv))return conditional_algebraic_constant(f,roots,budget,depth+1,contextptr);
    if(f.type!=_VECT || f._VECTptr->size()>8)return false;
    if(g.is_symb_of_sommet(at_pow))return f._VECTptr->size()==2 && f[1].type==_INT_ && f[1].val>=-8 && f[1].val<=8 && conditional_algebraic_constant(f[0],roots,budget,depth+1,contextptr);
    if(!g.is_symb_of_sommet(at_plus) && !g.is_symb_of_sommet(at_prod) && !g.is_symb_of_sommet(at_division))return false;
    for(unsigned j=0;j<f._VECTptr->size();++j)if(!conditional_algebraic_constant(f[j],roots,budget,depth+1,contextptr))return false;
    return true;
  }
  if(!equation_rational_budget(radicand,bound) || !is_strictly_positive(radicand,contextptr))return false;
  if(!equalposcomp(roots,radicand))roots.push_back(radicand);
  return roots.size()<=1;
}

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool conditional_exact_equal(const gen &a,const gen &b,GIAC_CONTEXT){
  unsigned budget=32;
  if(has_op(a,*at_sin) || has_op(a,*at_cos) || has_op(b,*at_sin) || has_op(b,*at_cos)){
    if(!conditional_pure_constant(a,budget,0) || !conditional_pure_constant(b,budget,0))return false;
  }
  else {vecteur roots;if(!conditional_algebraic_constant(a,roots,budget,0,contextptr) || !conditional_algebraic_constant(b,roots,budget,0,contextptr))return false;}
  gen aa=recursive_normal(a,contextptr),bb=recursive_normal(b,contextptr);
  return !is_undef(aa) && !is_undef(bb) && !is_inf(aa) && !is_inf(bb) && aa==bb;
}

// Mathematical when/piecewise keep an equality with free identifiers
// undecided. Structural "same" returning false is not a proof of inequality.
// Program if/else and the standalone == operator keep their usual semantics.
inline bool conditional_symbolic_equal(const gen &test,gen &result,GIAC_CONTEXT){
  if(!(test.is_symb_of_sommet(at_equal) || test.is_symb_of_sommet(at_same)) ||
     test._SYMBptr->feuille.type!=_VECT || test._SYMBptr->feuille._VECTptr->size()!=2 ||
     taille(test,65)>64)return false;
  const vecteur &v=*test._SYMBptr->feuille._VECTptr;
  if(v[0]==v[1])return false;
  vecteur ids=lidnt(test);
  bool variable=false;
  for(unsigned j=0;j<ids.size();++j){
    if(ids[j]==cst_pi)continue;
    // Inspect bindings, not the test's expressions: evaluating a function
    // call here and again in the ordinary selector could repeat side effects.
    gen value=ids[j].eval(1,contextptr);
    vecteur remaining=lidnt(value);
    for(unsigned k=0;k<remaining.size();++k)if(remaining[k]!=cst_pi){variable=true;break;}
    if(variable)break;
  }
  if(!variable){
    if(conditional_exact_equal(v[0],v[1],contextptr)){result=1;return true;}
    return false;
  }
  result=symb_equal(v[0],v[1]);return true;
}
}
#endif
