#ifndef KHICAS_DETERMINANT_SMALL_H
#define KHICAS_DETERMINANT_SMALL_H
#include "equation_normalize.h"
namespace giac {
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static vecteur determinant_atoms(const gen &g){
  vecteur powers=lop(g,at_pow),atoms;
  for(unsigned j=0;j<powers.size();++j){
    const gen &f=powers[j]._SYMBptr->feuille;
    if(f.type==_VECT && f._VECTptr->size()==2 && f[0].type==_SYMB && !equalposcomp(atoms,f[0]))atoms.push_back(f[0]);
  }
  return atoms;
}

// Bases of integer powers can be independent polynomial atoms during exact
// cancellation. Prove each atom separately, then bound the smaller formal
// polynomial. This never raises the global expansion budget or expands
// (x+y)^m*(x-y)^m merely to cancel another copy of the same product.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool determinant_polynomial_bound(const gen &g,unsigned &budget,unsigned depth,equation_polynomial_budget &out){
  unsigned trial=budget;
  if(equation_polynomial_bound(g,trial,depth,out) && out.terms<=16){budget=trial;return true;}
  if(!budget || depth>8)return false;--budget;
  vecteur atoms=determinant_atoms(g),names;if(atoms.empty() || atoms.size()>16)return false;
  unsigned degree=1,serial=0;
  for(unsigned j=0;j<atoms.size();++j){
    trial=128;equation_polynomial_budget bound;
    if(!equation_polynomial_bound(atoms[j],trial,0,bound) || bound.degree>16)return false;
    if(bound.degree>degree)degree=bound.degree;
    gen name;
    do {if(serial>=128)return false;name=identificateur(" determinant_bound_"+print_INT_(int(serial++)));}while(contains(g,name));
    names.push_back(name);
  }
  gen masked=quotesubst(g,atoms,names,context0);
  trial=128;if(!equation_polynomial_bound(masked,trial,0,out) || out.degree*degree>64)return false;
  out.degree*=degree;return true;
}

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static gen determinant_polynomial_normal(const gen &g,GIAC_CONTEXT){
  vecteur atoms=determinant_atoms(g),variables=lidnt(g);
  for(unsigned j=0;j<variables.size();++j)atoms.push_back(variables[j]);
  if(atoms.empty())return g;
  // Explicit algebraic variables avoid placeholder evaluation/assumptions.
  // The expression is a proved polynomial, so its denominator is numeric.
  fraction value=sym2r(g,atoms,contextptr);
  return r2sym(value,atoms,contextptr);
}

// Exact arithmetic decomposition without a general rational gcd. A sum
// may share a symbolic polynomial denominator or contain polynomial terms.
// Inverses of non-polynomials are deferred, so their inner holes cannot be
// lost by turning 1/(N/D) into D/N.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool determinant_fraction(const gen &g,gen &num,gen &den,unsigned &budget,unsigned depth){
  if(!budget || depth>8)return false;--budget;
  unsigned trial=128;equation_polynomial_budget bound;
  if(determinant_polynomial_bound(g,trial,0,bound) && bound.degree<=16 && bound.terms<=64){num=g;den=1;return true;}
  if(g.type!=_SYMB)return false;
  const gen &f=g._SYMBptr->feuille;
  if(g.is_symb_of_sommet(at_neg)){
    if(!determinant_fraction(f,num,den,budget,depth+1))return false;num=-num;return true;
  }
  if(g.is_symb_of_sommet(at_inv)){
    trial=128;if(!determinant_polynomial_bound(f,trial,0,bound) || bound.degree>8 || is_zero(f))return false;
    num=1;den=f;return true;
  }
  if(f.type!=_VECT)return false;const vecteur &v=*f._VECTptr;
  if(g.is_symb_of_sommet(at_division) && v.size()==2){
    trial=128;if(!determinant_polynomial_bound(v[1],trial,0,bound) || bound.degree>8 || is_zero(v[1]))return false;
    if(!determinant_fraction(v[0],num,den,budget,depth+1))return false;den=den*v[1];
  }
  else if(g.is_symb_of_sommet(at_pow) && v.size()==2 && v[1].type==_INT_ && v[1].val<0 && v[1].val>=-16){
    trial=128;if(!determinant_polynomial_bound(v[0],trial,0,bound) || is_zero(v[0]))return false;
    num=1;den=pow(v[0],-v[1].val);
  }
  else {
    bool product=g.is_symb_of_sommet(at_prod);
    if((!product && !g.is_symb_of_sommet(at_plus)) || v.empty())return false;
    num=product?1:0;den=1;
    for(unsigned k=0;k<v.size();++k){
      gen a,b;if(!determinant_fraction(v[k],a,b,budget,depth+1))return false;
      if(product){num=num*a;den=den*b;}
      else if(den==b)num+=a;
      else if(is_one(den)){num=num*b+a;den=b;}
      else if(is_one(b))num+=a*den;
      else return false;
      trial=128;if(!determinant_polynomial_bound(num,trial,0,bound) || bound.degree>16 || bound.terms>64)return false;
      trial=128;if(!determinant_polynomial_bound(den,trial,0,bound) || bound.degree>8)return false;
    }
  }
  trial=128;return determinant_polynomial_bound(den,trial,0,bound) && bound.degree<=8;
}

