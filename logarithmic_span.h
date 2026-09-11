#ifndef KHICAS_LOGARITHMIC_SPAN_H
#define KHICAS_LOGARITHMIC_SPAN_H
#include "equation_normalize.h"
namespace giac {
struct logarithmic_span_index_less {
  bool operator()(const index_t &a,const index_t &b) const {
    unsigned n=a.size()<b.size()?a.size():b.size();
    for(unsigned j=0;j<n;++j)if(a[j]!=b[j])return a[j]<b[j];
    return a.size()<b.size();
  }
};
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
// External inline linkage shares this identical proof walker across the
// integration, definite-integral and derivative translation units. GCC
// noinline keeps recursion out of callers; COMDAT removes duplicate code.
inline bool logarithmic_span_entire(const gen &g,const gen &x,vecteur &atoms,unsigned &budget,unsigned depth){
  if(!budget || depth>8)return false;--budget;
  if(g==x)return true;
  equation_polynomial_budget bound;
  if(equation_rational(g))return equation_rational_budget(g,bound);
  if(g.type!=_SYMB)return false;
  const gen &f=g._SYMBptr->feuille;
  if(g.is_symb_of_sommet(at_inv))
    return !is_zero(f) && equation_rational_budget(f,bound);
  if(g.is_symb_of_sommet(at_sin) || g.is_symb_of_sommet(at_cos) || g.is_symb_of_sommet(at_exp)){
    if(!logarithmic_span_entire(f,x,atoms,budget,depth+1))return false;
    if(!equalposcomp(atoms,g))atoms.push_back(g);
    return atoms.size()<=8;
  }
  if(g.is_symb_of_sommet(at_neg))return logarithmic_span_entire(f,x,atoms,budget,depth+1);
  if(f.type!=_VECT)return false;
  if(g.is_symb_of_sommet(at_pow) && f._VECTptr->size()==2)
    return f[1].type==_INT_ && f[1].val>0 && f[1].val<=8 && logarithmic_span_entire(f[0],x,atoms,budget,depth+1);
  if(!g.is_symb_of_sommet(at_prod) && !g.is_symb_of_sommet(at_plus))return false;
  for(unsigned j=0;j<f._VECTptr->size();++j)if(!logarithmic_span_entire(f[j],x,atoms,budget,depth+1))return false;
  return true;
}

// Seek c*x + sum(b_j*log|D_j|) by exact rational coefficient matching.
// The D_j are the original denominator factors, not guessed factors from
// a complex factorization. Sin/cos/exp atoms are treated as independent;
// a formal identity is sufficient even when it misses other identities.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool integrate_logarithmic_span(const gen &input,const gen &x,gen &result,GIAC_CONTEXT){
  if(!angle_radian(contextptr) || taille(input,97)>96 || x.type!=_IDNT)return false;
  vecteur factors=input.is_symb_of_sommet(at_prod) && input._SYMBptr->feuille.type==_VECT?*input._SYMBptr->feuille._VECTptr:makevecteur(input);
  gen numerator=1;vecteur denominators;
  for(unsigned j=0;j<factors.size();++j){
    if(factors[j].is_symb_of_sommet(at_inv)){
      const gen &d=factors[j]._SYMBptr->feuille;
      if(equation_rational(d) && !is_zero(d)){numerator=numerator/d;continue;}
      if(d.is_symb_of_sommet(at_prod) && d._SYMBptr->feuille.type==_VECT){
        const vecteur &v=*d._SYMBptr->feuille._VECTptr;
        for(unsigned k=0;k<v.size();++k)denominators.push_back(v[k]);
      }
      else denominators.push_back(d);
    }
    else numerator=numerator*factors[j];
  }
  if(denominators.empty() || denominators.size()>3)return false;
  vecteur atoms;unsigned budget=192;
  if(!logarithmic_span_entire(numerator,x,atoms,budget,0))return false;
  gen D=1;
  for(unsigned j=0;j<denominators.size();++j){
    if(!logarithmic_span_entire(denominators[j],x,atoms,budget,0))return false;
    D=D*denominators[j];
  }
  if(atoms.empty())return false; // ordinary rational integration is cheaper
  vecteur basis=makevecteur(D);
  for(unsigned j=0;j<denominators.size();++j){
    gen term=derive(denominators[j],x,contextptr);
    for(unsigned k=0;k<denominators.size();++k)if(k!=j)term=term*denominators[k];
    basis.push_back(term);
  }
  for(unsigned j=0;j<basis.size();++j){budget=192;if(!logarithmic_span_entire(basis[j],x,atoms,budget,0))return false;}
  // Replace outer atoms before nested atoms, without evaluating placeholders.
  std::reverse(atoms.begin(),atoms.end());atoms.push_back(x);
  vecteur names;
  unsigned serial=0;
  for(unsigned j=0;j<atoms.size();++j){
    gen name;
    do {if(serial>=128)return false;name=identificateur(" logarithmic_span_"+print_INT_(int(serial++)));}while(contains(atoms,name));
    names.push_back(name);
  }
  basis.push_back(numerator);unsigned columns=basis.size()-1;
  typedef std::map<index_t,vecteur,logarithmic_span_index_less> coefficient_table;
  coefficient_table coefficients;
  for(unsigned col=0;col<basis.size();++col){
    gen masked=quotesubst(basis[col],atoms,names,contextptr);
    budget=256;equation_polynomial_budget bound;
    if(!equation_polynomial_bound(masked,budget,0,bound) || bound.terms>64 || bound.degree>16)return false;
    fraction f=sym2r(basis[col],atoms,contextptr);
    if(!equation_rational(f.den) || is_zero(f.den))return false;
    if(f.num.type==_POLY){
      const polynome &poly=*f.num._POLYptr;
      if(poly.coord.size()>64)return false;
      for(unsigned k=0;k<poly.coord.size();++k){
        const monomial<gen> &m=poly.coord[k];
        if(!equation_rational(m.value))return false;
        index_t exponent(m.index.begin(),m.index.end());
        vecteur &row=coefficients[exponent];if(row.empty())row.resize(columns+1,0);
        row[col]=m.value/f.den;
      }
    }
    else {
      if(!equation_rational(f.num))return false;
      vecteur &row=coefficients[index_t(atoms.size(),0)];if(row.empty())row.resize(columns+1,0);
      row[col]=f.num/f.den;
    }
    if(coefficients.size()>64)return false;
  }
  std::vector<vecteur> rows;
  for(coefficient_table::const_iterator it=coefficients.begin();it!=coefficients.end();++it)rows.push_back(it->second);
  unsigned rank=0;std::vector<unsigned> pivots;
  for(unsigned col=0;col<columns && rank<rows.size();++col){
    unsigned pivot=rank;while(pivot<rows.size() && is_zero(rows[pivot][col]))++pivot;
    if(pivot==rows.size())continue;
    std::swap(rows[pivot],rows[rank]);gen divisor=rows[rank][col];
    for(unsigned k=col;k<=columns;++k){
      rows[rank][k]=rows[rank][k]/divisor;equation_polynomial_budget bound;
      if(!equation_rational_budget(rows[rank][k],bound))return false;
    }
    for(unsigned i=0;i<rows.size();++i)if(i!=rank && !is_zero(rows[i][col])){
      gen scale=rows[i][col];
      for(unsigned k=col;k<=columns;++k){
        rows[i][k]-=scale*rows[rank][k];equation_polynomial_budget bound;
        if(!equation_rational_budget(rows[i][k],bound))return false;
      }
    }
    pivots.push_back(col);++rank;
  }
  for(unsigned i=rank;i<rows.size();++i)if(!is_zero(rows[i][columns]))return false;
  vecteur weights(columns,0);for(unsigned i=0;i<rank;++i)weights[pivots[i]]=rows[i][columns];
  result=weights[0]*x;
  for(unsigned j=0;j<denominators.size();++j)if(!is_zero(weights[j+1]))
    result+=weights[j+1]*symbolic(at_ln,symbolic(at_abs,denominators[j]));
  // With one nonzero logarithm its argument already excludes D=0. Keep
  // that natural logarithmic endpoint so the ordinary limit engine can
  // classify divergence. Multiple logarithms can cancel at a common zero;
  // a zero coefficient can remove a pole too, so both need an explicit guard.
  if(denominators.size()!=1 || is_zero(weights[1]))
    result=symbolic(at_when,makesequence(symb_equal(D,0),undef,result));
  return true;
}
}
#endif