// Small symbolic matrices use division-free subset expansion (n<=4).
// When all 2x2 minors prove rank(A-I)<=1, use 1+trace(A-I) instead.
// Neither route divides by symbolic pivots or cancels the original poles.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool determinant_small(const gen &input,gen &result,GIAC_CONTEXT){
  if(input.type!=_VECT || input._VECTptr->size()<2 || input._VECTptr->size()>4 ||
     taille(input,513)>512 || !is_squarematrix(input) || lidnt(input).empty())return false;
  const vecteur &matrix=*input._VECTptr;unsigned n=matrix.size();
  vecteur numerators,denominators;gen common=1;
  for(unsigned i=0;i<n;++i)for(unsigned j=0;j<n;++j){
    gen num,den;unsigned budget=128;
    if(!determinant_fraction((*matrix[i]._VECTptr)[j],num,den,budget,0))return false;
    if(i==j)num-=den;
    unsigned trial=256;equation_polynomial_budget bound;
    if(!determinant_polynomial_bound(num,trial,0,bound) || bound.degree>16)return false;
    num=determinant_polynomial_normal(num,contextptr);
    if(!is_one(den)){
      if(is_one(common))common=den;
      else if(common!=den)return false;
    }
    numerators.push_back(num);denominators.push_back(den);
  }
  // Polynomial entries may be lifted to the shared denominator. Validate
  // sizes before constructing minors; none of these operations expands D.
  for(unsigned k=0;k<numerators.size();++k){
    if(is_one(denominators[k]) && !is_one(common))numerators[k]=numerators[k]*common;
    unsigned budget=128;equation_polynomial_budget bound;
    if(!determinant_polynomial_bound(numerators[k],budget,0,bound) || bound.degree>16 || bound.terms>16)return false;
  }
  bool rank_one=true;
  for(unsigned i=0;i<n && rank_one;++i)for(unsigned j=i+1;j<n && rank_one;++j)
    for(unsigned k=0;k<n && rank_one;++k)for(unsigned l=k+1;l<n && rank_one;++l){
      gen minor=numerators[i*n+k]*numerators[j*n+l]-numerators[i*n+l]*numerators[j*n+k];
      unsigned budget=256;equation_polynomial_budget bound;
      if(!determinant_polynomial_bound(minor,budget,0,bound) || !is_zero(determinant_polynomial_normal(minor,contextptr)))rank_one=false;
    }
  if(rank_one){
    gen trace=0;for(unsigned i=0;i<n;++i)trace+=numerators[i*n+i];
    result=1+(is_one(common)?trace:gen(symbolic(at_division,makesequence(trace,common))));
  }
  else {
    for(unsigned i=0;i<n;++i)numerators[i*n+i]+=common;
    vecteur partial(1u<<n,0);partial[0]=1;
    for(unsigned mask=0;mask<(1u<<n)-1;++mask){
      if(is_zero(partial[mask]))continue;
      unsigned row=0;for(unsigned bits=mask;bits;bits>>=1)row+=bits&1;
      for(unsigned j=0;j<n;++j){
        if(mask&(1u<<j))continue;
        const gen &entry=numerators[row*n+j];if(is_zero(entry))continue;
        gen term=is_one(partial[mask])?entry:gen(symbolic(at_prod,makesequence(partial[mask],entry)));
        bool negative=false;for(unsigned bits=mask>>(j+1);bits;bits>>=1)negative^=bool(bits&1);
        partial[mask|(1u<<j)]+=negative?-term:term;
      }
    }
    gen numerator=partial.back();
    // The fixed subset algorithm stores <=16 expressions. Keep its output
    // factored; do not invoke a gcd between det(N) and the common D^n.
    if(taille(numerator,2049)>2048){result=symbolic(at_det,input);return true;}
    result=is_one(common)?numerator:gen(symbolic(at_division,makesequence(numerator,pow(common,int(n)))));
  }
  // Even a nilpotent off-diagonal update can have a pole: a constant
  // determinant does not make the original matrix defined on that pole.
  if(!is_one(common) && !equation_rational(common))
    result=symbolic(at_when,makesequence(symb_equal(common,0),undef,result));
  return true;
}
}
#endif
