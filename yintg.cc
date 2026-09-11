// -*- mode:C++ ; compile-command: "g++ -I.. -g -c intg.cc -fno-strict-aliasing -DGIAC_GENERIC_CONSTANTS -DHAVE_CONFIG_H -DIN_GIAC " -*-
int confirm(const char * msg1,const char * msg2,bool acexit=false);
#include "giacPCH.h"
// #define LOGINT
#if defined(FXCG) || defined(KHICAS_TEST_INTEGRATION_LIMITS)
#include "integration_guard.h"
namespace giac { integration_guard *integration_guard::active_=0; }
#endif

/*
 *  Copyright (C) 2000,2014 B. Parisse, Institut Fourier, 38402 St Martin d'Heres
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
using namespace std;
#include <stdexcept>
#include "vector.h"
#include <cmath>
#include <cstdlib>
#include <limits>
#include "sym2poly.h"
#include "usual.h"
#include "intg.h"
#include "subst.h"
#include "derive.h"
#include "lin.h"
#include "vecteur.h"
#include "gausspol.h"
#include "plot.h"
#include "prog.h"
#include "modpoly.h"
#include "series.h"
#include "tex.h"
#include "ifactor.h"
#include "risch.h"
#include "solve.h"
#include "intgab.h"
#include "moyal.h"
#include "maple.h"
#include "rpn.h"
#include "modpoly.h"
#include "giacintl.h"
#include "dilogarithm.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_LIBGSL
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_psi.h>
#include <gsl/gsl_sf_zeta.h>
#include <gsl/gsl_odeiv.h>
#include <gsl/gsl_errno.h>
#endif

#if defined HAVE_LIBBERNMM && !defined BF2GMP_H
#include <bern_rat.h>
#endif

#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif // ndef NO_NAMESPACE_GIAC

  // Left redimension p to degree n, i.e. size n+1
  void lrdm(modpoly & p,int n){
    int s=int(p.size());
    if (n+1>s)
      p=mergevecteur(vecteur(n+1-s),p);
  }

  struct pf1 {
    vecteur num;
    vecteur den;
    vecteur fact;
    int mult; // den=cste*fact^mult
    pf1():num(0),den(makevecteur(1)),fact(makevecteur(1)),mult(1) {}
    pf1(const pf1 & a) : num(a.num),  den(a.den), fact(a.fact),mult(a.mult) {}
    pf1(const vecteur &n, const vecteur & d, const vecteur & f,int m) : num(n), den(d), fact(f), mult(m) {};
    pf1(const polynome & n,const polynome & d,const polynome & f,int m): num(polynome2poly1(n,1)),den(polynome2poly1(d,1)),fact(polynome2poly1(f,1)),mult(m) {}
  };

  gen complex_subst(const gen & e,const vecteur & substin,const vecteur & substout,GIAC_CONTEXT){
    bool save_complex_mode=complex_mode(contextptr);
    complex_mode(true,contextptr);
    bool save_eval_abs=eval_abs(contextptr);
    eval_abs(false,contextptr);
    gen res=simplifier(eval(subst(e,substin,substout,false,contextptr),1,contextptr),contextptr); 
    // eval is used since after subst * are not flattened
    complex_mode(save_complex_mode,contextptr);
    eval_abs(save_eval_abs,contextptr);
    return res;
  }

  gen complex_subst(const gen & e,const gen & x,const gen & newx,GIAC_CONTEXT){
    bool save_complex_mode=complex_mode(contextptr);
    complex_mode(true,contextptr);
    bool save_eval_abs=eval_abs(contextptr);
    eval_abs(false,contextptr);
    gen res=subst(e,x,newx,false,contextptr);
    eval_abs(save_eval_abs,contextptr);
    // avoid rewrite of fractional powers
    vecteur v=lop(newx,at_pow);
    int i=0;
    for (;i<v.size();++i){
      gen tmp=v[i];
      if (tmp.is_symb_of_sommet(at_pow)){
	tmp=tmp._SYMBptr->feuille;
	if (tmp.type==_VECT && tmp._VECTptr->size()==2){
	  tmp=tmp._VECTptr->back();
	  if (tmp.type==_FRAC && tmp._FRACptr->den.type==_INT_ ){
	    tmp=tmp._FRACptr->den;
	    if (tmp.val % 2==1)
	      break;
	  }
	}
      }
    }				 
    complex_mode(save_complex_mode,contextptr);
    if (i==v.size()) 
      res=eval(res,1,contextptr);
    return res;
  }

  static bool has_nop_var(const vecteur & v){
    const_iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it){
      if (contains(*it,at_nop))
	return true;
    }
    return false;
  }

  static gen nop_inv(const gen & e,GIAC_CONTEXT){
    return symbolic(at_nop,gen(symbolic(at_inv,e)));
  }
  static gen nop_pow(const gen & e,GIAC_CONTEXT){
    if ( (e.type!=_VECT) || (e._VECTptr->size()!=2))
      return symbolic(at_pow,e);
    if ( (e._VECTptr->back().type!=_INT_) || (e._VECTptr->back().val>=0) || ( (e._VECTptr->front().type==_SYMB) && (e._VECTptr->front()._SYMBptr->sommet==at_exp) ) )
      return symbolic(at_pow,change_subtype(e,_SEQ__VECT));
    return nop_inv(symbolic(at_pow,gen(makevecteur(e._VECTptr->front(),-e._VECTptr->back()),_SEQ__VECT)),contextptr);
  }

  static gen sin_over_cos(const gen & e,GIAC_CONTEXT){
    return rdiv(symb_sin(e),symb_cos(e),contextptr);
  }
  const gen_op_context invpowtan2_tab[]={nop_inv,nop_pow,sin_over_cos,0};
  // remove nop if nop() does not contain x
  gen remove_nop(const gen & g,const gen & x,GIAC_CONTEXT){
    if (g.type==_VECT){
      vecteur res(*g._VECTptr);
      iterateur it=res.begin(),itend=res.end();
      for (;it!=itend;++it){
	*it=remove_nop(*it,x,contextptr);
      }
      return gen(res,g.subtype);
    }
    if (g.type!=_SYMB)
      return g;
    if (g._SYMBptr->sommet!=at_nop)
      return symbolic(g._SYMBptr->sommet,remove_nop(g._SYMBptr->feuille,x,contextptr));
    if (is_zero(derive(g._SYMBptr->feuille,x,contextptr)))
      return g._SYMBptr->feuille;
    else
      return g;
  }
  vecteur lvarxwithinv(const gen &e,const gen & x,GIAC_CONTEXT){
    gen ee=subst(e,invpowtan_tab,invpowtan2_tab,false,contextptr);
    ee=remove_nop(ee,x,contextptr);
    vecteur v(lvarx(ee,x));
    return v; // to remove nop do a return *(eval(v)._VECTptr);
  }

  bool is_constant_wrt(const gen & e,const gen & x,GIAC_CONTEXT){
    if (e.type==_VECT){
      const_iterateur it=e._VECTptr->begin(),itend=e._VECTptr->end();
      for (;it!=itend;++it){
	if (!is_constant_wrt(*it,x,contextptr))
	  return false;
      }
      return true;
    }
    if (e==x)
      return false;
    if (e.type!=_SYMB)
      return true;
    return is_exactly_zero(derive(e,x,contextptr)); 
  }

  // return true if e=a*x+b
  bool is_linear_wrt(const gen & e,const gen &x,gen & a,gen & b,GIAC_CONTEXT){
    a=derive(e,x,contextptr);
    if (is_undef(a) || !is_constant_wrt(a,x,contextptr))
      return false;
    if (x*a==e)
      b=0;
    else
      b=ratnormal(e-a*x,contextptr);
    return lvarx(b,x).empty();
  }

  // return true if e=a*x+b
  bool is_quadratic_wrt(const gen & e,const gen &x,gen & a,gen & b,gen & c,GIAC_CONTEXT){
    gen tmp=derive(e,x,contextptr);
    if (is_undef(tmp) || !is_linear_wrt(tmp,x,a,b,contextptr))
      return false;
    a=ratnormal(rdiv(a,plus_two,contextptr),contextptr);
    c=ratnormal(e-a*x*x-b*x,contextptr);
    return true;
  }

  void decompose_plus(const vecteur & arg,const gen & x,vecteur & non_constant,gen & plus_constant,GIAC_CONTEXT){
    non_constant.clear();
    plus_constant=zero;
    const_iterateur it=arg.begin(),itend=arg.end();
    for (;it!=itend;++it){
      if (is_constant_wrt(*it,x,contextptr))
	plus_constant=plus_constant+(*it);
      else
	non_constant.push_back(*it);
    }
    // if (contains(plus_constant,x)) plus_constant=ratnormal(plus_constant,contextptr);
  }

  void decompose_prod(const vecteur & arg,const gen & x,vecteur & non_constant,gen & prod_constant,bool signcst,GIAC_CONTEXT){
    non_constant.clear();
    prod_constant=plus_one;
    const_iterateur it=arg.begin(),itend=arg.end();
    for (;it!=itend;++it){
      gen tst=*it;
      if (!signcst && it->is_symb_of_sommet(at_sign))
	tst=it->_SYMBptr->feuille;
      if (is_constant_wrt(tst,x,contextptr))
	prod_constant=prod_constant*(*it);
      else
	non_constant.push_back(*it);
    }
    // if (contains(prod_constant,x)) prod_constant=ratnormal(prod_constant,contextptr);
  }

  gen extract_cst(gen & u,const gen & x,GIAC_CONTEXT){
    if (!u.is_symb_of_sommet(at_prod) || u._SYMBptr->feuille.type!=_VECT)
      return 1;
    vecteur non_constant; gen prod_constant=1;
    decompose_prod(*u._SYMBptr->feuille._VECTptr,x,non_constant,prod_constant,false,contextptr);
    if (non_constant.size()==0)
      u=1;
    if (non_constant.size()==1)
      u=non_constant.front();
    if (non_constant.size()>1)
      u=symbolic(at_prod,gen(non_constant,_SEQ__VECT));
    return prod_constant;
  }

  // applies linearity of f. + & neg are distributed as well as * with respect
  // to terms that are constant w.r.t. x
  // e is assumed to be a scalar
  gen linear_apply(const gen & e,const gen & x,gen & remains, int intmode,GIAC_CONTEXT, gen (* f)(const gen &,const gen &,gen &,int,const context *)){
    if (is_constant_wrt(e,x,contextptr) || (e==x) )
      return f(e,x,remains,intmode,contextptr);
    // e must be of type _SYMB
    if (e.type==_VECT){
      vecteur v(*e._VECTptr);
      vecteur r(v.size());
      for (unsigned i=0;i<v.size();++i){
	v[i]=linear_apply(v[i],x,r[i],intmode,contextptr,f);
      }
      remains=r;
      return gen(v,e.subtype);
    }
    if (e.type!=_SYMB) return gensizeerr(gettext("in linear_apply"));
    unary_function_ptr u(e._SYMBptr->sommet);
    gen arg(e._SYMBptr->feuille);
    gen res;
    if (u==at_neg){
      res=-linear_apply(arg,x,remains,intmode,contextptr,f);
      remains=-remains;
      return res;
    } // end at_neg
    if (u==at_plus){
      if (arg.type!=_VECT)
	return linear_apply(arg,x,remains,intmode,contextptr,f);
      const_iterateur it=arg._VECTptr->begin(),itend=arg._VECTptr->end();
      for (gen tmp;it!=itend;++it){
	res = res + linear_apply(*it,x,tmp,intmode,contextptr,f);
	remains =remains + tmp;
      }
      return res;
    } // end at_plus
    if (u==at_prod){
      if (arg.type!=_VECT)
	return linear_apply(arg,x,remains,intmode,contextptr,f);
      // find all constant terms in the product
      vecteur non_constant;
      gen prod_constant;
      decompose_prod(*arg._VECTptr,x,non_constant,prod_constant,false,contextptr);
      if (non_constant.empty()) return gensizeerr(gettext("in linear_apply 2")); // otherwise the product would be constant
      if (non_constant.size()==1)
	res = linear_apply(non_constant.front(),x,remains,intmode,contextptr,f);
      else
	res = f(symbolic(at_prod,gen(non_constant,_SEQ__VECT)),x,remains,intmode,contextptr);
      remains = prod_constant * remains;
      return prod_constant * res;
    } // end at_prod
    return f(e,x,remains,intmode,contextptr);
  }

  gen lnabs(const gen & x,GIAC_CONTEXT){
    bool _lnabs=do_lnabs(contextptr);
    if (!complex_mode(contextptr) && _lnabs && !has_i(x))
      return ln(abs(x,contextptr),contextptr);
    else
      return ln(x,contextptr);
  }

  gen lnabs2(const gen & x,const gen & xvar,GIAC_CONTEXT){
    if (xvar.type!=_IDNT)
      return lnabs(x,contextptr);
    bool _lnabs=do_lnabs(contextptr);
    if (!complex_mode(contextptr) && _lnabs && !has_i(x)){
      return symbolic(at_ln,symbolic(at_abs,x));
    }
    else {
      if (is_positive(-x,contextptr))
	return symbolic(at_ln,-x);
      return symbolic(at_ln,x);
    }
  }

  static gen normal_norootof(const gen & g,GIAC_CONTEXT){
    gen res=normal(g,contextptr);
    if (!lop(res,at_rootof).empty())
      res=ratnormal(normalize_sqrt(g,contextptr),contextptr);
    return res;
  }

  // eval N at X=e with e=x*exp(i*dephasage*pi/n) and returns N*ln(X-e)+conj
  static gen substconj_(const gen & N,const gen & X,const gen & x,const gen & dephasage_,bool residue_only,GIAC_CONTEXT){
    int mode=angle_mode(contextptr);
    gen pi=cst_pi;
    gen dephasage(dephasage_);
    if (mode==1){
      dephasage=ratnormal(gen(180)/cst_pi*dephasage,contextptr);
      pi=180;
    }
    if (mode==2){
      dephasage=ratnormal(gen(200)/cst_pi*dephasage,contextptr);
      pi=200;
    }
    gen c=cos(dephasage,contextptr);
    gen s=sin(dephasage,contextptr);
    if (c.is_symb_of_sommet(at_cos) && c._SYMBptr->feuille==dephasage){
      gen c2=cos(ratnormal(2*dephasage,contextptr),contextptr);
      if (!c2.is_symb_of_sommet(at_cos)){
	c=sign(c,contextptr)*sqrt((1+c2)/2,contextptr);
	s=sign(s,contextptr)*sqrt((1-c2)/2,contextptr);
      }
    }
    gen e=x*(c+cst_i*s);
    gen b=subst(N,X,e,false,contextptr),rb,ib;
    reim(b,rb,ib,contextptr);
    gen N2=normal_norootof(-2*ib,contextptr); // same
    if (residue_only)
      return N2*sign(s*x,contextptr);
    gen res=normal_norootof(rb,contextptr)*symbolic(at_ln,pow(X,2)+ratnormal(-2*c*x,contextptr)*X+x.squarenorm(contextptr)); 
    gen atanterm=pi/cst_pi*symbolic(at_atan,(X-c*x)/(s*x));
    if (X.is_symb_of_sommet(at_tan))
      atanterm += pi*sign(s*x,contextptr)*symbolic(at_floor,X._SYMBptr->feuille/pi+plus_one_half);
    res=res+N2*atanterm;
    return res;
  }

  static gen substconj(const gen & N,const gen & X,const gen & x,const gen & dephasage,bool residue_only,GIAC_CONTEXT){
    if (has_i(N)){
      gen Nr,Ni;
      reim(N,Nr,Ni,contextptr);
      return substconj_(Nr,X,x,dephasage,residue_only,contextptr)+cst_i*substconj_(Ni,X,x,dephasage,residue_only,contextptr);
    }
    return substconj_(N,X,x,dephasage,residue_only,contextptr);
  }

  gen surd(const gen & c,int n,GIAC_CONTEXT){
    if (is_exactly_zero(c))
      return c;
    if (n%2 && is_positive(-c,contextptr)){
      if (c.type==_FLOAT_)
	return -exp(ln(-c,contextptr)/n,contextptr);
      return -pow(-c,inv(n,contextptr),contextptr);
    }
    else {
      if (c.type==_FLOAT_)
	return exp(ln(c,contextptr)/n,contextptr);
      return pow(c,inv(n,contextptr),contextptr);
    }
  }

  gen _surd(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type!=_VECT || args._VECTptr->size()!=2)
      return gensizeerr(contextptr);
    gen a=args._VECTptr->front(),aa,b=args._VECTptr->back(),c;
    if (a.is_symb_of_sommet(at_abs) || a.is_symb_of_sommet(at_exp))
      return pow(a,inv(b,contextptr),contextptr);
    if (is_equal(a)){
      gen a0=a._SYMBptr->feuille[0],a1=a._SYMBptr->feuille[1];
      return symbolic(at_equal,makesequence(_surd(makesequence(a0,b),contextptr),_surd(makesequence(a1,b),contextptr)));
    }
    if (is_undef(a)) return a;
    if (is_undef(b)) return b;
    if (is_inf(b)){
      if (is_inf(a) || is_zero(a))
	return undef;
      return 1;
    }
    if (is_zero(b))
      return undef;
    if (is_inf(a)){
      if (a==minus_inf && is_integral(b) && b.type==_INT_ && b.val%2)
        return -pow(plus_inf,inv(b,contextptr),contextptr);
      return pow(a,inv(b,contextptr),contextptr);
    }
    c=_floor(b,contextptr);
    if (c.type==_FLOAT_)
      c=get_int(c._FLOAT_val);
    if (!has_evalf(a,aa,1,contextptr)){
      if (c.type==_INT_ && c==b && (c.val %2 ==0 || (a.is_symb_of_sommet(at_pow) && a._SYMBptr->feuille[1].type==_INT_ && a._SYMBptr->feuille[1].val % c.val==0)) )
	return pow(a,inv(c,contextptr),contextptr);	
      return symbolic(at_NTHROOT,gen(makevecteur(b,a),_SEQ__VECT));
    }
    if (c.type==_INT_ && c==b)
      return surd(a,c.val,contextptr);
    else
      return pow(a,inv(b,contextptr),contextptr);
  }
  static const char _surd_s []="surd";
  static define_unary_function_eval (__surd,&_surd,_surd_s);
  define_unary_function_ptr5( at_surd ,alias_at_surd,&__surd,0,true);

  static gen makelnatan(const gen & N,const gen & X,const gen & c0,int n,bool residue_only,GIAC_CONTEXT){
    gen c(c0),res(0);
    if (n%2){
      if (is_positive(-c,contextptr))
	c=-pow(-c,inv(n,contextptr),contextptr);
      else
	c=pow(c,inv(n,contextptr),contextptr);
      if (!residue_only)
	res += subst(N,X,c,false,contextptr)*lnabs(X-c,contextptr);
      for (int i=1;i<=n/2;++i)
	res += substconj(N,X,c,gen(2*i)/n*cst_pi,residue_only,contextptr);
      return res;
    }
    if (is_positive(c,contextptr) ){
      if (n==2) 
	c=sqrt(c,contextptr);
      else
	c=pow(c,inv(n,contextptr),contextptr);
      if (!residue_only)
	res += normal_norootof(subst(N,X,c,false,contextptr),contextptr)*lnabs2(X-c,X,contextptr)+normal_norootof(subst(N,X,-c,false,contextptr),contextptr)*lnabs2(X+c,X,contextptr);
      for (int i=1;i<n/2;++i)
	res += substconj(N,X,c,gen(2*i)/n*cst_pi,residue_only,contextptr);
    }
    else {
      if (n==2) 
	c=sqrt(-c,contextptr);
      else
	c=pow(-c,inv(n,contextptr),contextptr);
      for (int i=0;i<n/2;++i)
	res += substconj(N,X,c,gen(2*i+1)/n*cst_pi,residue_only,contextptr);
    }
    return res;
  }

  gen symb_atan(const polynome & d_,const polynome & a_,const vecteur & l,GIAC_CONTEXT){
    polynome d(d_),a(a_);
    simplify(d,a);
    if (a.coord.empty())
      return 0;
    gen D=r2e(d,l,contextptr);
    if (is_positive(-D*a.coord.front().value,contextptr))
      return -symb_atan(r2e(-a,l,contextptr)/D);
    return symb_atan(r2e(a,l,contextptr)/D);
  }

  // im(ln(a+i*b)), a and b polynomials rewritten as sum of atan without denominators
  // im(ln(a+i*b)+ln(u-i*v))=im(ln(a*u+b*v)+i(b*u-a*v))
  gen ln2sumatan(const polynome & a,const polynome & b,const vecteur & l,GIAC_CONTEXT){
    if (a.lexsorted_degree()>b.lexsorted_degree())
      return -ln2sumatan(b,a,l,contextptr);
    polynome u,v,d;
    egcd(a,b,u,v,d);
    if (v.coord.empty()){ // a divides b
      return symb_atan(a,b,l,contextptr);
    }
    gen tmp=-ln2sumatan(v,u,l,contextptr);
    tmp += symb_atan(d,b*u-a*v,l,contextptr);
    return tmp;
  }
  gen ln2sumatan(const gen & a,const gen & b,const vecteur & l,GIAC_CONTEXT){
    //return symb_atan(b/a);
    gen A=e2r(a,l,contextptr),An,Ad;
    gen B=e2r(b,l,contextptr),Bn,Bd;
    fxnd(A,An,Ad);
    fxnd(B,Bn,Bd);
    An=Bd*An;
    Bn=Ad*Bn;
    if (An.type==_POLY && Bn.type==_POLY)
      return ln2sumatan(*An._POLYptr,*Bn._POLYptr,l,contextptr);
    if (Bn.type!=_POLY)
      return -symb_atan(a/b);
    return symb_atan(b/a);
  }

  static bool integrate_rothstein_trager(const polynome & num,const vecteur & v,const vecteur & l,const gen & X,gen & res,int intmode,GIAC_CONTEXT){
    // Improve: csolve for resultant(num-t*v',v)
    // Example a:=diff(atan((x^2-2x)/(x-1))); b:=int(a);
    // v=[1,-4,5,-2,1], roots for resultant +/-i/2
    // sum t*ln(gcd(n-t*d',d))
    // if t is complex and v real
    // t*ln()+conjugate=re(t)*ln(|gcd|^2)-im(t)*atan(im(gcd)/re(gcd))
    gen N=r2e(num,l,contextptr);
    vecteur Nv(lvar(N));
    if (1 || Nv==vecteur(1,X)){ // [commented: do it for univariate only]
      gen D=r2e(poly12polynome(v,1),l,contextptr),resadd;
#if 0
      gen Dprime=r2e(poly12polynome(derivative(v),1),l,contextptr);
      gen Dc=1;
#else
      gen Dc=_content(makesequence(D,X),contextptr);
      D=_quo(makesequence(D,Dc,X),contextptr);
      gen Dprime=derive(D,X,contextptr);
#endif
      int Ddeg=v.size()-1;
      gen tres(identificateur("tresultant"));
      gen R=_resultant(makesequence(N-tres*Dprime,D,X),contextptr);
      gen Rprime=derive(R,tres,contextptr);
      R=_quo(makesequence(R,gcd(R,Rprime,contextptr),tres),contextptr);
      gen Rdeg=_degree(makesequence(R,tres),contextptr);
      if (Rdeg.type==_INT_ && Rdeg.val==Ddeg){
	// it's easier to extract the roots of D
	gen racines=solve(D,X,1,contextptr);
	if (!has_i(racines) && racines.type==_VECT && racines._VECTptr->size()==Ddeg){
	  // apply sum_racines N/D'(racine)*log(x-racine)
	  gen ND=N/Dprime;
	  for (int i=0;i<Ddeg;++i){
	    gen racine=racines[i];
	    //if (has_op(normal(racine,contextptr),*at_rootof)) return false;
	    gen coeff=subst(ND,X,racine,false,contextptr);
	    resadd += coeff*symb_ln(X-racine);
	  }
	  res += resadd/Dc;
	  return true;
	}
      } // end Rdeg==Ddeg
      gen Rt=solve(R,tres,1,contextptr); // _cSolve(makesequence(R,tres),contextptr);
      if (Rdeg.type==_INT_ && Rt.type==_VECT && Rt._VECTptr->size()==Rdeg.val){
	vecteur w=*Rt._VECTptr;
	bool reel=vect_is_real(v,contextptr);
	if (!has_num_coeff(w)){
	  for (size_t wi=0;wi<w.size();++wi){
	    gen racine=w[wi],racinen=normal(racine,contextptr);
	    if (has_op(racinen,*at_rootof))
	      return false;
	    gen G;
#ifndef NO_STDEXCEPT
	    try {
#endif
	      G=_numer(gcd(N-racine*Dprime,D,contextptr),contextptr);
#ifndef NO_STDEXCEPT
	    }
	    catch (std::runtime_error & err){
	      return false;
	    }
#endif
	    if (reel){
	      gen racr,raci;
	      reim(racine,racr,raci,contextptr);
	      if (is_zero(raci,contextptr))
		resadd += racine*symb_ln(symb_abs(G));
	      else {
		// search conjugate
		size_t wj=wi+1; gen cwi=conj(w[wi],contextptr);
		for (;wj<w.size();++wj){
		  if (is_zero(ratnormal(cwi-w[wj],contextptr)))
		    break;
		}
		if (wj<w.size()){
		  gen gcdr,gcdi;
		  reim(G,gcdr,gcdi,contextptr);
		  resadd += racr*symb_ln(gcdr*gcdr+gcdi*gcdi)-2*raci*ln2sumatan(gcdr,gcdi,l,contextptr);
		  w.erase(w.begin()+wj);
		}
		else
		  resadd += racine*symb_ln(G);
	      }
	    }
	    else {
	      resadd += racine*symb_ln(G);
	    }
	  }
	  res += resadd/Dc;
	  return true;
	}
      }
    }
    return false;
  }

  // integration of cyclotomic-type denominators
  static bool integrate_deno_length_2(const polynome & num,const vecteur & v,const vecteur & l,const vecteur & lprime,gen & res,bool residue_only,int intmode,GIAC_CONTEXT){
    if (v.size()<2)
      return false;
    const_iterateur it=v.begin()+1,itend=v.end()-1;
    for (;it!=itend;++it){
      if (!is_zero(*it))
	break;
    }
    int n=int(v.size())-1,d=int(it-v.begin()),deg;
    gen X=l.front();
    if (X.type==_VECT)
      X=X._VECTptr->front();
    gen a=r2e(v.front(),lprime,contextptr);
    gen b=r2e(v.back(),lprime,contextptr);
    // check for deno of type a*x^2n + A*x^n + b
    // FIXME: improve some simplifications of sin/cos(asin()/k) and remove test d==2
    if (d==2 && 2*d==n){
      ++it;
      for (;it!=itend;++it){
	if (!is_zero(*it))
	  break;
      }
      if (it==itend){ // ok!
	gen c=b;
	b=r2e(v[d],lprime,contextptr);
	gen delta=b*b-4*a*c;
	if (is_zero(delta)) // if (is_positive(-delta,contextptr)) 
	  return false;
	if ( (intmode &2)==0)
	  gprintf(step_ratfrac,gettext("Integration of a rational fraction with denominator %gen\nroots are obtained by solving the 2nd order equation %gen=0 then extracting nth-roots"),makevecteur(a*symb_pow(vx_var,2*n)+b*symb_pow(vx_var,n)+c,a*symb_pow(vx_var,2)+b*vx_var+c),contextptr);
	// int(num/(a*X^2d+b*X^d+c),X) = 
	// sum(x=rootof(deno),num*x/(+/-d*sqrt(delta))*ln(X-x))
	gen sqrtdelta=sqrt(delta,contextptr);
	gen c1=(-b-sqrtdelta)/2/a;
	gen c2=(-b+sqrtdelta)/2/a;
	gen N=r2e(num,l,contextptr)*X/d/sqrtdelta;
	if (is_zero(im(a,contextptr)) && is_zero(im(b,contextptr)) && is_zero(im(c,contextptr))){
	  if (!is_positive(-delta,contextptr)){
	    res += makelnatan(N/c2,X,c2,d,residue_only,contextptr);
	    res -= makelnatan(N/c1,X,c1,d,residue_only,contextptr);
	    return true;
	  }
	  else {
	    gen module=sqrt(c/a,contextptr);
	    gen argument=acos(normal(-b/a/2/module,contextptr),contextptr);
	    // roots are module^(1/d)*exp(i*argument/d)*exp(2*i*pi*k/d)
	    // for k=0..d-1 and conjugates
	    gen moduled=pow(c/a,inv(n,contextptr),contextptr);
	    for (int i=0;i<d;++i)
	      res += substconj_(N/c2,X,moduled,(argument+2*i*cst_pi)/d,residue_only,contextptr);
	  }
	  return true;
	}
	if (residue_only)
	  return true;
	gen c1s=surd(c1,d,contextptr);
	gen c2s=surd(c2,d,contextptr);
	for (int i=0;i<d;++i){
	  gen x=c1s*exp((2*i*cst_i*cst_pi)/d,contextptr);
	  res -= normal(subst(N,X,x,false,contextptr)/c1,contextptr)*ln(X-x,contextptr);
	  x=c2s*exp((2*i*cst_i*cst_pi)/d,contextptr);
	  res += normal(subst(N,X,x,false,contextptr)/c2,contextptr)*ln(X-x,contextptr);
	}
	return true;
      }
    } // end if d==2 and n==2d
    gen c=normal(-b/a,contextptr);
    if (n%d)
      return residue_only?false:integrate_rothstein_trager(num,v,l,X,res,intmode,contextptr);
    if (d!=n){ 
      // rescale and check cyclotomic
      gen tw=v.back()/pow(*it/v.front(),n/d);
      if (tw.type!=_INT_ && tw.type!=_POLY)
	return residue_only?false:integrate_rothstein_trager(num,v,l,X,res,intmode,contextptr);
      tw=r2e(v/v.front(),lprime,contextptr);
      if (tw.type!=_VECT)
	return residue_only?false:integrate_rothstein_trager(num,v,l,X,res,intmode,contextptr);
      vecteur w=*tw._VECTptr;
      vecteur w_copy=w;
      c=pow(r2e(*it/v.front(),lprime,contextptr),inv(d,contextptr),contextptr);
      iterateur jt=w.begin()+1,jtend=w.end();
      for (int k=1;jt!=jtend;++jt,++k){
	*jt=normal(*jt * pow(c,-k),contextptr);
	if (jt->type!=_INT_ && jt->type!=_POLY)
	  break;
      }
      deg=is_cyclotomic(w,epsilon(contextptr));
      if (!deg){
	w=w_copy;
	c=pow(r2e(-*it/v.front(),lprime,contextptr),inv(d,contextptr),contextptr);
	jt=w.begin()+1,jtend=w.end();
	for (int k=1;jt!=jtend;++jt,++k){
	  *jt=normal(*jt * pow(c,-k),contextptr);
	  if (jt->type!=_INT_ && jt->type!=_POLY)
	    break;
	}
	deg=is_cyclotomic(w,epsilon(contextptr));
      }
      if (!deg)
	return residue_only?false:integrate_rothstein_trager(num,v,l,X,res,intmode,contextptr);
      if ( (intmode &2)==0)
	gprintf(step_cyclotomic,gettext("Integrate rational fraction with denominator a cyclotomic polynomial, roots are primitive roots of %gen=0"),makevecteur(a*symb_pow(vx_var,deg)+b),contextptr);
      // int(num/(a*X^n+b),X)=sum(x=rootof(-b/a),num*x/(-n*b)*ln(X-x))
      vecteur vprime=derivative(v),V,Vprime,d;
      egcd(v,vprime,0,V,Vprime,d);
      if (d.size()!=1)
	return residue_only?false:integrate_rothstein_trager(num,v,l,X,res,intmode,contextptr);
      gen dd=d.front();
      // 1/vprime=Vprime/d      
      gen N=normal(_quorem(makesequence(r2e(num,l,contextptr)*horner(r2e(Vprime,lprime,contextptr),X),horner(r2e(v,lprime,contextptr),X),X),contextptr)[1]/r2e(dd,lprime,contextptr),contextptr);
      if (complex_mode(contextptr) && !residue_only){
	for (int i=1;i<deg;++i){
	  if (gcd(i,deg)!=1)
	    continue;
	  gen x=c*exp((2*i*cst_i*cst_pi)/deg,contextptr);
	  res += normal(subst(N,X,x,false,contextptr),contextptr)*ln(X-x,contextptr);
	}
      }
      else {
	for (int i=1;i<=deg/2;++i){
	  if (gcd(i,deg)!=1)
	    continue;
	  res += substconj(N,X,c,2*i*cst_pi/deg,residue_only,contextptr);
	}
      }
      return true;
    } // if (d!=n)
    else {
      if ( (intmode &2)==0)
	gprintf(step_nthroot,gettext("Integrate rational fraction with denominator %gen=0\nroots are deduced from nth-roots of unity"),makevecteur(a*symb_pow(vx_var,n)+b),contextptr);
      // int(num/(a*X^n+b),X)=sum(x=rootof(-b/a),num*x/(-n*b)*ln(X-x))
      gen N=r2e(num,l,contextptr)*X/(r2e(-n*b,l,contextptr));
      if (complex_mode(contextptr) && !residue_only){
	c=pow(c,inv(n,contextptr),contextptr);
	for (int i=0;i<n;++i){
	  gen x=c*exp((2*i*cst_i*cst_pi)/n,contextptr);
	  res += normal(subst(N,X,x,false,contextptr),contextptr)*ln(X-x,contextptr);
	}
	return true;
      }
      res += makelnatan(N,X,c,n,residue_only,contextptr);
      return true;
    }
  }

  // tests if v is symmetric or antisymmetric
  // if it is, compute res such that res[t+-1/t]=v/t^[deg(v)/2]
  static int is_symmetric(const vecteur & v,vecteur & res,bool sym){
    if (v.empty())
      return 0;
    int n=int(v.size());
    vecteur w;
    if (n%2)
      w=v;
    else {
      if (!is_zero(v[n-1]))
	return 0;
      w=vecteur(v.begin(),v.end()-1);
      --n;
    }
    vecteur w1(w);
    reverse(w1.begin(),w1.end());
    if (!sym){
      for (int i=1;i<n;i+=2){
	w1[i] = -w1[i];
      }
    }
    int rescoeff=0;
    if (w==w1)
      rescoeff=1;
    if (w==-w1)
      rescoeff=-1;
    if (!rescoeff)
      return 0;
    // if antisym, n/2 is the number of power of (t^2-1), check if it is odd
    if (!sym && (n/2)%2)
      rescoeff = -rescoeff;
    vecteur test(makevecteur(1,0,sym?1:-1)),q,r;
    res.clear();
    for (n/=2;n>0;n--){
      DivRem(w,powmod(test,n,0,0),0,q,r);
      if (q.size()>1)
	return 0;
      w=r.empty()?r:vecteur(r.begin(),r.end()-1);
      res.push_back(q.empty()?0:q.front());
    }
    if (w.empty())
      res.push_back(0); // was return 0;
    else
      res.push_back(w.front());
    return rescoeff;
  }

  // n/d(x) -> newn/newd(t) with x=a/t, 
  // if dx is true multiplies by dx/dt=-a/t^2
  static void xtoinvx(const gen & a,const modpoly & n,const modpoly & d,modpoly & newn, modpoly & newd,bool dx){
    int ns=int(n.size()); int nd=int(d.size());
    newn=vecteur(ns); newd=vecteur(nd);
    gen ad(1);
    for (int i=ns-1;i>=0;--i){
      newn[ns-1-i]=ad*n[i];
      ad = ad*a;
    }
    ad=1;
    for (int i=nd-1;i>=0;--i){
      newd[nd-1-i]=ad*d[i];
      ad = ad*a;
    }
    if (dx){
      newn=operator_times(-a,newn,0);
      ns+=2;
    }
    trim(newn,0);
    trim(newd,0);
    for (;ns>nd;--ns){
      newd.push_back(0);
    }
    for (;nd>ns;--nd){
      newn.push_back(0);
    }
  }

  static gen integrate_rational(const gen & e, const gen & x, gen & remains_to_integrate,gen & xvar,int intmode,GIAC_CONTEXT);

  static void solve_aPprime_plus_P(const gen & anum,const gen & aden,const vecteur & Q,vecteur & R,gen & Pden){
    // a P+P'=Q, a=anum/aden, on cherche P sous la forme R/Pden
    // On a (k+1)p_(k+1)+ anum/aden*p_k=q_k
    // Donc p_k=aden/anum*(q_k-(k+1)p_(k+1))
    // n=deg[Q], on a donc Pden=anum^(n+1), puis on cherche R=P*anum^(n+1)
    // on multiplie donc Q par anum^(n+1) S=Q*anum^(n+1)/a
    // on a aR+R'=aS
    // on a donc par ordre decr. r_(n+1)=0 
    // r_k= s_k - (k+1)*r_(k+1)/a
    // avec des divisions sans creation de denominateurs
    // par ex. P'+3P=x^2+5x+7 -> r_2=9, r_1=39, r_0=50, a=3, n=2, a^n=9
    R.clear();
    if (Q.empty()){
      Pden=plus_one;
      return;
    }
    int n=int(Q.size())-1;
    R.reserve(n+1);
    Pden=pow(anum,n);
    vecteur S;
    multvecteur(Pden*aden,Q,S);
    Pden=Pden*anum;
    const_iterateur it=S.begin(),itend=S.end();
    R.push_back(*it);
    ++it;
    for (int k=n-1;it!=itend;++it,--k){
      R.push_back(*it-rdiv(gen(k+1)*R.back()*aden,anum,context0));
    }
    // should simplify R with Pden
  }

  static gen integrate_linearizable(const gen & e,const gen & gen_x,gen & remains_to_integrate,int intmode,bool do_risch,GIAC_CONTEXT){
    // exp linearization
    vecteur vexp;
    gen res;
    const identificateur & id_x=*gen_x._IDNTptr;
    lin(e,vexp,contextptr); // vexp = coeff, arg of exponential
    if ( (intmode &2)==0 ){
      gen tmp=unlin(vexp,contextptr);
      if (vexp.size()>2 || !is_zero(ratnormal(tmp-e,contextptr)))
	gprintf(step_linearizable,gettext("Integrate linearizable expression %gen -> %gen"),makevecteur(e,tmp),contextptr);
    }
    const_iterateur it=vexp.begin(),itend=vexp.end();
    for (;it!=itend;){
      // trig linearization
      vecteur vtrig;
      gen coeff=*it;
      ++it; // it -> on the arg of the exp that must be linear
      gen rex2,rea,reb,reaxb=*it;
      ++it;
      if (!is_quadratic_wrt(reaxb,gen_x,rex2,rea,reb,contextptr)){
	// IMPROVE using int(exp(-x^a))=1/a*igamma(1/a,x^a)
	vecteur lv=lvarxwithinv(makevecteur(reaxb,coeff),gen_x,contextptr);
	if (lv.size()==1){
	  gen C=_coeff(makesequence(reaxb,gen_x),contextptr);
	  if (C.type==_VECT && C._VECTptr->size()>2){
	    vecteur Cv=*C._VECTptr;
	    int n=int(Cv.size())-1;
	    gen c=Cv[0];
	    gen a=-Cv[1]/(n*c);
	    // must be c*(x-a)^n
	    if (C==_coeff(makesequence(c*pow(gen_x-a,n,contextptr),gen_x),contextptr) && ((n%2) || is_positive(-c,contextptr))){
	      // c=surd(c,n,contextptr);
	      C=_coeff(makesequence(coeff,gen_x),contextptr);
	      C=_ptayl(makesequence(C,a,gen_x),contextptr);
	      if (C.type==_VECT){
		c=-c;
		gen ca=surd(c,n,contextptr);
		Cv=*C._VECTptr;
		int m=int(Cv.size())-1;
		gen ires=0;
		// 1/n*igamma(1/n+b/n,c*x^n)'=x^b*exp(-c*x^n)*c^(b+1)/n
		for (int b=0;b<=m;++b){
		  ires += Cv[m-b]*_lower_incomplete_gamma(makesequence(gen(b+1)/gen(n),c*pow(gen_x-a,n)),contextptr)/pow(ca,b+1,contextptr);
		}
		if (n%2==0){		  
		  ires=ires*abs(gen_x,contextptr)/gen_x; // sign(gen_x,contextptr);
		}
		ires=ires/n;
		res += ires;
		continue;
	      }
	    }
	  }
	}
	remains_to_integrate = remains_to_integrate + coeff*exp(reaxb,contextptr);
	continue;
      }
      if (!is_zero(rex2)){
	if (1 
	    //&&is_zero(im(rex2,contextptr)) 
	    //&& is_positive(-rex2,contextptr)
	    ){
	  const vecteur & vx2=lvarxpow(coeff,gen_x);
	  if ( vx2.size()>1 || (!vx2.empty() && vx2.front()!=gen_x) ){
	    remains_to_integrate = remains_to_integrate + coeff*exp(reaxb,contextptr);
	    continue;
	  }
	  // int(exp(rex2*x^2+rea*x+reb)*P(x),x)
	  gen decal=rea/rex2/2;
	  gen cst=normal(reb-rex2*decal*decal,contextptr);
	  // exp(cst)*int(exp(rex2*(x+decal)^2)*P(x),x)
	  coeff=quotesubst(coeff,gen_x,gen_x-decal,contextptr);
	  // exp(cst)*subst(int(exp(rex2*x^2)*coeff(x),x),x,x+decal)
	  vecteur les_var(1,gen_x); // insure x is the main var
	  lvar(makevecteur(coeff,rex2),les_var);
	  int les_vars=int(les_var.size());
	  gen in_coeff,in_coeffnum,in_coeffden,ina;
	  in_coeff=e2r(coeff,les_var,contextptr);
	  ina=e2r(rex2,vecteur(les_var.begin()+1,les_var.end()),contextptr);
	  fxnd(in_coeff,in_coeffnum,in_coeffden);
	  vecteur in_coeffnumv;
	  if (in_coeffnum.type==_POLY)
	    in_coeffnumv=polynome2poly1(*in_coeffnum._POLYptr,1);
	  else
	    in_coeffnumv.push_back(in_coeffnum);
	  // now find int(exp(ina*x^2)*P(x)), coeffs of P are in in_coeffnumv
	  int vs=int(in_coeffnumv.size())-1;
	  vecteur vres(vs+1);
	  // integration by part to decrease vs
	  for (int i=vs;i>=1;--i){ 
	    // i is the degree of the term to integrate
	    gen tmp=in_coeffnumv[vs-i]/ina/2;
	    vres[vs-(i-1)]=tmp;
	    if (i>1)
	      in_coeffnumv[vs-(i-2)] -= (i-1)*tmp;
	  }
	  gen vresden;
	  lcmdeno(vres,vresden,contextptr); // lcmdeno_converted?
	  gen ppart=subst(r2e(poly12polynome(vres,1,les_vars),les_var,contextptr),gen_x,gen_x+decal,false,contextptr)/r2e(vresden,vecteur(les_var.begin()+1,les_var.end()),contextptr)*exp(reaxb,contextptr);
	  // add erf part from the last coeff vres[vs]
	  gen a=-rex2; // r2e(-ina,les_var,contextptr);
	  gen sqrta=sqrt_noabs(a,contextptr);
	  gen erfpart=r2e(in_coeffnumv[vs],cdr_VECT(les_var),contextptr)*symbolic(at_sqrt,cst_pi)/sqrta*exp(cst,contextptr)/2*_erf(sqrta*(gen_x+decal),contextptr);
	  res += (ppart + erfpart)/r2e(in_coeffden,les_var,contextptr);
	  continue;
	}
	remains_to_integrate = remains_to_integrate + coeff*exp(reaxb,contextptr);
	continue;
      }
      gen reai=im(rea,contextptr),rebi=im(reb,contextptr);
      if (!is_zero(reai) || !is_zero(rebi)){
	gen reaxbi=reai*gen_x+rebi;
	coeff=coeff*(cos(reaxbi,contextptr)+cst_i*sin(reaxbi,contextptr));
	rea=re(rea,contextptr);
	reb=re(reb,contextptr);
	reaxb=rea*gen_x+reb;
      }
      tlin(coeff,vtrig,contextptr); // vtrig = coeff , sin/cos(arg)/1
      if ( (intmode &2)==0 ){
	gen tmp=tunlin(vtrig,contextptr);
	if (vtrig.size()>2 || !is_zero(ratnormal(tmp-coeff,contextptr)))
	  gprintf(step_triglinearizable,gettext("Integrate trigonometric linearizable expression %gen -> %gen"),makevecteur(coeff,tmp),contextptr);
      }
      const_iterateur jt=vtrig.begin(),jtend=vtrig.end();
      for (;jt!=jtend;){
	// now check that each arg is linear and coeff polynomial
	coeff=*jt;
	++jt;
	gen ima,imb,imaxb=*jt;
	++jt;
	if (is_constant_wrt(imaxb,gen_x,contextptr)){
	  coeff = coeff*imaxb;
	  imaxb=1;
	}
	int trig_type=0; // 0 for 1, 1 for sin, 2 for cos
	if (imaxb.type==_SYMB){
	  if (imaxb._SYMBptr->sommet==at_sin)
	    trig_type=1;
	  if (imaxb._SYMBptr->sommet==at_cos)
	    trig_type=2;
	}
	else
	  imaxb=0;
	// check polynomial
	const vecteur vx2=lvarxpow(coeff,gen_x);
	bool coeffnotpoly=(vx2.size()>1) || ( (!vx2.empty()) && (vx2.front()!=gen_x));
	gen imc;
	bool quad=imaxb.type==_SYMB && is_quadratic_wrt(imaxb._SYMBptr->feuille,gen_x,ima,imb,imc,contextptr);
	if (!coeffnotpoly && quad && !is_zero(ima) && angle_radian(contextptr)){
	  imc=_trig2exp(coeff*exp(reaxb,contextptr)*imaxb,contextptr);
	  res += integrate_linearizable(imc,gen_x,remains_to_integrate,intmode,true,contextptr);
	  continue;
	}
	if ( coeffnotpoly || ( imaxb.type==_SYMB && !is_linear_wrt(imaxb._SYMBptr->feuille,gen_x,ima,imb,contextptr)) ) {
	  if (trig_type) imaxb=imaxb._SYMBptr->feuille;
	  gen tmp(plus_one);
	  if (trig_type==1)
	    tmp=sin(imaxb,contextptr);
	  if (trig_type==2)
	    tmp=cos(imaxb,contextptr);
	  remains_to_integrate = remains_to_integrate + coeff * exp(reaxb,contextptr) * tmp;
	  continue;
	}
	// everything OK coeff*exp(rea*x+reb)* 1/cos/sin(ima*x+imb)
	if (trig_type)
	  imaxb=imaxb._SYMBptr->feuille;
	else {
	  if (is_zero(rea)){
	    gen tmprem,xvar(gen_x);
	    res= res + exp(reb,contextptr)*integrate_rational(coeff,gen_x,tmprem,xvar,intmode,contextptr);
	    remains_to_integrate = remains_to_integrate+exp(reb,contextptr)*tmprem;
	    continue;
	  }
	}
	bool coeff_is_real=false;
	if (trig_type){
	  gen imcoeff=im(coeff,contextptr);
	  rewrite_with_t_real(imcoeff,gen_x,contextptr);
	  if (is_zero(imcoeff) && is_zero(im(rea,contextptr)) && is_zero(im(ima,contextptr)))
	    coeff_is_real=true;
	}
	// find vars of coeff,rea,reb,ima,imb
	vecteur les_var(1,gen_x); // insure x is the main var
	lvar(makevecteur(coeff,rea,ima),les_var);
	int les_vars=int(les_var.size());
	gen in_coeff,in_coeffnum,in_coeffden,in_rea,in_ima,in_anum,in_aden;
	in_coeff=e2r(coeff,les_var,contextptr);
	fxnd(in_coeff,in_coeffnum,in_coeffden);
	vecteur in_coeffnumv;
	if (in_coeffnum.type==_POLY)
	  in_coeffnumv=polynome2poly1(*in_coeffnum._POLYptr,1);
	else
	  in_coeffnumv.push_back(in_coeffnum);
	in_coeffden=firstcoefftrunc(in_coeffden);
	in_rea=firstcoefftrunc(e2r(rea,les_var,contextptr));
	in_ima=firstcoefftrunc(e2r(ima,les_var,contextptr));
	vecteur resnum;
	gen resden,resplus;
	fxnd(in_rea+cst_i*in_ima,in_anum,in_aden);
	solve_aPprime_plus_P(in_anum,in_aden,in_coeffnumv,resnum,resden);
	resplus=rdiv(r2e(poly12polynome(resnum,1,les_vars),les_var,contextptr),r2e(poly12polynome(vecteur(1,resden*in_coeffden),1,les_vars),les_var,contextptr),contextptr);
	if (step_infolevel(contextptr)){
	  gprintf(step_polyexp,gettext("Primitive of %gen is polynomial of same degree*same exponential %gen"),makevecteur(coeff*symb_exp(reaxb+cst_i*imaxb),resplus*symb_exp(reaxb+cst_i*imaxb)),contextptr);
	}
	if (!trig_type){
	  res = res + resplus*exp(reaxb,contextptr);
	  continue;
	}
	if (coeff_is_real){
	  gen resre=re(resplus,contextptr);
	  rewrite_with_t_real(resre,gen_x,contextptr);
	  gen resim=im(resplus,contextptr);
	  rewrite_with_t_real(resim,gen_x,contextptr);
	  if (trig_type==1)
	    res = res + exp(reaxb,contextptr)*(resim*cos(imaxb,contextptr)+resre*sin(imaxb,contextptr));
	  else
	    res = res + exp(reaxb,contextptr)*(resre*cos(imaxb,contextptr)-resim*sin(imaxb,contextptr));
	  continue;
	}
	fxnd(in_rea-cst_i*in_ima,in_anum,in_aden);
	solve_aPprime_plus_P(in_anum,in_aden,in_coeffnumv,resnum,resden);
	gen resmoins=rdiv(r2e(poly12polynome(resnum,1,les_vars),les_var,contextptr),r2e(poly12polynome(vecteur(1,resden*in_coeffden),1,les_vars),les_var,contextptr),contextptr);
	if (trig_type==1)
	  res = res + exp(reaxb,contextptr)*rdiv(resplus*exp(cst_i*imaxb,contextptr)-resmoins*exp(-cst_i*imaxb,contextptr),plus_two*cst_i,contextptr);
	else
	  res = res +  exp(reaxb,contextptr)*rdiv(resplus*exp(cst_i*imaxb,contextptr)+resmoins*exp(-cst_i*imaxb,contextptr),plus_two,contextptr);
      } // end for (jt)
    } // end for (it)
    if (do_risch){
      gen tmp=remains_to_integrate;
      remains_to_integrate=0;
      res=res+risch(tmp,id_x,remains_to_integrate,contextptr);
    }
    if (is_undef(res)){
      remains_to_integrate=e;
      return 0;
    }
    if (is_zero(im(e,contextptr)) &&has_i(res) && lop(res,at_erf).empty()){
      remains_to_integrate=re(remains_to_integrate,contextptr);
      res=ratnormal(re(res,contextptr),contextptr);
    }
    return res;
  } // end linearizable

  gen linear_integrate_nostep(const gen & e,const gen & x,gen & remains_to_integrate,int intmode,GIAC_CONTEXT);

  bool is_a_monomial(const gen & g,int & expo){
    expo=0;
    if (g.type!=_POLY) return true;
    const polynome & p=*g._POLYptr;
    expo=p.lexsorted_degree();
    vector< monomial<gen> >::const_iterator it=p.coord.begin(),itend=p.coord.end();
    for (;it!=itend;++it){
      if (it->index.front()!=expo)
	return false;
    }
    return true;
  }

  // return 1 if power is a fraction of int, 2 if a sqrt or more general exponent where x->1/x change of var can be tested
  static int integrate_sqrt(gen & e,const gen & gen_x,const vecteur & rvar,gen & res,gen & remains_to_integrate,int intmode,GIAC_CONTEXT){ // x and a power
    // subcase 1: power is a fraction of int
    // find rational parametrization if possible
    // subcase 2: 1st argument of power is linear, 2nd is constant && no inv
    gen argument=rvar.back()._SYMBptr->feuille._VECTptr->front();
    gen exposant=rvar.back()._SYMBptr->feuille._VECTptr->back();
    if (exposant.is_symb_of_sommet(at_inv) && exposant._SYMBptr->feuille.type==_INT_)
      exposant=fraction(1,exposant._SYMBptr->feuille);
    if ( (exposant.type==_FRAC) && (exposant._FRACptr->num.type==_INT_) && (exposant._FRACptr->den.type==_INT_) ){
      int d=exposant._FRACptr->den.val,exponum=exposant._FRACptr->num.val;
      gen a,b,c,tmprem,tmpres,tmpe;
      if (is_linear_wrt(argument,gen_x,a,b,contextptr)){
	// argument=(ax+b)=t^d -> x=(t^d-a)/b and dx=d/a*t^(d-1)*dt
	vecteur substin(makevecteur(argument,gen_x));
	vecteur substout(makevecteur(pow(gen_x,d),rdiv(pow(gen_x,d)-b,a,contextptr)));
	tmpe=complex_subst(e,substin,substout,contextptr)*pow(gen_x,d-1);
	tmpres=linear_integrate_nostep(tmpe,gen_x,tmprem,intmode,contextptr);
	gen fnc_inverse=pow(a*gen_x+b,fraction(1,d),contextptr);
	remains_to_integrate=rdiv(d,a,contextptr)*complex_subst(tmprem,gen_x,fnc_inverse,contextptr);
	res=rdiv(d,a,contextptr)*complex_subst(tmpres,gen_x,fnc_inverse,contextptr);
	return 2;
      }
      vecteur tmpv(1,gen_x);
      lvar(argument,tmpv);
      gen fr,fr_n,fr_np,fr_d,fr_dp,ap,bp;
      fr=e2r(argument,tmpv,contextptr);
      fxnd(fr,fr_np,fr_dp);
      int fr_nexpo,fr_dexpo;
      if (is_a_monomial(fr_np,fr_nexpo) && is_a_monomial(fr_dp,fr_dexpo)){
	gen pui=fraction((fr_nexpo-fr_dexpo)*exponum,d);
	res=e*gen_x/(pui+1);
	return 2;
      }
      fr_n=r2e(fr_np,tmpv,contextptr);
      fr_d=r2e(fr_dp,tmpv,contextptr);
      if (is_linear_wrt(fr_n,gen_x,a,b,contextptr) && is_linear_wrt(fr_d,gen_x,ap,bp,contextptr) ){
	// argument=(a*x+b)/(ap*x+bp)=t^d
	// -> x=(bp*t^d-b)/(a-ap*t^d) 
	// -> dx= d*(b*ap-a*bp)*t^(d-1)/(a-ap*t^d)^2
	vecteur substin(makevecteur(argument,gen_x));
	vecteur substout(makevecteur(pow(gen_x,d),rdiv(bp*pow(gen_x,d)-b,a-ap*pow(gen_x,d),contextptr)));
	tmpe=complex_subst(e,substin,substout,contextptr)*rdiv(pow(gen_x,d-1),pow(a-ap*pow(gen_x,d),2),contextptr);
	tmpres=linear_integrate_nostep(tmpe,gen_x,tmprem,intmode,contextptr);
	gen fnc_inverse=pow(rdiv(a*gen_x+b,ap*gen_x+bp,contextptr),fraction(1,d),contextptr);
	gen tmp=gen(d)*(a*bp-b*ap);
	remains_to_integrate=tmp*complex_subst(tmprem,gen_x,fnc_inverse,contextptr);
	res=tmp*complex_subst(tmpres,gen_x,fnc_inverse,contextptr);
	return 2;
      }
      bool frdconst=is_constant_wrt(fr_d,gen_x,contextptr);
      if (frdconst){
	// multiply denominator by conjugate
	gen tmpx(identificateur("tmpx"));
	gen e1=complex_subst(e,rvar.back(),tmpx,contextptr); // sqrt(argument,contextptr)
	vecteur lv(1,tmpx);
	lvar(e1,lv);
	gen e2=e2r(e1,lv,contextptr),num,den;
	fxnd(e2,num,den);
	den=r2e(den,lv,contextptr);
	num=r2e(num,lv,contextptr);
	// multiply denominator of e2 by conjugate 
	gen pmini=pow(tmpx,d)-argument;
	gen C=_egcd(makesequence(den,pmini,tmpx),contextptr);
	if (is_undef(C)){
	  res=C;
	  return 2;
	}
	num=_rem(makesequence(num*C[0],pmini,tmpx),contextptr);
	if (is_undef(num)){
	  res= num;
	  return 2;
	}
	den=C[2];
	// int(num/den), den does not depend on y=tmpx, the fractional power
	// if num does not, then we can integrate
	if (is_constant_wrt(num,tmpx,contextptr)){
	  res=linear_integrate_nostep(num/den,gen_x,remains_to_integrate,intmode,contextptr);
	  return 1;
	}
	/* Possible improvement: remove multiplicities in denominator
	   for each monomial of num n*tmpx^deg, set gamma=deg/d
	   then make a partial fraction decomposition of n/den ->
	   int(n*y^gamma/D^(k+1)) for D squarefree and coprime with y
	   Bezout find U and V such that n=D*U+D'*y*V
	   let N=-V/k and r=U-(y*N'+(gamma+1)*N*y'), then
	   int(n*y^gamma/D^(k+1))=N*y^(gamma+1)/D^k+int(r*y^gamma/D^k)
	 */
	if (d==2){
	  // write e as alpha+beta*sqrt(argument)
	  /* ( * 	2nd order: dispatch for y=ax^2+bx+c	           * )
	     ( * 	a>0	->	x=[m^2-c]/[b-2*sqrt[a]*m]          * )
	     ( *			m=sqrt[y]-sqrt[a]*x	           * )
	     ( * 			dx/sqrt[y]=2*dm/[b-2*sqrt[a]*m]	   * )
	  */
	  gen alpha,beta,xvar(gen_x);
	  if (!is_linear_wrt(num,tmpx,beta,alpha,contextptr)){
	    res=gensizeerr(contextptr);
	    return 2;
	  }
	  alpha=integrate_rational(alpha/den,gen_x,remains_to_integrate,xvar,intmode,contextptr);
	  if (is_undef(alpha)){
	    res=alpha;
	    return 2;
	  }
	  /* Instead we should factor argument in den 
	     FIXME in usual.cc diff of ln should expand * and / and rm abs
	     write y=argument, P=beta
	     we want to integrate P*sqrt(y)/den=(P*y)/den* y^(-1/2)
	     *IF* den=y^l*D where D is prime with y (not always true...)
	     P/Dy^l = P_y/y^l + P_D/D <--> P = P_y*D + P_D*y^l,
	     find P_D and P_y by Bezout, find
	     g = Q*D+R*y^l then Pg = P*Q*D + P*R*y^l hence
	     P_D = P*R mod D/g  and  P_y = P*Q /g + [P*R div D] *y^l /g 
	  */
	  gen y=argument,P=beta,D=den; // P*y/den
	  C=_quorem(makesequence(D,y,gen_x),contextptr);
	  if (is_undef(C)){
	    res= C;
	    return 2;
	  }
	  int l=0;
	  if (is_zero(C[1])){ // P/(den/y)
	    D=C[0];
	    for (;;++l){
	      C=_quorem(makesequence(D,y,gen_x),contextptr);
	      if (is_undef(C)){
		res=C;
		return 2;
	      }
	      if (!is_zero(C[1]))
		break;
	      D=C[0];
	    }
	  }
	  else
	    P=P*y;
	  gen yl=pow(y,l);
	  C=_egcd(makesequence(D,yl,gen_x),contextptr);
	  if (is_undef(C)){
	    res= C;
	    return 2;
	  }
	  gen g=C[2],Q=C[0],R=C[1];
	  C=_quorem(makesequence(P*R,D,gen_x),contextptr);
	  if (is_undef(C)){
	    res=C;
	    return 2;
	  }
	  gen PD=C[1]/g;
	  // changed made for int(1/(sin(x)*sqrt(sin(2*x)^3)));
	  C=_quorem(makesequence(P*Q+C[0]*yl,g,gen_x),contextptr);
	  if (!is_zero(C[1]))
	    return 0;
	  gen Py=C[0];  
	  // gen Py=(P*Q+C[0]*yl)/g;
	  C=_quorem(makesequence(Py,y,gen_x),contextptr);
	  if (is_undef(C)){
	    res= C; 
	    return 2;
	  }
	  /*
	    int[ Py/y^l*y^-1/2 ] = Q*y^[1/2-l] + int[ C*y^-1/2 ]
	    degre[Py]=n, degre[y]=k, find Q degre[Q]=n+1-k and C degre[C]=k-2
	    so that Py = Q'*y + Q*y'*[1/2-l] + C y^l   
	    to do this we represent Q by a n+2-k-vector, C by a k-1-vector
	    gluing Q and C we get a n+1-vector that must be solution of a
	    n+1*n+1 linear system. Now we build the matrix of this system
	    The n+2-k first columns are				    
	    y'[1/2-l]  ...  x^alpha*y'*[1/2-l]+alpha*x^[alpha-1]*y ...    
	    The k-1 last columns are 
	    y^l  ...  x^beta*y^l 
	    Note 1: to avoid rational input in the matrix we multiply by 2
	    coef of Q and C are found in the reverse order
	    for l!=0 n is more precisely max[deg[Py],k[l+1]-2]
	    Note 2: at the end we integrate only (C+PD/D)*y^(-1/2)
	  */
	  gen tmpv=_e2r(makesequence(Py,gen_x),contextptr);
	  if (tmpv.type!=_VECT){
	    if (tmpv.type==_FRAC){
	      if (tmpv._FRACptr->den.type==_VECT){
		if (tmpv._FRACptr->den._VECTptr->size()!=1){
		  *logptr(contextptr) << "Internal error integrating sqrt" << '\n';
		  return 0;
		}
		tmpv._FRACptr->den=tmpv._FRACptr->den._VECTptr->front();
	      }
	      if (tmpv._FRACptr->num.type==_VECT)
		tmpv=multvecteur(inv(tmpv._FRACptr->den,contextptr),*tmpv._FRACptr->num._VECTptr);
	    }
	    if (tmpv.type!=_VECT)
	      tmpv=vecteur(1,tmpv); // change 3/1/2013 for int(sqrt(1+x^2)/(-2*x^2))
	    // res= gensizeerr(contextptr);
	    // return 2;
	  }
	  vecteur colP=*tmpv._VECTptr;
	  int n=int(colP.size())-1;
	  tmpv=_e2r(makesequence(y,gen_x),contextptr);
	  if (tmpv.type!=_VECT){
	    res= gensizeerr(contextptr);
	    return 2;
	  }
	  int k=int(tmpv._VECTptr->size())-1;
	  n=giacmax(n,k*(l+1)-2);
	  n=giacmax(n,k-1);
	  if (n){
	    lrdm(colP,n);
	    gen yprime=(1-2*l)*derive(y,gen_x,contextptr);
	    if (is_undef(yprime)){
	      res= yprime;
	      return 2;
	    }
	    matrice sys;
	    tmpv=_e2r(makesequence(yprime,gen_x),contextptr);
	    if (tmpv.type!=_VECT){
	      res=gensizeerr(contextptr);
	      return 2;
	    }
	    vecteur col0(*tmpv._VECTptr);
	    vecteur col(col0);
	    lrdm(col,n);
	    sys.push_back(col);
	    col0.push_back(zero);
	    tmpv=_e2r(makesequence(2*y,gen_x),contextptr);
	    if (tmpv.type!=_VECT){ 
	      res=gensizeerr(contextptr);
	      return 2;
	    }
	    vecteur col1(*tmpv._VECTptr);
	    for (int i=1;i<n+2-k;++i){
	      col=col0+gen(i)*col1;
	      lrdm(col,n);
	      col0.push_back(zero);
	      col1.push_back(zero);
	      sys.push_back(col);
	    }
	    tmpv=_e2r(makesequence(2*yl,gen_x),contextptr);
	    if (tmpv.type!=_VECT){
	      res= gensizeerr(contextptr);
	      return 2;
	    }
	    col0=*tmpv._VECTptr;
	    for (int i=0;i<k-1;++i){
	      col=col0;
	      lrdm(col,n);
	      sys.push_back(col);
	      col0.push_back(zero);
	    }
	    sys=mtran(sys);
	    int st=step_infolevel(contextptr);
	    step_infolevel(contextptr)=0;
	    col0=linsolve(sys,colP,contextptr);
	    step_infolevel(contextptr)=st;
	    if (!col0.empty() && is_undef(col0.front())){
	      res= col0.front();
	      return 2;
	    }
	    if (col0.size()<k-1)
	      return 2;
	    reverse(col0.begin(),col0.end()); // C at the beginning, Q at the end
	    C=2*horner(vecteur(col0.begin(),col0.begin()+k-1),gen_x);
	    Q=2*horner(vecteur(col0.begin()+k-1,col0.end()),gen_x);
	    alpha=alpha+Q*sqrt(y,contextptr)/yl;
	  }
	  else
	    C=Py;
	  e=(C+PD/D)/sqrt(argument,contextptr);
	  if (is_quadratic_wrt(argument,gen_x,a,b,c,contextptr)){
	    if (!is_positive(-a,contextptr)){
	      gen sqrta(sqrt(a,contextptr));
	      gen id_m(identificateur("tmpm"));
	      gen m(id_m);
	      tmpe=eval(rdiv(complex_subst(e*sqrt(argument,contextptr),argument,pow(m+sqrta*gen_x,2),contextptr),b-plus_two*sqrta*m,contextptr),1,contextptr);
	      tmpe=ratnormal(complex_subst(tmpe,gen_x,rdiv(m*m-c,b-plus_two*sqrta*m,contextptr),contextptr),contextptr);
	      // factor?
	      //tmpe=_factor(tmpe,contextptr);
	      tmpres=linear_integrate_nostep(tmpe,m,tmprem,intmode | 2,contextptr);
	      remains_to_integrate=remains_to_integrate+complex_subst(plus_two*tmprem,m,sqrt(argument,contextptr)-sqrta*gen_x,contextptr);
	      res= alpha+complex_subst(plus_two*tmpres,m,sqrt(argument,contextptr)-sqrta*gen_x,contextptr);
	      return 2;
	    }
	    else {
	      gen D=b*b-gen(4)*a*c;
	      if (is_strictly_positive(-D,contextptr))
		return 0;
	      D=sqrt_noabs(D,contextptr);
	      gen sD=sign(D,contextptr);
	      if (is_minus_one(sD)){
		D=-D;
		sD=1;
	      }
	      /*
		( *	D=sqrt(b^2-4ac)                                    * )
		( * 	a<0 and D>0 ->	x=[D*2u/[1+u^2]-b]/2a		   * )
		( *			u=[D-2*sqrt[-a]*sqrt[y]]/[2ax+b]   * )
		( *			dx/sqrt[y]=-2*du/[sqrt[-a]*[1+u^2]] * )
	      */
	      gen sqrta(sqrt(-a,contextptr));
	      gen id_u(identificateur("tmpu"));
	      gen u(id_u),uu(u);
	      gen uasx=rdiv(D-plus_two*sqrta*sqrt(argument,contextptr),plus_two*a*gen_x+b,contextptr);
	      tmpe=ratnormal(e*sqrt(argument,contextptr),contextptr);
	      tmpe=complex_subst(tmpe,gen_x,rdiv(rdiv(plus_two*u*D,1+u*u,contextptr)-b,plus_two*a,contextptr),contextptr);
	      tmpe=-rdiv(plus_two,sqrta,contextptr)*tmpe/(1+u*u);
	      tmpres=integrate_rational(tmpe,u,tmprem,uu,intmode,contextptr);
	      // sqrt(a*x^2+b*x+c) -> a*[(x+b/2/a)^2-(D/a)^2]
	      // -> asin(a*x+b/2)
	      vecteur vin(makevecteur(u,symbolic(at_atan,u))),vout(makevecteur(uasx,inv(-2,contextptr)*sD*asin(ratnormal((-2*a*gen_x-b)/D,contextptr),contextptr)));
	      remains_to_integrate=remains_to_integrate+complex_subst(tmprem,vin,vout,contextptr);
	      res=alpha+complex_subst(tmpres,vin,vout,contextptr);
	      return 2;
	    }
	  } // end sqrt of quadratic
	  else {
	    remains_to_integrate=e;
	    res=alpha;
	    return 2;
	  }
	} // end if d==2 
      } // end if denominator of argument of frac power is constant
      int numdeg=0,numval=0,dendeg=0;
      if (fr_dp.type==_POLY) dendeg=fr_dp._POLYptr->lexsorted_degree();
      if (dendeg>0 || fr_np.type!=_POLY) return 1;
      numdeg=fr_np._POLYptr->lexsorted_degree();
      numval=fr_np._POLYptr->valuation(0);
      return numval>0?2:1;
    } // end exposant=fraction of integers
    return 0;
  } // end recusive var size==2 i.e. of integrate_sqrt 

  static gen integrate_piecewise(gen& e,const gen & piece,const gen & gen_x,gen & remains_to_integrate,GIAC_CONTEXT,int intmode){
    gen & piecef=piece._SYMBptr->feuille;
    if (piecef.type!=_VECT){
      e=subst(e,piece,piecef,false,contextptr);
      return integrate_id_rem(e,gen_x,remains_to_integrate,contextptr,intmode);
    }
    vecteur piecev=*piecef._VECTptr,remainsv(piecev);
    int nargs=int(piecev.size());
    bool addremains=false;
    for (int i=0;i<nargs/2;++i){
      remainsv[2*i+1]=0;
      gen tmp=subst(e,piece,piecev[2*i+1],false,contextptr);
      piecev[2*i+1]=integrate_id_rem(tmp,gen_x,remainsv[2*i+1],contextptr,intmode);
      addremains = addremains || !is_zero(remainsv[2*i+1]);
    }
    if (nargs%2){
      remainsv[nargs-1]=0;
      gen tmp=subst(e,piece,piecev[nargs-1],false,contextptr);
      piecev[nargs-1]=integrate_id_rem(tmp,gen_x,remainsv[nargs-1],contextptr,intmode);
      addremains = addremains || !is_zero(remainsv[nargs-1]);
    }
    if (addremains)
      remains_to_integrate=symbolic(at_piecewise,gen(remainsv,_SEQ__VECT));
    return symbolic(at_piecewise,gen(piecev,_SEQ__VECT));
    // FIXME: make the antiderivative continuous
  }

  static gen integrate_trig_fraction(gen & e,const gen & gen_x,vecteur & var,const gen & coeff_trig,int trig_fraction,gen& remains_to_integrate,int intmode,GIAC_CONTEXT){
    const_iterateur vart=var.begin(),vartend=var.end();
    vecteur substout;
    gen a,b,coeff_cst;
    is_linear_wrt(vart->_SYMBptr->feuille,gen_x,a,b,contextptr);
    coeff_cst=ratnormal(rdiv(a,coeff_trig,contextptr),contextptr)*b;
    // express all angles in vart as n*(coeff_trig*x+coeff_cst)+angle=a*x+b, 
    // t=coeff_trig*x+coeff_cst
    for (;vart!=vartend;++vart){
      is_linear_wrt(vart->_SYMBptr->feuille,gen_x,a,b,contextptr);
      gen n=ratnormal(rdiv(a,coeff_trig,contextptr),contextptr);
      if (n.type!=_INT_) return gensizeerr(gettext("trig_fraction"));
      gen angle=ratnormal(b-n*coeff_cst,contextptr);
      substout.push_back(symbolic(vart->_SYMBptr->sommet,n*gen_x+angle));
    }
    gen f=complex_subst(e,var,substout,contextptr); // should be divided by coeff_trig
    f=_texpand(f,contextptr);
    gen tmprem,tmpres;
    if (trig_fraction==4){ // everything depends on exp(x)
      f=complex_subst(f,exp(gen_x,contextptr),gen_x,contextptr)*inv(gen_x,contextptr);
      if ( (intmode &2)==0)
	gprintf(step_ratfracexp,gettext("Integrate rational fraction of exponential %gen by %gen change of variable, leading to integral of %gen"),makevecteur(e,exp(gen_x,contextptr),f),contextptr);
      tmpres=linear_integrate_nostep(f,gen_x,tmprem,intmode,contextptr);
      gen expx=exp(coeff_trig*gen_x+coeff_cst,contextptr);
      if ( (intmode & 2)==0)
	gprintf(step_backsubst,gettext("Back substitution %gen->%gen in %gen"),makevecteur(gen_x,expx,tmprem),contextptr);
      remains_to_integrate = expx*complex_subst(tmprem,gen_x,expx,contextptr);
      return inv(coeff_trig,contextptr)*complex_subst(tmpres,gen_x,expx,contextptr);
    }
    f=halftan(f,contextptr); // now everything depends on tan(x/2)
    // t=tan(x/2), dt=1/2(1+t^2)*dx
    gen xsur2=rdiv(coeff_trig*gen_x+coeff_cst,plus_two,contextptr);
    gen tanxsur2=tan(xsur2,contextptr);
    f=complex_subst(f,tan(rdiv(gen_x,plus_two,contextptr),contextptr),gen_x,contextptr)*inv(plus_one+pow(gen_x,2),contextptr);
    if ( (intmode &2)==0)
      gprintf(step_ratfractrig,gettext("Integrate rational fraction of trigonometric %gen by %gen change of variable, leading to integral of %gen"),makevecteur(e,tanxsur2,f),contextptr);
    vecteur vf(1,gen_x);
    rlvarx(f,gen_x,vf);
    if (vf.size()<=1)
      tmpres=integrate_rational(f,gen_x,tmprem,tanxsur2,intmode,contextptr);
    else {
      tmpres=linear_integrate_nostep(f,gen_x,tmprem,intmode,contextptr);
      if ( (intmode & 2)==0)
	gprintf(step_backsubst,gettext("Back substitution %gen->%gen in %gen"),makevecteur(gen_x,tanxsur2,tmpres),contextptr);
      tmpres=complex_subst(tmpres,gen_x,tanxsur2,contextptr);
      // tmprem=complex_subst(tmprem,gen_x,tanxsur2,contextptr);
    }
    if (tmpres==0)
      remains_to_integrate = e;
    else
      remains_to_integrate = rdiv(plus_two,coeff_trig,contextptr)*tmprem*(1+pow(tanxsur2,2));
    return rdiv(plus_two,coeff_trig,contextptr)*tmpres;
  }

  // reduce g, a rational fraction wrt to x, to a sqff lnpart
  // and adds the non sqff integrated part to ratpart
  bool intgab_ratfrac(const gen & e,const gen & x,gen & value,GIAC_CONTEXT){
    vecteur l;
    l.push_back(x); // insure x is the main var
    l=vecteur(1,l);
    alg_lvar(e,l);
    int s=int(l.front()._VECTptr->size());
    if (!s){
      l.erase(l.begin());
      s=int(l.front()._VECTptr->size());
    }
    if (!s)
      return false;
    gen r=e2r(e,l,contextptr);
    gen r_num,r_den;
    fxnd(r,r_num,r_den);
    if (r_num.type==_EXT)
      return false;
    polynome num(s);
    if (r_num.type==_POLY)
      num=*r_num._POLYptr;
    else
      num=polynome(r_num,s);
    if (r_den.type!=_POLY){ // not convergent
      if (num.lexsorted_degree()%2)
	value=undef;
      else
	value=subst(r2e(r_num,l,contextptr),x,1,false,contextptr)/r2e(r_den,l,contextptr)*plus_inf;
      return true;
    }
    polynome den(*r_den._POLYptr);
    if (num.lexsorted_degree()>den.lexsorted_degree()-2){ // not convergent
      if ( (num.lexsorted_degree()-den.lexsorted_degree())%2 )
	value=undef;
      else
	value=subst(r2e(r_num,l,contextptr)/r2e(r_den,l,contextptr),x,1,false,contextptr)*plus_inf;
      return true;
    }
    l.front()._VECTptr->front()=x;
    vecteur lprime(l);
    if (lprime.front().type!=_VECT){ 
      value=gensizeerr(gettext("in intgab_rational"));
      return false;
    }
    lprime.front()=cdr_VECT(*(lprime.front()._VECTptr));
    // quick check for length 2 deno
    vecteur vtmp;
    polynome2poly1(den,1,vtmp);
    if (integrate_deno_length_2(num,vtmp,l,lprime,value,true,2/* no step info*/,contextptr)){
      value=ratnormal(value,contextptr)*cst_pi;
      return true;
    }
    polynome p_content(lgcd(den));
    factorization vden(sqff(den/p_content)); // first square-free factorization
    vector< pf<gen> > pfde_VECT;
    polynome ipnum(s),ipden(s),temp(s),tmp(s);
    partfrac(num,den,vden,pfde_VECT,ipnum,ipden);
    vector< pf<gen> >::iterator it=pfde_VECT.begin();
    vector< pf<gen> >::const_iterator itend=pfde_VECT.end();
    vector< pf<gen> > intdecomp,finaldecomp;
    for (;it!=itend;++it){
      pf<gen> single(intreduce_pf(*it,intdecomp,true));
      // Now final factorization for single.den, 
      // then compute single.num/single.den'(root) for roots with im>0
      // this is the residue
      // Example 1/(x^4+1) roots in C^+: exp(i*pi/4), exp(3*i*pi/4),
      // num/den'=1/4/x^3=-x/4 -> -1/4*exp(i*pi/4)-1/4*exp(3*i*pi/4)
      // -> -1/2*sin(pi/4)*i  [*2*i*pi -> sqrt(2)/2*pi]
      vden.clear();
      gen extra_div=1;
      factor(single.den,p_content,vden,false,false,false,1,extra_div);
      partfrac(single.num,single.den,vden,finaldecomp,temp,tmp);
    }
    it=finaldecomp.begin();
    itend=finaldecomp.end();
    gen lnpart(0),deuxaxplusb,sqrtdelta;
    polynome a(s),b(s),c(s);
    polynome d(s),E(s),lnpartden(s);
    polynome delta(s),atannum(s),alpha(s);
    for (;it!=itend;++it){
      int deg=it->fact.lexsorted_degree();
      // polynome & itnum=it->num;
      // polynome & itden=it->den;
      gen Delta;
      switch (deg) { 
      case 1: // 1st order
	value=undef;
	return true;
      case 2: // 2nd order
	findabcdelta(it->fact,a,b,c,delta);
	Delta=r2e(delta,lprime,contextptr);
	if (is_positive(Delta,contextptr)){
	  value=undef;
	  return true;
	}
	alpha=(it->den/it->fact).trunc1()*a*gen(2);
	findde(it->num,d,E);
	atannum=a*E*gen(2)-b*d;
	atannum=atannum*gen(2);
	simplify(atannum,alpha);
	sqrtdelta=normalize_sqrt(sqrt(-Delta,contextptr),contextptr);
	value += rdiv(r2e(atannum,lprime,contextptr),(r2e(alpha,lprime,contextptr))*sqrtdelta,contextptr);
	break; 
      default: // divide a*it->num =b*it->den.derivative()+c 
	it->num.TPseudoDivRem(it->den.derivative(),b,c,a);
	// remaining pf
	if (!c.coord.empty()){
	  vtmp=polynome2poly1(a*it->den,1);
	  if (!integrate_deno_length_2(c,vtmp,l,lprime,value,true,2/* no step info*/,contextptr))
	    return false;
	}
	break ;
      }
    }
    value=ratnormal(value,contextptr)*cst_pi;
    return true;
  }

  static gen integrate_rational_end(vector< pf<gen> >::iterator & it,vector< pf<gen> >::const_iterator & itend,const gen & x,const gen & xvar,const vecteur & l,const vecteur & lprime,const polynome & ipnum,const polynome & ipden,const gen & ratpart,gen & remains_to_integrate,int intmode,GIAC_CONTEXT){
    gen lnpart(0),deuxaxplusb,sqrtdelta;
    int s=ipnum.dim;
    polynome a(s),b(s),c(s);
    polynome d(s),E(s),lnpartden(s);
    polynome delta(s),atannum(s),alpha(s);
    bool uselog;
    remains_to_integrate=0;
    for (;it!=itend;++it){
      int deg=it->fact.lexsorted_degree();
      // polynome & itnum=it->num;
      // polynome & itden=it->den;
      gen Delta;
      switch (deg) { 
      case 1: // 1st order
	lnpart=lnpart+rdiv(r2e(it->num,l,contextptr),r2e(firstcoeff(it->den),l,contextptr),contextptr)*lnabs2(r2e(it->fact,l,contextptr),xvar,contextptr);
	break; 
      case 2: // 2nd order
	findabcdelta(it->fact,a,b,c,delta);
	Delta=r2e(delta,lprime,contextptr);
	uselog=is_positive(Delta,contextptr);
	alpha=(it->den/it->fact).trunc1()*a*gen(2);
	findde(it->num,d,E);
	atannum=a*E*gen(2)-b*d;
	// ln part d/alpha*ln(fact)
	lnpartden=alpha;
	simplify(d,lnpartden);
	lnpart=lnpart+rdiv(r2e(d,lprime,contextptr),r2e(lnpartden,lprime,contextptr),contextptr)*gen(uselog?lnabs2(r2e(it->fact,l,contextptr),xvar,contextptr):symbolic(at_ln,r2e(it->fact,l,contextptr)));
	// atan or _FUNCnd ln part
	deuxaxplusb=r2e(it->fact.derivative(),l,contextptr);
	if (uselog){
	  sqrtdelta=normalize_sqrt(sqrt(Delta,contextptr),contextptr);
	  simplify(atannum,alpha);
	  lnpart=lnpart+rdiv(r2e(atannum,lprime,contextptr),(r2e(alpha,lprime,contextptr))*sqrtdelta,contextptr)*lnabs2(rdiv(deuxaxplusb-sqrtdelta,deuxaxplusb+sqrtdelta,contextptr),xvar,contextptr);
	}
	else {
	  vecteur v=solve(x*x+Delta,x,0,contextptr);
	  if (v.size()==2 && !is_undef(v[0]) && !is_undef(v[1])){
	    if (is_positive(-v[0],contextptr))
	      sqrtdelta=v[1];
	    else
	      sqrtdelta=v[0];
	  }
	  else
	    sqrtdelta=normalize_sqrt(sqrt(-Delta,contextptr),contextptr);
	  atannum=atannum*gen(2);
	  simplify(atannum,alpha);
	  gen tmpatan=ratnormal(rdiv(deuxaxplusb,sqrtdelta,contextptr),contextptr);
	  gen residue;
	  if (tmpatan.is_symb_of_sommet(at_tan))
	    tmpatan=tmpatan._SYMBptr->feuille;
	  else {
	    // avoid floor if possible
	    // atan(beta*tan(theta)+gamma)+floor() for beta>0 and gamma>-1
	    // -> atan( cos(theta)*((beta-1)*sin(theta)+gamma*cos(theta))/
	    //          (cos(theta)^2+beta*sin(theta)^2+gamma*sin(theta)*cos()) )
	    gen beta,gamma;
	    if (  //0 && 
		  xvar.is_symb_of_sommet(at_tan) && is_linear_wrt(tmpatan,xvar,beta,gamma,contextptr) && is_strictly_greater(4*beta,gamma*gamma,contextptr) ){
	      gen argtan=ratnormal(2*xvar._SYMBptr->feuille,contextptr);
	      gen si=symbolic(at_sin,argtan),ci=symbolic(at_cos,argtan);
	      tmpatan=symbolic(at_atan,ratnormal(((beta-1)*si+gamma*(1+ci))/(1+beta+gamma*si+(1-beta)*ci),contextptr));
	      residue=xvar._SYMBptr->feuille;
	    }
	    else {
	      tmpatan=atan(tmpatan,contextptr);
	      if (xvar.is_symb_of_sommet(at_tan)){
		if (do_lnabs(contextptr)){
		  // add residue
		  residue=r2e(it->fact.derivative().derivative(),l,contextptr);
		  residue=cst_pi*sign(residue,contextptr)*_floor((xvar._SYMBptr->feuille/cst_pi+plus_one_half),contextptr);
		}
	      }
	      else {
		// if xvar has a singularity at 0 e.g. xvar =x+1/x or x-1/x, 
		// add the residue at 0
		if (xvar.type!=_IDNT){
		  // replacing tmpatan by atan(inv(tmpatan)) would avoid residue for int((x^2+1)/(x^4+3x^2+1)); but then it would not be continuous at 1 and -1
		  residue=ratnormal(limit(tmpatan,*x._IDNTptr,0,-1,contextptr)-limit(tmpatan,*x._IDNTptr,0,1,contextptr),contextptr);
		  residue=residue*sign(x,contextptr)/2;
		}
	      }
	    }
	  }
	  if (!angle_radian(contextptr)){
	    if (angle_degree(contextptr))
	      tmpatan=tmpatan*deg2rad_e;
	    //grad
	    else
	      tmpatan = tmpatan*grad2rad_e;
	  }
	  tmpatan += residue;
	  lnpart=lnpart+rdiv(r2e(atannum,lprime,contextptr),(r2e(alpha,lprime,contextptr))*sqrtdelta,contextptr)*tmpatan;
	} // end else uselof
	break; 
      default: // divide a*it->num =b*it->den.derivative()+c 
	it->num.TPseudoDivRem(it->den.derivative(),b,c,a);
	// remaining pf
	if (!c.coord.empty()){
	  vecteur vtmp=polynome2poly1(a*it->den,1);
	  if (!integrate_deno_length_2(c,vtmp,l,lprime,lnpart,false,intmode,contextptr))
	    remains_to_integrate += r2sym(vector< pf<gen> >(1,pf<gen>(c,a*it->den,it->fact,1)),l,contextptr);
	}
	// extract log part b/a*ln[fact]
	simplify(b,a);
	if (!is_zero(b))
	  lnpart=lnpart+rdiv(r2e(b,l,contextptr),r2e(a,l,contextptr),contextptr)*lnabs(r2e(it->fact,l,contextptr),contextptr);
	break ;
      }
    }
    return rdiv(r2e(ipnum.integrate(),l,contextptr),r2e(ipden,l,contextptr),contextptr)+ratpart+lnpart;
  }

  // integration of a rational fraction
  static gen integrate_rational(const gen & e, const gen & x, gen & remains_to_integrate,gen & xvar,int intmode,GIAC_CONTEXT){
    if (x.type!=_IDNT) return gensizeerr(contextptr); // see limit
    if (has_num_coeff(e)){
      gen ee=exact(e,contextptr);
      if (!has_num_coeff(ee)){
	ee=integrate_rational(ee,x,remains_to_integrate,xvar,intmode,contextptr);
	ee=evalf(ee,1,contextptr);
	remains_to_integrate=evalf(remains_to_integrate,1,contextptr);
	return ee;
      }
    }
    const vecteur & varx=lvarx(e,x);
    int varxs=int(varx.size());
    if (!varxs){
      remains_to_integrate=zero;
      return e*xvar;
    }
    if ( (varxs>1) || (varx.front()!=x) ) {
      remains_to_integrate = e;
      return zero;
    }
    vecteur l;
    l.push_back(x); // insure x is the main var
    l=vecteur(1,l);
    alg_lvar(e,l);
    vecteur l_orig=l;
    int s=int(l.front()._VECTptr->size());
    if (!s){
      l.erase(l.begin());
      s=int(l.front()._VECTptr->size());
    }
    if (!s)
      return gensizeerr(contextptr);
    vecteur lprime(l);
    if (lprime.front().type!=_VECT) return gensizeerr(gettext("in integrate_rational"));
    lprime.front()=cdr_VECT(*(lprime.front()._VECTptr));
    gen r=e2r(e,l,contextptr);
    // cout << "Int " << r << '\n';
    gen r_num,r_den;
    fxnd(r,r_num,r_den);
    if (r_num.type==_EXT){
      remains_to_integrate=e;
      return zero;
    }
    if (r_den.type!=_POLY){
      l.front()._VECTptr->front()=xvar;
      if (r_num.type==_POLY)
	return rdiv(r2e(r_num._POLYptr->integrate(),l,contextptr),r2sym(r_den,l,contextptr),contextptr);
      else 
	return e*xvar;
    }
    polynome den(*r_den._POLYptr),num(s);
    if (r_num.type==_POLY)
      num=*r_num._POLYptr;
    else
      num=polynome(r_num,s);
    // cyclotomic-like polys
    vecteur vtmp;
    if (den.coord.size()==2 && den.lexsorted_degree()!=1 && num.lexsorted_degree() < den.lexsorted_degree() && den.coord.back().index.is_zero() && xvar.type==_IDNT){
      polynome2poly1(den,1,vtmp);
      r=0;
      if (!integrate_deno_length_2(num,vtmp,l,lprime,r,false,intmode,contextptr))
	remains_to_integrate=e;
      return r;
    }
    // check for a t=x^aa change of variable: a divides deg(num)+1-deg(den)
    // as well as all differences of degrees in the poly num and den
    int den_deg=den.lexsorted_degree(),num_deg=num.lexsorted_degree();
    int aa=num_deg+1-den_deg,precedent,actuel;
    vector< monomial<gen> > ::const_iterator num_it=num.coord.begin(),num_itend=num.coord.end();
    if (num_itend-num_it>1){
      precedent=num_it->index.front(); 
      ++num_it;
      for (;num_it!=num_itend;++num_it){
	actuel=num_it->index.front();
	aa=gcd(aa,actuel-precedent);
	precedent=actuel;
      }
    }
    num_it=den.coord.begin(),num_itend=den.coord.end();
    if (num_itend-num_it>1){
      precedent=num_it->index.front(); 
      ++num_it;
      for (;num_it!=num_itend;++num_it){
	actuel=num_it->index.front();
	aa=gcd(aa,actuel-precedent);
	precedent=actuel;
      }
    }
    if (!aa)
      aa=den_deg;
    if (aa>1){ // Apply
      if ( (intmode & 2)==0)
	gprintf(step_ratfracpow,gettext("Integrate rational fraction %gen, change of variable %gen"),makevecteur(e,symb_equal(x,symb_pow(x,aa))),contextptr);
      int k=0;
      k=den_deg % aa;
      // shift num and den by x^k
      index_t k1(num.dim);
      k1.front()=k+1;
      num=(num.shift(k1)).dividedegrees(aa);
      index_t ka(num.dim);
      ka.front()=k+aa;
      den=gen(aa)*(den.shift(ka)).dividedegrees(aa);
      if (!(aa%2) && xvar.is_symb_of_sommet(at_tan)){
	// t=tan(x)^2: c=cos(2x)=(1-t^2)/(1+t^2) -> t^2=(1-c)/(1+c)
	xvar=symbolic(at_cos,ratnormal(2*xvar._SYMBptr->feuille,contextptr));
	xvar=(1-xvar)/(1+xvar);
	aa/=2;
      }
      xvar=pow(xvar,aa);
      return integrate_rational(r2e(fraction(num,den),l,contextptr),x,remains_to_integrate,xvar,intmode,contextptr);
    }
    int den_val=den.valuation(0),num_val=num.valuation(0);
    if (den_deg+den_val==num_deg+num_val+2){
      // now detect pattern that simplifies trig fraction integration
      /* cos is already detected by x^aa above with aa=2
       *
       * sin: if the fraction, including dt, is invariant by t->v=1/t 
       * e.g (1-t^2)/(1+t^2)^2 dt = v^2(v^2-1)/(v^2+1)^2*( -1/v^2) dv
       * or [equivalent] tF(t) must change sign
       * let u=t+1/t, du=(1-1/t^2)dt, F(t)dt= F(t) * t^2/(t^2-1) du
       * e.g. t^2/(1+t^2)^2 du
       * N/(t^2-1) and D must be symm
       * 
       * tan: if the fraction, including dt, is invariant by t->-1/t 
       * u=t-1/t, du=(1+1/t^2)dt, F(t)dt= F(t) * t^2/(t^2+1) du
       * N/(t^2+1) and D must be antism.
       */
      vecteur Nsave,Dsave,N,D,test(makevecteur(1,0,-1)),q,r;
      polynome2poly1(num,1,N);
      if (num_val && num_val<signed(N.size()))
	N=vecteur(N.begin(),N.begin()+N.size()-num_val);
      polynome2poly1(den,1,D);
      if (den_val && den_val<signed(D.size()))
	D=vecteur(D.begin(),D.begin()+D.size()-den_val);
      Nsave=N; Dsave=D;
      bool type=false;
      if (N.size()>=test.size() && DivRem(N,test,0,q,r) && r.empty()){
	r=D;
	if (is_symmetric(q,N,true)*is_symmetric(r,D,true)==1){
	  // yes!
	  type = (xvar.type==_SYMB) || D.size()>1;
	}
      }
      if (!type) {
	N=Nsave; D=Dsave;
      }
      if (!type && D.size()>=test.size() && DivRem(D,test,0,q,r) && r.empty()){
	r=N;
	if (is_symmetric(r,N,true)*is_symmetric(q,D,true)==1){
	  // yes!
	  type = (xvar.type==_SYMB) || D.size()>1;
	  if (type) 
	    D=operator_times(D,makevecteur(1,0,-4),0);
	}
      }
      if (type){
	if (xvar.is_symb_of_sommet(at_tan)){
	  q=N; r=D;
	  xtoinvx(2,q,r,N,D,true);
	  xvar=symbolic(at_sin,ratnormal(2*xvar._SYMBptr->feuille,contextptr));
	}
	else {
	  xvar=xvar+inv(xvar,contextptr);
	}
	num=poly12polynome(N,1,num.dim);
	den=poly12polynome(D,1,den.dim);
	return integrate_rational(r2e(fraction(num,den),l,contextptr),x,remains_to_integrate,xvar,intmode,contextptr);
      }
      test[2]=1;
      N=Nsave; D=Dsave;
      if (N.size()>=test.size() && DivRem(N,test,0,q,r) && r.empty()){
	r=D;
	if (is_symmetric(q,N,false)*is_symmetric(r,D,false)==1){
	  // yes!
	  type = (xvar.type==_SYMB) || D.size()>1;
	}
      }
      if (!type){
	N=Nsave; D=Dsave;
      }
      if (!type && D.size()>=test.size() && DivRem(D,test,0,q,r) && r.empty()){
	r=N;
	if (is_symmetric(r,N,false)*is_symmetric(q,D,false)==1){
	  // yes!
	  type = (xvar.type==_SYMB && xvar._SYMBptr->sommet!=at_tan) || D.size()>1;
	  if (type)
	    D=operator_times(D,makevecteur(1,0,4),0);
	}
      }
      if (type){
	if (xvar.is_symb_of_sommet(at_tan)){
	  q=N; r=D;
	  xtoinvx(-2,q,r,N,D,true);
	  xvar=symbolic(at_tan,ratnormal(2*xvar._SYMBptr->feuille,contextptr));
	}
	else
	  xvar=xvar-inv(xvar,contextptr);
	num=poly12polynome(N,1);
	den=poly12polynome(D,1);
	for (;den.dim!=num.dim;){
	  vector< monomial<gen> >::iterator dt,dtend;
	  if (den.dim<num.dim){
	    dt=den.coord.begin();
	    dtend=den.coord.end();
	    ++den.dim;
	  }
	  else {
	    dt=num.coord.begin();
	    dtend=num.coord.end();
	    ++num.dim;
	  }
	  for (;dt!=dtend;++dt){
	    index_t::const_iterator it=dt->index.begin(),itend=dt->index.end();
	    index_m new_i(itend-it+1);
	    index_t::iterator newit=new_i.begin();    
	    for (;it!=itend;++newit,++it)
	      *newit=*it;
	    *newit=0;
	    dt->index=new_i;
	  }
	}
	simplify(num,den);
	return integrate_rational(r2e(fraction(num,den),l,contextptr),x,remains_to_integrate,xvar,intmode,contextptr);
      }
    }
    if ( (intmode & 2)==0)
      gprintf(step_ratfracsqrfree,gettext("Integrate rational fraction %gen"),makevecteur(_sqrfree(e,contextptr)),contextptr);
    vecteur lf=*l.front()._VECTptr;
    lf.front()=xvar;
    l.front()=lf;
    // l.front()._VECTptr->front()=xvar;
    polynome p_content(lgcd(den));
    polynome primden(den/p_content);
    factorization vden(sqff(primden)); // first square-free factorization
    vector< pf<gen> > pfdecomp;
    polynome ipnum(s),ipden(s),temp(s),tmp(s);
    partfrac(num,den,vden,pfdecomp,ipnum,ipden);
    vector< pf<gen> >::iterator it=pfdecomp.begin();
    vector< pf<gen> >::const_iterator itend=pfdecomp.end();
    vector< pf<gen> > intdecomp,finaldecomp;
    for (;it!=itend;++it){
      if (it->den.lexsorted_degree()==0) 
	continue;
      const pf<gen> & single =intreduce_pf(*it,intdecomp);
      if ( (it->mult>1) && (intmode & 2)==0){
	gen fact1=pow(r2e(it->fact,l,contextptr),it->mult,contextptr);
	gen fact2=it->den/pow(it->fact,it->mult);
	gprintf(step_ratfrachermite,gettext("Partial fraction %gen -> Hermite reduction -> integrate squarefree part %gen"),makevecteur(inv(r2sym(fact2,l,contextptr),contextptr)*r2e(it->num,l,contextptr)/fact1,r2e(single.num,l,contextptr)/r2e(single.den,l,contextptr)),contextptr);
      }
      // factor(single.den,p_content,vden,false,withsqrt(contextptr),complex_mode(contextptr));
      gen extra_div=1;
      factor(single.den,p_content,vden,false,false,false,1,extra_div);
      partfrac(single.num,single.den,vden,finaldecomp,temp,tmp);
    }
    if ( (intmode & 2)==0)
      gprintf(step_ratfracfinal,gettext("Partial fraction integration of %gen"),makevecteur(r2sym(finaldecomp,l_orig,contextptr)),contextptr);
    it=finaldecomp.begin();
    itend=finaldecomp.end();
    gen ratpart=r2sym(intdecomp,l,contextptr);
    // should remove constants in ratpart
    gen tmp1=_fxnd(ratpart,contextptr);
    if (xvar.type==_IDNT && tmp1.type==_VECT && tmp1._VECTptr->size()==2){
      gen tmp2=_quorem(makesequence(tmp1._VECTptr->front(),tmp1._VECTptr->back(),xvar),contextptr);
      if (tmp2.type==_VECT && tmp2._VECTptr->size()==2){
	gen q=tmp2._VECTptr->front(),r=tmp2._VECTptr->back();
	gen C=subst(q,xvar,0,false,contextptr);
	if (!is_zero(C)){
	  q=ratnormal(q-C,contextptr);
	  tmp1=tmp1._VECTptr->back();
	  tmp1=_collect(tmp1,contextptr);
	  tmp1=r*inv(tmp1,contextptr);
	  ratpart=q+tmp1;
	}
      }
    }
    return integrate_rational_end(it,itend,x,xvar,l,lprime,ipnum,ipden,ratpart,remains_to_integrate,intmode,contextptr);
  }

  // integration of e when linear operations have been applied
  static gen xln_x(const gen & x,GIAC_CONTEXT){
    return x*ln(x,contextptr)-x;
  }

  static gen int_exp(const gen & x,GIAC_CONTEXT){
    return exp(x,contextptr);
  }

  static gen int_sinh(const gen & x,GIAC_CONTEXT){
    return cosh(x,contextptr);
  }

  static gen int_cosh(const gen & x,GIAC_CONTEXT){
    return sinh(x,contextptr);
  }

  static gen int_sin(const gen & x,GIAC_CONTEXT){
    if (angle_radian(contextptr))
      return -cos(x,contextptr);
    else if(angle_degree(contextptr))
      return -cos(x,contextptr)*gen(180)/cst_pi;
    //grad
    else
      return -cos(x, contextptr)*gen(200) / cst_pi;
  }

  static gen int_cos(const gen & x,GIAC_CONTEXT){
    if (angle_radian(contextptr))
      return sin(x,contextptr);
    else if(angle_degree(contextptr))
      return sin(x,contextptr)*gen(180)/cst_pi;
    //grad
    else
      return sin(x, contextptr)*gen(200) / cst_pi;
  }

  static gen int_tan(const gen & x,GIAC_CONTEXT){
    gen g=-lnabs(cos(x,contextptr),contextptr);
    if (angle_radian(contextptr))
      return g;
    else if(angle_degree(contextptr))
      return g*gen(180)/cst_pi;
    //grad
    else
      return g*gen(200) / cst_pi;
  }

  static gen int_tanh(const gen & x,GIAC_CONTEXT){
    return -ln(cosh(x,contextptr),contextptr);
  }

  static gen int_asin(const gen & x,GIAC_CONTEXT){
    if (angle_radian(contextptr))
      return x*asin(x,contextptr)+sqrt(1-pow(x,2),contextptr);
    else if(angle_degree(contextptr))
      return x*asin(x,contextptr)*deg2rad_e+sqrt(1-pow(x,2),contextptr);
    //grad
    else
      return x*asin(x, contextptr)*grad2rad_e + sqrt(1 - pow(x, 2), contextptr);
  }

  static gen int_acos(const gen & x,GIAC_CONTEXT){
    if (angle_radian(contextptr))
      return x*acos(x,contextptr)-sqrt(1-pow(x,2),contextptr);
    else if(angle_degree(contextptr))
      return x*acos(x,contextptr)*deg2rad_e-sqrt(1-pow(x,2),contextptr);
    //grad
    else
      return x*acos(x, contextptr)*grad2rad_e - sqrt(1 - pow(x, 2), contextptr);
  }

  static gen int_atan(const gen & x,GIAC_CONTEXT){
    if (angle_radian(contextptr)) 
      return x*atan(x,contextptr)-rdiv(ln(pow(x,2)+1,contextptr),plus_two,contextptr);
    else if(angle_degree(contextptr))
      return x*atan(x,contextptr)*deg2rad_e-rdiv(ln(pow(x,2)+1,contextptr),plus_two,contextptr);
    //grad
    else
      return x*atan(x, contextptr)*grad2rad_e - rdiv(ln(pow(x, 2) + 1, contextptr), plus_two, contextptr);
  }

  static gen int_asinh(const gen & x,GIAC_CONTEXT){
    return x*asinh(x,contextptr)-sqrt(pow(x,2)+1,contextptr);
  }

  static gen int_acosh(const gen & x,GIAC_CONTEXT){
    return x*acosh(x,contextptr)-sqrt(pow(x,2)-1,contextptr);
  }

  static gen int_atanh(const gen & x,GIAC_CONTEXT){
    return x*atan(x,contextptr)-rdiv(ln(abs(pow(x,2)-1,contextptr),contextptr),plus_two,contextptr);
  }

  static const gen_op_context primitive_tab_primitive[]={int_sin,int_cos,int_tan,int_exp,int_sinh,int_cosh,int_tanh,int_asin,int_acos,int_atan,xln_x,int_asinh,int_acosh,int_atanh};

#if 0
  static void insure_real_deno(gen & n,gen & d,GIAC_CONTEXT){
    gen i=im(d,contextptr),c=conj(d,contextptr);
    if (!is_zero(i)){
      n=n*c;
      d=d*c;
    }
  }
#endif
  bool is_rewritable_as_f_of0(const gen & fu,const gen & u,gen & fx,const gen & gen_x,GIAC_CONTEXT);

  static bool in_is_rewritable_as_f_of(const gen & fu,const gen & u,gen & fx,const gen & gen_x,GIAC_CONTEXT){
    if (fu.type==_VECT){
      vecteur res;
      const_iterateur it=fu._VECTptr->begin(),itend=fu._VECTptr->end();
      gen tmp;
      for (;it!=itend;++it){
	if (!is_rewritable_as_f_of0(*it,u,tmp,gen_x,contextptr))
	  return false;
	res.push_back(tmp);
      }
      fx=gen(res,fu.subtype);
      return true;
    }
    if (fu.type==_IDNT){
      if (fu!=gen_x){
	fx=fu;
	return true;
      }
      return false;
    }
    if (fu.type!=_SYMB){
      fx=fu;
      return true;
    }
    // symbolic
    if (fu==u){
      fx=gen_x;
      return true;
    }
    // decompose
    unary_function_ptr s=fu._SYMBptr->sommet;
    gen f=fu._SYMBptr->feuille,tmpfx;
    if (in_is_rewritable_as_f_of(f,u,tmpfx,gen_x,contextptr)){
      fx=symbolic(s,tmpfx);
      return true;
    }
    // try special treatment for integral powers
    int fexp,uexp;
    if ( (u.type!=_SYMB) || (s!=at_pow) || (f._VECTptr->back().type!=_INT_) )
      return false;
    fexp=f._VECTptr->back().val;
    if ( (u._SYMBptr->sommet==at_pow) && (u._SYMBptr->feuille._VECTptr->back().type==_INT_) && (u._SYMBptr->feuille._VECTptr->front()==f._VECTptr->front()) ){
      uexp=u._SYMBptr->feuille._VECTptr->back().val;
      if (fexp%uexp)
	return false;
      fx=pow(gen_x,fexp/uexp);
      return true;
    }
    // trigonometric fcns to an even power
    f=f._VECTptr->front();
    if ( (fexp %2) || (f.type!=_SYMB) )
      return false;
    fexp=fexp/2;
    int ftrig=equalposcomp(primitive_tab_op,f._SYMBptr->sommet);
    if (!ftrig)
      return false;
    int utrig=equalposcomp(primitive_tab_op,u._SYMBptr->sommet);
    if (utrig==2 && u._SYMBptr->feuille==2*f._SYMBptr->feuille){
      // sin^2/cos^2/tan^2 in terms of cos(2x)
      switch (ftrig){
      case 1: // sin
	fx=pow((1-gen_x)/2,fexp);
	return true;
      case 2: // cos
	fx=pow((1+gen_x)/2,fexp);
	return true;
      case 3:
	fx=pow((1-gen_x)/(1+gen_x),fexp);
	return true;
      }
    }
    if (!utrig || f._SYMBptr->feuille!=u._SYMBptr->feuille)
      return false;
    switch (ftrig){
    case 1: // sin
      switch (utrig){
      case 2: // sin^2=1-cos^2
	fx=pow(1-pow(gen_x,2),fexp);
	return true;
      case 3: // sin^2=1-1/(tan^2+1)
	fx=pow(1-inv(pow(gen_x,2)+1,contextptr),fexp);
	return true;
      default:
	return false;
      }
    case 2: // cos
      switch (utrig){
      case 1: // cos^2=1-sin^2
	fx=pow(1-pow(gen_x,2),fexp);
	return true;
      case 3: // cos^2=1/(tan^2+1)
	fx=pow(pow(gen_x,2)+1,-fexp);
	return true;
      default:
	return false;
      }
    case 3: // tan
      switch (utrig){
      case 1: // tan^2=1/(1-sin^2)-1
	fx=pow(inv(1-pow(gen_x,2),contextptr)-1,fexp);
	return true;
      case 2: // tan^2=1/cos^2-1
	fx=pow(pow(gen_x,-2)-1,fexp);
	return true;
      default:
	return false;
      }
    }
    return false;
  }

  // try to rewrite fu(x), function of x as a fonction of u(x), if possible 
  // return fx(x) such that fu(x)=fx(u(x))
  // FIXME: should detect u=pow(.,inv(n)) with n integer
  bool is_rewritable_as_f_of0(const gen & fu,const gen & u,gen & fx,const gen & gen_x,GIAC_CONTEXT){
    gen a,b;
    if (is_linear_wrt(u,gen_x,a,b,contextptr)){
      fx=complex_subst(fu,gen_x,rdiv(gen_x-b,a,contextptr),contextptr);
      return false;// true?
    }
    // try first if u is a linear expression of something else
    if (u.type==_SYMB){
      if (u._SYMBptr->sommet==at_neg){
	gen tmpu=u._SYMBptr->feuille,tmpfx;
	if (!is_rewritable_as_f_of0(fu,tmpu,tmpfx,gen_x,contextptr))
	  return false;
	fx=complex_subst(tmpfx,gen_x,-gen_x,contextptr);
	return true;
      }
      if (u._SYMBptr->sommet==at_pow){
	gen tmpu=u._SYMBptr->feuille,tmpfx;
	if (tmpu.type==_VECT && tmpu._VECTptr->size()==2){
	  gen expo=ratnormal(inv(tmpu._VECTptr->back(),contextptr),contextptr);
	  tmpu=tmpu._VECTptr->front();
	  if (expo.type==_INT_){ 
	    if (is_linear_wrt(tmpu,gen_x,a,b,contextptr)){
	      fx=complex_subst(fu,makevecteur(u,gen_x),makevecteur(gen_x,rdiv(pow(gen_x,expo,contextptr)-b,a,contextptr)),contextptr);
	      return true;
	    }
	    if (is_rewritable_as_f_of0(fu,tmpu,fx,gen_x,contextptr)){
	      fx=complex_subst(fx,gen_x,pow(gen_x,expo,contextptr),contextptr);
	      return true;
	    }
	  }
	}
      }
      gen alpha;
      vecteur non_constant;
      if (u._SYMBptr->sommet==at_prod ){
	if (u._SYMBptr->feuille.type!=_VECT)
	  return is_rewritable_as_f_of0(fu,u._SYMBptr->feuille,fx,gen_x,contextptr);
	decompose_prod(*u._SYMBptr->feuille._VECTptr,gen_x,non_constant,alpha,true,contextptr);
	if (non_constant.empty()) return false; // setsizeerr(gettext("in is_rewritable_as_f_of_f"));
	if (!is_one(alpha)){
	  gen tmpu,tmpfx;
	  tmpu=_prod(non_constant,contextptr);
	  if (!is_rewritable_as_f_of0(fu,tmpu,tmpfx,gen_x,contextptr))
	    return false;
	  fx=complex_subst(tmpfx,gen_x,rdiv(gen_x,alpha,contextptr),contextptr);
	  return true;
	}
      }
      if (u._SYMBptr->sommet==at_plus){
	if (u._SYMBptr->feuille.type!=_VECT)
	  return is_rewritable_as_f_of0(fu,u._SYMBptr->feuille,fx,gen_x,contextptr);
	if (_is_polynomial(makesequence(fu,gen_x),contextptr)==1 && _is_polynomial(makesequence(u,gen_x),contextptr)==1){
	  gen FU=_symb2poly(makesequence(fu,gen_x),contextptr);
	  gen U=_symb2poly(makesequence(u,gen_x),contextptr);
	  if (FU.type==_VECT && U.type==_VECT){
	    vecteur vfu=*FU._VECTptr;
	    vecteur vu=*U._VECTptr;
	    int N=vfu.size()-1,M=vu.size()-1;
	    vecteur vfx(N/M+1);
	    for (;!vfu.empty();){
	      int n=vfu.size()-1,m=vu.size()-1;
	      if (n % m)
		break;
	      gen c=vfu[0]/pow(vu[0],n/m,contextptr);
	      vfx[N/M-n/m]=c;
	      vecteur vtmp;
	      gen cunm=_symb2poly(makesequence(c*pow(u,n/m),gen_x),contextptr);
	      submodpoly(vfu,gen2vecteur(cunm),vtmp);
	      vfu=*normal(vtmp,contextptr)._VECTptr;
	      vfu=trim(vfu,0);
	    }
	    if (vfu.empty()){
	      fx=_poly2symb(makesequence(vfx,gen_x),contextptr);
	      return true;
	    }
	  }
	}
	decompose_plus(*u._SYMBptr->feuille._VECTptr,gen_x,non_constant,alpha,contextptr);
	if (non_constant.empty()) return false; // setsizeerr(gettext("in is_rewritable_as_f_of_f 2"));
	if (!is_zero(alpha)){
	  gen tmpu,tmpfx;
	  tmpu=_plus(non_constant,contextptr);
	  if (!is_rewritable_as_f_of0(fu,tmpu,tmpfx,gen_x,contextptr))
	    return in_is_rewritable_as_f_of(fu,u,fx,gen_x,contextptr);;
	  fx=complex_subst(tmpfx,gen_x,gen_x-alpha,contextptr);
	  if (!has_i(fx)) // FIX for int(x^3/sqrt(1-x^2),x,-1,0);
	    return true;
	}
      }
    }
    return in_is_rewritable_as_f_of(fu,u,fx,gen_x,contextptr);
  }

  bool is_rewritable_as_f_of(const gen & fu_,const gen & u,gen & fx,const gen & gen_x,GIAC_CONTEXT){
    gen tempu(identificateur("tmpu"));
    gen fu=complex_subst(fu_,u,tempu,contextptr);
    if (is_undef(fu) || !is_rewritable_as_f_of0(fu,u,fx,gen_x,contextptr))
      return false;
    fx=complex_subst(fx,tempu,gen_x,contextptr);
    return true;
  }    

  gen firstcoefftrunc(const gen & e){
    if (e.type==_FRAC)
      return fraction(firstcoefftrunc(e._FRACptr->num),firstcoefftrunc(e._FRACptr->den));
    if (e.type==_POLY)
      return firstcoeff(*e._POLYptr).trunc1();
    return e;
  }

  // special version of lvarx that does not remove cst powers
  vecteur lvarxpow(const gen &e,const gen & x){
    const vecteur & v=lvar(e);
    vecteur res;
    vecteur::const_iterator it=v.begin(),itend=v.end();
    for (;it!=itend;++it){
      if (contains(*it,x))
	res.push_back(*it);
    }
    // do lvar again to comprim the first arg of ^
    return lvar(res);
  }

  gen invexptoexpneg(const gen& g,GIAC_CONTEXT){
    if (g.type==_SYMB && g._SYMBptr->sommet==at_exp)
      return exp(-g._SYMBptr->feuille,contextptr);
    else
      return symb_inv(g);
  }

  gen integrate_gen_rem(const gen & e_orig,const gen & x_orig,gen & remains_to_integrate,int intmode,GIAC_CONTEXT){
    if (x_orig.type!=_IDNT){
      gen x(identificateur("tmp_x"));
      gen e=subst(e_orig,x_orig,x,false,contextptr);
      e=integrate_id_rem(e,x,remains_to_integrate,contextptr,intmode);
      remains_to_integrate=quotesubst(remains_to_integrate,x,x_orig,contextptr);
      return quotesubst(e,x,x_orig,contextptr);
    }
    return integrate_id_rem(e_orig,x_orig,remains_to_integrate,contextptr,intmode);
  }

  static bool integrate_step0(gen & e,const gen & gen_x,vecteur & l1,vecteur & m1,gen & res,gen & remains_to_integrate,GIAC_CONTEXT,int intmode){
    const identificateur & id_x=*gen_x._IDNTptr;
    vecteur l2,m2,l3,l4;
    const_iterateur it=l1.begin(),itend=l1.end();
    int i=0;
    for (;it!=itend;++it,++i){
      gen tmp=it->_SYMBptr->feuille;
      gen tmpi(identificateur("tmps"+print_INT_(i)));
      l2.push_back(tmpi*tmp);
      l3.push_back(tmpi);
      l4.push_back(symbolic(at_sign,tmp));
    }
    it=m1.begin(),itend=m1.end();
    for (;it!=itend;++it,++i){
      l1.push_back(*it);
      gen tmp=it->_SYMBptr->feuille;
      gen tmpi(identificateur("tmps"+print_INT_(i)));
      l2.push_back(tmpi);
      l3.push_back(tmpi);
      l4.push_back(*it);
    }      
    *logptr(contextptr) << gettext("Warning, integration of abs or sign assumes constant sign by intervals (correct if the argument is real):\nCheck ") << l1 << '\n';
    e=complex_subst(e,l1,l2,contextptr);
    res=integrate_id_rem(e,gen_x,remains_to_integrate,contextptr,intmode);
    gen resadd;
    if (is_undef(res)) return true;
    // check what happens when si==0
    for (int j=0;j<i;++j){
      gen val=l4[j],a,b,r;
      if (val.is_symb_of_sommet(at_sign)){
	gen val2=val._SYMBptr->feuille;
	if (val2.is_symb_of_sommet(at_sin) || val2.is_symb_of_sommet(at_tan))
	  val2=val2._SYMBptr->feuille;
	bool warn=true;
	if (is_linear_wrt(val2,gen_x,a,b,contextptr) && ((has_evalf(a,r,1,contextptr) && has_evalf(b,r,1,contextptr)) || lvar(res)==lidnt(res))){
	  warn=val._SYMBptr->feuille!=val2;
	  r=-b/a;
	  vecteur l5(l4);
#if 1
	  l5[j]=1;
	  gen limsup=subst(res,l3,l5,false,contextptr);
	  l5[j]=-1;
	  gen liminf=subst(res,l3,l5,false,contextptr);
#else
	  l5[j]=1;
	  bool dolim=l3.size()==1 && l5.size()==1 && l3.front().type==_IDNT;
	  gen limsup=dolim?limit(res,*l3.front()._IDNTptr,l5.front(),0,contextptr):subst(res,l3,l5,false,contextptr);
	  l5[j]=-1;
	  gen liminf=dolim?limit(res,*l3.front()._IDNTptr,l5.front(),0,contextptr):subst(res,l3,l5,false,contextptr);
#endif
	  gen tmp=ratnormal((limit(liminf,id_x,r,-1,contextptr)-limit(limsup,id_x,r,1,contextptr))/2,contextptr)*val;
	  if (is_undef(tmp) || is_inf(tmp))
	    *logptr(contextptr) << gettext("Unable to cancel step at ")+r.print(contextptr) + " of " << limsup << "-" << liminf << '\n';
	  else
	    resadd += tmp;
	}
	if (warn)
	  *logptr(contextptr) << gettext("Discontinuities at zeroes of ") << val._SYMBptr->feuille << " were not checked" << '\n';
      }
    }
    remains_to_integrate=complex_subst(remains_to_integrate,l3,l4,contextptr);
    res=resadd+complex_subst(res,l3,l4,contextptr);
    return true;
  }

  static bool detect_inv_trigln(gen & e,vecteur & rvar,const gen & gen_x,gen & res,gen & remains_to_integrate,bool additional_check,int intmode,GIAC_CONTEXT){
    const_iterateur rvt=rvar.begin(),rvtend=rvar.end();
    for (;rvt!=rvtend;++rvt){
      if (rvt->type!=_SYMB)
	continue;
      int rvtt=equalposcomp(inverse_tab_op,rvt->_SYMBptr->sommet);
      if (!rvtt || rvtt==3 || rvtt==7) // exclude atan and atanh
	continue;
      rvtt=equalposcomp(primitive_tab_op,rvt->_SYMBptr->sommet);
      if (rvtt>7){
	unary_function_ptr inverse_sommet=primitive_tab_op[rvtt-8];
	gen feuille=rvt->_SYMBptr->feuille,a,b;
	if (!is_linear_wrt(feuille,gen_x,a,b,contextptr))
	  continue;
	if (additional_check){
	  // Additionaly check that e is polynomial wrt x
	  gen tmpidnt(identificateur("tmpt"));
	  gen tmpcheck=subst(e,*rvt,tmpidnt,false,contextptr);
	  vecteur vx2(rlvarx(tmpcheck,gen_x));
	  if ( vx2.size()>1)
	    continue;
	  if (vx2.size()){
	    lvar(tmpcheck,vx2);
	    fraction ftemp=sym2r(tmpcheck,vx2,contextptr);
	    if (ftemp.den.type==_POLY && ftemp.den._POLYptr->lexsorted_degree())
	      continue;
	  }
	}
	// make the change of var ln[ax+b]=t -> x=rdiv(exp(t)-b,a)
	gen tmprem,tmpres,tmpe,xt,dxt,sqrtxt;
	xt=rdiv(symbolic(inverse_sommet,gen_x)-b,a,contextptr);
	dxt=derive(xt,gen_x,contextptr);
	if (is_undef(dxt)){
	  res=dxt;
	  return true;
	}
	// should add sqrt(1-.^2)
	vecteur substin(makevecteur(gen_x,*rvt));
	vecteur substout(makevecteur(xt,gen_x));
	if ((rvtt==8 || rvtt==9)){
	  vecteur tmpv=lop(e,at_pow);
	  for (unsigned tmpi=0;tmpi<tmpv.size();++tmpi){
	    gen tmpvi=tmpv[tmpi]._SYMBptr->feuille;
	    if (tmpvi.type==_VECT && tmpvi._VECTptr->size()==2){
	      gen tmpvi0=tmpvi._VECTptr->front();
	      if (ratnormal(tmpvi0-1+pow(a*gen_x+b,2,contextptr),contextptr)==0){
		substin.push_back(tmpv[tmpi]);
		substout.push_back(pow(symbolic(rvtt==8?at_cos:at_sin,gen_x),2*tmpvi._VECTptr->back(),contextptr));
	      }
	    }
	  }
	}
	tmpe=ratnormal(complex_subst(e,substin,substout,contextptr)*dxt,contextptr);
	if ( (intmode & 2)==0)
	  gprintf(step_ratfracchgvar,gettext("Integrate %gen, change of variable %gen->%gen, new integral %gen"),makevecteur(e,gen_x,xt,tmpe),contextptr);
	tmpres=linear_integrate_nostep(tmpe,gen_x,tmprem,intmode,contextptr);
	if ( (intmode & 2)==0)
	  gprintf(step_backsubst,gettext("Back substitution %gen->%gen in %gen"),makevecteur(gen_x,*rvt,tmpres),contextptr);
	remains_to_integrate=complex_subst(rdiv(tmprem,dxt,contextptr),gen_x,*rvt,contextptr);
	// replace tan(asin/2) or tan(acos/2) and cos(asin) and sin(acos)
	if ((rvtt==8 || rvtt==9) && has_op(tmpres,*at_tan))
	  tmpres=tan2sincos2(tmpres,contextptr);
	tmpres=_texpand(tmpres,contextptr);
	res=complex_subst(tmpres,substout,substin,contextptr);
	return true;
      }
    }
    return false;
  }

  gen integrate_id_rem(const gen & e_orig,const gen & gen_x,gen & remains_to_integrate,GIAC_CONTEXT){
    return integrate_id_rem(e_orig,gen_x,remains_to_integrate,contextptr,0);
  }

  gen add_lnabs(const gen & g,GIAC_CONTEXT){
    return symbolic(at_ln,abs(g,contextptr));
  }

  void surd2pow(const gen & e,vecteur & subst1,vecteur & subst2,GIAC_CONTEXT){
    vecteur l1surd(lop(e,at_surd));
    vecteur l2surd(l1surd);
    for (unsigned i=0;i<l1surd.size();++i){
      gen & g=l2surd[i];
      if (g._SYMBptr->feuille.type==_VECT && g._SYMBptr->feuille._VECTptr->size()==2){
	vecteur gv=*g._SYMBptr->feuille._VECTptr;
	gv=makevecteur(gv[0],inv(gv[1],contextptr));
	g=_pow(gen(gv,_SEQ__VECT),contextptr);//symbolic(at_pow,gen(gv,_SEQ__VECT));
      }
    }
    vecteur l1NTHROOT(lop(e,at_NTHROOT));
    vecteur l2NTHROOT(l1NTHROOT);
    for (unsigned i=0;i<l1NTHROOT.size();++i){
      gen & g=l2NTHROOT[i];
      if (g._SYMBptr->feuille.type==_VECT && g._SYMBptr->feuille._VECTptr->size()==2){
	vecteur gv=*g._SYMBptr->feuille._VECTptr;
#if defined GIAC_GGB
	  gv=makevecteur(subst(gv[1],l1NTHROOT,l2NTHROOT,false,contextptr),inv(gv[0],contextptr));
#else
	  gv=makevecteur(gv[1],inv(gv[0],contextptr));
#endif
	g=_pow(gen(gv,_SEQ__VECT),contextptr);//symbolic(at_pow,gen(gv,_SEQ__VECT));
      }
    }
    subst1=mergevecteur(l1surd,l1NTHROOT);
    subst2=mergevecteur(l2surd,l2NTHROOT);
    if (!subst1.empty())
      *logptr(contextptr) << gettext("Temporary replacing surd/NTHROOT by fractional powers") << '\n';
  }

  bool is_elementary(const vecteur & v,const gen & x){
    const_iterateur it=v.begin(),itend=v.end();
    for (;it!=itend;++it){
      if (*it==x)
	continue;
      if (!it->is_symb_of_sommet(at_exp) && (!it->is_symb_of_sommet(at_ln)) )
	return false;
    }
    return true;
  }

  bool when2sign(gen &e,const gen &gen_x,GIAC_CONTEXT){
    vecteur lwhen(lop(e,at_when));
    if (!lwhen.empty()) lwhen=lvarx(lwhen,gen_x);
    if (!lwhen.empty()){
      vecteur l2;
      const_iterateur it=lwhen.begin(),itend=lwhen.end();
      int i=0;
      for (;it!=itend;++it,++i){
	gen tmp=it->_SYMBptr->feuille,repl;
	if (tmp.type!=_VECT || tmp._VECTptr->size()!=3)
	  return false;
	vecteur & whenargs = *tmp._VECTptr;
	tmp = whenargs[0];
	if ( (tmp.is_symb_of_sommet(at_superieur_strict) || 
	     tmp.is_symb_of_sommet(at_superieur_egal) ) &&
	     (repl=tmp._SYMBptr->feuille).type==_VECT && repl._VECTptr->size()==2){
	  repl=repl._VECTptr->back()-repl._VECTptr->front();
	  repl=(symbolic(at_sign,repl)+1)/2;
	}
	else {
	  repl=symbolic(at_same,gen(makevecteur(tmp,0),_SEQ__VECT));
	  repl=symbolic(at_sign,repl);
	}
	l2.push_back(whenargs[1]+repl*(whenargs[2]-whenargs[1]));
      }
      e=complex_subst(e,lwhen,l2,contextptr);      
    }
    return true;
  }

  // A bounded syntax check: no expansion, poles, branches, or symbolic
  // coefficients. Such a polynomial is continuous at every finite real x.
  static bool small_polynomial(const gen &e,const gen &x,unsigned &budget){
    if (!budget) return false;
    --budget;
    if (e==x || e.type==_INT_ || e.type==_ZINT) return true;
    if (e.type==_FRAC)
      return (e._FRACptr->num.type==_INT_ || e._FRACptr->num.type==_ZINT) &&
        (e._FRACptr->den.type==_INT_ || e._FRACptr->den.type==_ZINT) &&
        !is_zero(e._FRACptr->den);
    if (e.type!=_SYMB) return false;
    const gen &f=e._SYMBptr->feuille;
    unary_function_ptr op=e._SYMBptr->sommet;
    if (op==at_neg) return small_polynomial(f,x,budget);
    if (f.type!=_VECT) return false;
    const vecteur &v=*f._VECTptr;
    if (op==at_pow)
      return v.size()==2 && v[1].type==_INT_ && v[1].val>=0 &&
        small_polynomial(v[0],x,budget);
    if (op!=at_plus && op!=at_prod) return false;
    for (unsigned i=0;i<v.size();++i)
      if (!small_polynomial(v[i],x,budget)) return false;
    return true;
  }

  // Syntactic parity for a small class of everywhere-continuous functions.
  // -1 means not proved; 0 even; 1 odd. In particular 1/x and tan(x)
  // are rejected: symmetry alone does not make an improper integral exist.
  static int continuous_parity(const gen &e,const gen &x,unsigned &budget){
    if (!budget) return -1;
    --budget;
    if (e==x) return 1;
    if (e.type==_INT_ || e.type==_ZINT) return 0;
    if (e.type==_FRAC){
      unsigned b=3;
      return small_polynomial(e,x,b)?0:-1;
    }
    if (e.type!=_SYMB) return -1;
    const gen &f=e._SYMBptr->feuille;
    unary_function_ptr op=e._SYMBptr->sommet;
    if (op==at_neg || op==at_sin || op==at_cos){
      int p=continuous_parity(f,x,budget);
      return p<0?-1:(op==at_cos?0:p);
    }
    if (f.type!=_VECT) return -1;
    const vecteur &v=*f._VECTptr;
    if (op==at_pow){
      if (v.size()!=2 || v[1].type!=_INT_ || v[1].val<0) return -1;
      int p=continuous_parity(v[0],x,budget);
      return p<0?-1:(p && v[1].val%2);
    }
    if ((op!=at_plus && op!=at_prod) || v.empty()) return -1;
    int p=continuous_parity(v[0],x,budget);
    for (unsigned i=1;p>=0 && i<v.size();++i){
      int q=continuous_parity(v[i],x,budget);
      p=q<0?-1:(op==at_prod?(p^q):(p==q?p:-1));
    }
    return p;
  }

  // Parse only an already expanded sum of a few monomials. The work and
  // storage depend on term count, not degree; never allocate a dense table.
  static bool small_sparse_polynomial(const gen &e,const gen &x,sparse_poly1 &out,GIAC_CONTEXT){
    out.clear();
    const vecteur *terms=e.is_symb_of_sommet(at_plus) && e._SYMBptr->feuille.type==_VECT?
      e._SYMBptr->feuille._VECTptr:0;
    unsigned count=terms?terms->size():1;
    if (count>8) return false;
    for (unsigned i=0;i<count;++i){
      gen t=terms?(*terms)[i]:e,c=1;int degree=0;
      if (t.is_symb_of_sommet(at_neg)){c=-1;t=t._SYMBptr->feuille;}
      c=c*extract_cst(t,x,contextptr);
      if (t==x) degree=1;
      else if (t.is_symb_of_sommet(at_pow) && t._SYMBptr->feuille.type==_VECT){
        const vecteur &p=*t._SYMBptr->feuille._VECTptr;
        if (p.size()!=2 || p[0]!=x || p[1].type!=_INT_ || p[1].val<0 || p[1].val>32767)
          return false;
        degree=p[1].val;
      }
      else {
        if (t.type!=_INT_ && t.type!=_ZINT && t.type!=_FRAC) return false;
        c=c*t;
      }
      if (c.type!=_INT_ && c.type!=_ZINT && c.type!=_FRAC) return false;
      unsigned budget=3;
      if (!small_polynomial(c,x,budget)) return false;
      unsigned j=0;
      for (;j<out.size();++j)
        if (out[j].exponent.val==degree){out[j].coeff+=c;break;}
      if (j==out.size()) out.push_back(monome(c,degree));
    }
    return true;
  }

  // Recognize U'/(A+U^2), A>0, using bounded sparse coefficient matching.
  // This avoids factoring a degree-4000 denominator with only four terms.
  static bool integrate_sparse_atan(const gen &e,const gen &x,gen &primitive,GIAC_CONTEXT){
    if (!e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &f=*e._SYMBptr->feuille._VECTptr;
    if (f.size()!=2) return false;
    unsigned di=f[0].is_symb_of_sommet(at_inv)?0:1;
    if (!f[di].is_symb_of_sommet(at_inv)) return false;
    sparse_poly1 u,den;
    if (!small_sparse_polynomial(f[1-di],x,u,contextptr) || u.empty() || u.size()>3)
      return false;
    bool large=false;
    for (unsigned i=0;i<u.size();++i){
      large=large || u[i].exponent.val>=32;
      u[i].exponent+=1;
      u[i].coeff=rdiv(u[i].coeff,u[i].exponent,contextptr);
    }
    if (!large || !small_sparse_polynomial(f[di]._SYMBptr->feuille,x,den,contextptr)) return false;
    for (unsigned i=0;i<u.size();++i) for (unsigned j=0;j<u.size();++j){
      int degree=u[i].exponent.val+u[j].exponent.val;
      gen coeff=u[i].coeff*u[j].coeff;
      if (is_zero(coeff)) continue;
      unsigned k=0;
      for (;k<den.size();++k)
        if (den[k].exponent.val==degree){den[k].coeff-=coeff;break;}
      if (k==den.size()) return false;
    }
    gen a=0;
    for (unsigned i=0;i<den.size();++i){
      if (den[i].exponent.val==0) a=den[i].coeff;
      else if (!is_zero(den[i].coeff)) return false;
    }
    if (!is_strictly_positive(a,contextptr)) return false;
    gen U=0;
    for (unsigned i=0;i<u.size();++i)
      U+=u[i].coeff*pow(x,u[i].exponent,contextptr);
    a=sqrt(a,contextptr);
    primitive=rdiv(atan(rdiv(U,a,contextptr),contextptr),a,contextptr);
    return true;
  }

  // Complete periods need neither a high-degree trig expansion nor one
  // singularity search per zero of abs(sin). Keep exact interval arithmetic.
  static bool integrate_trig_periods(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (angle_mode(contextptr)) return false;
    gen f=e,c=extract_cst(f,x,contextptr),n;
    bool absolute=f.is_symb_of_sommet(at_abs);
    if (absolute) f=f._SYMBptr->feuille;
    else {
      if (!f.is_symb_of_sommet(at_pow) || f._SYMBptr->feuille.type!=_VECT) return false;
      const vecteur &v=*f._SYMBptr->feuille._VECTptr;
      if (v.size()!=2 || v[1].type!=_INT_ || v[1].val<32 ||
          v[1].val>8192 || v[1].val%2) return false;
      n=v[1];f=v[0];
    }
    if (f.is_symb_of_sommet(at_neg)) f=f._SYMBptr->feuille;
    if (!f.is_symb_of_sommet(at_sin) && !f.is_symb_of_sommet(at_cos)) return false;
    gen a,b;
    if (!is_linear_wrt(f._SYMBptr->feuille,x,a,b,contextptr) || is_zero(a)) return false;
    gen realvals=evalf_double(makevecteur(a,b,lo,hi),1,contextptr);
    if (realvals.type!=_VECT) return false;
    for (unsigned i=0;i<realvals._VECTptr->size();++i){
      const gen &v=(*realvals._VECTptr)[i];
      if (v.type!=_DOUBLE_ || is_inf(v) || is_undef(v)) return false;
    }
    gen periods=ratnormal(a*(hi-lo)/cst_pi,contextptr);
    if (periods.type!=_INT_ && periods.type!=_ZINT) return false;
    res=c*(hi-lo)*(absolute?rdiv(2,cst_pi,contextptr):rdiv(comb(n,n/2,contextptr),pow(2,n,contextptr),contextptr));
    return true;
  }

  // Recognize c*u'*u^n structurally before rational normalization expands
  // large powers. Only integer polynomial powers use this shortcut, so a
  // finite definite integral also needs no singularity/branch search.
  static bool integrate_large_power(const gen &e,const gen &x,gen &primitive,GIAC_CONTEXT){
    bool negative=e.is_symb_of_sommet(at_neg);
    const gen &product=negative?e._SYMBptr->feuille:e;
    if (!product.is_symb_of_sommet(at_prod) || product._SYMBptr->feuille.type!=_VECT)
      return false;
    unsigned budget=64;
    if (!small_polynomial(product,x,budget)) return false;
    const vecteur &v=*product._SYMBptr->feuille._VECTptr;
    for (unsigned i=0;i<v.size();++i){
      if (!v[i].is_symb_of_sommet(at_pow)) continue;
      const vecteur &p=*v[i]._SYMBptr->feuille._VECTptr;
      if (p[1].val<32 || p[0].type!=_SYMB) continue;
      gen d=derive(p[0],x,contextptr),rest=1;
      if (is_zero(d) || is_undef(d)) continue;
      for (unsigned j=0;j<v.size();++j)
        if (j!=i) rest=rest*v[j];
      gen dc=1,rc=1;
      if (d.is_symb_of_sommet(at_neg)){dc=-1;d=d._SYMBptr->feuille;}
      if (rest.is_symb_of_sommet(at_neg)){rc=-1;rest=rest._SYMBptr->feuille;}
      dc=dc*extract_cst(d,x,contextptr);
      rc=rc*extract_cst(rest,x,contextptr);
      if (d!=rest || is_zero(dc)) continue;
      gen n=p[1]+1;
      primitive=rdiv(rc,dc*n,contextptr)*symbolic(at_pow,makesequence(p[0],n));
      if (negative) primitive=-primitive;
      return true;
    }
    return false;
  }

  static bool integration_rational(const gen &g){
    if (g.type==_INT_ || g.type==_ZINT) return true;
    return g.type==_FRAC && (g._FRACptr->num.type==_INT_ || g._FRACptr->num.type==_ZINT) &&
      (g._FRACptr->den.type==_INT_ || g._FRACptr->den.type==_ZINT) && !is_zero(g._FRACptr->den);
  }

  // Canonicalize arithmetic only. Calling eval here would turn exp(k*x)
  // into powers of exp(x) and erase useful substitution structure.
  // Keep these bounded dispatch paths compact in the calculator ROM.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_syntax(const gen &g,GIAC_CONTEXT){
    if (g.type!=_SYMB) return g;
    const gen &f=g._SYMBptr->feuille;
    unary_function_ptr op=g._SYMBptr->sommet;
    if (f.type!=_VECT){
      gen a=integration_syntax(f,contextptr);
      if (op==at_neg) return -a;
      if (op==at_inv){
        if (a.is_symb_of_sommet(at_prod) && a._SYMBptr->feuille.type==_VECT){
          gen r=1;const vecteur &p=*a._SYMBptr->feuille._VECTptr;
          for (unsigned i=0;i<p.size();++i) r=r*inv(p[i],contextptr);
          return r;
        }
        return inv(a,contextptr);
      }
      return symbolic(op,a);
    }
    vecteur v=*f._VECTptr;
    for (unsigned i=0;i<v.size();++i) v[i]=integration_syntax(v[i],contextptr);
    // Canonical identities without expanding a power or evaluating its base.
    // Use the engine for exponent zero to preserve its literal 0^0 handling.
    if (op==at_pow && v.size()==2){
      if (is_one(v[1])) return v[0];
      if (is_zero(v[1])) return pow(v[0],0);
    }
    if (op==at_division && v.size()==2)
      return v[0]*integration_syntax(symbolic(at_inv,v[1]),contextptr);
    if (op==at_prod || op==at_plus){
      gen r=op==at_prod?1:0;
      for (unsigned i=0;i<v.size();++i) r=op==at_prod?r*v[i]:r+v[i];
      return r;
    }
    return symbolic(op,gen(v,f.subtype));
  }

  static bool integration_power(const gen &g,gen &base,int n){
    if (n==-1 && g.is_symb_of_sommet(at_inv)){base=g._SYMBptr->feuille;return true;}
    if (g.is_symb_of_sommet(at_inv)) return integration_power(g._SYMBptr->feuille,base,-n);
    if (!g.is_symb_of_sommet(at_pow) || g._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if (v.size()!=2 || v[1]!=n) return false;
    base=v[0];return true;
  }

  static bool integration_one_plus(const gen &g,gen &other){
    if (!g.is_symb_of_sommet(at_plus) || g._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if (v.size()!=2) return false;
    if (is_one(v[0])){other=v[1];return true;}
    if (is_one(v[1])){other=v[0];return true;}
    return false;
  }

  static gen integration_coefficient(gen &g,const gen &x,GIAC_CONTEXT){
    gen c=1;
    if (g.is_symb_of_sommet(at_neg)){c=-1;g=g._SYMBptr->feuille;}
    return c*extract_cst(g,x,contextptr);
  }

  // Bounded monomial arithmetic; no dense polynomial conversion.
  static bool integration_monomial(gen g,const gen &x,gen &c,gen &n,GIAC_CONTEXT){
    c=integration_coefficient(g,x,contextptr);
    if (!integration_rational(c)) return false;
    if (integration_rational(g)){c=c*g;n=0;return true;}
    if (g==x){n=1;return true;}
    if (g.is_symb_of_sommet(at_inv)){
      gen d;
      if (!integration_monomial(g._SYMBptr->feuille,x,d,n,contextptr) || is_zero(d)) return false;
      c=c/d;n=-n;return true;
    }
    if (g.is_symb_of_sommet(at_sqrt) && g._SYMBptr->feuille==x){n=gen(1)/2;return true;}
    if (!g.is_symb_of_sommet(at_pow) || g._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if (v.size()!=2 || !integration_rational(v[1])) return false;
    if (v[0]!=x){
      gen d,m;
      if (v[1].type!=_INT_ || v[1].val < -16 || v[1].val>16 ||
          !integration_monomial(v[0],x,d,m,contextptr) || is_zero(d)) return false;
      c=c*pow(d,v[1].val);n=m*v[1];return true;
    }
    n=v[1];return true;
  }

  // x^m*(a+b*x^k)^n: if q=(m+1)/k is a small positive integer,
  // u=a+b*x^k leaves just (u-a)^(q-1)*u^n. Expand at most eight
  // terms, independently of n. Fractional powers are local primitives only.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_binomial_chain(const gen &e,const gen &x,gen &res,bool polynomial_only,GIAC_CONTEXT){
    if (!e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()>4) return false;
    for (unsigned i=0;i<v.size();++i){
      vecteur p;
      if (v[i].is_symb_of_sommet(at_sqrt)) p=makevecteur(v[i]._SYMBptr->feuille,gen(1)/2);
      else if (v[i].is_symb_of_sommet(at_pow) && v[i]._SYMBptr->feuille.type==_VECT) p=*v[i]._SYMBptr->feuille._VECTptr;
      else continue;
      // Evaluation may express u^(2/3) as (u^(1/3))^2. Only unwrap an
      // integer outer power, which preserves principal-power branches.
      if (p.size()==2 && p[1].type==_INT_ && p[0].is_symb_of_sommet(at_pow) && p[0]._SYMBptr->feuille.type==_VECT){
        const vecteur inner=*p[0]._SYMBptr->feuille._VECTptr;
        if (inner.size()==2 && integration_rational(inner[1])){p[0]=inner[0];p[1]=p[1]*inner[1];}
      }
      if (p.size()!=2 || !integration_rational(p[1]) || !is_strictly_positive(p[1],contextptr) ||
          is_strictly_greater(p[1],8192,contextptr) || !p[0].is_symb_of_sommet(at_plus) ||
          p[0]._SYMBptr->feuille.type!=_VECT) continue;
      const vecteur &u=*p[0]._SYMBptr->feuille._VECTptr;
      if (u.size()!=2) continue;
      gen a,b,k,c0,k0,c1,k1;
      if (!integration_monomial(u[0],x,c0,k0,contextptr) || !integration_monomial(u[1],x,c1,k1,contextptr)) continue;
      if (is_zero(k0)){a=c0;b=c1;k=k1;}
      else if (is_zero(k1)){a=c1;b=c0;k=k0;}
      else continue;
      if (is_zero(b) || !is_strictly_positive(k,contextptr)) continue;
      gen rest=1,c,m;
      for (unsigned j=0;j<v.size();++j) if (i!=j) rest=rest*v[j];
      if (!integration_monomial(rest,x,c,m,contextptr)) continue;
      gen q=(m+1)/k;
      if (q.type!=_INT_ || q.val<1 || q.val>8) continue;
      if (polynomial_only && (k.type!=_INT_ || m.type!=_INT_ || m.val<0 || p[1].type!=_INT_)) continue;
      res=0;
      for (int j=0;j<q.val;++j){
        gen exponent=p[1]+j+1;
        gen coefficient=comb(q.val-1,j,contextptr)*pow(-a,q.val-1-j);
        res+=coefficient*symbolic(at_pow,makesequence(p[0],exponent))/exponent;
      }
      res=c*res/(k*pow(b,q.val));return true;
    }
    return false;
  }

  static bool integration_quadratic(const gen &e,const gen &x,gen &a,gen &b,GIAC_CONTEXT){
    sparse_poly1 p;
    if (!small_sparse_polynomial(e,x,p,contextptr)) return false;
    a=0;b=0;
    for (unsigned i=0;i<p.size();++i){
      if (p[i].exponent==2) a+=p[i].coeff;
      else if (p[i].exponent==0) b+=p[i].coeff;
      else if (!is_zero(p[i].coeff)) return false;
    }
    return !is_zero(a);
  }

  // Reciprocal quartic under a square root, reduced by u=x/(x^2+d).
  // Positive d has no real poles. Negative d uses a separate real atanh
  // primitive on each interval separated by the quadratic poles.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_reciprocal_quartic(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr)) return false;
    if (!e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=3) return false;
    gen A,B,radical,numerator=1;bool have_q=false,have_r=false;
    for (unsigned i=0;i<v.size();++i){
      gen base;
      if (integration_power(v[i],base,-1)){
        if (!have_q && integration_quadratic(base,x,A,B,contextptr)){have_q=true;continue;}
        if (base.is_symb_of_sommet(at_sqrt)){radical=base._SYMBptr->feuille;have_r=true;continue;}
        if (base.is_symb_of_sommet(at_pow) && base._SYMBptr->feuille.type==_VECT &&
            base._SYMBptr->feuille._VECTptr->size()==2 && base._SYMBptr->feuille[1]==gen(1)/2){
          radical=base._SYMBptr->feuille[0];have_r=true;continue;
        }
      }
      if (v[i].is_symb_of_sommet(at_pow) && v[i]._SYMBptr->feuille.type==_VECT &&
          v[i]._SYMBptr->feuille._VECTptr->size()==2 && v[i]._SYMBptr->feuille[1]==-gen(1)/2){
        radical=v[i]._SYMBptr->feuille[0];have_r=true;continue;
      }
      numerator=numerator*v[i];
    }
    gen N,M;
    if (!have_q || !have_r || !integration_quadratic(numerator,x,N,M,contextptr)) return false;
    gen d=B/A;
    if (is_zero(d) || M!=-N*d) return false;
    sparse_poly1 p;
    if (!small_sparse_polynomial(radical,x,p,contextptr)) return false;
    gen r4=0,r2=0,r0=0;
    for (unsigned i=0;i<p.size();++i){
      if (p[i].exponent==4) r4+=p[i].coeff;
      else if (p[i].exponent==2) r2+=p[i].coeff;
      else if (p[i].exponent==0) r0+=p[i].coeff;
      else if (!is_zero(p[i].coeff)) return false;
    }
    if (!is_strictly_positive(r4,contextptr) || r0!=r4*d*d) return false;
    if(is_strictly_positive(-d,contextptr)){
      gen K=r2/r4-2*d;
      if(!is_strictly_positive(K,contextptr))return false;
      // Q-K*x^2=(x^2+d)^2: the atanh argument stays in (-1,1)
      // on every real interval away from x^2=-d.
      gen scale=sqrt(K,contextptr);
      res=-N*atanh(scale*x/sqrt(radical/r4,contextptr),contextptr)/(A*sqrt(r4,contextptr)*scale);
      return true;
    }
    if (!is_strictly_positive(d,contextptr) || !is_strictly_positive(r2/r4+2*d,contextptr))return false;
    gen m=2*d-r2/r4,u=x/(pow(x,2)+d),primitive;
    if (is_zero(m)) primitive=-u;
    else if (is_strictly_positive(m,contextptr)){
      gen s=sqrt(m,contextptr);primitive=-asin(s*u,contextptr)/s;
    }
    else {gen s=sqrt(-m,contextptr);primitive=-asinh(s*u,contextptr)/s;}
    res=N*primitive/(A*sqrt(r4,contextptr));return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_quartic_trig(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    gen den;
    if (!angle_radian(contextptr) || !integration_power(e,den,-1) ||
        !den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*den._SYMBptr->feuille._VECTptr;
    gen s,t,a,b;
    if (v.size()!=2 || !integration_power(v[0],s,4) || !integration_power(v[1],t,4)) return false;
    if (s.is_symb_of_sommet(at_cos)) swapgen(s,t);
    if (!s.is_symb_of_sommet(at_sin) || !t.is_symb_of_sommet(at_cos) || s._SYMBptr->feuille!=t._SYMBptr->feuille) return false;
    gen u=s._SYMBptr->feuille;
    if (!is_linear_wrt(u,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a)) return false;
    gen root=sqrt(gen(2),contextptr);
    // The atan denominator is strictly positive: this representation has
    // no artificial tan poles or branch jumps on the real axis.
    res=(root*u-atan(sin(4*u,contextptr)/(3+2*root+cos(4*u,contextptr)),contextptr)/root)/a;
    return true;
  }

  // Product-rule/addition-formula primitives; cost is independent of n.
  // d[sin((n+1)u)*sin(u)^(n+1)]/du
  //   = (n+1)*sin((n+2)u)*sin(u)^n.
  // Replacing both factors of the integrand by cos gives the same sine
  // carrier in the primitive and cos(u)^(n+1). Integer powers are entire,
  // so these identities introduce no real or complex branch choices.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_high_frequency_trig(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || taille(e,65)>64 ||
        !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=2) return false;
    for (unsigned j=0;j<2;++j){
      const gen &carrier=v[j];gen base=v[1-j];int n=1;
      if (base.is_symb_of_sommet(at_pow)){
        const gen &f=base._SYMBptr->feuille;
        if (f.type!=_VECT || f._VECTptr->size()!=2) continue;
        const vecteur &p=*f._VECTptr;
        if (p[1].type!=_INT_ || p[1].val<1 || p[1].val>32765) continue;
        n=p[1].val;base=gen(p[0]);
      }
      bool sine=base.is_symb_of_sommet(at_sin);
      if ((!sine && !base.is_symb_of_sommet(at_cos)) ||
          !carrier.is_symb_of_sommet(sine?at_sin:at_cos)) continue;
      gen u=base._SYMBptr->feuille,a,b,A,B;
      if (!is_linear_wrt(u,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) ||
          !is_linear_wrt(carrier._SYMBptr->feuille,x,A,B,contextptr) ||
          !integration_rational(A) || is_inf(b) || is_inf(B) || is_undef(b) || is_undef(B)) continue;
      int sign=1;
      if (!is_zero(A-(n+2)*a)){
        if (!is_zero(A+(n+2)*a)) continue;
        sign=-1;
      }
      // Matching the slope alone would silently accept a wrong phase.
      if (!is_zero(ratnormal(B-sign*(n+2)*b,contextptr))) continue;
      gen factor=sine?sin(u,contextptr):cos(u,contextptr);
      res=sin((n+1)*u,contextptr)*pow(factor,n+1)/((n+1)*a);
      if (sine && sign<0) res=-res;
      return true;
    }
    return false;
  }

  static bool integration_resource_rational(const gen &g){
    if(g.type==_INT_)return true;
    if(g.type==_ZINT)return mpz_sizeinbase(*g._ZINTptr,2)<=256;
    if(g.type!=_FRAC)return false;
    const gen &n=g._FRACptr->num,&d=g._FRACptr->den;
    return (n.type==_INT_ || (n.type==_ZINT && mpz_sizeinbase(*n._ZINTptr,2)<=256)) &&
      (d.type==_INT_ || (d.type==_ZINT && mpz_sizeinbase(*d._ZINTptr,2)<=256)) && !is_zero(d);
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_composed_binomial(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    if(taille(e,129)>128 || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()>4)return false;
    for(unsigned i=0;i<v.size();++i){
      vecteur p;
      if(v[i].is_symb_of_sommet(at_plus))p=makevecteur(v[i],1);
      else if(v[i].is_symb_of_sommet(at_pow) && v[i]._SYMBptr->feuille.type==_VECT)p=*v[i]._SYMBptr->feuille._VECTptr;
      else continue;
      if(p.size()!=2 || p[1].type!=_INT_ || p[1].val<1 || p[1].val>8192 ||
         !p[0].is_symb_of_sommet(at_plus) || p[0]._SYMBptr->feuille.type!=_VECT)continue;
      const vecteur &sum=*p[0]._SYMBptr->feuille._VECTptr;if(sum.size()!=2)continue;
      unsigned ci=integration_resource_rational(sum[0])?0:1;
      gen a=sum[ci],term=sum[1-ci];if(!integration_resource_rational(a))continue;
      gen b=integration_syntax(integration_coefficient(term,x,contextptr),contextptr);
      if(!integration_resource_rational(b) || is_zero(b) || !term.is_symb_of_sommet(at_pow) || term._SYMBptr->feuille.type!=_VECT)continue;
      const vecteur &kp=*term._SYMBptr->feuille._VECTptr;
      if(kp.size()!=2 || kp[1].type!=_INT_ || kp[1].val<1 || kp[1].val>64 || kp[0]==x || taille(kp[0],33)>32)continue;
      gen u=kp[0];int k=kp[1].val,m=0;sparse_poly1 up;
      if(!small_sparse_polynomial(u,x,up,contextptr) || up.empty() || up.size()>4)continue;
      bool ok=true;unsigned derivative_terms=0;
      for(unsigned j=0;j<up.size();++j){if(up[j].exponent.val>8 || !integration_resource_rational(up[j].coeff))ok=false;if(up[j].exponent.val && !is_zero(up[j].coeff))++derivative_terms;}
      if(!ok || !derivative_terms)continue;
      gen rest=1;
      for(unsigned j=0;j<v.size();++j)if(i!=j){
        if(v[j]==u){++m;continue;}
        if(v[j].is_symb_of_sommet(at_pow) && v[j]._SYMBptr->feuille.type==_VECT){
          const vecteur &mp=*v[j]._SYMBptr->feuille._VECTptr;
          if(mp.size()==2 && mp[0]==u){
            if(mp[1].type!=_INT_ || mp[1].val<0 || mp[1].val>32767){ok=false;break;}
            m+=mp[1].val;continue;
          }
        }
        rest=rest*v[j];
      }
      if(!ok || m>32767 || (m+1)%k || (m+1)/k>8)continue;
      sparse_poly1 rp;if(!small_sparse_polynomial(rest,x,rp,contextptr))continue;
      gen scale=undef;unsigned matches=0;
      for(unsigned j=0;j<rp.size();++j){
        if(is_zero(rp[j].coeff))continue;
        if(!integration_resource_rational(rp[j].coeff)){ok=false;break;}
        unsigned l=0;for(;l<up.size();++l)if(up[l].exponent.val==rp[j].exponent.val+1 && !is_zero(up[l].coeff))break;
        if(l==up.size()){ok=false;break;}
        gen ratio=rp[j].coeff/(up[l].exponent*up[l].coeff);
        if(is_undef(scale))scale=ratio;else if(scale!=ratio){ok=false;break;}
        ++matches;
      }
      if(!ok || matches!=derivative_terms || is_undef(scale) || !integration_resource_rational(scale))continue;
      int q=(m+1)/k;gen answer=0;
      for(int j=0;j<q;++j){
        gen exponent=p[1]+j+1;
        answer+=comb(q-1,j,contextptr)*pow(-a,q-1-j)*symbolic(at_pow,makesequence(p[0],exponent))/exponent;
      }
      answer=scale*answer/(gen(k)*pow(b,q));
      if(taille(answer,513)>512)continue;
      res=answer;return true;
    }
    return false;
  }

  // Read a bounded pullback of log(1-u)*du/u, optionally divided by
  // (1-u). Monomial, affine and exponential substitutions stay factored.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integration_dilog_form(const gen &e,const gen &x,gen &u,gen &scale,bool &extra,GIAC_CONTEXT){
    if(taille(e,97)>96)return false;
    gen f=e,coefficient=integration_coefficient(f,x,contextptr),weight=1,argument;
    if(!integration_resource_rational(coefficient))return false;
    vecteur v=f.is_symb_of_sommet(at_prod) && f._SYMBptr->feuille.type==_VECT?*f._SYMBptr->feuille._VECTptr:vecteur(1,f);
    if(v.size()>4)return false;
    bool found=false;extra=false;
    for(unsigned i=0;i<v.size();++i){
      if(v[i].is_symb_of_sommet(at_ln)){if(found)return false;found=true;argument=v[i]._SYMBptr->feuille;}
      else weight=weight*v[i];
    }
    if(!found)return false;
    gen other,a,b;
    if(integration_one_plus(argument,other))u=-other;
    else {
      if(!is_linear_wrt(argument,x,a,b,contextptr) || !integration_resource_rational(a) || !integration_resource_rational(b) || is_zero(a))return false;
      u=1-argument;
    }
    // The extra logarithmic pole has a separate log-square primitive.
    vecteur weights=weight.is_symb_of_sommet(at_prod) && weight._SYMBptr->feuille.type==_VECT?*weight._SYMBptr->feuille._VECTptr:vecteur(1,weight);
    weight=1;
    for(unsigned i=0;i<weights.size();++i){
      gen base;
      if(!extra && integration_power(weights[i],base,-1) && base==argument)extra=true;
      else weight=weight*weights[i];
    }
    gen c,q,w,n;
    if(integration_monomial(u,x,c,q,contextptr) && integration_resource_rational(c) && !is_zero(c) &&
       integration_resource_rational(q) && !is_zero(q) && !is_strictly_greater(abs(q,contextptr),32,contextptr)){
      if(q.type==_FRAC && (q._FRACptr->den.type!=_INT_ || q._FRACptr->den.val>16))return false;
      if(!integration_monomial(weight,x,w,n,contextptr) || n!=-1 || !integration_resource_rational(w))return false;
      scale=coefficient*w/q;return true;
    }
    gen base=u;gen uc=integration_coefficient(base,x,contextptr);
    if(base.is_symb_of_sommet(at_exp) && integration_resource_rational(uc) && !is_zero(uc) &&
       is_linear_wrt(base._SYMBptr->feuille,x,a,b,contextptr) &&
       (integration_resource_rational(a) || (taille(a,17)<=16 && integration_resource_rational(re(a,contextptr)) && integration_resource_rational(im(a,contextptr)))) &&
       !is_zero(a) && integration_resource_rational(b) &&
       integration_monomial(weight,x,w,n,contextptr) && is_zero(n) && integration_resource_rational(w)){
      scale=coefficient*w/a;return true;
    }
    if(!is_linear_wrt(u,x,a,b,contextptr) || !integration_resource_rational(a) || is_zero(a) || !integration_resource_rational(b))return false;
    w=integration_coefficient(weight,x,contextptr);
    if(!integration_resource_rational(w) || !integration_power(weight,base,-1))return false;
    gen A,B;
    if(!is_linear_wrt(base,x,A,B,contextptr) || !integration_resource_rational(A) || is_zero(A) || !integration_resource_rational(B) || B*a!=b*A)return false;
    scale=coefficient*w/A;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_dilog_primitive(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    gen u,scale;bool extra;
    if(!integration_dilog_form(e,x,u,scale,extra,contextptr))return false;
    res=-scale*gen(symbolic(at_Li2,u));
    if(extra){gen L=ln(1-u,contextptr);res-=scale*L*L/2;}
    return true;
  }

  // On each real interval avoiding sine zeros, the imaginary part of
  // Li2(exp(2*i*u)) differentiates to -2*log(2*abs(sin(u))).
  // The periodic ramp adds the principal-log imaginary part on negative
  // sine half-periods, so plain log(sin) is not changed into log(abs(sin)).
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_trig_primitive(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || taille(e,65)>64)return false;
    if(e.is_symb_of_sommet(at_plus) && e._SYMBptr->feuille.type==_VECT){
      const vecteur &terms=*e._SYMBptr->feuille._VECTptr;
      if(terms.size()>4)return false;
      gen total=0;
      for(unsigned j=0;j<terms.size();++j){
        gen t=terms[j],c=integration_coefficient(t,x,contextptr),primitive;
        if(!integration_resource_rational(c) || !integrate_log_trig_primitive(t,x,primitive,contextptr))return false;
        total+=c*primitive;
      }
      res=total;return true;
    }
    gen logarithm=e,weight=1;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*e._SYMBptr->feuille._VECTptr;
      bool found=false;
      for(unsigned j=0;j<v.size();++j){
        if(v[j].is_symb_of_sommet(at_ln)){if(found)return false;logarithm=v[j];found=true;}
        else weight=weight*v[j];
      }
      if(!found)return false;
    }
    if(!logarithm.is_symb_of_sommet(at_ln))return false;
    gen f=logarithm._SYMBptr->feuille,c=integration_coefficient(f,x,contextptr),u,a,b;
    bool absolute=f.is_symb_of_sommet(at_abs);
    if(absolute){
      if(!is_strictly_positive(c,contextptr))return false;
      f=gen(f._SYMBptr->feuille);c=c*integration_coefficient(f,x,contextptr);
      c=abs(c,contextptr);
    }
    if(!integration_resource_rational(c) || is_zero(c) ||
       (!f.is_symb_of_sommet(at_sin) && !f.is_symb_of_sommet(at_cos)))return false;
    u=f._SYMBptr->feuille;
    bool linear=is_linear_wrt(u,x,a,b,contextptr) && integration_resource_rational(a) && !is_zero(a);
    gen scale;
    if(!is_one(weight)){
      // Match coefficients of weight = scale*u', without dividing by a
      // variable derivative and losing regular stationary points.
      sparse_poly1 up,wp;
      gen phase=integration_syntax(u,contextptr),phase_scale=integration_syntax(integration_coefficient(phase,x,contextptr),contextptr);
      if(!integration_resource_rational(phase_scale) || is_zero(phase_scale) ||
         !small_sparse_polynomial(phase,x,up,contextptr) || up.empty() || up.size()>4 ||
         !small_sparse_polynomial(weight,x,wp,contextptr) || wp.size()>4)return false;
      scale=undef;unsigned count=0,matches=0;
      for(unsigned j=0;j<up.size();++j){
        if(up[j].exponent.val>8 || !integration_resource_rational(up[j].coeff))return false;
        if(up[j].exponent.val && !is_zero(up[j].coeff))++count;
      }
      for(unsigned j=0;j<wp.size();++j){
        if(is_zero(wp[j].coeff))continue;
        if(!integration_resource_rational(wp[j].coeff))return false;
        unsigned k=0;for(;k<up.size();++k)if(up[k].exponent.val==wp[j].exponent.val+1 && !is_zero(up[k].coeff))break;
        if(k==up.size())return false;
        gen ratio=wp[j].coeff/(phase_scale*up[k].exponent*up[k].coeff);
        if(is_undef(scale))scale=ratio;else if(scale!=ratio)return false;
        ++matches;
      }
      if(!count || count!=matches || !integration_resource_rational(scale))return false;
    }
    else {if(!linear)return false;scale=gen(1)/a;}
    // Phase constants can be rational plus a rational multiple of pi.
    gen pa,pb;
    if(is_one(weight) && (!is_linear_wrt(b,cst_pi,pa,pb,contextptr) || !integration_resource_rational(pa) || !integration_resource_rational(pb)))return false;
    if(f.is_symb_of_sommet(at_cos))u+=cst_pi/2;
    if(is_strictly_positive(-c,contextptr)){c=-c;u+=cst_pi;}
    gen dilog=symbolic(at_Li2,symbolic(at_exp,2*cst_i*u));
    res=(is_one(weight)?x:scale*u)*ln(c/2,contextptr)-scale*gen(symbolic(at_im,dilog))/2;
    if(!absolute){
      gen n=symbolic(at_floor,u/(2*cst_pi)),r=u-2*cst_pi*n-cst_pi;
      gen ramp=(u-cst_pi+gen(symbolic(at_abs,r)))/2;
      res+=scale*cst_i*cst_pi*ramp;
    }
    return true;
  }

  // Integral of 1/(1+k*u^2), on each real interval excluding its poles.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integrate_real_quadratic(const gen &k,const gen &u,GIAC_CONTEXT){
    if(is_zero(k))return u;
    bool positive=is_strictly_positive(k,contextptr);
    gen r=sqrt(positive?k:-k,contextptr);
    if(positive)return atan(r*u,contextptr)/r;
    return symbolic(at_ln,symbolic(at_abs,(1+r*u)/(1-r*u)))/(2*r);
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_compact_primitive(const gen &input,const gen &x,gen &res,GIAC_CONTEXT){
    if (taille(input,129)>128 || complex_mode(contextptr) || complex_variables(contextptr)) return false;
    gen e=integration_syntax(input,contextptr),c=integration_coefficient(e,x,contextptr);
    gen p;
    if (!is_undef(c) && !is_inf(c) && integrate_high_frequency_trig(e,x,p,contextptr)){res=c*p;return true;}
    // Real logarithms of C*sqrt(Q)+L, with Q-(L/C)^2 a positive
    // constant, give a globally regular inverse-hyperbolic substitution.
    // Match its derivative rather than entering a general algebraic search.
    if(angle_radian(contextptr) && has_op(e,*at_ln) && taille(e,97)<=96 && taille(c,17)<=16 && !contains(c,x) &&
       integration_resource_rational(normal(c*c,contextptr)) && is_zero(im(c,contextptr))){
      vecteur logs=lop(e,at_ln);
      if(logs.size()==1 && e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT){
        gen t=logs[0],arg=t._SYMBptr->feuille;
        vecteur roots=mergevecteur(lop(arg,at_sqrt),lop(arg,at_pow));
        for(unsigned j=0;j<roots.size();++j){
          gen Q;
          if(roots[j].is_symb_of_sommet(at_sqrt))Q=roots[j]._SYMBptr->feuille;
          else {const gen &f=roots[j]._SYMBptr->feuille;if(f.type!=_VECT || f._VECTptr->size()!=2 || f[1]!=gen(1)/2)continue;Q=f[0];}
          sparse_poly1 polynomial;
          if(!small_sparse_polynomial(Q,x,polynomial,contextptr) || polynomial.empty() || polynomial.front().exponent.val>4)continue;
          bool bounded=true;
          for(unsigned h=0;h<polynomial.size();++h)if(!integration_resource_rational(polynomial[h].coeff))bounded=false;
          vecteur variables=lvar(arg);
          for(unsigned h=0;h<variables.size();++h)if(variables[h]!=x && variables[h]!=roots[j])bounded=false;
          if(bounded){
            // Direct logarithmic chain: compare a bounded exact derivative.
            // The returned primitive never divides by t', so stationary points
            // remain regular. No logarithm product/branch identity is applied.
            const vecteur &chain=*e._SYMBptr->feuille._VECTptr;
            for(unsigned k=0;k<chain.size();++k)if(chain[k]==t){
              gen weight=1;for(unsigned h=0;h<chain.size();++h)if(h!=k)weight=weight*chain[h];
              gen dt=derive(t,x,contextptr);
              if(!is_zero(dt) && taille(dt,129)<=128){
                gen scale=normal(c*weight/dt,contextptr);
                if(!contains(scale,x) && integration_resource_rational(scale)){
                  res=scale*pow(t,2)/2;return true;
                }
              }
            }
          }
          if(polynomial.front().exponent.val>2)continue;
          gen C,L,a,b;
          if(!is_linear_wrt(arg,roots[j],C,L,contextptr) || taille(C,17)>16 || contains(C,x) || !integration_resource_rational(normal(C*C,contextptr)) || !is_strictly_positive(C,contextptr) ||
             !is_linear_wrt(L,x,a,b,contextptr) || !integration_resource_rational(a) || !integration_resource_rational(b))continue;
          gen gap=normal(Q-pow(L/C,2),contextptr);
          if(!integration_resource_rational(gap) || !is_strictly_positive(gap,contextptr))continue;
          const vecteur &factors=*e._SYMBptr->feuille._VECTptr;
          for(unsigned k=0;k<factors.size();++k){
            gen den,A,B;
            if(!integration_power(factors[k],den,-1) || !is_linear_wrt(den,pow(t,2),B,A,contextptr) ||
               !integration_resource_rational(A) || !integration_resource_rational(B) || !is_strictly_positive(A,contextptr) || !is_strictly_positive(B,contextptr))continue;
            gen weight=1;for(unsigned h=0;h<factors.size();++h)if(h!=k)weight=weight*factors[h];
            gen dt=derive(t,x,contextptr);if(is_zero(dt) || taille(dt,97)>96)continue;
            gen scale=normal(c*weight/dt,contextptr);
            if(taille(scale,17)>16 || contains(scale,x) || !integration_resource_rational(normal(scale*scale,contextptr)) || !is_zero(im(scale,contextptr)))continue;
            res=scale*atan(t*sqrt(B/A,contextptr),contextptr)/sqrt(A*B,contextptr);return true;
          }
        }
      }
    }
    if (!integration_rational(c)) return false;
    // A power of a Mobius map divided by the product of its two
    // affine factors: R'/R=det/(A*B). Read only small sparse polynomials
    // and compare their numeric coefficients, avoiding algebraic search.
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT && e._SYMBptr->feuille._VECTptr->size()==2){
      const vecteur &v=*e._SYMBptr->feuille._VECTptr;
      for(unsigned j=0;j<2;++j){
        gen R,power,root=v[j];bool reciprocal=root.is_symb_of_sommet(at_inv);
        if(reciprocal)root=gen(root._SYMBptr->feuille);
        if(root.is_symb_of_sommet(at_sqrt)){R=root._SYMBptr->feuille;power=gen(1)/2;}
        else if(root.is_symb_of_sommet(at_pow) && root._SYMBptr->feuille.type==_VECT && root._SYMBptr->feuille._VECTptr->size()==2){R=root._SYMBptr->feuille[0];power=root._SYMBptr->feuille[1];}
        else continue;
        if(reciprocal)power=-power;
        if(power==-gen(1)/2){R=inv(R,contextptr);power=gen(1)/2;}
        // Positive square root of an affine ratio: use its own real
        // parameter, avoiding a tangent-half-angle pole inside the interval.
        if(power==gen(1)/2 && angle_radian(contextptr) && taille(R,17)<=16 && lop(R,at_pow).empty()){
          gen den,num,div,a,b,d,f,h,k;
          gen nd=fxnd(R);
          if(nd.type!=_VECT || nd._VECTptr->size()!=2)continue;
          num=nd[0];div=nd[1];
          if(integration_power(v[1-j],den,-1) &&
             is_linear_wrt(num,x,a,b,contextptr) && is_linear_wrt(div,x,d,f,contextptr) &&
             is_linear_wrt(den,x,h,k,contextptr) && integration_resource_rational(a) &&
             integration_resource_rational(b) && integration_resource_rational(d) &&
             integration_resource_rational(f) && integration_resource_rational(h) && integration_resource_rational(k)){
            gen delta=a*f-b*d,E=k*a-h*b,F=h*f-k*d,J=a*F+d*E;
            if(!is_zero(a) && !is_zero(E) && !is_zero(J) && !is_zero(delta)){
              gen u=v[j];
              res=2*c*delta/J*(integrate_real_quadratic(-d/a,u,contextptr)-integrate_real_quadratic(F/E,u,contextptr));return true;
            }
          }
        }
        gen den;
        if(!integration_resource_rational(power) || is_zero(power) || is_strictly_greater(abs(power,contextptr),8,contextptr) ||
           !integration_power(v[1-j],den,-1))continue;
        gen ratio=integration_syntax(R,contextptr),factor=integration_syntax(integration_coefficient(ratio,x,contextptr),contextptr);
        if(!integration_resource_rational(factor) || is_zero(factor) || !ratio.is_symb_of_sommet(at_prod) ||
           ratio._SYMBptr->feuille.type!=_VECT || ratio._SYMBptr->feuille._VECTptr->size()!=2)continue;
        const vecteur &rv=*ratio._SYMBptr->feuille._VECTptr;
        unsigned k=rv[0].is_symb_of_sommet(at_inv)?0:1;gen B;
        if(!integration_power(rv[k],B,-1))continue;
        gen A=rv[1-k];sparse_poly1 polys[3];
        if(!small_sparse_polynomial(A,x,polys[0],contextptr) || !small_sparse_polynomial(B,x,polys[1],contextptr) ||
           !small_sparse_polynomial(den,x,polys[2],contextptr))continue;
        gen coeff[3][3];bool valid=true;
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<polys[row].size();++col){
          const monome &term=polys[row][col];int degree=term.exponent.val;
          if(degree>(row==2?2:1) || !integration_resource_rational(term.coeff)){valid=false;break;}
          coeff[row][degree]=coeff[row][degree]+term.coeff;
        }
        if(!valid)continue;
        gen delta=coeff[0][1]*coeff[1][0]-coeff[0][0]*coeff[1][1];
        if(is_zero(delta))continue;
        gen product[3]={coeff[0][0]*coeff[1][0],coeff[0][1]*coeff[1][0]+coeff[0][0]*coeff[1][1],coeff[0][1]*coeff[1][1]};
        gen scale=undef;
        for(unsigned col=0;col<3;++col){
          if(is_zero(product[col])){if(!is_zero(coeff[2][col]))valid=false;continue;}
          gen next=coeff[2][col]/product[col];
          if(is_undef(scale))scale=next;else if(scale!=next)valid=false;
        }
        if(!valid || !integration_resource_rational(scale) || is_zero(scale))continue;
        res=c*v[j]/(power*delta*scale);return true;
      }
    }
    if (!integrate_log_trig_primitive(e,x,p,contextptr) &&
        !integrate_dilog_primitive(e,x,p,contextptr) &&
        !integrate_composed_binomial(e,x,p,contextptr) &&
        !integrate_binomial_chain(e,x,p,false,contextptr) &&
        !integrate_reciprocal_quartic(e,x,p,contextptr) && !integrate_quartic_trig(e,x,p,contextptr)) return false;
    res=c*p;return true;
  }

  // Odd sine powers have zero mean and a bounded primitive. Their /x
  // integral is conditionally convergent; a phase shift is not allowed.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_sine_dirichlet(const vecteur &v,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || v.size()!=2) return false;
    unsigned i=v[0].is_symb_of_sommet(at_inv)?0:1;gen base;
    if (!integration_power(v[i],base,-1) || base!=x) return false;
    gen f=v[1-i];int n=1;
    if (f.is_symb_of_sommet(at_pow) && f._SYMBptr->feuille.type==_VECT){
      const vecteur &p=*f._SYMBptr->feuille._VECTptr;
      if (p.size()!=2 || p[1].type!=_INT_ || p[1].val<1 || p[1].val>31 || p[1].val%2==0) return false;
      n=p[1].val;f=p[0];
    }
    gen a,b;
    if (!f.is_symb_of_sommet(at_sin) || !is_linear_wrt(f._SYMBptr->feuille,x,a,b,contextptr) ||
        !integration_rational(a) || is_zero(a) || !is_zero(b)) return false;
    int halves=(lo==minus_inf && hi==plus_inf)?2:
      ((is_zero(lo) && hi==plus_inf) || (lo==minus_inf && is_zero(hi))?1:0);
    if (!halves) return false;
    int sign=is_strictly_positive(a,contextptr)?1:-1;
    res=gen(sign*halves)*cst_pi*comb(n-1,(n-1)/2,contextptr)/pow(gen(2),n);return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_logistic_moment(const vecteur &v,const gen &x,gen &res,GIAC_CONTEXT){
    if (v.size()!=2) return false;
    for (unsigned i=0;i<2;++i){
      gen den;
      if (!integration_power(v[i],den,-1) || !den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT) continue;
      const vecteur &d=*den._SYMBptr->feuille._VECTptr;
      if (d.size()!=3) continue;
      gen sum=0,offset_sum=0,freq=0,offset=0;unsigned count=0;bool valid=true;
      for (unsigned j=0;j<3;++j){
        if (d[j].is_symb_of_sommet(at_exp)){
          gen a,b;
          if (!is_linear_wrt(d[j]._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) || !integration_rational(b)){valid=false;break;}
          sum+=a;offset_sum+=b;freq=a;offset=b;++count;
        }
        else if (d[j]!=2){valid=false;break;}
      }
      gen c,n;
      if (!valid || count!=2 || !is_zero(sum) || !is_zero(offset_sum) || !integration_monomial(v[1-i],x,c,n,contextptr) ||
          n.type!=_INT_ || n.val<0 || n.val>16) continue;
      gen a=abs(freq,contextptr),mu=-offset/freq;
      res=0;
      // Shifted moments from the centered even moments; at most nine terms.
      for (int j=0;j<=n.val;j+=2){
        if (is_zero(mu) && j!=n.val) continue;
        gen moment=j?(pow(gen(2),j)-2)*abs(_bernoulli(j,contextptr),contextptr)*pow(cst_pi,j):gen(1);
        res+=comb(n.val,j,contextptr)*(j==n.val?gen(1):pow(mu,n.val-j))*moment/pow(a,j+1);
      }
      res=c*res;return true;
    }
    return false;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_weighted_reflection(const vecteur &v,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || (v.size()!=2 && v.size()!=3) || is_inf(lo) || is_inf(hi)) return false;
    if(v.size()==2){
      gen den,weight=v[0];
      if(!integration_power(v[1],den,-1)){
        weight=v[1];if(!integration_power(v[0],den,-1))return false;
      }
      if(!den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT || den._SYMBptr->feuille._VECTptr->size()!=2)return false;
      gen sine=den._SYMBptr->feuille[0],cosine=den._SYMBptr->feuille[1];
      gen sc=integration_coefficient(sine,x,contextptr),cc=integration_coefficient(cosine,x,contextptr);
      if(sine.is_symb_of_sommet(at_cos)){swapgen(sine,cosine);swapgen(sc,cc);}
      if(!sine.is_symb_of_sommet(at_sin) || !cosine.is_symb_of_sommet(at_cos) || sc!=cc ||
         !integration_rational(sc) || is_zero(sc) || sine._SYMBptr->feuille!=cosine._SYMBptr->feuille)return false;
      gen a,b,p,q;
      if(!is_linear_wrt(sine._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) ||
         !is_linear_wrt(weight,x,p,q,contextptr) || !integration_rational(p) || !integration_rational(q))return false;
      gen l=ratnormal((a*lo+b)/cst_pi,contextptr),r=ratnormal((a*hi+b)/cst_pi,contextptr);
      if(!((is_zero(l) && r==gen(1)/2) || (is_zero(r) && l==gen(1)/2)))return false;
      gen root=sqrt(gen(2),contextptr);
      res=(p*(lo+hi)/2+q)*root*ln(1+root,contextptr)/(sc*abs(a,contextptr));return true;
    }
    gen weight=1,arg,den;bool have_sin=false,have_den=false;
    for (unsigned i=0;i<v.size();++i){
      if (v[i].is_symb_of_sommet(at_sin)){arg=v[i]._SYMBptr->feuille;have_sin=true;}
      else if (integration_power(v[i],den,-1)) have_den=true;
      else weight=weight*v[i];
    }
    gen a,b,p,q;
    if (!have_sin || !have_den || !is_linear_wrt(arg,x,a,b,contextptr) ||
        !integration_rational(a) || !integration_rational(b) || is_zero(a) ||
        !is_linear_wrt(weight,x,p,q,contextptr) || !integration_rational(p) || !integration_rational(q)) return false;
    gen l=ratnormal(a*lo+b,contextptr),r=ratnormal(a*hi+b,contextptr);
    if (!((is_zero(l) && r==cst_pi) || (is_zero(r) && l==cst_pi))) return false;
    if (!den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &d=*den._SYMBptr->feuille._VECTptr;
    if (d.size()!=2) return false;
    gen A=0,B=0;
    for (unsigned i=0;i<2;++i){
      if (integration_rational(d[i])){A+=d[i];continue;}
      gen t=d[i],c=integration_coefficient(t,x,contextptr),u;
      if (!integration_rational(c) || !integration_power(t,u,2) || !u.is_symb_of_sommet(at_cos) || u._SYMBptr->feuille!=arg) return false;
      B+=c;
    }
    if (!is_strictly_positive(A,contextptr) || !is_strictly_positive(A+B,contextptr) || is_zero(B)) return false;
    gen factor=is_strictly_positive(B,contextptr)?
      atan(sqrt(B/A,contextptr),contextptr)/sqrt(A*B,contextptr):
      atanh(sqrt(-B/A,contextptr),contextptr)/sqrt(-A*B,contextptr);
    res=(p*(lo+hi)/2+q)*gen(2)*factor/abs(a,contextptr);return true;
  }

  // Fourier/Laplace integrals with explicit real convergence conditions.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_decay_transform(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()<2 || v.size()>3) return false;
    gen exponent,arg,rest=1;bool exponential=false,trig=false,cosine=false;
    for (unsigned i=0;i<v.size();++i){
      if (v[i].is_symb_of_sommet(at_exp) && !exponential){exponent=v[i]._SYMBptr->feuille;exponential=true;}
      else if (!trig && (v[i].is_symb_of_sommet(at_sin) || v[i].is_symb_of_sommet(at_cos))){
        arg=v[i]._SYMBptr->feuille;cosine=v[i].is_symb_of_sommet(at_cos);trig=true;
      }
      else rest=rest*v[i];
    }
    gen b,c,a,d;
    if (!exponential || !trig || !is_linear_wrt(arg,x,b,c,contextptr) || !is_zero(im(b,contextptr)) || contains(b,x) || taille(b,33)>32 || !integration_rational(c)) return false;
    gen denominator;
    if (is_zero(lo) && hi==plus_inf && !cosine && is_zero(c) &&
        integration_power(rest,denominator,-1) && denominator==x &&
        is_linear_wrt(exponent,x,a,d,contextptr) && is_zero(im(a,contextptr)) && !contains(a,x) && taille(a,33)<=32 && integration_rational(d) && is_strictly_positive(-a,contextptr)){
      res=exp(d,contextptr)*atan(b/(-a),contextptr);return true;
    }
    if (lo!=minus_inf || hi!=plus_inf || !is_one(rest)) return false;
    sparse_poly1 p;
    if (!small_sparse_polynomial(exponent,x,p,contextptr)) return false;
    gen q=0,l=0,k=0;
    for (unsigned i=0;i<p.size();++i){
      if (p[i].exponent==2) q+=p[i].coeff;
      else if (p[i].exponent==1) l+=p[i].coeff;
      else if (p[i].exponent==0) k+=p[i].coeff;
      else if (!is_zero(p[i].coeff)) return false;
    }
    if (!is_strictly_positive(-q,contextptr)) return false;
    gen phase=c-b*l/(2*q);
    res=sqrt(cst_pi/(-q),contextptr)*exp(k+(b*b-l*l)/(4*q),contextptr)*
      (cosine?cos(phase,contextptr):sin(phase,contextptr));return true;
  }

  static bool integration_square_root(const gen &g,gen &radical){
    if (g.is_symb_of_sommet(at_sqrt)){radical=g._SYMBptr->feuille;return true;}
    if (g.is_symb_of_sommet(at_pow) && g._SYMBptr->feuille.type==_VECT &&
        g._SYMBptr->feuille._VECTptr->size()==2 && g._SYMBptr->feuille[1]==gen(1)/2){
      radical=g._SYMBptr->feuille[0];return true;
    }
    return false;
  }

  // The reciprocal atan becomes a double integral over [0,L]^2. Swapping
  // its variables cancels the shared quadratic denominator, giving a product
  // of two elementary atan integrals. This is a family, not a stored answer.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_square(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !is_zero(lo) || !integration_rational(hi) ||
        !is_strictly_positive(hi,contextptr) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=3) return false;
    gen a=0,b=0,ra=0,rb=0,ta=0,tb=0,k=0;bool root=false,den=false,trig=false,reciprocal=false;
    for (unsigned i=0;i<v.size();++i){
      gen base,radical;
      if (v[i].is_symb_of_sommet(at_atan)){
        if (trig) return false;
        base=v[i]._SYMBptr->feuille;k=ratnormal(integration_coefficient(base,x,contextptr),contextptr);
        gen inverse;
        if (integration_power(base,inverse,-1)){base=inverse;reciprocal=true;}
        if (!integration_rational(k) || !integration_square_root(base,radical) ||
            !integration_quadratic(radical,x,ta,tb,contextptr)) return false;
        trig=true;continue;
      }
      if (!integration_power(v[i],base,-1)) return false;
      if (integration_square_root(base,radical)){
        if (root || !integration_quadratic(radical,x,ra,rb,contextptr)) return false;
        root=true;
      }
      else {
        if (den || !integration_quadratic(base,x,a,b,contextptr)) return false;
        den=true;
      }
    }
    if (!trig || !root || !den || ta!=ra || tb!=rb || !is_strictly_positive(ra,contextptr) ||
        !is_strictly_positive(k,contextptr) || is_zero(a) || !is_strictly_positive(b/a,contextptr)) return false;
    gen B=b/a;
    if (rb!=2*ra*B || (reciprocal?k*k!=ra*hi*hi:k*k*ra*hi*hi!=1)) return false;
    gen t=atan(hi/sqrt(B,contextptr),contextptr);
    gen square=t*t;
    res=(reciprocal?square:cst_pi*atan(hi/sqrt(hi*hi+2*B,contextptr),contextptr)-square)/(2*B*a*sqrt(ra,contextptr));return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_trig(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || !is_zero(im(lo,contextptr)) ||
        !is_zero(im(hi,contextptr))) return false;
    gen f=e,mean=1;bool weighted=false;
    if (f.is_symb_of_sommet(at_prod) && f._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*f._SYMBptr->feuille._VECTptr;
      if (v.size()!=2) return false;
      unsigned j=v[0].is_symb_of_sommet(at_ln)?0:1;
      gen a,b;
      if (!is_linear_wrt(v[1-j],x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b)) return false;
      mean=a*(lo+hi)/2+b;f=gen(v[j]);weighted=true;
    }
    if (!f.is_symb_of_sommet(at_ln)) return false;
    gen t=f._SYMBptr->feuille;
    bool absolute=t.is_symb_of_sommet(at_abs);
    if (absolute) t=t._SYMBptr->feuille;
    bool cosine=t.is_symb_of_sommet(at_cos);
    if (!cosine && !t.is_symb_of_sommet(at_sin)) return false;
    gen a,b;
    if (!is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a)) return false;
    gen l=ratnormal((a*lo+b)/cst_pi,contextptr),r=ratnormal((a*hi+b)/cst_pi,contextptr);
    if (is_strictly_greater(l,r,contextptr)) swapgen(l,r);
    gen periods=ratnormal(r-l,contextptr);
    if (absolute && (periods.type==_INT_ || periods.type==_ZINT)){
      gen center=ratnormal(l+r,contextptr);
      if (weighted && center.type!=_INT_ && center.type!=_ZINT) return false;
      res=-mean*(hi-lo)*ln(gen(2),contextptr);return true;
    }
    if (cosine){l+=gen(1)/2;r+=gen(1)/2;}
    // The positive half-wave or either quarter: endpoints may be logarithmic
    // singularities, but the integral converges. Never integrate log(negative).
    if ((l==0 || l==gen(1)/2) && (r==gen(1)/2 || r==1) && l!=r){
      if (weighted && (l!=0 || r!=1)) return false;
      res=-mean*(hi-lo)*ln(gen(2),contextptr);return true;
    }
    return false;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_thermal_moment(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!is_zero(lo) || hi!=plus_inf) return false;
    vecteur v;
    if (e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT) v=*e._SYMBptr->feuille._VECTptr;
    else v=makevecteur(1,e);
    if (v.size()!=2) return false;
    for (unsigned i=0;i<2;++i){
      gen den;
      if (!integration_power(v[i],den,-1) || !den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT) continue;
      const vecteur &d=*den._SYMBptr->feuille._VECTptr;
      if (d.size()!=2) continue;
      unsigned j=d[0].is_symb_of_sommet(at_exp)?0:1;
      if (!d[j].is_symb_of_sommet(at_exp) || (d[1-j]!=1 && d[1-j]!=-1)) continue;
      gen a,b,c,n;
      if (!is_linear_wrt(d[j]._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !is_zero(b) ||
          !is_strictly_positive(a,contextptr) || !integration_monomial(v[1-i],x,c,n,contextptr) || n.type!=_INT_ || n.val<0 || n.val>64) continue;
      bool fermi=d[1-j]==1;
      if (n.val==0){res=c*(fermi?ln(gen(2),contextptr)/a:plus_inf);return true;}
      res=c*factorial(n.val)*_Zeta(n+1,contextptr)/pow(a,n.val+1);
      if (fermi) res=res*(1-inv(pow(gen(2),n.val),contextptr));
      return true;
    }
    return false;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_beta_psi(gen z,unsigned order,GIAC_CONTEXT){
    gen correction=0,scale=(order%2?-1:1)*factorial(order);
    for(unsigned step=0;step<32 && is_strictly_greater(z,1,contextptr);++step){
      z-=1;correction+=scale/pow(z,int(order)+1);
    }
    // Rational digamma values with other denominators expand into many
    // trigonometric logarithms. Keep their exact special-function form.
    gen value=(z==1 || z==gen(1)/2)?Psi(z,order,contextptr):
      symbolic(at_Psi,makesequence(z,int(order)));
    return value+correction;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static void integration_beta_partitions(unsigned remaining,unsigned k,const gen &coefficient,vecteur &factors,const vecteur &cumulant,vecteur &terms,GIAC_CONTEXT){
    if(!remaining){
      vecteur product(factors);if(!is_one(coefficient) || product.empty())product.push_back(coefficient);
      terms.push_back(product.size()==1?product[0]:symbolic(at_prod,gen(product)));
      return;
    }
    if(!k)return;
    integration_beta_partitions(remaining,k-1,coefficient,factors,cumulant,terms,contextptr);
    if(is_zero(cumulant[k]))return;
    gen denominator=1,kfactorial=factorial(k);
    for(unsigned count=1;count*k<=remaining;++count){
      denominator=denominator*kfactorial*gen(int(count));
      factors.push_back(count==1?cumulant[k]:symbolic(at_pow,makesequence(cumulant[k],int(count))));
      integration_beta_partitions(remaining-count*k,k-1,coefficient/denominator,factors,cumulant,terms,contextptr);
      factors.pop_back();
    }
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static unsigned integration_beta_terms(const gen &g,unsigned limit){
    if(is_zero(g))return 0;
    if(g.type==_FRAC)return integration_beta_terms(g._FRACptr->num,limit);
    if(g.is_symb_of_sommet(at_neg))return integration_beta_terms(g._SYMBptr->feuille,limit);
    if(g.type!=_SYMB || g._SYMBptr->feuille.type!=_VECT)return 1;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if(g.is_symb_of_sommet(at_pow) && v.size()==2 && v[1].type==_INT_ && v[1].val>=0){
      unsigned base=integration_beta_terms(v[0],limit),count=1;
      if(base<=1)return base;
      for(int k=0;k<v[1].val;++k){count*=base;if(count>=limit)return limit;}
      return count;
    }
    bool product=g.is_symb_of_sommet(at_prod);
    if(!product && !g.is_symb_of_sommet(at_plus))return 1;
    unsigned count=product?1:0;
    for(unsigned i=0;i<v.size();++i){
      unsigned next=integration_beta_terms(v[i],limit);count=product?count*next:count+next;
      if(count>=limit)return limit;
    }
    return count;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_cumulant_moment(const vecteur &cumulant,unsigned n,bool &compact,GIAC_CONTEXT){
    vecteur moment(n+1);moment[0]=1;compact=false;
    for(unsigned j=1;j<=n;++j){
      unsigned expansion=0;
      for(unsigned k=1;k<=j;++k){
        expansion+=integration_beta_terms(cumulant[k],65)*integration_beta_terms(moment[j-k],65);
        if(expansion>64)break;
      }
      if(expansion>64){
        vecteur factors,terms;
        integration_beta_partitions(n,n,factorial(n),factors,cumulant,terms,contextptr);
        compact=true;
        if(terms.empty())return 0;
        return terms.size()==1?terms[0]:symbolic(at_plus,gen(terms));
      }
      gen sum=0;unsigned choose=1;
      for(unsigned k=1;k<=j;++k){
        if(!is_zero(cumulant[k]) && !is_zero(moment[j-k]))sum+=int(choose)*cumulant[k]*moment[j-k];
        choose=choose*(j-k)/k;
      }
      moment[j]=ratnormal(sum,contextptr);
    }
    return moment[n];
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_beta_moment(const gen &p,const gen &q,const gen &u,const gen &v,const gen &shift,unsigned n,bool &compact,GIAC_CONTEXT){
    vecteur cumulant(n+1);
    gen up=1,vp=1,sp=1;
    for(unsigned k=1;k<=n;++k){
      up=up*u;vp=vp*v;sp=sp*(u+v);
      cumulant[k]=ratnormal(up*integration_beta_psi(p,k-1,contextptr)+vp*integration_beta_psi(q,k-1,contextptr)-sp*integration_beta_psi(p+q,k-1,contextptr),contextptr);
      if(k==1)cumulant[k]+=shift;
    }
    return integration_cumulant_moment(cumulant,n,compact,contextptr);
  }

  static bool integration_outer_power(gen &base,gen &power){
    for(unsigned depth=0;depth<8;++depth){
      if(base.is_symb_of_sommet(at_inv)){power=-power;base=gen(base._SYMBptr->feuille);continue;}
      if(base.is_symb_of_sommet(at_sqrt)){power=power/2;base=gen(base._SYMBptr->feuille);continue;}
      if(base.is_symb_of_sommet(at_pow) && base._SYMBptr->feuille.type==_VECT){
        const vecteur &v=*base._SYMBptr->feuille._VECTptr;
        if(v.size()!=2 || !integration_rational(v[1]))return false;
        power=power*v[1];base=gen(v[0]);continue;
      }
      return true;
    }
    return false;
  }

  static bool integration_mellin_monomial(const gen &g,const gen &x,gen &c,gen &n,unsigned &budget,GIAC_CONTEXT){
    if(!budget)return false;--budget;
    if(integration_monomial(g,x,c,n,contextptr))return true;
    // Evaluation may rewrite x^(17/18) as (x^(1/18))^17. Flatten only
    // a pure x power under an integer power; no coefficient is exponentiated.
    // For x!=0 this identity also holds on the principal complex branch.
    if(g.is_symb_of_sommet(at_pow) && g._SYMBptr->feuille.type==_VECT){
      const vecteur &outer=*g._SYMBptr->feuille._VECTptr;
      if(outer.size()==2 && outer[1].type==_INT_ && outer[1].val>=-64 && outer[1].val<=64 &&
         outer[0].is_symb_of_sommet(at_pow) && outer[0]._SYMBptr->feuille.type==_VECT){
        const vecteur &inner=*outer[0]._SYMBptr->feuille._VECTptr;
        if(inner.size()==2 && inner[0]==x && integration_rational(inner[1]) &&
           (inner[1].type!=_FRAC || (inner[1]._FRACptr->den.type==_INT_ && inner[1]._FRACptr->den.val>0 && inner[1]._FRACptr->den.val<=64))){
          c=1;n=inner[1]*outer[1];return true;
        }
      }
    }
    if(!g.is_symb_of_sommet(at_prod) || g._SYMBptr->feuille.type!=_VECT)return false;
    c=1;n=0;const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    for(unsigned i=0;i<v.size();++i){gen a,b;if(!integration_mellin_monomial(v[i],x,a,b,budget,contextptr))return false;c=c*a;n+=b;}
    return true;
  }

  // Mellin derivatives: logarithmic weights cost a fixed number of terms.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_mellin_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf)return false;
    vecteur singleton;const vecteur *terms=0;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)terms=e._SYMBptr->feuille._VECTptr;
    else {singleton.push_back(e);terms=&singleton;}
    const vecteur &v=*terms;if(v.size()>8)return false;
    gen m=0,c=1,A=0,B=0,q=0,r=0;unsigned logs=0,budget=64;bool denominator=false;gen logbase=0,denbase=0;
    for(unsigned i=0;i<v.size();++i){
      gen base=v[i],power=1;
      if(!integration_outer_power(base,power))return false;
      if(base.is_symb_of_sommet(at_ln)){
        // Logarithms need not be positive. Do not flatten sqrt(log(x)^2)
        // or other fractional outer powers into a signed logarithm.
        if(v[i]!=base && (!v[i].is_symb_of_sommet(at_pow) ||
           v[i]._SYMBptr->feuille[0]!=base || v[i]._SYMBptr->feuille[1].type!=_INT_))return false;
        if(power.type!=_INT_ || power.val<0 || power.val>16)return false;
        if(logs && logbase!=base._SYMBptr->feuille)return false;
        logbase=base._SYMBptr->feuille;
        logs+=power.val;if(logs>16 || (logbase!=x && logs>4))return false;continue;
      }
      if(base.is_symb_of_sommet(at_plus) && is_strictly_positive(-power,contextptr)){
        // Evaluation can split D^(-3/2) into D^(-1)/sqrt(D).
        // The shared base is proved positive before combining its exponents.
        if(denominator){if(base!=denbase)return false;r-=power;continue;}
        if(base._SYMBptr->feuille.type!=_VECT || base._SYMBptr->feuille._VECTptr->size()!=2)return false;
        const vecteur &d=*base._SYMBptr->feuille._VECTptr;
        unsigned j=integration_rational(d[0])?0:1;
        if(!integration_rational(d[j]) || !integration_mellin_monomial(d[1-j],x,B,q,budget,contextptr))return false;
        A=d[j];r=-power;denbase=base;denominator=true;continue;
      }
      gen f,n;if(!integration_mellin_monomial(v[i],x,f,n,budget,contextptr))return false;
      c=c*f;m+=n;
    }
    gen p=m+1;
    if(!denominator || !is_strictly_positive(A,contextptr) || !is_strictly_positive(B,contextptr) ||
       !is_strictly_positive(q,contextptr) || !is_strictly_positive(p,contextptr) ||
       !is_strictly_positive(q*r-p,contextptr) || is_strictly_greater(r,16,contextptr))return false;
    if(logs && logbase!=x && logbase!=denbase)return false;
    // A logarithm of the positive denominator is a derivative in its
    // exponent: d^n B(a,b)/db^n, with sign (-1)^n and shift log(A).
    bool base_log=logs && logbase==denbase;
    // Keep the compact reflection formula for the previously supported case.
    if(r==1 && logs<=2 && !base_log){
      gen theta=cst_pi*p/q,s=sin(theta,contextptr);
      res=c*pow(A/B,p/q,contextptr)*cst_pi/(A*q*s);
      if(logs){gen L=(ln(A/B,contextptr)-cst_pi*cos(theta,contextptr)/s)/q;
        res=res*(logs==1?L:L*L+cst_pi*cst_pi/(q*q*s*s));}
      return true;
    }
    gen a=p/q,b=r-a;
    res=c*pow(A,-r,contextptr)*pow(A/B,a,contextptr)*Gamma(a,contextptr)*Gamma(b,contextptr)/(q*Gamma(r,contextptr));
    bool compact=false;
    if(logs)res=res*integration_beta_moment(a,b,base_log?gen(0):inv(q,contextptr),base_log?gen(-1):-inv(q,contextptr),base_log?ln(A,contextptr):ln(A/B,contextptr)/q,logs,compact,contextptr);
    if(!compact)res=ratnormal(res,contextptr);return true;
  }

  // Only positive bases x and 1-x on (0,1); splitting their real powers is safe.
  static bool integration_beta_weight(const gen &g,const gen &x,gen &p,gen &q,const gen &power,unsigned &budget,GIAC_CONTEXT){
    if (!budget--) return false;
    if (is_one(g)) return true;
    if (g==x){p+=power;return true;}
    gen a,b;
    if (g.is_symb_of_sommet(at_plus) && is_linear_wrt(g,x,a,b,contextptr) && a==-1 && b==1){q+=power;return true;}
    if (g.is_symb_of_sommet(at_inv)) return integration_beta_weight(g._SYMBptr->feuille,x,p,q,-power,budget,contextptr);
    if (g.is_symb_of_sommet(at_sqrt)) return integration_beta_weight(g._SYMBptr->feuille,x,p,q,power/2,budget,contextptr);
    if (g.is_symb_of_sommet(at_pow) && g._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*g._SYMBptr->feuille._VECTptr;
      return v.size()==2 && integration_rational(v[1]) && integration_beta_weight(v[0],x,p,q,power*v[1],budget,contextptr);
    }
    if (!g.is_symb_of_sommet(at_prod) || g._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    for (unsigned i=0;i<v.size();++i) if (!integration_beta_weight(v[i],x,p,q,power,budget,contextptr)) return false;
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_beta_joint(const gen *u,const gen *v,unsigned n,const gen &p,const gen &q,bool &compact,GIAC_CONTEXT){
    // Joint second/third moments of arbitrary log directions, using the
    // partition formula for cumulants. Only three directions are stored.
    gen pp[3],pq[3],ps[3],mean[3],answer=1;
    for (unsigned k=0;k<n;++k){
      pp[k]=integration_beta_psi(p,k,contextptr);
      pq[k]=integration_beta_psi(q,k,contextptr);
      ps[k]=integration_beta_psi(p+q,k,contextptr);
    }
    for (unsigned i=0;i<n;++i){
      mean[i]=u[i]*pp[0]+v[i]*pq[0]-(u[i]+v[i])*ps[0];
      answer=answer*mean[i];
    }
    for (unsigned i=0;i<n;++i) for (unsigned j=i+1;j<n;++j){
      gen covariance=u[i]*u[j]*pp[1]+v[i]*v[j]*pq[1]-(u[i]+v[i])*(u[j]+v[j])*ps[1];
      answer+=n==2?covariance:covariance*mean[3-i-j];
    }
    if (n==3)
      answer+=u[0]*u[1]*u[2]*pp[2]+v[0]*v[1]*v[2]*pq[2]-(u[0]+v[0])*(u[1]+v[1])*(u[2]+v[2])*ps[2];
    // Three mixed directions can still distribute beyond the common
    // 64-term budget when rational parameter shifts add constant terms.
    compact=integration_beta_terms(answer,65)>64;
    return compact?answer:ratnormal(answer,contextptr);
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_beta_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=1)return false;
    vecteur singleton;const vecteur *terms=0;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)terms=e._SYMBptr->feuille._VECTptr;
    else {singleton.push_back(e);terms=&singleton;}
    const vecteur &v=*terms;if(v.size()>8)return false;
    gen p=1,q=1,u=0,w=0,ud[3],vd[3];unsigned order=0,stored=0,budget=64;bool directional=false,mixed=false;
    for(unsigned i=0;i<v.size();++i){
      gen base=v[i],power=1;if(!integration_outer_power(base,power))return false;
      if(base.is_symb_of_sommet(at_ln)){
        if(v[i]!=base && (!v[i].is_symb_of_sommet(at_pow) ||
           v[i]._SYMBptr->feuille[0]!=base || v[i]._SYMBptr->feuille[1].type!=_INT_))return false;
        if(power.type!=_INT_ || power.val<1 || power.val>16)return false;
        gen a=0,b=0;unsigned argbudget=32;
        if(!integration_beta_weight(base._SYMBptr->feuille,x,a,b,1,argbudget,contextptr) || (is_zero(a)&&is_zero(b)))return false;
        for (int j=0;j<power.val && stored<3;++j){ud[stored]=a;vd[stored]=b;++stored;}
        if(!directional){u=a;w=b;directional=true;}
        else if(a!=u || b!=w)mixed=true;
        order+=power.val;if(order>16)return false;
      }
      else if(!integration_beta_weight(v[i],x,p,q,1,budget,contextptr))return false;
    }
    if(!order || !is_strictly_positive(p,contextptr) || !is_strictly_positive(q,contextptr) ||
       is_strictly_greater(p,16,contextptr) || is_strictly_greater(q,16,contextptr))return false;
    gen beta=Gamma(p,contextptr)*Gamma(q,contextptr)/Gamma(p+q,contextptr);bool compact=false;
    // Different log directions use joint cumulants through total degree 3.
    // A single direction keeps the bounded higher-order Bell recurrence.
    if(mixed){
      if(order>3)return false;
      res=beta*integration_beta_joint(ud,vd,order,p,q,compact,contextptr);
    }
    else {
      res=beta*integration_beta_moment(p,q,u,w,0,order,compact,contextptr);
    }
    if(!compact)res=ratnormal(res,contextptr);return true;
  }

  static bool integration_harmonic_real(const gen &g,GIAC_CONTEXT){
    if(taille(g,65)>64 || is_undef(g) || is_inf(g) || !is_zero(im(g,contextptr)))return false;
    vecteur names=lidnt(g);
    for(unsigned i=0;i<names.size();++i)if(names[i]!=cst_pi)return false;
    return true;
  }

  // One bounded product-to-sum step; no generic trigonometric expansion.
  static bool integration_harmonic_product(const gen &left,const gen &right,const gen &x,vecteur &out,GIAC_CONTEXT){
    if(out.size()>=32)return false;
    gen a=left,b=right,scale=integration_coefficient(a,x,contextptr)*integration_coefficient(b,x,contextptr);
    if(is_constant_wrt(a,x,contextptr)){out.push_back(scale*a*b);return true;}
    if(is_constant_wrt(b,x,contextptr)){out.push_back(scale*a*b);return true;}
    bool sa=a.is_symb_of_sommet(at_sin),sb=b.is_symb_of_sommet(at_sin);
    if((!sa && !a.is_symb_of_sommet(at_cos)) || (!sb && !b.is_symb_of_sommet(at_cos)) || out.size()>30)return false;
    gen sum=ratnormal(a._SYMBptr->feuille+b._SYMBptr->feuille,contextptr),difference=ratnormal(a._SYMBptr->feuille-b._SYMBptr->feuille,contextptr);
    const unary_function_ptr *op=sa==sb?at_cos:at_sin;
    out.push_back((sa&&sb?-scale:scale)*symbolic(op,sum)/2);
    out.push_back((!sa&&sb?-scale:scale)*symbolic(op,difference)/2);
    return true;
  }

  // At most 64 syntax visits and 32 harmonic terms; powers must be integers
  // from 0 through 4. Failure leaves the caller's original expression intact.
  static bool integration_harmonic_terms(const gen &g,const gen &x,vecteur &out,unsigned &budget,GIAC_CONTEXT){
    if(!budget || out.size()>=32)return false;--budget;
    if(g.is_symb_of_sommet(at_sin) || g.is_symb_of_sommet(at_cos)){
      gen a,b;
      if(!is_linear_wrt(g._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) ||
         !integration_harmonic_real(b,contextptr))return false;
      out.push_back(symbolic(g._SYMBptr->sommet,a*x+b));return true;
    }
    if(is_constant_wrt(g,x,contextptr)){out.push_back(g);return true;}
    if(g.is_symb_of_sommet(at_neg)){
      vecteur v;if(!integration_harmonic_terms(g._SYMBptr->feuille,x,v,budget,contextptr) || out.size()+v.size()>32)return false;
      for(unsigned i=0;i<v.size();++i)out.push_back(-v[i]);return true;
    }
    if(g.type!=_SYMB || g._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if(g.is_symb_of_sommet(at_plus)){
      for(unsigned i=0;i<v.size();++i)if(!integration_harmonic_terms(v[i],x,out,budget,contextptr))return false;
      return true;
    }
    bool power=g.is_symb_of_sommet(at_pow);
    if(power && (v.size()!=2 || v[1].type!=_INT_ || v[1].val<0 || v[1].val>4))return false;
    if(!power && !g.is_symb_of_sommet(at_prod))return false;
    vecteur result(1,1),base;
    if(power && !integration_harmonic_terms(v[0],x,base,budget,contextptr))return false;
    unsigned count=power?v[1].val:v.size();
    for(unsigned i=0;i<count;++i){
      if(!power){base.clear();if(!integration_harmonic_terms(v[i],x,base,budget,contextptr))return false;}
      vecteur next;
      for(unsigned j=0;j<result.size();++j)for(unsigned k=0;k<base.size();++k)
        if(!integration_harmonic_product(result[j],base[k],x,next,contextptr))return false;
      result.swap(next);
    }
    if(out.size()+result.size()>32)return false;
    for(unsigned i=0;i<result.size();++i)out.push_back(result[i]);return true;
  }

  // Recognize a sum of rational linear terms and squared rational affines.
  // This computes three coefficients without polynomial expansion/derivatives.
  static bool integration_gaussian_quadratic(const gen &g,const gen &x,gen &q,gen &l,gen &constant,GIAC_CONTEXT){
    const vecteur *terms=g.is_symb_of_sommet(at_plus) && g._SYMBptr->feuille.type==_VECT?g._SYMBptr->feuille._VECTptr:0;
    unsigned count=terms?terms->size():1;if(count>8)return false;
    q=0;l=0;constant=0;
    for(unsigned i=0;i<count;++i){
      gen t=terms?(*terms)[i]:g,scale=integration_coefficient(t,x,contextptr),base,a,b;
      if(!integration_rational(scale))return false;
      if(integration_power(t,base,2)){
        if(!is_linear_wrt(base,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b))return false;
        q+=scale*a*a;l+=2*scale*a*b;constant+=scale*b*b;
      }
      else {
        if(!is_linear_wrt(t,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b))return false;
        l+=scale*a;constant+=scale*b;
      }
    }
    return true;
  }

  // Completion of the Gaussian square followed by the erf convolution or
  // the centered two-erf correlation identity. Real slopes can have either sign.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_gaussian_erf(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    bool full=lo==minus_inf && hi==plus_inf,half=is_zero(lo) && hi==plus_inf;
    if((!full && !half) || !angle_radian(contextptr) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()<2 || v.size()>3)return false;
    gen exponent;bool gaussian=false;vecteur slopes,shifts;
    for(unsigned i=0;i<v.size();++i){
      if(v[i].is_symb_of_sommet(at_exp)){
        if(gaussian)return false;gaussian=true;exponent=v[i]._SYMBptr->feuille;continue;
      }
      gen term=v[i],base,a,b;unsigned multiplicity=1;
      if(integration_power(term,base,2)){term=gen(base);multiplicity=2;}
      if(!term.is_symb_of_sommet(at_erf) || slopes.size()+multiplicity>2 ||
         !is_linear_wrt(term._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b))return false;
      for(unsigned j=0;j<multiplicity;++j){slopes.push_back(a);shifts.push_back(b);}
    }
    gen q,l,constant;
    if(!gaussian || slopes.empty() || !integration_gaussian_quadratic(exponent,x,q,l,constant,contextptr) || !is_strictly_positive(-q,contextptr))return false;
    gen a=-q,mean=-l/(2*q),factor=exp(constant-l*l/(4*q),contextptr);
    if(half && !is_zero(mean))return false;
    for(unsigned i=0;i<slopes.size();++i)shifts[i]+=slopes[i]*mean;
    if(slopes.size()==1){
      gen b=slopes[0];
      if(full)res=factor*sqrt(cst_pi/a,contextptr)*erf(shifts[0]*sqrt(a/(a+b*b),contextptr),contextptr);
      else {
        if(!is_zero(shifts[0]))return false;
        res=factor*atan(b/sqrt(a,contextptr),contextptr)/sqrt(cst_pi*a,contextptr);
      }
      return true;
    }
    if(!is_zero(shifts[0]) || !is_zero(shifts[1]))return false;
    gen b=slopes[0],c=slopes[1];
    res=(full?2:1)*factor*asin(b*c/sqrt((a+b*b)*(a+c*c),contextptr),contextptr)/sqrt(cst_pi*a,contextptr);
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_laplace_difference(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()<3 || v.size()>6) return false;
    gen a=0,d=0,numerator=1;bool exponential=false;unsigned inverse=0;
    for (unsigned i=0;i<v.size();++i){
      gen base;
      if (v[i].is_symb_of_sommet(at_exp)){
        if (exponential || !is_linear_wrt(v[i]._SYMBptr->feuille,x,a,d,contextptr) ||
            !is_zero(im(a,contextptr)) || contains(a,x) || taille(a,33)>32 || !integration_rational(d) || !is_strictly_positive(-a,contextptr)) return false;
        exponential=true;
      }
      else if (integration_power(v[i],base,-1) && base==x){if (inverse) return false;inverse=1;}
      else if (integration_power(v[i],base,-2) && base==x){if (inverse) return false;inverse=2;}
      else numerator=numerator*v[i];
    }
    if (!exponential || !inverse) return false;
    vecteur expanded;unsigned harmonic_budget=64;
    if(integration_harmonic_terms(numerator,x,expanded,harmonic_budget,contextptr)){
      gen sum=0;for(unsigned i=0;i<expanded.size();++i)sum+=expanded[i];numerator=sum;
    }
    if (!numerator.is_symb_of_sommet(at_plus) || numerator._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &terms=*numerator._SYMBptr->feuille._VECTptr;
    if (terms.size()>32) return false;
    gen at_zero=0,derivative_zero=0,answer=0;
    for (unsigned i=0;i<terms.size();++i){
      gen t=terms[i],c=integration_coefficient(t,x,contextptr),b,phase;
      if (is_constant_wrt(t,x,contextptr)) {at_zero+=subst(c*t,x,0,false,contextptr).eval(1,contextptr);continue;}
      // For 1/x^2, convergence requires both the constant and linear Taylor
      // coefficients to cancel. Keep the polynomial contribution implicit.
      if (inverse==2 && t==x){
        c=c.eval(1,contextptr);
        if (!integration_harmonic_real(c,contextptr)) return false;
        derivative_zero+=c;continue;
      }
      bool sine=t.is_symb_of_sommet(at_sin);
      c=c.eval(1,contextptr);
      if ((!sine && !t.is_symb_of_sommet(at_cos)) || !integration_harmonic_real(c,contextptr) ||
          !is_linear_wrt(t._SYMBptr->feuille,x,b,phase,contextptr) || (!is_zero(im(b,contextptr)) || contains(b,x) || taille(b,33)>32)) return false;
      phase=phase.eval(1,contextptr);
      if(!integration_harmonic_real(phase,contextptr))return false;
      gen s=sin(phase,contextptr),co=cos(phase,contextptr),ratio=b/(-a);
      gen value=sine?s:co,slope=sine?co:-s;
      at_zero+=c*value;derivative_zero+=c*b*slope;
      gen angle=atan(ratio,contextptr),logarithm=ln(1+ratio*ratio,contextptr)/2;
      if (inverse==1) answer+=c*(slope*angle-value*logarithm);
      else {
        // Integration by parts gives the transforms of cos(bx)-1 and
        // sin(bx)-bx. This avoids differentiating or expanding expressions.
        answer+=c*(value*(-b*angle-a*logarithm)+slope*(b+a*angle-b*logarithm));
      }
    }
    if (!is_zero(ratnormal(at_zero,contextptr)) ||
        (inverse==2 && !is_zero(ratnormal(derivative_zero,contextptr)))) return false;
    res=exp(d,contextptr)*answer;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_inverse_gaussian(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!is_zero(lo) || hi!=plus_inf) return false;
    const gen *exponential=&e;gen weight=1;
    if (e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*e._SYMBptr->feuille._VECTptr;
      if (v.size()!=2) return false;
      unsigned j=v[0].is_symb_of_sommet(at_exp)?0:1;
      exponential=&v[j];weight=v[1-j];
    }
    if (!exponential->is_symb_of_sommet(at_exp)) return false;
    gen c,n;
    if (!integration_monomial(weight,x,c,n,contextptr)) return false;
    const gen &arg=exponential->_SYMBptr->feuille;
    vecteur terms;
    if (arg.is_symb_of_sommet(at_plus) && arg._SYMBptr->feuille.type==_VECT) terms=*arg._SYMBptr->feuille._VECTptr;
    else terms.push_back(arg);
    if (terms.size()>3) return false;
    gen a=0,b=0,d=0,k=0;bool expanded_square=false;
    for (unsigned i=0;i<terms.size();++i){
      gen coefficient,p;
      if (!integration_monomial(terms[i],x,coefficient,p,contextptr)){
        // Expand only one square of two reciprocal monomials, at most three
        // terms. For x>0 their rational powers combine without branch changes.
        gen core=terms[i],scale=integration_coefficient(core,x,contextptr),sum;
        if (expanded_square || !integration_rational(scale) || !integration_power(core,sum,2) ||
            !sum.is_symb_of_sommet(at_plus) || sum._SYMBptr->feuille.type!=_VECT ||
            sum._SYMBptr->feuille._VECTptr->size()!=2) return false;
        const vecteur &v=*sum._SYMBptr->feuille._VECTptr;
        gen c1,p1,c2,p2;
        if (!integration_monomial(v[0],x,c1,p1,contextptr) ||
            !integration_monomial(v[1],x,c2,p2,contextptr) || is_zero(p1) || p1+p2!=0) return false;
        coefficient=2*scale*c1*c2;p=0;
        terms.push_back(scale*c1*c1*pow(x,2*p1,contextptr));
        terms.push_back(scale*c2*c2*pow(x,2*p2,contextptr));
        expanded_square=true;
      }
      if (is_zero(p)){d+=coefficient;continue;}
      bool positive=is_strictly_positive(p,contextptr);gen degree=positive?p:-p;
      if (is_zero(k)) k=degree;
      else if (degree!=k) return false;
      if (positive) a-=coefficient;else b-=coefficient;
    }
    if (is_zero(k) || !is_strictly_positive(a,contextptr) || !is_strictly_positive(b,contextptr)) return false;
    // u=x^k makes the Bessel order (n+1)/k. Half-integer orders have
    // finite expressions. Form at most 17 terms by the coefficient recurrence,
    // avoiding the duplicated expression tree of the three-term recurrence.
    gen order=ratnormal((n+1)/k,contextptr);bool negative=is_strictly_positive(-order,contextptr);
    if (negative) order=-order;
    gen m=ratnormal(order-gen(1)/2,contextptr);
    if (m.type!=_INT_ || m.val<0 || m.val>16) return false;
    gen z=2*sqrt(a*b,contextptr),coefficient=1,polynomial=1;
    for (int i=1;i<=m.val;++i){
      coefficient=coefficient*gen((m.val+i)*(m.val-i+1))/(2*i);
      polynomial+=coefficient/pow(z,i);
    }
    gen scale=negative?a/b:b/a;
    res=c*sqrt(cst_pi/(negative?b:a),contextptr)*exp(d-z,contextptr)*
      pow(sqrt(scale,contextptr),m.val)*polynomial/k;return true;
  }

  // u=b*x^q maps (0,L) to (0,1). Termwise integration of log(1+/-u)
  // gives zeta moments; the logarithmic power is bounded before factorial work.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_zeta(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!is_zero(lo) || !integration_rational(hi) || !is_strictly_positive(hi,contextptr) ||
        !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()<2 || v.size()>3) return false;
    bool inverse=false,kernel=false;gen u=0,logarg=0,b=0,q=0;int m=0,sign=0;
    for (unsigned i=0;i<v.size();++i){
      gen t=v[i],base;
      if (integration_power(t,base,-1) && base==x){if (inverse) return false;inverse=true;continue;}
      int n=1;
      if (t.is_symb_of_sommet(at_pow) && t._SYMBptr->feuille.type==_VECT){
        const vecteur &p=*t._SYMBptr->feuille._VECTptr;
        if (p.size()!=2 || p[1].type!=_INT_ || p[1].val<1 || p[1].val>8) return false;
        n=p[1].val;t=gen(p[0]);
      }
      if (!t.is_symb_of_sommet(at_ln)) return false;
      gen other;
      if (integration_one_plus(t._SYMBptr->feuille,other)){
        if (kernel || n!=1 || !integration_monomial(other,x,b,q,contextptr) ||
            is_zero(b) || !is_strictly_positive(q,contextptr) || is_strictly_greater(q,32,contextptr)) return false;
        sign=is_strictly_positive(b,contextptr)?1:-1;u=sign*other;b=sign*b;kernel=true;
      }
      else {if (m) return false;m=n;logarg=t._SYMBptr->feuille;}
    }
    if (!inverse || !kernel || !is_zero(ratnormal(b*pow(hi,q,contextptr)-1,contextptr)) ||
        (m && !is_zero(ratnormal(logarg-u,contextptr)))) return false;
    res=factorial(m)*_Zeta(m+2,contextptr)/q;
    if (sign==1) res=res*(1-inv(pow(gen(2),m+1),contextptr));
    if ((m+(sign==-1))%2) res=-res;
    return true;
  }

  // Differentiate integral sech(t)^p dt = sqrt(pi)*Gamma(p/2)/Gamma((p+1)/2).
  // Positive p supplies exponential decay at both ends even with log weights.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_hyperbolic_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=2) return false;
    for (unsigned i=0;i<2;++i){
      gen t=v[i],base;int logs=1;
      if (integration_power(t,base,2)){t=base;logs=2;}
      if (!t.is_symb_of_sommet(at_ln) || !t._SYMBptr->feuille.is_symb_of_sommet(at_cosh)) continue;
      const gen &ch=t._SYMBptr->feuille;gen p=0,d=v[1-i];
      if (d.is_symb_of_sommet(at_inv)){p=1;d=gen(d._SYMBptr->feuille);}
      if (d.is_symb_of_sommet(at_pow) && d._SYMBptr->feuille.type==_VECT){
        const vecteur &w=*d._SYMBptr->feuille._VECTptr;
        if (w.size()!=2 || !integration_rational(w[1])) continue;
        p=is_one(p)?w[1]:-w[1];d=gen(w[0]);
      }
      if (d!=ch || !is_strictly_positive(p,contextptr) || is_strictly_greater(p,16,contextptr)) continue;
      gen a,b;
      if (!is_linear_wrt(ch._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) ||
          !integration_rational(b) || is_zero(a)) continue;
      bool full=lo==minus_inf && hi==plus_inf;
      bool half=(hi==plus_inf && is_zero(ratnormal(a*lo+b,contextptr))) ||
        (lo==minus_inf && is_zero(ratnormal(a*hi+b,contextptr)));
      if (!full && !half) continue;
      gen A=p/2,B=(p+1)/2,L=(Psi(B,contextptr)-Psi(A,contextptr))/2;
      res=sqrt(cst_pi,contextptr)*Gamma(A,contextptr)/(abs(a,contextptr)*Gamma(B,contextptr));
      res=res*(logs==1?L:L*L+(Psi(A,1,contextptr)-Psi(B,1,contextptr))/4);
      if (half && !full) res=res/2;
      res=ratnormal(res,contextptr);return true;
    }
    return false;
  }

  // t=atan(a*x) gives moments of t^n*cot(t) on (0,pi/2).
  // Integrate the Fourier series of log(sin(t)); two integrations by parts
  // reduce each moment, using at most four odd-zeta terms for n<=8.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_log_moment(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf || !e.is_symb_of_sommet(at_prod) ||
        e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=3) return false;
    bool inverse=false,denominator=false,angle=false;gen a=0,A=0,B=0;int n=0;
    for (unsigned i=0;i<v.size();++i){
      gen t=v[i],base;
      if (integration_power(t,base,-1)){
        if (base==x){if (inverse) return false;inverse=true;}
        else {if (denominator || !integration_quadratic(base,x,A,B,contextptr)) return false;denominator=true;}
        continue;
      }
      n=1;
      if (t.is_symb_of_sommet(at_pow) && t._SYMBptr->feuille.type==_VECT){
        const vecteur &p=*t._SYMBptr->feuille._VECTptr;
        if (p.size()!=2 || p[1].type!=_INT_ || p[1].val<1 || p[1].val>8) return false;
        n=p[1].val;t=gen(p[0]);
      }
      gen b;
      if (angle || !t.is_symb_of_sommet(at_atan) || !is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) ||
          !integration_rational(a) || is_zero(a) || !is_zero(b)) return false;
      angle=true;
    }
    if (!inverse || !denominator || !angle || !integration_rational(A) || !integration_rational(B) ||
        !is_strictly_positive(B,contextptr) || !is_zero(ratnormal(A-B*a*a,contextptr))) return false;
    gen L=cst_pi/2,sum=0,c=1;int m=n-1,s=3;
    while(m>0){
      gen z=_Zeta(s,contextptr),eta=(1-inv(pow(gen(2),s-1),contextptr))*z;
      if (m==1){sum-=c*(eta+z)/4;break;}
      sum-=c*gen(m)*pow(L,m-1)*eta/4;
      c=-c*gen(m)*gen(m-1)/4;m-=2;s+=2;
    }
    res=(pow(L,n)*ln(gen(2),contextptr)+n*sum)/B;
    if (!is_strictly_positive(a,contextptr) && n%2) res=-res;
    return true;
  }

  // Read a bounded affine expression in one cosine without polynomial expansion.
  static bool integration_beta_affine_weight(const gen &g,const gen &x,const gen &lo,const gen &hi,gen &p,gen &q,gen &coefficient,const gen &power,unsigned &budget,GIAC_CONTEXT){
    if(!budget)return false;--budget;
    // Affine widths raised to huge integer powers can allocate large exact
    // coefficients even when logarithm arguments later cancel those factors.
    if(is_strictly_greater(power,32,contextptr) || is_strictly_greater(-power,32,contextptr))return false;
    if(integration_rational(g)){
      if(!is_strictly_positive(g,contextptr))return false;
      coefficient=coefficient*pow(g,power,contextptr);return true;
    }
    if(g.is_symb_of_sommet(at_inv))return integration_beta_affine_weight(g._SYMBptr->feuille,x,lo,hi,p,q,coefficient,-power,budget,contextptr);
    if(g.is_symb_of_sommet(at_sqrt))return integration_beta_affine_weight(g._SYMBptr->feuille,x,lo,hi,p,q,coefficient,power/2,budget,contextptr);
    if(g.is_symb_of_sommet(at_pow) && g._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*g._SYMBptr->feuille._VECTptr;
      return v.size()==2 && integration_rational(v[1]) && integration_beta_affine_weight(v[0],x,lo,hi,p,q,coefficient,power*v[1],budget,contextptr);
    }
    if(g.is_symb_of_sommet(at_prod) && g._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*g._SYMBptr->feuille._VECTptr;
      for(unsigned i=0;i<v.size();++i)if(!integration_beta_affine_weight(v[i],x,lo,hi,p,q,coefficient,power,budget,contextptr))return false;
      return true;
    }
    gen poly[3];
    if(g==x)poly[1]=1;
    else {
      if(!g.is_symb_of_sommet(at_plus) || g._SYMBptr->feuille.type!=_VECT)return false;
      const vecteur &v=*g._SYMBptr->feuille._VECTptr;if(v.size()>3)return false;
      for(unsigned i=0;i<v.size();++i){gen c,n;
        if(!integration_monomial(v[i],x,c,n,contextptr) || n.type!=_INT_ || n.val<0 || n.val>2)return false;
        poly[n.val]+=c;
      }
    }
    gen left=ratnormal(poly[0]+lo*(poly[1]+lo*poly[2]),contextptr);
    gen right=ratnormal(poly[0]+hi*(poly[1]+hi*poly[2]),contextptr),width=hi-lo,scale;
    if(is_zero(poly[2])){
      if(is_zero(left) && is_strictly_positive(right,contextptr)){p+=power;scale=right;}
      else if(is_zero(right) && is_strictly_positive(left,contextptr)){q+=power;scale=left;}
      else return false;
    }
    else {
      if(!is_zero(left) || !is_zero(right) || !is_strictly_positive(-poly[2],contextptr))return false;
      p+=power;q+=power;scale=-poly[2]*width*width;
    }
    coefficient=coefficient*pow(scale,power,contextptr);return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_beta_affine_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !integration_rational(lo) || !integration_rational(hi) ||
       !is_strictly_positive(hi-lo,contextptr) || (is_zero(lo) && hi==1))return false;
    vecteur singleton;const vecteur *terms;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)terms=e._SYMBptr->feuille._VECTptr;
    else {singleton.push_back(e);terms=&singleton;}
    const vecteur &v=*terms;if(v.size()>8)return false;
    gen t(identificateur(" khicas_beta_affine"));
    if(contains(e,t) || eval(t,1,contextptr)!=t)return false;
    gen p=0,q=0,c=1,mapped=1;unsigned budget=64,logs=0;
    for(unsigned i=0;i<v.size();++i){
      gen f=v[i],base;int order=1;
      if(f.is_symb_of_sommet(at_pow) && f._SYMBptr->feuille.type==_VECT){
        const vecteur &power=*f._SYMBptr->feuille._VECTptr;
        if(power.size()==2 && power[0].is_symb_of_sommet(at_ln) && power[1].type==_INT_){order=power[1].val;f=gen(power[0]);}
      }
      if(f.is_symb_of_sommet(at_ln)){
        if(order<1 || order>16 || (logs+=order)>16)return false;
        gen a=0,b=0,scale=1;
        if(!integration_beta_affine_weight(f._SYMBptr->feuille,x,lo,hi,a,b,scale,1,budget,contextptr) ||
           !is_zero(ratnormal(scale-1,contextptr)))return false;
        gen argument=pow(t,a,contextptr)*pow(1-t,b,contextptr);
        mapped=mapped*pow(symbolic(at_ln,argument),order);continue;
      }
      if(!integration_beta_affine_weight(v[i],x,lo,hi,p,q,c,1,budget,contextptr))return false;
    }
    if(!logs)return false;
    mapped=mapped*pow(t,p,contextptr)*pow(1-t,q,contextptr);
    if(!integrate_beta_log(mapped,t,0,1,res,contextptr))return false;
    res=(hi-lo)*c*res;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_acos_circle_value(const gen &q,GIAC_CONTEXT){
    gen h=atan(sqrt(q,contextptr),contextptr);
    return 2*cst_pi*h-2*h*h-2*cst_pi*atan(sqrt(1+2*q,contextptr),contextptr)+cst_pi*cst_pi/2;
  }

  static bool integration_arc_binomial(gen g,const gen &x,gen &constant,gen &coefficient,gen &degree,unsigned &budget,GIAC_CONTEXT){
    gen scale=integration_coefficient(g,x,contextptr);
    if(!integration_rational(scale) || !g.is_symb_of_sommet(at_plus) || g._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    constant=0;coefficient=0;degree=0;
    for(unsigned i=0;i<v.size();++i){gen c,n;
      if(!integration_mellin_monomial(v[i],x,c,n,budget,contextptr))return false;
      if(is_zero(n))constant+=scale*c;
      else {if(!is_zero(degree))return false;degree=n;coefficient=scale*c;}
    }
    return !is_zero(degree) && !is_zero(coefficient);
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_acos_monomial_pullback(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || is_inf(hi) || !is_zero(im(hi,contextptr)) ||
       !is_strictly_positive(hi,contextptr) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()>5)return false;
    gen angle,denominator,weight=1;unsigned budget=64;
    for(unsigned i=0;i<v.size();++i){gen base;
      if(v[i].is_symb_of_sommet(at_acos)){if(!is_zero(angle))return false;angle=v[i];}
      else if(integration_power(v[i],base,-1) && base.is_symb_of_sommet(at_plus)){
        if(!is_zero(denominator))return false;denominator=base;
      }
      else weight=weight*v[i];
    }
    gen E,d,k,c,m;
    if(is_zero(angle) || is_zero(denominator) || !integration_arc_binomial(denominator,x,E,d,k,budget,contextptr) ||
       !is_strictly_positive(E,contextptr) || !is_strictly_positive(d,contextptr) || !is_strictly_positive(k,contextptr) ||
       !integration_mellin_monomial(weight,x,c,m,budget,contextptr) || m+1!=k/2)return false;
    d=d/E;c=c/E;
    gen arg=angle._SYMBptr->feuille,outer=integration_coefficient(arg,x,contextptr),num=outer,den;
    if(!integration_rational(outer) || !arg.is_symb_of_sommet(at_prod) || arg._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &a=*arg._SYMBptr->feuille._VECTptr;if(a.size()>4)return false;
    for(unsigned i=0;i<a.size();++i){gen base;
      if(integration_power(a[i],base,-1) && !integration_rational(base)){if(!is_zero(den))return false;den=base;}
      else num=num*a[i];
    }
    gen A,B,C,D,kn,kd;
    if(is_zero(den) || !integration_arc_binomial(num,x,A,B,kn,budget,contextptr) ||
       !integration_arc_binomial(den,x,C,D,kd,budget,contextptr) || kn!=k || kd!=k || B!=D)return false;
    gen unit=(C-A)/2;
    if(is_zero(unit) || B!=-d*unit)return false;
    gen q=(A+C)/(4*unit);
    if(!is_strictly_positive(q,contextptr) || !is_zero(ratnormal(pow(hi,k,contextptr)*d-q,contextptr)))return false;
    res=c*integration_acos_circle_value(q,contextptr)/(k*sqrt(d,contextptr));return true;
  }

  static bool integration_affine_cosine(const gen &g,const gen &x,gen &arg,gen &constant,gen &coefficient,GIAC_CONTEXT){
    gen core=g,scale=ratnormal(integration_coefficient(core,x,contextptr),contextptr);
    if (!integration_rational(scale)) return false;
    vecteur v;
    if (core.is_symb_of_sommet(at_plus) && core._SYMBptr->feuille.type==_VECT) v=*core._SYMBptr->feuille._VECTptr;
    else v=makevecteur(core);
    if (v.size()>4) return false;
    constant=0;coefficient=0;
    for (unsigned i=0;i<v.size();++i){
      if (integration_rational(v[i])){constant+=scale*v[i];continue;}
      gen t=v[i],c=ratnormal(integration_coefficient(t,x,contextptr),contextptr);
      if (!integration_rational(c) || !t.is_symb_of_sommet(at_cos)) return false;
      if (is_undef(arg)) arg=t._SYMBptr->feuille;
      else if (arg!=t._SYMBptr->feuille) return false;
      coefficient+=scale*c;
    }
    return true;
  }

  // Half-angle substitution gives a weighted quarter disk of radius sqrt(2q).
  // Reflection across its diagonal leaves the inscribed square [0,sqrt(q)]^2.
  // The disk integral is pi*(atan(sqrt(1+2q))-pi/4), evaluated in polar
  // coordinates. This determines the whole positive-q family without a table.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_acos_circle(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !e.is_symb_of_sommet(at_acos)) return false;
    gen f=e._SYMBptr->feuille,c=ratnormal(integration_coefficient(f,x,contextptr),contextptr);
    if (!integration_rational(c)) return false;
    if (!f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;
    if (v.size()<2 || v.size()>4) return false;
    gen numerator=c,denominator=undef;
    for (unsigned i=0;i<v.size();++i){
      gen base;
      if (integration_power(v[i],base,-1) && !integration_rational(base)){
        if (!is_undef(denominator)) return false;
        denominator=base;
      }
      else numerator=numerator*v[i];
    }
    if (is_undef(denominator)) return false;
    gen arg=undef,A,B,C,D;
    if (!integration_affine_cosine(numerator,x,arg,A,B,contextptr) ||
        !integration_affine_cosine(denominator,x,arg,C,D,contextptr) || is_undef(arg) || B!=C) return false;
    gen delta=B-A;
    if (is_zero(delta) || D-B!=delta) return false;
    gen q=B/delta;
    if (!is_strictly_positive(q,contextptr)) return false;
    gen a,b;
    if (!is_linear_wrt(arg,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) ||
        !(integration_rational(b) || integration_rational(ratnormal(b/cst_pi,contextptr)))) return false;
    gen left=ratnormal(a*lo+b,contextptr),right=ratnormal(a*hi+b,contextptr);
    bool reverse=is_zero(right);
    if (!is_zero(left) && !reverse) return false;
    gen h=atan(sqrt(q,contextptr),contextptr);
    gen endpoint=reverse?left:right;
    if (!is_zero(ratnormal(endpoint-2*h,contextptr))){
      gen half=ratnormal(endpoint/2,contextptr);
      if (!half.is_symb_of_sommet(at_atan)) return false;
      const gen &t=half._SYMBptr->feuille;
      if (!is_strictly_positive(t,contextptr) || !is_zero(ratnormal(ratnormal(t*t-q,contextptr),contextptr))) return false;
    }
    // t=tan(u/2) ranges from 0 to sqrt(q). Then 2q-t^2 >= q > 0,
    // so the acos argument stays in (-1,1) and its denominator is positive.
    res=integration_acos_circle_value(q,contextptr)/a;
    if (reverse) res=-res;
    res=ratnormal(res,contextptr);return true;
  }

  // Frullani's identity for a bounded linear combination of atan(a*x).
  // Positive real scales have a common pi/2 limit; zero coefficient sum
  // cancels that limit and proves absolute convergence after division by x.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_frullani(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf ||
       !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if(v.size()<2 || v.size()>4)return false;
    gen numerator;bool inverse=false;unsigned logs=0;
    for(unsigned i=0;i<v.size();++i){
      gen t=v[i],base;unsigned n=1;
      if(integration_power(t,base,-1) && base==x){if(inverse)return false;inverse=true;continue;}
      if(integration_power(t,base,2)){t=base;n=2;}
      if(t.is_symb_of_sommet(at_ln) && t._SYMBptr->feuille==x){logs+=n;if(logs>2)return false;continue;}
      if(!is_zero(numerator) || !v[i].is_symb_of_sommet(at_plus))return false;
      numerator=v[i];
    }
    if(!inverse || !numerator.is_symb_of_sommet(at_plus) || numerator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*numerator._SYMBptr->feuille._VECTptr;
    if(terms.size()<2 || terms.size()>8)return false;
    gen sum=0,result=0,reference,degree=0;
    for(unsigned j=0;j<terms.size();++j){
      gen term=terms[j],c=integration_coefficient(term,x,contextptr),a,b;
      if(!integration_rational(c) || !term.is_symb_of_sommet(at_atan))return false;
      gen argument=term._SYMBptr->feuille;
      a=integration_coefficient(argument,x,contextptr);unsigned monomial_budget=16;gen multiplier;
      if(!integration_mellin_monomial(argument,x,multiplier,b,monomial_budget,contextptr) || is_zero(b))return false;
      a=a*multiplier;if(is_zero(degree))degree=b;else if(degree!=b)return false;
      if(taille(a,33)>32 || is_inf(a) || !is_zero(im(a,contextptr)) ||
         !is_strictly_positive(a,contextptr))return false;
      if(!logs){
        if(!j)reference=a;
        else result+=c*ln(a/reference,contextptr);
      }
      else {
        gen L=ln(a,contextptr);
        result+=c*(logs==1?-L*L/2:L*L*L/3+cst_pi*cst_pi*L/4);
      }
      sum+=c;
    }
    if(!is_zero(sum))return false;
    // A negative common power reverses both transformed endpoints; each
    // log(x) contributes one further factor 1/degree.
    res=cst_pi*result/(2*abs(degree,contextptr)*pow(degree,int(logs)));return true;
  }

  // t=x^q reduces this family to the derivative at s=1 of Gamma(s)*beta(s).
  // The beta functional equation and Hurwitz zeta derivative at zero give
  // integral_0^1 log(-log(t))/(1+t^2)dt in terms of Gamma(1/4),Gamma(3/4).
  // The positive logarithm scale supplies a separate elementary log term.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_loglog_mellin(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=1 ||
       !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if(v.size()>8)return false;
    gen q=0,h=0,c=1,m=0;bool denominator=false,logarithm=false;unsigned budget=64;
    for(unsigned i=0;i<v.size();++i){
      gen t=v[i],base,other;
      if(t.is_symb_of_sommet(at_ln)){
        if(logarithm)return false;
        gen inner=t._SYMBptr->feuille,k=integration_coefficient(inner,x,contextptr),d,s;
        if(!inner.is_symb_of_sommet(at_ln) ||
           !integration_mellin_monomial(inner._SYMBptr->feuille,x,d,s,budget,contextptr) || d!=1)return false;
        h=-k*s;
        if(taille(h,33)>32 || is_inf(h) || !is_zero(im(h,contextptr)) || !is_strictly_positive(h,contextptr))return false;
        logarithm=true;continue;
      }
      if(integration_power(t,base,-1) && integration_one_plus(base,other)){
        gen d,n;
        if(denominator || !integration_mellin_monomial(other,x,d,n,budget,contextptr) || d!=1)return false;
        q=n/2;denominator=true;continue;
      }
      gen d,n;
      if(!integration_mellin_monomial(t,x,d,n,budget,contextptr))return false;
      c=c*d;m+=n;
    }
    if(!denominator || !logarithm || !integration_rational(q) ||
       !is_strictly_positive(q,contextptr) || m+1!=q)return false;
    gen quarter=gen(1)/4;
    gen base=cst_pi*ln(2*cst_pi,contextptr)/4+
      cst_pi*ln(Gamma(3*quarter,contextptr)/Gamma(quarter,contextptr),contextptr)/2;
    res=c*(base+cst_pi*ln(h/q,contextptr)/4)/q;return true;
  }

  // A scaled logarithm of a pure monomial equals slope*log(x) on x>0.
  // This does not flatten a fractional power of a signed logarithm.
  static bool integration_log_slope(gen e,const gen &x,gen &slope,unsigned &budget,GIAC_CONTEXT){
    gen k=integration_coefficient(e,x,contextptr),a,p;
    if(!e.is_symb_of_sommet(at_ln) ||
       !integration_mellin_monomial(e._SYMBptr->feuille,x,a,p,budget,contextptr) || a!=1)return false;
    slope=k*p;
    return taille(slope,33)<=32 && !is_inf(slope) && is_zero(im(slope,contextptr)) &&
      (is_strictly_positive(slope,contextptr) || is_strictly_positive(-slope,contextptr));
  }

  // x=exp(-t) gives Gamma(s)*sum(c_j*(m_j+1)^(-s)). Its apparent
  // pole at s=0 cancels exactly when sum(c_j)=0. The first two derivatives
  // yield log(-log(x)) moments without splitting divergent summands.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_loglog_frullani(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=1 || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if(v.size()>8)return false;
    gen numerator,den=0,h=1,scale=1,shift=0;unsigned logs=0,budget=64;
    for(unsigned i=0;i<v.size();++i){
      gen t=v[i],base,slope;
      if(integration_power(t,base,-1) && integration_log_slope(base,x,slope,budget,contextptr)){
        if(!is_zero(den))return false;den=-slope;continue;
      }
      unsigned n=1;
      if(integration_power(t,base,2)){t=base;n=2;}
      if(t.is_symb_of_sommet(at_ln)){
        if(logs || !integration_log_slope(t._SYMBptr->feuille,x,slope,budget,contextptr) ||
           !is_strictly_positive(-slope,contextptr))return false;
        logs=n;h=-slope;continue;
      }
      if(v[i].is_symb_of_sommet(at_plus)){
        if(!is_zero(numerator))return false;numerator=v[i];continue;
      }
      gen a,p;if(!integration_mellin_monomial(v[i],x,a,p,budget,contextptr))return false;
      scale=scale*a;shift+=p;
    }
    if(is_zero(den) || !numerator.is_symb_of_sommet(at_plus) || numerator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*numerator._SYMBptr->feuille._VECTptr;
    if(terms.size()<2 || terms.size()>8)return false;
    gen sum=0,S1=0,S2=0,S3=0;
    for(unsigned j=0;j<terms.size();++j){
      gen c,m;
      if(!integration_mellin_monomial(terms[j],x,c,m,budget,contextptr) || !is_strictly_positive(m+shift+1,contextptr))return false;
      gen L=ln(m+shift+1,contextptr);sum+=c;S1+=c*L;
      if(logs)S2+=c*L*L;
      if(logs==2)S3+=c*L*L*L;
    }
    if(!is_zero(sum))return false;
    gen value=-S1;
    if(logs){
      gen L=ln(h,contextptr),first=S2/2+cst_euler_gamma*S1;
      if(logs==1)value=first-L*S1;
      else value=-S3/3-cst_euler_gamma*S2-(cst_euler_gamma*cst_euler_gamma+cst_pi*cst_pi/6)*S1+2*L*first-L*L*S1;
    }
    res=scale*value/den;return true;
  }
  // Rational half-angle coordinates for the same weighted-circle identity.
  // With q>0, z=(2q-1-x^2)/(2q+1-x^2) stays strictly inside (-1,1)
  // on [0,sqrt(q)]. Thus asin(z)=pi/2-acos(z) has no branch correction.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_arc_rational_circle(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !is_zero(lo) || taille(hi,33)>32 ||
        !is_strictly_positive(hi,contextptr) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()!=2) return false;
    unsigned j=(v[0].is_symb_of_sommet(at_acos) || v[0].is_symb_of_sommet(at_asin))?0:1;
    bool sine=v[j].is_symb_of_sommet(at_asin);
    if (!sine && !v[j].is_symb_of_sommet(at_acos)) return false;
    gen den,A,B;
    if (!integration_power(v[1-j],den,-1) || !integration_quadratic(den,x,A,B,contextptr) || A!=B || is_zero(A)) return false;
    gen f=v[j]._SYMBptr->feuille,c=ratnormal(integration_coefficient(f,x,contextptr),contextptr);
    if (!integration_rational(c) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &terms=*f._SYMBptr->feuille._VECTptr;
    if (terms.size()<2 || terms.size()>4) return false;
    gen numerator=c,denominator=undef;
    for (unsigned i=0;i<terms.size();++i){
      gen base;
      if (integration_power(terms[i],base,-1) && !integration_rational(base)){
        if (!is_undef(denominator)) return false;
        denominator=base;
      }
      else numerator=numerator*terms[i];
    }
    if (is_undef(denominator)) return false;
    gen nc=ratnormal(integration_coefficient(numerator,x,contextptr),contextptr);
    gen dc=ratnormal(integration_coefficient(denominator,x,contextptr),contextptr);
    if (!integration_rational(nc) || !integration_rational(dc) || is_zero(dc)) return false;
    gen n2,n0,d2,d0;
    if (!integration_quadratic(numerator,x,n2,n0,contextptr) ||
        !integration_quadratic(denominator,x,d2,d0,contextptr)) return false;
    n2=n2*nc;n0=n0*nc;d2=d2*dc;d0=d0*dc;
    if (is_zero(n2) || n2!=d2 || d0-n0!=-2*n2) return false;
    gen q=-(n0+d0)/(4*n2);
    if (!is_strictly_positive(q,contextptr) || !is_zero(ratnormal(ratnormal(hi*hi-q,contextptr),contextptr))) return false;
    gen h=atan(sqrt(q,contextptr),contextptr);
    gen value=integration_acos_circle_value(q,contextptr);
    res=(sine?cst_pi*h-value:value)/(2*A);
    res=ratnormal(res,contextptr);return true;
  }

  // A bounded exact real-constant check: rational values and rational*pi sums.
  static bool integration_period_real_bound(const gen &g,GIAC_CONTEXT){
    if (integration_rational(g)) return true;
    if (taille(g,65)>64) return false;
    gen a,b;
    return is_linear_wrt(g,cst_pi,a,b,contextptr) && integration_rational(a) && integration_rational(b);
  }

  // A+B*cos(u)+C*sin(u) has constant sign if A^2>B^2+C^2.
  // A phase shift reduces it to A+sqrt(B^2+C^2)*cos(u); tangent half-angle
  // integration over a whole period gives 2*pi*sign(A)/sqrt(A^2-B^2-C^2).
  // No primitive branch repair or singularity search is needed in this domain.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_reciprocal_trig_period(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !integration_period_real_bound(lo,contextptr) ||
        !integration_period_real_bound(hi,contextptr)) return false;
    gen f=e,c=ratnormal(integration_coefficient(f,x,contextptr),contextptr),den;
    if (!integration_rational(c) || !integration_power(f,den,-1)) return false;
    gen scale=ratnormal(integration_coefficient(den,x,contextptr),contextptr);
    if (!integration_rational(scale) || is_zero(scale) || !den.is_symb_of_sommet(at_plus) ||
        den._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*den._SYMBptr->feuille._VECTptr;
    if (v.size()>4) return false;
    gen A=0,B=0,C=0,arg=undef;
    for (unsigned i=0;i<v.size();++i){
      if (integration_rational(v[i])){A+=v[i];continue;}
      gen t=v[i],k=ratnormal(integration_coefficient(t,x,contextptr),contextptr);
      bool cosine=t.is_symb_of_sommet(at_cos);
      if (!integration_rational(k) || (!cosine && !t.is_symb_of_sommet(at_sin))) return false;
      if (is_undef(arg)) arg=t._SYMBptr->feuille;
      else if (arg!=t._SYMBptr->feuille) return false;
      if (cosine) B+=k;else C+=k;
    }
    if (is_undef(arg)) return false;
    gen discriminant=A*A-B*B-C*C,a,b;
    if (!is_strictly_positive(discriminant,contextptr) ||
        !is_linear_wrt(arg,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) ||
        !integration_period_real_bound(b,contextptr)) return false;
    gen width=hi-lo,periods=ratnormal(a*width/(2*cst_pi),contextptr);
    if (periods.type!=_INT_ && periods.type!=_ZINT) return false;
    res=c*width/(scale*sqrt(discriminant,contextptr));
    if (!is_strictly_positive(A,contextptr)) res=-res;
    res=ratnormal(res,contextptr);return true;
  }

  // t=x^q reduces two distinct positive binomials to a divided difference of
  // their Mellin transforms. The combined integrand converges for 0<s<2,
  // including s=1 where the sine quotient has the exact logarithmic limit.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_mellin_two_binomials(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if (!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf ||
        !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    if (v.size()<2 || v.size()>3) return false;
    gen m=0,c=1,q=0,a=0,b=0,scale=1;unsigned denominators=0;
    for (unsigned i=0;i<v.size();++i){
      gen base;
      if (integration_power(v[i],base,-1) && base.is_symb_of_sommet(at_plus) && base._SYMBptr->feuille.type==_VECT){
        const vecteur &terms=*base._SYMBptr->feuille._VECTptr;
        if (denominators==2 || terms.size()!=2) return false;
        unsigned j=integration_rational(terms[0])?0:1;
        gen A=terms[j],B,n;
        if (!integration_rational(A) || !is_strictly_positive(A,contextptr) ||
            !integration_monomial(terms[1-j],x,B,n,contextptr) ||
            !is_strictly_positive(B,contextptr) || !is_strictly_positive(n,contextptr)) return false;
        if (!denominators){q=n;a=B/A;}
        else {if (n!=q) return false;b=B/A;}
        scale=scale*A;++denominators;
      }
      else {
        gen C,n;
        if (!integration_monomial(v[i],x,C,n,contextptr)) return false;
        c=c*C;m+=n;
      }
    }
    if (denominators!=2 || a==b) return false;
    gen s=(m+1)/q;
    if (!is_strictly_positive(s,contextptr) || !is_strictly_positive(2-s,contextptr)) return false;
    if (is_one(s)) res=c*ln(a/b,contextptr)/(scale*q*(a-b));
    else res=c*cst_pi*(pow(a,1-s,contextptr)-pow(b,1-s,contextptr))/(scale*q*(a-b)*sin(cst_pi*s,contextptr));
    return true;
  }

  // exp(real affine)^r is an exponential of a real affine; its positive
  // base makes rational outer powers unambiguous. At most eight wrappers.
  static bool integration_exp_affine(const gen &g,const gen &x,gen &rate,gen &shift,GIAC_CONTEXT){
    gen base=g,power=1,a,b;
    if(!integration_outer_power(base,power) || !base.is_symb_of_sommet(at_exp) ||
       !is_linear_wrt(base._SYMBptr->feuille,x,a,b,contextptr) ||
       !is_zero(im(a,contextptr)) || contains(a,x) || taille(a,33)>32 || !integration_rational(b))return false;
    rate=-power*a;shift=power*b;return true;
  }

  // Frullani differences and their once-integrated form. Check cancellation
  // before forming logarithms; never integrate divergent summands separately.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_exp_difference(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=plus_inf || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &factors=*e._SYMBptr->feuille._VECTptr;
    if(factors.size()<2 || factors.size()>6)return false;
    gen numerator=1,envelope_rate=0,envelope_shift=0;gen frequency=0;bool oscillatory=false;bool envelope=false,have_numerator=false;unsigned inverse=0,logs=0;
    for(unsigned i=0;i<factors.size();++i){
      gen base,rate,shift;
      if(integration_power(factors[i],base,-1) && base==x){if(inverse)return false;inverse=1;}
      else if(integration_power(factors[i],base,-2) && base==x){if(inverse)return false;inverse=2;}
      else if(factors[i].is_symb_of_sommet(at_ln) && factors[i]._SYMBptr->feuille==x){if(++logs>2)return false;}
      else if(integration_power(factors[i],base,2) && base.is_symb_of_sommet(at_ln) && base._SYMBptr->feuille==x){logs+=2;if(logs>2)return false;}
      else if(factors[i].is_symb_of_sommet(at_cos)){
        if(oscillatory || !angle_radian(contextptr) || !is_linear_wrt(factors[i]._SYMBptr->feuille,x,frequency,shift,contextptr) ||
           !integration_resource_rational(frequency) || !is_zero(shift))return false;
        oscillatory=true;
      }
      else if(integration_exp_affine(factors[i],x,rate,shift,contextptr)){
        if(envelope)return false;envelope=true;envelope_rate=rate;envelope_shift=shift;
      }
      else {if(have_numerator)return false;have_numerator=true;numerator=factors[i];}
    }
    if(!inverse || !have_numerator || !numerator.is_symb_of_sommet(at_plus) || numerator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*numerator._SYMBptr->feuille._VECTptr;
    if(terms.size()<2 || terms.size()>8)return false;
    vecteur rates,coefficients;gen sum=0,first=0,common_shift=0;
    for(unsigned i=0;i<terms.size();++i){
      gen term=terms[i],c=ratnormal(integration_coefficient(term,x,contextptr),contextptr),rate,shift;
      if(!integration_rational(c) || !integration_exp_affine(term,x,rate,shift,contextptr))return false;
      rate+=envelope_rate;shift+=envelope_shift;
      if(!is_strictly_positive(rate,contextptr))return false;
      if(!i)common_shift=shift;else if(shift!=common_shift)return false;
      rates.push_back(rate);coefficients.push_back(c);sum+=c;first+=c*rate;
    }
    if(!is_zero(sum) || (inverse==2 && !is_zero(first)))return false;
    if(oscillatory){
      if(inverse!=1 || logs)return false;
      gen answer=0;
      for(unsigned i=0;i<rates.size();++i)answer-=coefficients[i]*ln(rates[i]*rates[i]+frequency*frequency,contextptr)/2;
      res=exp(common_shift,contextptr)*answer;return true;
    }
    // Derivatives at the removable Mellin pole: Gamma(s) for 1/x,
    // Gamma(s-1) for 1/x^2. The cancellation checks above justify all
    // logarithmic moments without integrating divergent summands.
    gen S1=0,S2=0,S3=0;
    for(unsigned i=0;i<rates.size();++i){
      gen L=ln(rates[i],contextptr),weight=coefficients[i]*(inverse==2?rates[i]:gen(1));
      S1+=weight*L;if(logs)S2+=weight*L*L;if(logs==2)S3+=weight*L*L*L;
    }
    gen gamma=cst_euler_gamma,answer=inverse==2?S1:-S1;
    if(logs==1)answer=inverse==2?(1-gamma)*S1-S2/2:S2/2+gamma*S1;
    if(logs==2)answer=inverse==2?S3/3+(gamma-1)*S2+(2-2*gamma+gamma*gamma+cst_pi*cst_pi/6)*S1:
      -S3/3-gamma*S2-(gamma*gamma+cst_pi*cst_pi/6)*S1;
    res=exp(common_shift,contextptr)*answer;return true;
  }

  static bool integration_chain_add(vecteur &powers,vecteur &coefficients,const gen &p,const gen &c){
    // Enforce the same bound after convolution and differentiation as at
    // leaves; inspecting canonical rational fields cannot expand an expression.
    const gen &numerator=p.type==_FRAC?p._FRACptr->num:p;
    if(numerator.type!=_INT_ || numerator.val < -64 || numerator.val>64)return false;
    if(p.type==_FRAC && (p._FRACptr->den.type!=_INT_ || p._FRACptr->den.val<1 || p._FRACptr->den.val>8))return false;
    if(is_zero(c))return true;
    for(unsigned i=0;i<powers.size();++i)if(powers[i]==p){coefficients[i]+=c;return true;}
    if(powers.size()>=32)return false;powers.push_back(p);coefficients.push_back(c);return true;
  }

  static bool integration_chain_terms(const gen &g,const gen &x,vecteur &powers,vecteur &coefficients,unsigned &budget,GIAC_CONTEXT){
    if(!budget)return false;--budget;
    if(integration_rational(g))return integration_chain_add(powers,coefficients,0,g);
    if(g==x)return integration_chain_add(powers,coefficients,1,1);
    if(g.is_symb_of_sommet(at_neg)){
      vecteur p,c;if(!integration_chain_terms(g._SYMBptr->feuille,x,p,c,budget,contextptr))return false;
      for(unsigned i=0;i<p.size();++i)if(!integration_chain_add(powers,coefficients,p[i],-c[i]))return false;return true;
    }
    gen base=g,power=1;
    if(integration_outer_power(base,power) && base==x && integration_rational(power)){
      return integration_chain_add(powers,coefficients,power,1);
    }
    if(g.type!=_SYMB || g._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if(g.is_symb_of_sommet(at_plus)){
      for(unsigned i=0;i<v.size();++i)if(!integration_chain_terms(v[i],x,powers,coefficients,budget,contextptr))return false;return true;
    }
    if(!g.is_symb_of_sommet(at_prod) || v.size()>8)return false;
    vecteur rp(1,0),rc(1,1);
    for(unsigned i=0;i<v.size();++i){
      vecteur p,c,np,nc;if(!integration_chain_terms(v[i],x,p,c,budget,contextptr) || p.size()*rp.size()>64)return false;
      for(unsigned j=0;j<p.size();++j)for(unsigned k=0;k<rp.size();++k)
        if(!integration_chain_add(np,nc,p[j]+rp[k],c[j]*rc[k]))return false;
      rp.swap(np);rc.swap(nc);
    }
    for(unsigned i=0;i<rp.size();++i)if(!integration_chain_add(powers,coefficients,rp[i],rc[i]))return false;return true;
  }

  static bool integration_chain_endpoint(const vecteur &powers,const vecteur &coefficients,const gen &bound,gen &value,GIAC_CONTEXT){
    if(bound!=plus_inf && (!integration_rational(bound) || is_strictly_positive(-bound,contextptr)))return false;
    if(!is_zero(bound) && bound!=plus_inf){
      gen u=0;for(unsigned i=0;i<powers.size();++i)u+=coefficients[i]*pow(bound,powers[i],contextptr);
      value=erf(u,contextptr);return true;
    }
    bool found=false;gen dominant=0,coefficient=0,constant=0;
    for(unsigned i=0;i<powers.size();++i){
      if(is_zero(coefficients[i]))continue;
      if(is_zero(powers[i]))constant+=coefficients[i];
      if(!found || (bound==plus_inf?is_strictly_greater(powers[i],dominant,contextptr):is_strictly_greater(dominant,powers[i],contextptr))){dominant=powers[i];coefficient=coefficients[i];found=true;}
    }
    if(found && (bound==plus_inf?is_strictly_positive(dominant,contextptr):is_strictly_positive(-dominant,contextptr)))value=is_strictly_positive(coefficient,contextptr)?1:-1;
    else value=erf(constant,contextptr);
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_erf_chain(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(taille(e,129)>128)return false;
    gen f=e,outside=integration_coefficient(f,x,contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    if((!is_zero(lo) && (!integration_rational(lo) || !is_strictly_positive(lo,contextptr))) ||
       (hi!=plus_inf && (!integration_rational(hi) || !is_strictly_greater(hi,lo,contextptr))))return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()>8)return false;
    gen u,exponent,weight=1;unsigned order=0;bool gaussian=false,found=false;
    for(unsigned i=0;i<v.size();++i){
      if(v[i].is_symb_of_sommet(at_exp)){if(gaussian)return false;gaussian=true;exponent=v[i]._SYMBptr->feuille;continue;}
      gen base=v[i],power=1;
      if(base.is_symb_of_sommet(at_pow) && base._SYMBptr->feuille.type==_VECT && base._SYMBptr->feuille._VECTptr->size()==2){power=base._SYMBptr->feuille[1];base=gen(base._SYMBptr->feuille[0]);}
      if(base.is_symb_of_sommet(at_erf)){
        if(found || power.type!=_INT_ || power.val<1 || power.val>16)return false;
        found=true;order=power.val;u=base._SYMBptr->feuille;
      }
      else weight=weight*v[i];
    }
    if(!gaussian || !found)return false;
    vecteur up,uc,wp,wc;unsigned budget=64;
    if(!integration_chain_terms(u,x,up,uc,budget,contextptr) || up.size()>4 || !integration_chain_terms(weight,x,wp,wc,budget,contextptr))return false;
    // Match exp(-u^2) structurally first, then allow its bounded expanded form.
    gen square=exponent;gen scale=integration_coefficient(square,x,contextptr),inside;
    bool matched=scale==-1 && integration_power(square,inside,2) && inside==u;
    if(!matched){
      vecteur ep,ec;if(!integration_chain_terms(exponent,x,ep,ec,budget,contextptr))return false;
      for(unsigned i=0;i<up.size();++i)for(unsigned j=0;j<up.size();++j)
        if(!integration_chain_add(ep,ec,up[i]+up[j],uc[i]*uc[j]))return false;
      for(unsigned i=0;i<ec.size();++i)if(!is_zero(ec[i]))return false;
    }
    vecteur dp,dc;
    for(unsigned i=0;i<up.size();++i)if(!integration_chain_add(dp,dc,up[i]-1,uc[i]*up[i]))return false;
    gen multiplier=0;bool ratio=false;
    for(unsigned i=0;i<dp.size();++i){
      if(is_zero(dc[i]))continue;gen w=0;
      for(unsigned j=0;j<wp.size();++j)if(wp[j]==dp[i])w+=wc[j];
      if(!ratio){multiplier=w/dc[i];ratio=true;}
      else if(w!=multiplier*dc[i])return false;
    }
    if(!ratio || is_zero(multiplier))return false;
    for(unsigned j=0;j<wp.size();++j){gen d=0;for(unsigned i=0;i<dp.size();++i)if(wp[j]==dp[i])d+=dc[i];if(wc[j]!=multiplier*d)return false;}
    gen a,b;if(!integration_chain_endpoint(up,uc,lo,a,contextptr) || !integration_chain_endpoint(up,uc,hi,b,contextptr))return false;
    res=outside*multiplier*sqrt(cst_pi,contextptr)*(pow(b,int(order+1),contextptr)-pow(a,int(order+1),contextptr))/gen(2*int(order+1));return true;
  }

  static bool integration_rectangle_term(gen e,const gen &x,gen &side,gen &radial,gen &common,gen &factor,GIAC_CONTEXT){
    gen coefficient=integration_coefficient(e,x,contextptr);
    if(!integration_rational(coefficient) || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()!=3)return false;
    gen a=0,b=0,ra=0,rb=0,ta=0,tb=0;bool root=false,den=false,trig=false;
    for(unsigned i=0;i<v.size();++i){
      gen base,radical;
      if(v[i].is_symb_of_sommet(at_atan)){
        if(trig)return false;trig=true;base=v[i]._SYMBptr->feuille;side=integration_coefficient(base,x,contextptr);
        if(!integration_rational(side) || !integration_power(base,radical,-1) || !integration_square_root(radical,base) || !integration_quadratic(base,x,ta,tb,contextptr))return false;
      }
      else {
        if(!integration_power(v[i],base,-1))return false;
        if(integration_square_root(base,radical)){
          if(root || !integration_quadratic(radical,x,ra,rb,contextptr))return false;root=true;
        }
        else {if(den || !integration_quadratic(base,x,a,b,contextptr))return false;den=true;}
      }
    }
    if(!root || !den || !trig || ta!=ra || tb!=rb || !is_strictly_positive(ra,contextptr) || !is_strictly_positive(rb,contextptr) || !is_strictly_positive(side,contextptr) || is_zero(a) || 2*b*ra!=a*rb)return false;
    radial=ra;common=rb/2;factor=coefficient*sqrt(ra,contextptr)/a;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_rectangle_pair(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || !integration_rational(hi) || !is_strictly_positive(hi,contextptr) || !e.is_symb_of_sommet(at_plus) || e._SYMBptr->feuille.type!=_VECT || e._SYMBptr->feuille._VECTptr->size()!=2)return false;
    gen s1,r1,c1,f1,s2,r2,c2,f2;
    if(!integration_rectangle_term(e._SYMBptr->feuille[0],x,s1,r1,c1,f1,contextptr) || !integration_rectangle_term(e._SYMBptr->feuille[1],x,s2,r2,c2,f2,contextptr) || c1!=c2 || s1*s1!=r2*hi*hi || s2*s2!=r1*hi*hi || !is_zero(ratnormal(f1-f2,contextptr)))return false;
    res=f1*atan(s1/sqrt(c1,contextptr),contextptr)*atan(s2/sqrt(c1,contextptr),contextptr)/c1;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_gaussian_erf_exp(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(lo!=minus_inf || hi!=plus_inf || !e.is_symb_of_sommet(at_exp) || taille(e,97)>96)return false;
    const gen &exponent=e._SYMBptr->feuille;
    const vecteur *terms=exponent.is_symb_of_sommet(at_plus) && exponent._SYMBptr->feuille.type==_VECT?exponent._SYMBptr->feuille._VECTptr:0;
    unsigned count=terms?terms->size():1;if(count>8)return false;
    gen a=0,b=0,c=0,remainder=0;bool found=false;
    for(unsigned i=0;i<count;++i){
      gen t=terms?(*terms)[i]:exponent;
      gen coefficient=integration_syntax(integration_coefficient(t,x,contextptr),contextptr);
      if(t.is_symb_of_sommet(at_erf)){
        if(found || !integration_rational(coefficient) || !is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a))return false;
        c=coefficient;found=true;
      }
      else remainder+=coefficient*t;
    }
    gen q,l,k;
    if(!integration_gaussian_quadratic(remainder,x,q,l,k,contextptr) || !is_strictly_positive(-q,contextptr))return false;
    if(!found){
      // The c=0 limit contains no erf after arithmetic normalization.
      res=exp(k-l*l/(4*q),contextptr)*sqrt(cst_pi/(-q),contextptr);return true;
    }
    if(q!=-a*a || l!=-2*a*b)return false;
    gen scale=exp(k+b*b,contextptr)*sqrt(cst_pi,contextptr)/abs(a,contextptr);
    res=is_zero(c)?scale:scale*sinh(c,contextptr)/c;return true;
  }

// Insert before integrate_compact_definite and dispatch before generic methods.
// t=tanh(u/2) maps [0,infinity) to [0,1):
// integral = 2 int_0^1 dt / (A+B+(B-A)*t^2).
// B>0 and A+B>0 guarantee ordinary real convergence without any poles.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_reciprocal_cosh(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    bool full=lo==minus_inf && hi==plus_inf,half=is_zero(lo) && hi==plus_inf;
    if((!full && !half) || !angle_radian(contextptr) || taille(e,65)>64)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr),den;
    if(!integration_rational(outside) || !integration_power(f,den,-1))return false;
    const vecteur *terms=den.is_symb_of_sommet(at_plus) && den._SYMBptr->feuille.type==_VECT?den._SYMBptr->feuille._VECTptr:0;
    unsigned count=terms?terms->size():1;if(count>4)return false;
    gen A=0,B=0,a=0,b=0;bool found=false;
    for(unsigned i=0;i<count;++i){
      gen t=integration_syntax(terms?(*terms)[i]:den,contextptr);
      if(integration_rational(t)){A+=t;continue;}
      gen coefficient=integration_syntax(integration_coefficient(t,x,contextptr),contextptr);
      if(found || !integration_rational(coefficient) || !t.is_symb_of_sommet(at_cosh) || !is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a))return false;
      B=coefficient;found=true;
    }
    if(!found || !is_strictly_positive(B,contextptr) || !is_strictly_positive(A+B,contextptr) || (half && !is_zero(b)))return false;
    gen value;
    if(A==B)value=inv(B,contextptr);
    else if(is_strictly_positive(A-B,contextptr)){
      gen root=sqrt(A*A-B*B,contextptr);
      value=ln((A+root)/B,contextptr)/root;
    }
    else value=2*atan(sqrt((B-A)/(B+A),contextptr),contextptr)/sqrt(B*B-A*A,contextptr);
    res=(full?2:1)*outside*value/abs(a,contextptr);return true;
  }

// Insert before integrate_compact_definite; dispatch before linear splitting.
// Odd Gaussian-atan moments use IBP and a two-coefficient rational recurrence.
// No repeated differentiation or expansion of erfc is needed; n<=8.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_gaussian_atan_moment(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    bool full=lo==minus_inf && hi==plus_inf,half=is_zero(lo) && hi==plus_inf;
    if((!full && !half) || !angle_radian(contextptr) || taille(e,97)>96)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()>5)return false;
    gen exponent,b=0,weight=outside;bool gaussian=false,angle=false;
    for(unsigned i=0;i<v.size();++i){
      gen base=v[i],power=1;
      if(integration_outer_power(base,power) && base.is_symb_of_sommet(at_exp)){
        if(gaussian)return false;gaussian=true;exponent=integration_syntax(power*base._SYMBptr->feuille,contextptr);
      }
      else if(v[i].is_symb_of_sommet(at_atan)){
        gen intercept;
        if(angle || !is_linear_wrt(v[i]._SYMBptr->feuille,x,b,intercept,contextptr) || !integration_rational(b) || is_zero(b) || !is_zero(intercept))return false;
        angle=true;
      }
      else weight=weight*v[i];
    }
    gen coefficient,degree,q,l,d;unsigned budget=32;
    if(!gaussian || !angle || !integration_mellin_monomial(weight,x,coefficient,degree,budget,contextptr) || degree.type!=_INT_ || degree.val<1 || degree.val>17 || degree.val%2!=1 || !integration_gaussian_quadratic(exponent,x,q,l,d,contextptr) || !is_zero(l) || !is_strictly_positive(-q,contextptr))return false;
    gen a=-q,bb=b*b,j0=1,jg=0,gaussian_moment=gen(1)/2;
    gen i0=b/(2*a),ig=0;unsigned n=(degree.val-1)/2;
    for(unsigned k=1;k<=n;++k){
      j0=-j0/bb;jg=(gaussian_moment-jg)/bb;
      i0=gen(int(k))*i0/a+b*j0/(2*a);
      ig=gen(int(k))*ig/a+b*jg/(2*a);
      gaussian_moment=gaussian_moment*gen(2*int(k)-1)/(2*a);
    }
    gen root=sqrt(a,contextptr),ab=abs(b,contextptr);
    gen kernel=cst_pi*exp(a/bb,contextptr)*erfc(root/ab,contextptr)/(2*ab);
    res=(full?2:1)*coefficient*exp(d,contextptr)*(i0*kernel+ig*sqrt(cst_pi,contextptr)/root);return true;
  }

// x^m log(1+s exp(-a*x^q)), s=+/-1, maps to a Mellin log moment.
// N=(m+1)/q is restricted to integers1..12: exact finite factorial/zeta form.
// a,q>0 and N>0 prove ordinary real convergence, also for s=-1 at zero.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_exponential_log_moment(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=plus_inf || taille(e,97)>96)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside))return false;
    const vecteur *factors=f.is_symb_of_sommet(at_prod) && f._SYMBptr->feuille.type==_VECT?f._SYMBptr->feuille._VECTptr:0;
    unsigned count=factors?factors->size():1;if(count>5)return false;
    gen argument,weight=outside;bool found=false;
    for(unsigned i=0;i<count;++i){const gen &t=factors?(*factors)[i]:f;
      if(t.is_symb_of_sommet(at_ln)){if(found)return false;found=true;argument=t._SYMBptr->feuille;}
      else weight=weight*t;
    }
    gen other;
    if(!found || !integration_one_plus(argument,other))return false;
    gen sign=integration_syntax(integration_coefficient(other,x,contextptr),contextptr),power=1;
    if((sign!=1 && sign!=-1) || !integration_outer_power(other,power) || !other.is_symb_of_sommet(at_exp))return false;
    gen rate,q,coefficient,m;unsigned budget=32;
    if(!integration_mellin_monomial(other._SYMBptr->feuille,x,rate,q,budget,contextptr) || !integration_mellin_monomial(weight,x,coefficient,m,budget,contextptr))return false;
    gen a=-rate*power;
    if(!is_strictly_positive(a,contextptr) || !is_strictly_positive(q,contextptr) || is_strictly_greater(q,16,contextptr))return false;
    gen N=(m+1)/q;if(N.type!=_INT_ || N.val<1 || N.val>12)return false;
    gen factor=sign==1?1-inv(pow(gen(2),N.val),contextptr):gen(-1);
    res=coefficient*factorial(N.val-1)*factor*_Zeta(N.val+1,contextptr)/(q*pow(a,N.val,contextptr));return true;
  }

  // H(n*pi/4)=integral_0^(n*pi/4) log|sin(u)| du
  // =-n*pi*log(2)/4-G*sigma(n)/2, sigma=(0,1,0,-1).
  static int integration_quarter_sigma(int n){n%=4;if(n<0)n+=4;return n==1?1:(n==3?-1:0);}

  // Exact zeroth/first Fourier moments on a quarter of a positive sine wave.
  // An absolute value permits other half-waves, with the affine weight shifted.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_trig_quarters(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || taille(e,129)>128)return false;
    gen f=e,outside=integration_coefficient(f,x,contextptr),weight=1;
    if(!integration_rational(outside))return false;
    if(f.is_symb_of_sommet(at_prod) && f._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
      unsigned j=v[0].is_symb_of_sommet(at_ln)?0:1;weight=v[1-j];f=gen(v[j]);
    }
    if(!f.is_symb_of_sommet(at_ln))return false;
    gen t=f._SYMBptr->feuille;bool absolute=t.is_symb_of_sommet(at_abs);
    if(absolute)t=gen(t._SYMBptr->feuille);
    bool cosine=t.is_symb_of_sommet(at_cos);
    if(!cosine && !t.is_symb_of_sommet(at_sin))return false;
    gen a,b,A,B;
    if(!is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a) ||
       !is_linear_wrt(weight,x,A,B,contextptr) || !integration_rational(A) || !integration_rational(B))return false;
    if(cosine)b+=cst_pi/2;
    gen l=ratnormal((a*lo+b)/cst_pi,contextptr),r=ratnormal((a*hi+b)/cst_pi,contextptr);
    bool reversed=false;if(is_strictly_greater(l,r,contextptr)){swapgen(l,r);reversed=true;}
    if(is_zero(A)){
      gen ql=4*l,qr=4*r;
      if(ql.type!=_INT_ || qr.type!=_INT_ || ql.val < -128 || qr.val>128)return false;
      int wave=ql.val>=0?ql.val/4:(ql.val-3)/4;
      if(!absolute && (wave%2 || qr.val>4*(wave+1)))return false;
      int delta=integration_quarter_sigma(qr.val)-integration_quarter_sigma(ql.val);
      gen value=-(r-l)*cst_pi*ln(gen(2),contextptr);
      if(delta){
        gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
        value-=delta*G/2;
      }
      res=outside*B*value/a;if(reversed)res=-res;return true;
    }
    gen twice=2*l;
    if(twice.type!=_INT_ || twice.val < -64 || twice.val>64)return false;
    int wave=twice.val>=0?twice.val/2:(twice.val-1)/2;
    if(!absolute && wave%2)return false;
    l-=wave;r-=wave;
    if(!((l==0 && r==gen(1)/2) || (l==gen(1)/2 && r==1) || (l==0 && r==1)))return false;
    gen L=ln(gen(2),contextptr),m0=-(r-l)*cst_pi*L,m1;
    if(is_zero(A)){res=outside*B*m0/a;if(reversed)res=-res;return true;}
    if(l==0 && r==1)m1=-cst_pi*cst_pi*L/2;
    else {
      gen first=-cst_pi*cst_pi*L/8+gen(7)*_Zeta(3,contextptr)/16;
      m1=l==0?first:-cst_pi*cst_pi*L/2-first;
    }
    res=outside*(A*m1/a+(B+A*(gen(wave)*cst_pi-b)/a)*m0)/a;
    if(reversed)res=-res;
    return true;
  }

  // The three log products follow from a zeta series and polarization.
  // u=b*x^q, dx/x=du/(q*u); the endpoint condition keeps log(1-u) real.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_product_zeta(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || !integration_rational(hi) || !is_strictly_positive(hi,contextptr) || taille(e,129)>128)return false;
    gen f=e,outside=integration_coefficient(f,x,contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()<2 || v.size()>3)return false;
    bool inverse=false;unsigned logs=0,negative=0;gen b=0,q=0;
    for(unsigned i=0;i<v.size();++i){
      gen t=v[i],base;
      if(integration_power(t,base,-1) && base==x){if(inverse)return false;inverse=true;continue;}
      unsigned count=1;
      if(integration_power(t,base,2)){t=base;count=2;}
      if(!t.is_symb_of_sommet(at_ln))return false;
      gen other,c,n;
      if(!integration_one_plus(t._SYMBptr->feuille,other) || !integration_monomial(other,x,c,n,contextptr) ||
         !integration_rational(c) || !integration_rational(n) || is_zero(c) || !is_strictly_positive(n,contextptr) || is_strictly_greater(n,32,contextptr))return false;
      bool minus=is_strictly_positive(-c,contextptr);if(minus){negative+=count;c=-c;}
      if(!logs){b=c;q=n;}else if(c!=b || n!=q)return false;
      logs+=count;
    }
    if(!inverse || logs!=2 || !is_zero(ratnormal(b*pow(hi,q,contextptr)-1,contextptr)))return false;
    gen factor=negative==2?gen(2):(negative==1?-gen(5)/8:gen(1)/4);
    res=outside*factor*_Zeta(3,contextptr)/q;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_trig_sum(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || taille(e,129)>128)return false;
    gen f=e,outside=integration_coefficient(f,x,contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_ln))return false;
    gen arg=f._SYMBptr->feuille,scale=integration_coefficient(arg,x,contextptr);
    if(!integration_rational(scale) || !is_strictly_positive(scale,contextptr) || !arg.is_symb_of_sommet(at_plus) || arg._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*arg._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    gen A=0,B=0,a=0,b=0;bool found=false,cosine=false;gen square_arg;bool squared=false;
    for(unsigned i=0;i<v.size();++i){
      if(!contains(v[i],x) && is_zero(im(v[i],contextptr))){A+=v[i];continue;}
      gen t=v[i],coefficient=integration_coefficient(t,x,contextptr);
      gen base;bool square=integration_power(t,base,2);if(square)t=base;
      if(!is_zero(im(coefficient,contextptr)) || !(t.is_symb_of_sommet(at_sin) || t.is_symb_of_sommet(at_cos)))return false;
      if(!is_linear_wrt(t._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a))return false;
      if(square){
        if(found && (!squared || t._SYMBptr->feuille!=square_arg))return false;
        square_arg=t._SYMBptr->feuille;squared=true;
        A+=coefficient/2;B+=(t.is_symb_of_sommet(at_cos)?coefficient:-coefficient)/2;
        a=2*a;b=2*b;cosine=true;
      }
      else {if(found)return false;B=coefficient;cosine=t.is_symb_of_sommet(at_cos);}
      found=true;
    }
    if(!found || !is_strictly_positive(A,contextptr))return false;
    if(is_strictly_positive(A-abs(B,contextptr),contextptr)){
      gen l=ratnormal((a*lo+b)/cst_pi,contextptr),r=ratnormal((a*hi+b)/cst_pi,contextptr);
      gen periods=ratnormal((r-l)/2,contextptr);
      bool whole=periods.type==_INT_ && periods.val>=-64 && periods.val<=64;
      bool half=cosine && l.type==_INT_ && r.type==_INT_ && l.val>=-128 && l.val<=128 && r.val>=-128 && r.val<=128;
      if(!whole && !half)return false;
      gen mean=squared?2*ln(sqrt(scale,contextptr)*(sqrt(A+B,contextptr)+sqrt(A-B,contextptr))/2,contextptr):
        ln(scale*(A+sqrt(A*A-B*B,contextptr))/2,contextptr);
      res=outside*(hi-lo)*mean;return true;
    }
    if(B!=A && B!=-A)return false;
    gen l=ratnormal(2*(a*lo+b)/cst_pi,contextptr),r=ratnormal(2*(a*hi+b)/cst_pi,contextptr);
    if(l.type!=_INT_ || r.type!=_INT_ || l.val < -32 || l.val>32 || r.val < -32 || r.val>32 || l==r)return false;
    int offset=cosine?(B==A?2:0):(B==A?1:3);
    int delta=integration_quarter_sigma(r.val+offset)-integration_quarter_sigma(l.val+offset);
    gen result=(hi-lo)*ln(scale*A/2,contextptr);
    if(delta){
      gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
      result-=gen(2*delta)*G/a;
    }
    res=outside*result;return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_log_measure(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !integration_rational(lo) || !integration_rational(hi) || lo==hi || taille(e,129)>128)return false;
    gen f=e,outside=integration_coefficient(f,x,contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()<2 || v.size()>3)return false;
    gen u,den,weight=1;bool found=false,inverse=false;
    for(unsigned i=0;i<v.size();++i){
      gen base;
      if(v[i].is_symb_of_sommet(at_atan)){if(found)return false;found=true;u=v[i]._SYMBptr->feuille;}
      else if(integration_power(v[i],base,-1) && base.is_symb_of_sommet(at_plus)){
        if(inverse)return false;inverse=true;den=base;
      }
      else weight=weight*v[i];
    }
    if(!found || !inverse)return false;
    gen c,q,a,b,multiplier,dl,dh,A=0,B=0;
    if(integration_monomial(u,x,c,q,contextptr) && is_strictly_positive(c,contextptr) && is_strictly_positive(q,contextptr)){
      if(is_strictly_greater(q,32,contextptr) || is_strictly_positive(-lo,contextptr) || is_strictly_positive(-hi,contextptr))return false;
      gen wc,wn;if(!integration_monomial(weight,x,wc,wn,contextptr) || wn!=q-1)return false;
      if(den._SYMBptr->feuille.type!=_VECT || den._SYMBptr->feuille._VECTptr->size()!=2)return false;
      const vecteur &d=*den._SYMBptr->feuille._VECTptr;
      for(unsigned i=0;i<2;++i){gen dc,dn;if(!integration_monomial(d[i],x,dc,dn,contextptr))return false;
        if(is_zero(dn))A+=dc;else if(dn==q)B+=dc;else return false;
      }
      if(is_zero(A) || B!=A*c)return false;
      dl=c*pow(lo,q,contextptr);dh=c*pow(hi,q,contextptr);multiplier=wc/(c*q*A);
    }
    else {
      gen wa,wb;
      if(!is_linear_wrt(u,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a) ||
         !is_linear_wrt(weight,x,wa,wb,contextptr) || !is_zero(wa) || !integration_rational(wb) ||
         !is_linear_wrt(den,x,A,B,contextptr) || !integration_rational(A) || !integration_rational(B) || is_zero(A) || B*a!=A*(1+b))return false;
      dl=a*lo+b;dh=a*hi+b;multiplier=wb/A;
    }
    dl=ratnormal(dl,contextptr);dh=ratnormal(dh,contextptr);
    if(!((is_zero(dl) && dh==1) || (dl==1 && is_zero(dh))))return false;
    res=outside*multiplier*cst_pi*ln(gen(2),contextptr)/8;
    if(dh==0)res=-res;
    return true;
  }

  // F(s)=integral_0^infinity u^(s-1)/((1+u)*(1+u^2))du.
  // Partial fractions followed by analytic continuation gives this real
  // expression on 0<s<3. Resolve the removable poles at s=1,2 first.
  static gen integration_mixed_mellin_value(const gen &s,unsigned logs,const gen &L,GIAC_CONTEXT){
    gen pi=cst_pi,F,first,second;
    if(s==1 || s==2){
      F=pi/4;
      if(logs)first=(s==1?-1:1)*pi*pi/16;
      if(logs==2)second=pow(pi,3)/16;
    }
    else {
      // Both sine factors are positive on 0<s<3. Reuse their values
      // for the moment and both derivatives, without cancellation poles.
      gen u=pi*s/4,v=u+pi/4,A=inv(sin(u,contextptr),contextptr),B=inv(sin(v,contextptr),contextptr);
      F=pi*A*B/(4*sqrt(gen(2),contextptr));
      if(logs){
        gen D=cos(u,contextptr)*A+cos(v,contextptr)*B;
        first=-pi*F*D/4;
        if(logs==2)second=pi*pi*F*(D*D+A*A+B*B)/16;
      }
    }
    if(!logs)return F;
    if(logs==1)return first-L*F;
    return second-2*L*first+L*L*F;
  }
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_mixed_mellin_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf || taille(e,129)>128)return false;
    vecteur factors;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)factors=*e._SYMBptr->feuille._VECTptr;
    else factors.push_back(e);
    if(factors.size()>8)return false;
    gen coefficient=1,power=0,a[2],k[2];unsigned count=0,logs=0,budget=64;
    for(unsigned i=0;i<factors.size();++i){
      gen t=factors[i],base,other;
      unsigned n=1;
      if(integration_power(t,base,2)){t=base;n=2;}
      if(t.is_symb_of_sommet(at_ln) && t._SYMBptr->feuille==x){logs+=n;if(logs>2)return false;continue;}
      if(integration_power(factors[i],base,-1) && integration_one_plus(base,other)){
        if(count>=2 || !integration_mellin_monomial(other,x,a[count],k[count],budget,contextptr) ||
           !integration_rational(a[count]) || !integration_rational(k[count]) ||
           !is_strictly_positive(a[count],contextptr) || !is_strictly_positive(k[count],contextptr))return false;
        ++count;continue;
      }
      gen c,p;if(!integration_mellin_monomial(factors[i],x,c,p,budget,contextptr) || !integration_rational(c))return false;
      coefficient=coefficient*c;power+=p;
    }
    if(count!=2)return false;
    if(k[0]==2*k[1]){swapgen(k[0],k[1]);swapgen(a[0],a[1]);}
    if(k[1]!=2*k[0] || a[1]!=a[0]*a[0])return false;
    gen s=(power+1)/k[0];
    if(!integration_rational(s) || !is_strictly_positive(s,contextptr) || !is_strictly_positive(3-s,contextptr))return false;
    // Limit exact trigonometric constants; do not construct high-degree
    // algebraic extensions for arbitrary rational angles on the small stack.
    if(s.type==_FRAC && (s._FRACptr->den.type!=_INT_ || s._FRACptr->den.val>12))return false;
    gen L=logs && !is_one(a[0])?ln(a[0],contextptr):gen(0);
    gen value=integration_mixed_mellin_value(s,logs,L,contextptr);
    res=coefficient*pow(a[0],-s,contextptr)*value/pow(k[0],int(logs+1));return true;
  }
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_oscillatory_cancellation(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || taille(e,129)>128)return false;
    int halves=(lo==minus_inf && hi==plus_inf)?2:
      ((is_zero(lo) && hi==plus_inf)||(lo==minus_inf && is_zero(hi)))?1:0;
    if(!halves)return false;
    vecteur factors;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)factors=*e._SYMBptr->feuille._VECTptr;
    else return false;
    if(factors.size()>6)return false;
    gen coefficient=1,power=0,numerator;bool have_numerator=false;
    for(unsigned i=0;i<factors.size();++i){
      if(factors[i].is_symb_of_sommet(at_plus)){
        if(have_numerator)return false;numerator=factors[i];have_numerator=true;continue;
      }
      gen c,p;if(!integration_monomial(factors[i],x,c,p,contextptr) || !integration_rational(c))return false;
      coefficient=coefficient*c;power+=p;
    }
    if(!have_numerator || (power!=-2 && power!=-3) || numerator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*numerator._SYMBptr->feuille._VECTptr;
    if(terms.size()<2 || terms.size()>16)return false;
    gen cancellation=0,value=0;
    for(unsigned i=0;i<terms.size();++i){
      gen term=terms[i],c=integration_coefficient(term,x,contextptr),p=0,frequency=0;
      c=integration_syntax(c,contextptr);
      if(!integration_rational(c))return false;
      vecteur atoms;
      if(term.is_symb_of_sommet(at_prod) && term._SYMBptr->feuille.type==_VECT)atoms=*term._SYMBptr->feuille._VECTptr;
      else atoms.push_back(term);
      if(atoms.size()>4)return false;
      bool sine=false,have_trig=false;
      for(unsigned j=0;j<atoms.size();++j){
        const gen &atom=atoms[j];
        if(atom.is_symb_of_sommet(at_sin) || atom.is_symb_of_sommet(at_cos)){
          gen shift;
          if(have_trig || !is_linear_wrt(atom._SYMBptr->feuille,x,frequency,shift,contextptr) ||
             !integration_rational(frequency) || !is_zero(shift))return false;
          sine=atom.is_symb_of_sommet(at_sin);have_trig=true;continue;
        }
        gen d,q;if(!integration_monomial(atom,x,d,q,contextptr) || !integration_rational(d))return false;
        c=c*d;p+=q;
      }
      gen magnitude=abs(frequency,contextptr);
      if(power==-2){
        if(sine || !is_zero(p))return false;
        cancellation+=c;value-=c*magnitude/2;
      }
      else {
        if(sine){if(!is_zero(p))return false;cancellation+=c*frequency;value-=c*frequency*magnitude/4;}
        else {if(p!=1)return false;cancellation+=c;value-=c*magnitude/2;}
      }
    }
    if(!is_zero(cancellation))return false;
    res=halves*coefficient*cst_pi*value;return true;
  }

  // Integration by parts of two complementary Gaussian tails. With
  // u=x^q the supported weight is exactly x^(q-1); no polynomial expansion.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_erf_tail_product(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=plus_inf)return false;
    vecteur single;const vecteur *v;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)v=e._SYMBptr->feuille._VECTptr;
    else {single.push_back(e);v=&single;}
    if(v->size()>6)return false;
    gen slope[2],degree[2],weight=1,m=0;bool complement[2];unsigned count=0,budget=48;
    for(unsigned i=0;i<v->size();++i){
      const gen &term=(*v)[i];gen base=term;unsigned repeat=1;
      if(term.is_symb_of_sommet(at_pow) && term._SYMBptr->feuille.type==_VECT &&
         term._SYMBptr->feuille._VECTptr->size()==2 && term._SYMBptr->feuille[1]==2){base=term._SYMBptr->feuille[0];repeat=2;}
      // Evaluating erfc(u) may print it as 1-erf(u). Recognize this
      // exact two-term identity without expanding a product of tails.
      gen other;
      if(integration_one_plus(base,other)){
        gen c=integration_coefficient(other,x,contextptr);
        if(c==-1 && other.is_symb_of_sommet(at_erf))base=symbolic(at_erfc,other._SYMBptr->feuille);
      }
      if(base.is_symb_of_sommet(at_erf) || base.is_symb_of_sommet(at_erfc)){
        if(count+repeat>2)return false;
        gen a,q;
        if(!integration_mellin_monomial(base._SYMBptr->feuille,x,a,q,budget,contextptr) ||
           !integration_rational(a) || is_zero(a) || !integration_rational(q) || !is_strictly_positive(q,contextptr))return false;
        while(repeat--){slope[count]=a;degree[count]=q;complement[count]=base.is_symb_of_sommet(at_erfc);++count;}
      }
      else {gen a,q;if(!integration_mellin_monomial(term,x,a,q,budget,contextptr) || !integration_rational(a))return false;weight=weight*a;m+=q;}
    }
    if(count!=2 || degree[0]!=degree[1] || m+1!=degree[0] || (!complement[0] && !complement[1]))return false;
    if(!complement[1]){swapgen(slope[0],slope[1]);bool t=complement[0];complement[0]=complement[1];complement[1]=t;}
    gen a=slope[0],b=slope[1];
    if(!is_strictly_positive(b,contextptr) || (complement[0] && !is_strictly_positive(a,contextptr)))return false;
    gen h=sqrt(a*a+b*b,contextptr);
    res=weight*(complement[0]?a+b-h:h-b)/(degree[0]*a*b*sqrt(cst_pi,contextptr));return true;
  }

  // J_n = int_1^infinity log(t)^n/(1+t^2) dt = n!*beta(n+1).
  // Even orders follow the finite Euler-number recurrence; odd orders keep
  // a compact, exact quarter-polygamma difference instead of expanding it.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static gen integration_cauchy_log_moment(unsigned n,GIAC_CONTEXT){
    if(n%2)return (gen(symbolic(at_Psi,makesequence(gen(1)/4,int(n))))-
                     gen(symbolic(at_Psi,makesequence(gen(3)/4,int(n)))))/pow(gen(4),int(n+1));
    int E[9]={1,0,0,0,0,0,0,0,0};
    for(unsigned j=2;j<=n;j+=2){
      int choose=1;
      for(unsigned k=0;k<j;++k){if(!(k%2))E[j]-=choose*E[k];choose=choose*int(j-k)/int(k+1);}
    }
    return gen(E[n]<0?-E[n]:E[n])*pow(cst_pi,int(n+1))/pow(gen(2),int(n+2));
  }

  static bool integration_log_affine_ratio(gen f,const gen &x,gen &scale,gen &n1,gen &n0,gen &d1,gen &d0,GIAC_CONTEXT){
    scale=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(scale) || is_zero(scale) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    gen numerator,denominator;bool found=false;
    for(unsigned i=0;i<2;++i){gen base;
      if(integration_power(v[i],base,-1)){if(found)return false;denominator=base;found=true;}
      else numerator=v[i];
    }
    return found && is_linear_wrt(numerator,x,n1,n0,contextptr) && is_linear_wrt(denominator,x,d1,d0,contextptr) &&
      integration_rational(n1) && integration_rational(n0) && integration_rational(d1) && integration_rational(d0);
  }

  // Map the quadratic denominator to A*w^2*(1+u^2), u=(x-h)/w.
  // The logarithm's zero/pole must be h-w,h+w, in either order. This proves
  // a positive real argument throughout the accepted half/full interval.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_mobius_cauchy_log(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !integration_rational(lo) || !integration_rational(hi) || lo==hi || taille(e,129)>128)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    gen logarg,den;bool found=false,inverse=false;unsigned order=0;
    for(unsigned i=0;i<2;++i){
      gen t=v[i],power=1,base;
      if(integration_power(t,base,-1)){if(inverse)return false;inverse=true;den=base;continue;}
      if(t.is_symb_of_sommet(at_pow) && t._SYMBptr->feuille.type==_VECT && t._SYMBptr->feuille._VECTptr->size()==2){power=t._SYMBptr->feuille[1];t=gen(t._SYMBptr->feuille[0]);}
      if(found || !t.is_symb_of_sommet(at_ln) || power.type!=_INT_ || power.val<1 || power.val>8)return false;
      found=true;order=power.val;logarg=t._SYMBptr->feuille;
    }
    if(!found || !inverse)return false;
    gen A,B,C;
    if(!integration_gaussian_quadratic(den,x,A,B,C,contextptr) || !integration_rational(A) || !integration_rational(B) || !integration_rational(C) || is_zero(A))return false;
    gen center=-B/(2*A),width2=C/A-center*center;
    if(!is_strictly_positive(width2,contextptr))return false;
    gen width=sqrt(width2,contextptr);if(!integration_rational(width))return false;
    gen l=ratnormal((lo-center)/width,contextptr),r=ratnormal((hi-center)/width,contextptr);
    bool reverse=false;if(is_strictly_greater(l,r,contextptr)){swapgen(l,r);reverse=true;}
    if(!((l==-1 && (r==0 || r==1)) || (l==0 && r==1)))return false;
    gen scale,n1,n0,d1,d0;
    if(!integration_log_affine_ratio(logarg,x,scale,n1,n0,d1,d0,contextptr))return false;
    gen nc=n1*center+n0,dc=d1*center+d0;
    if(is_zero(nc) || is_zero(dc))return false;
    gen sign=n1*width/nc;
    if((sign!=1 && sign!=-1) || d1*width/dc!=-sign)return false;
    gen k=scale*nc/dc;if(!is_strictly_positive(k,contextptr))return false;
    gen shift=ln(k,contextptr),answer=0;int choose=1;
    for(unsigned j=0;j<=order;++j){
      int parity=(j%2?-1:1),side=l==0?1:(r==0?parity:1+parity);
      if(side && (!is_zero(shift) || j==order)){
        gen term=gen(choose*side)*integration_cauchy_log_moment(j,contextptr);
        if(sign==-1 && j%2)term=-term;
        if(j!=order)term=term*pow(shift,int(order-j),contextptr);
        answer+=term;
      }
      choose=choose*int(order-j)/int(j+1);
    }
    res=outside*answer/(A*width);if(reverse)res=-res;return true;
  }

  // Equal-amplitude real sine/cosine sums are a shifted sine. The phase is
  // one of +/-pi/4,+/-3pi/4, so the existing quarter-period Fourier moments
  // suffice. Without abs, check a complete positive half-wave before taking
  // a logarithm; squaring alone would incorrectly accept negative sums.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_log_sine_cosine_sum(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || taille(e,129)>128)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_ln))return false;
    gen arg=f._SYMBptr->feuille;bool absolute=arg.is_symb_of_sommet(at_abs);
    if(absolute)arg=gen(arg._SYMBptr->feuille);
    gen scale=integration_syntax(integration_coefficient(arg,x,contextptr),contextptr);
    if(!integration_rational(scale) || is_zero(scale) || !arg.is_symb_of_sommet(at_plus) || arg._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*arg._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    gen A=0,B=0,angle;bool sine=false,cosine=false;
    for(unsigned i=0;i<2;++i){
      gen t=v[i],c=integration_syntax(integration_coefficient(t,x,contextptr),contextptr);
      if(!integration_rational(c))return false;c=c*scale;
      if(t.is_symb_of_sommet(at_sin)){if(sine)return false;sine=true;A=c;}
      else if(t.is_symb_of_sommet(at_cos)){if(cosine)return false;cosine=true;B=c;}
      else return false;
      if(i && t._SYMBptr->feuille!=angle)return false;
      angle=t._SYMBptr->feuille;
    }
    if(!sine || !cosine || is_zero(A) || (A!=B && A!=-B))return false;
    gen a,b;
    if(!is_linear_wrt(angle,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) ||
       (!integration_rational(b) && !integration_rational(ratnormal(b/cst_pi,contextptr))))return false;
    int phase=is_strictly_positive(A,contextptr)?(is_strictly_positive(B,contextptr)?1:-1):(is_strictly_positive(B,contextptr)?3:-3);
    gen l=ratnormal(4*(a*lo+b)/cst_pi,contextptr),r=ratnormal(4*(a*hi+b)/cst_pi,contextptr);
    if(l.type!=_INT_ || r.type!=_INT_ || l.val < -64 || l.val>64 || r.val < -64 || r.val>64 || l==r)return false;
    int nlo=l.val+phase,nhi=r.val+phase;
    if(!absolute){
      int lower=nlo<nhi?nlo:nhi,upper=nlo<nhi?nhi:nlo;
      int wave=lower>=0?lower/4:(lower-3)/4;
      if(wave%2 || upper>4*(wave+1))return false;
    }
    int delta=integration_quarter_sigma(nhi)-integration_quarter_sigma(nlo);
    gen coefficient=is_strictly_positive(A,contextptr)?A:-A;
    gen result=(hi-lo)*(ln(coefficient,contextptr)-ln(gen(2),contextptr)/2);
    if(delta){
      gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
      result-=gen(delta)*G/(2*a);
    }
    res=outside*result;return true;
  }

  // Mellin substitution u=a*x^q. Logarithmic moments are derivatives of
  // Gamma(s), using the same bounded cumulant recurrence as Beta moments.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_gamma_log_moment(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf)return false;
    gen input=e,outside=integration_syntax(integration_coefficient(input,x,contextptr),contextptr);
    if(!integration_rational(outside))return false;
    vecteur singleton;const vecteur *v;
    if(input.is_symb_of_sommet(at_prod) && input._SYMBptr->feuille.type==_VECT)v=input._SYMBptr->feuille._VECTptr;
    else {singleton.push_back(input);v=&singleton;}
    if(v->size()>8)return false;
    gen rate=0,q=0,coefficient=outside,m=0,logscale=1,logpower=1;
    bool exponential=false;unsigned logs=0,budget=64;
    for(unsigned i=0;i<v->size();++i){
      const gen &term=(*v)[i];gen base=term,power=1;
      if(!integration_outer_power(base,power))return false;
      if(base.is_symb_of_sommet(at_exp)){
        if(exponential || !integration_rational(power) || is_strictly_greater(abs(power,contextptr),16,contextptr))return false;
        gen a;
        if(!integration_mellin_monomial(base._SYMBptr->feuille,x,a,q,budget,contextptr) || !integration_rational(a))return false;
        rate=-a*power;exponential=true;continue;
      }
      gen logbase=base;
      bool negative_log=base.is_symb_of_sommet(at_neg) && base._SYMBptr->feuille.is_symb_of_sommet(at_ln);
      if(negative_log)base=gen(base._SYMBptr->feuille);
      if(base.is_symb_of_sommet(at_ln)){
        if(logs || power.type!=_INT_ || power.val<1 || power.val>4 ||
           (term!=logbase && (!term.is_symb_of_sommet(at_pow) || term._SYMBptr->feuille[0]!=logbase || term._SYMBptr->feuille[1].type!=_INT_)))return false;
        if(!integration_mellin_monomial(base._SYMBptr->feuille,x,logscale,logpower,budget,contextptr) ||
           !integration_rational(logscale) || !integration_rational(logpower) ||
           !is_strictly_positive(logscale,contextptr) || is_zero(logpower))return false;
        logs=power.val;if(negative_log && logs%2)coefficient=-coefficient;continue;
      }
      gen a,p;if(!integration_mellin_monomial(term,x,a,p,budget,contextptr) || !integration_rational(a))return false;
      coefficient=coefficient*a;m+=p;
    }
    if(!exponential || !integration_rational(q) || !is_strictly_positive(q,contextptr) || !is_strictly_positive(rate,contextptr))return false;
    gen s=(m+1)/q;
    if(!integration_rational(s) || !is_strictly_positive(s,contextptr) || is_strictly_greater(s,16,contextptr) ||
       (s.type==_FRAC && (s._FRACptr->den.type!=_INT_ || s._FRACptr->den.val>16)))return false;
    gen value=coefficient*Gamma(s,contextptr)/(q*pow(rate,s,contextptr));
    if(!logs){res=value;return true;}
    gen ratio=logpower/q,scale=1;vecteur cumulant(logs+1);
    for(unsigned k=1;k<=logs;++k){
      scale=scale*ratio;cumulant[k]=scale*integration_beta_psi(s,k-1,contextptr);
      if(k==1)cumulant[k]+=ln(logscale,contextptr)-ratio*ln(rate,contextptr);
    }
    bool compact=false;res=value*integration_cumulant_moment(cumulant,logs,compact,contextptr);
    if(!compact)res=ratnormal(res,contextptr);return true;
  }

  // The coordinate u maps the accepted real interval onto [0,1], [1,inf)
  // or [0,inf). Its measure is du/(1+u^2), and the logarithm becomes
  // shift+power*ln(u). No logarithm is expanded outside this positive domain.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_log_cauchy(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || taille(e,129)>128)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()<3 || v.size()>5)return false;
    gen u,logarg,den,weight=1;bool angle=false,logarithm=false,inverse=false;
    for(unsigned i=0;i<v.size();++i){gen base;
      if(v[i].is_symb_of_sommet(at_atan)){if(angle)return false;angle=true;u=v[i]._SYMBptr->feuille;}
      else if(v[i].is_symb_of_sommet(at_ln)){if(logarithm)return false;logarithm=true;logarg=v[i]._SYMBptr->feuille;}
      else if(integration_power(v[i],base,-1) && base.is_symb_of_sommet(at_plus)){if(inverse)return false;inverse=true;den=base;}
      else weight=weight*v[i];
    }
    if(!angle || !logarithm || !inverse)return false;
    gen c,q,a,b,dl,dh,multiplier,shift,power;
    if(integration_monomial(u,x,c,q,contextptr) && is_strictly_positive(c,contextptr) && is_strictly_positive(q,contextptr)){
      if(is_strictly_greater(q,32,contextptr) || (!integration_rational(lo) && lo!=plus_inf) || (!integration_rational(hi) && hi!=plus_inf) || is_strictly_positive(-lo,contextptr) || is_strictly_positive(-hi,contextptr))return false;
      gen wc,wn,lc,ln;
      if(!integration_monomial(weight,x,wc,wn,contextptr) || wn!=q-1 || !integration_monomial(logarg,x,lc,ln,contextptr) || !is_strictly_positive(lc,contextptr))return false;
      if(den._SYMBptr->feuille.type!=_VECT || den._SYMBptr->feuille._VECTptr->size()!=2)return false;
      gen A=0,B=0;const vecteur &terms=*den._SYMBptr->feuille._VECTptr;
      for(unsigned i=0;i<2;++i){gen dc,dn;if(!integration_monomial(terms[i],x,dc,dn,contextptr))return false;
        if(is_zero(dn))A+=dc;else if(dn==2*q)B+=dc;else return false;
      }
      if(is_zero(A) || B!=A*c*c)return false;
      power=ln/q;shift=giac::ln(lc,contextptr)-power*giac::ln(c,contextptr);
      multiplier=wc/(A*c*q);
      dl=lo==plus_inf?plus_inf:c*pow(lo,q,contextptr);dh=hi==plus_inf?plus_inf:c*pow(hi,q,contextptr);
    }
    else {
      gen wa,wb,A,B,C,la,lb;
      if(!is_linear_wrt(u,x,a,b,contextptr) || !integration_rational(a) || !integration_rational(b) || is_zero(a) ||
         !is_linear_wrt(weight,x,wa,wb,contextptr) || !is_zero(wa) || !integration_rational(wb) ||
         !integration_gaussian_quadratic(den,x,A,B,C,contextptr) || is_zero(A) || B*a!=2*A*b || C*a*a!=A*(1+b*b) ||
         !is_linear_wrt(logarg,x,la,lb,contextptr) || !integration_rational(la) || !integration_rational(lb) || la*b!=lb*a || !is_strictly_positive(la/a,contextptr))return false;
      power=1;shift=ln(la/a,contextptr);multiplier=wb*a/A;
      dl=ratnormal(a*lo+b,contextptr);dh=ratnormal(a*hi+b,contextptr);
    }
    if(is_strictly_greater(power,32,contextptr) || is_strictly_greater(-power,32,contextptr))return false;
    bool reverse=dl==plus_inf || (dl==1 && dh==0);if(reverse)swapgen(dl,dh);
    bool lower=dl==0 && dh==1,upper=dl==1 && dh==plus_inf,full=dl==0 && dh==plus_inf;
    if(!lower && !upper && !full)return false;
    gen result=0;
    if(!is_zero(shift))result=shift*cst_pi*cst_pi*(full?gen(1)/8:(upper?gen(3)/32:gen(1)/32));
    if(!is_zero(power)){
      gen moment=gen(7)*_Zeta(3,contextptr)/(full?8:16);
      if(!full){
        gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
        moment+=(upper?1:-1)*cst_pi*G/4;
      }
      result+=power*moment;
    }
    res=outside*multiplier*result;if(reverse)res=-res;return true;
  }

  static bool integration_trig_unit_log(const gen &f,const gen &x,gen &scale,gen &angle,bool &cosine,int &sign,GIAC_CONTEXT){
    if(!f.is_symb_of_sommet(at_ln))return false;
    gen arg=f._SYMBptr->feuille,outer=integration_syntax(integration_coefficient(arg,x,contextptr),contextptr);
    if(!integration_rational(outer) || !arg.is_symb_of_sommet(at_plus) || arg._SYMBptr->feuille.type!=_VECT || arg._SYMBptr->feuille._VECTptr->size()!=2)return false;
    const vecteur &v=*arg._SYMBptr->feuille._VECTptr;gen A=0,B=0;bool found=false;
    for(unsigned i=0;i<2;++i){
      if(integration_rational(v[i])){A+=v[i];continue;}
      gen t=v[i],c=integration_syntax(integration_coefficient(t,x,contextptr),contextptr);
      if(found || !integration_rational(c) || !(t.is_symb_of_sommet(at_sin) || t.is_symb_of_sommet(at_cos)))return false;
      found=true;B=c;angle=t._SYMBptr->feuille;cosine=t.is_symb_of_sommet(at_cos);
    }
    if(!found || is_zero(A) || (B!=A && B!=-A))return false;
    scale=outer*A;if(!is_strictly_positive(scale,contextptr))return false;
    sign=B==A?1:-1;return true;
  }

  // The unscaled product is even, pi-periodic and symmetric in each quarter.
  // Positive independent log scales add two known quarter-period moments.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_opposite_trig_logs(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || taille(e,129)>128)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT || f._SYMBptr->feuille._VECTptr->size()!=2)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;gen A,B,theta,other;bool cosine,othercosine;int sign,othersign;
    if(!integration_trig_unit_log(v[0],x,A,theta,cosine,sign,contextptr) || !integration_trig_unit_log(v[1],x,B,other,othercosine,othersign,contextptr) || theta!=other || cosine!=othercosine || sign!=-othersign)return false;
    gen a,b;
    if(!is_linear_wrt(theta,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) || (!integration_rational(b) && !integration_rational(ratnormal(b/cst_pi,contextptr))))return false;
    gen l=ratnormal(2*(a*lo+b)/cst_pi,contextptr),r=ratnormal(2*(a*hi+b)/cst_pi,contextptr);
    if(l.type!=_INT_ || r.type!=_INT_ || l.val < -64 || l.val>64 || r.val < -64 || r.val>64 || l==r)return false;
    gen L=ln(gen(2),contextptr),LA=ln(A,contextptr),LB=ln(B,contextptr);
    gen result=(hi-lo)*(LA*LB-(LA+LB)*L+L*L-cst_pi*cst_pi/6);
    int offset=cosine?(sign==1?2:0):(sign==1?1:3),otheroffset=cosine?(sign==1?0:2):(sign==1?3:1);
    int delta=integration_quarter_sigma(r.val+offset)-integration_quarter_sigma(l.val+offset);
    int otherdelta=integration_quarter_sigma(r.val+otheroffset)-integration_quarter_sigma(l.val+otheroffset);
    gen term=gen(otherdelta)*LA+gen(delta)*LB;
    if(!is_zero(term)){
      gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
      result-=2*G*term/a;
    }
    res=outside*result;return true;
  }

  // Extract scalar coefficients before the sparse parser: arithmetic product
  // construction may encode a rational coefficient as an inverse integer.
  static bool integration_scaled_chain_terms(const gen &g,const gen &x,vecteur &powers,vecteur &coefficients,unsigned &budget,GIAC_CONTEXT){
    const vecteur *terms=g.is_symb_of_sommet(at_plus) && g._SYMBptr->feuille.type==_VECT?g._SYMBptr->feuille._VECTptr:0;
    unsigned count=terms?terms->size():1;if(count>4)return false;
    for(unsigned i=0;i<count;++i){
      gen t=integration_syntax(terms?(*terms)[i]:g,contextptr),c=integration_syntax(integration_coefficient(t,x,contextptr),contextptr);
      if(!integration_rational(c))return false;
      vecteur p,v;if(!integration_chain_terms(t,x,p,v,budget,contextptr))return false;
      for(unsigned j=0;j<p.size();++j)if(!integration_chain_add(powers,coefficients,p[j],c*v[j]))return false;
    }
    return true;
  }

// Positive-axis monomial substitution followed by bounded odd Gaussian-erf
// moments. Only scalar rational recurrence coefficients are retained.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_monomial_gaussian_erf(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=plus_inf || taille(e,97)>96)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()>6)return false;
    gen exponent,argument,weight=outside;bool gaussian=false,error=false;
    for(unsigned i=0;i<v.size();++i){
      gen base=v[i],power=1;
      if(integration_outer_power(base,power) && base.is_symb_of_sommet(at_exp)){
        if(gaussian)return false;gaussian=true;exponent=integration_syntax(power*base._SYMBptr->feuille,contextptr);
      }
      else if(v[i].is_symb_of_sommet(at_erf)){
        if(error)return false;error=true;argument=v[i]._SYMBptr->feuille;
      }
      else weight=weight*v[i];
    }
    if(!gaussian || !error)return false;
    vecteur ep,ec,ap,ac,wp,wc;unsigned budget=64;
    if(!integration_scaled_chain_terms(exponent,x,ep,ec,budget,contextptr) || ep.size()>2 ||
       !integration_scaled_chain_terms(argument,x,ap,ac,budget,contextptr) || ap.size()!=1 ||
       !integration_scaled_chain_terms(weight,x,wp,wc,budget,contextptr) || wp.size()!=1)return false;
    gen a=0,q=0,d=0;
    for(unsigned i=0;i<ep.size();++i){
      if(is_zero(ep[i]))d+=ec[i];
      else {if(!is_zero(a))return false;a=-ec[i];q=ep[i];}
    }
    if(!is_strictly_positive(a,contextptr) || !is_strictly_positive(q,contextptr) ||
       is_strictly_greater(q,16,contextptr) || q!=2*ap[0] || is_zero(ac[0]))return false;
    gen N=(wp[0]+1)/q;if(N.type!=_INT_ || N.val<1 || N.val>8)return false;
    gen b=ac[0],v0=a+b*b,polynomial=1,moment=1;
    for(int j=1;j<N.val;++j){moment=moment*gen(2*j-1)/(2*v0);polynomial=gen(j)*polynomial/a+moment;}
    res=wc[0]*exp(d,contextptr)*b*polynomial/(q*a*sqrt(v0,contextptr));return true;
  }

// t=atan(|a|*x^q) maps the positive half-line to (0,pi/2). A denominator
// power k leaves the finite harmonic polynomial cos(t)^(2*k-2).
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_atan_cauchy_power(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || hi!=plus_inf || taille(e,97)>96)return false;
    gen f=e,outside=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(outside) || !f.is_symb_of_sommet(at_prod) || f._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*f._SYMBptr->feuille._VECTptr;if(v.size()>6)return false;
    gen argument,denominator,weight=outside;int n=0,k=0;
    for(unsigned i=0;i<v.size();++i){
      gen angle=v[i];int order=1;
      if(angle.is_symb_of_sommet(at_pow) && angle._SYMBptr->feuille.type==_VECT){
        const vecteur &p=*angle._SYMBptr->feuille._VECTptr;
        if(p.size()==2 && p[1].type==_INT_ && p[1].val>=1 && p[1].val<=8){order=p[1].val;angle=gen(p[0]);}
      }
      gen angle_coefficient=integration_syntax(integration_coefficient(angle,x,contextptr),contextptr);
      if(angle.is_symb_of_sommet(at_atan)){
        if(n || !integration_rational(angle_coefficient))return false;n=order;argument=angle._SYMBptr->feuille;weight=weight*pow(angle_coefficient,order);continue;
      }
      gen base=v[i],power=1;
      if(integration_outer_power(base,power) && base.is_symb_of_sommet(at_plus) && power.type==_INT_ && power.val<=-1 && power.val>=-4){
        if(k)return false;k=-power.val;denominator=base;
      }
      else weight=weight*v[i];
    }
    if(!n || !k)return false;
    vecteur ap,ac,dp,dc,wp,wc;unsigned budget=64;
    if(!integration_scaled_chain_terms(argument,x,ap,ac,budget,contextptr) || ap.size()!=1 ||
       !integration_scaled_chain_terms(denominator,x,dp,dc,budget,contextptr) || dp.size()!=2 ||
       !integration_scaled_chain_terms(weight,x,wp,wc,budget,contextptr) || wp.size()!=1)return false;
    gen a=ac[0],q=ap[0],A=0,B=0;
    if(is_zero(a) || !is_strictly_positive(q,contextptr) || is_strictly_greater(q,16,contextptr) || wp[0]!=q-1)return false;
    for(unsigned i=0;i<dp.size();++i){
      if(is_zero(dp[i]))A+=dc[i];
      else if(dp[i]==2*q)B+=dc[i];else return false;
    }
    if(!is_strictly_positive(A,contextptr) || B!=A*a*a)return false;
    int h=k-1;gen L=cst_pi/2,divisor=pow(gen(4),h);
    gen value=comb(2*h,h,contextptr)*pow(L,n+1)/(gen(n+1)*divisor);
    for(int j=1;j<=h;++j){
      gen omega2=gen(4*j*j),sign=j%2?gen(-1):gen(1),previous=0,current=(sign-1)/omega2;
      for(int m=2;m<=n;++m){gen next=gen(m)*pow(L,m-1)*sign/omega2-gen(m*(m-1))*previous/omega2;previous=current;current=next;}
      value+=2*comb(2*h,h-j,contextptr)*current/divisor;
    }
    if(is_strictly_positive(-a,contextptr) && n%2)value=-value;
    res=ratnormal(wc[0]*value/(q*abs(a,contextptr)*pow(A,k)),contextptr);return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_exponential_beta(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!is_zero(lo) || hi!=plus_inf || taille(e,97)>96 || !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    for(unsigned i=0;i<2;++i){
      gen base=v[i],n=1;if(!integration_outer_power(base,n) || !integration_resource_rational(n) ||
        !is_strictly_greater(n,-1,contextptr) || is_strictly_greater(n,8192,contextptr))continue;
      gen other;if(!integration_one_plus(base,other))continue;
      gen sign=integration_syntax(integration_coefficient(other,x,contextptr),contextptr);
      if(sign!=-1)continue;
      gen bp=1;if(!integration_outer_power(other,bp) || !other.is_symb_of_sommet(at_exp))continue;
      gen slope,phase;if(!is_linear_wrt(other._SYMBptr->feuille,x,slope,phase,contextptr) || !is_zero(phase))continue;
      gen b=-slope*bp;if(!integration_resource_rational(b) || !is_strictly_positive(b,contextptr))continue;
      gen carrier=v[1-i],ap=1;if(!integration_outer_power(carrier,ap) || !carrier.is_symb_of_sommet(at_exp))continue;
      if(!is_linear_wrt(carrier._SYMBptr->feuille,x,slope,phase,contextptr) || !is_zero(phase))continue;
      gen a=-slope*ap;if(!integration_resource_rational(a) || !is_strictly_positive(a,contextptr))continue;
      gen r=a/b;if(r.type!=_INT_ || r.val<1 || r.val>16)continue;
      gen answer=gen(1)/(n+1);for(int j=2;j<=r.val;++j)answer=answer*gen(j-1)/(n+j);
      res=answer/b;return true;
    }
    return false;
  }

  // Reflection pairs with a strictly positive denominator throughout the
  // interval. Equal powers of complementary nonnegative factors sum to one.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_complementary_ratio(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || is_inf(lo) || is_inf(hi) || taille(e,97)>96 ||
       !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
    gen denominator,numerator;
    if(integration_power(v[0],denominator,-1))numerator=v[1];
    else if(integration_power(v[1],denominator,-1))numerator=v[0];else return false;
    if(!denominator.is_symb_of_sommet(at_plus) || denominator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*denominator._SYMBptr->feuille._VECTptr;
    if(terms.size()!=2 || (numerator!=terms[0] && numerator!=terms[1]))return false;
    gen left=terms[0],right=terms[1],p=1,q=1;
    if(!integration_outer_power(left,p) || !integration_outer_power(right,q) || p!=q ||
       p.type!=_INT_ || p.val<1 || p.val>64)return false;
    gen a,b,c,d;
    bool reflected=false;
    if(is_linear_wrt(left,x,a,b,contextptr) && is_linear_wrt(right,x,c,d,contextptr) &&
       integration_rational(a) && integration_rational(b) && integration_rational(c) && integration_rational(d) &&
       integration_rational(lo) && integration_rational(hi) && !is_zero(a) && c==-a && d==a*(lo+hi)+b){
      gen l=a*lo+b,r=a*hi+b;
      reflected=(is_zero(l) || is_strictly_positive(l,contextptr)) &&
        (is_zero(r) || is_strictly_positive(r,contextptr)) && !is_zero(l+r);
    }
    else {
      if(left.is_symb_of_sommet(at_cos))swapgen(left,right);
      if(!left.is_symb_of_sommet(at_sin) || !right.is_symb_of_sommet(at_cos) ||
         left._SYMBptr->feuille!=right._SYMBptr->feuille ||
         !is_linear_wrt(left._SYMBptr->feuille,x,a,b,contextptr) ||
         !integration_rational(a) || is_zero(a) || !integration_period_real_bound(b,contextptr) ||
         !integration_period_real_bound(lo,contextptr) || !integration_period_real_bound(hi,contextptr))return false;
      gen l=ratnormal(2*(a*lo+b)/cst_pi,contextptr),r=ratnormal(2*(a*hi+b)/cst_pi,contextptr);
      reflected=(is_zero(l) && r==1) || (l==1 && is_zero(r));
    }
    if(!reflected)return false;
    res=ratnormal((hi-lo)/2,contextptr);return true;
  }

  // Fourier transform of a positive quadratic C+A*x^2. Keeping abs(b)
  // explicitly is required for an unrestricted real frequency.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_cauchy_fourier(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || taille(e,65)>64)return false;
    int halves=(lo==minus_inf && hi==plus_inf)?2:(is_zero(lo) && hi==plus_inf)?1:0;
    if(!halves)return false;
    vecteur factors;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT)factors=*e._SYMBptr->feuille._VECTptr;
    else factors.push_back(e);
    if(factors.size()>2)return false;
    gen frequency=0,phase,denominator;bool have_den=false,have_cos=false;
    for(unsigned i=0;i<factors.size();++i){
      gen base;
      if(factors[i].is_symb_of_sommet(at_cos)){
        if(have_cos || !is_linear_wrt(factors[i]._SYMBptr->feuille,x,frequency,phase,contextptr) ||
           !is_zero(phase) || contains(frequency,x) || !is_zero(im(frequency,contextptr)))return false;
        have_cos=true;
      }
      else if(!have_den && integration_power(factors[i],base,-1)){denominator=base;have_den=true;}
      else return false;
    }
    if(!have_den || !denominator.is_symb_of_sommet(at_plus) || denominator._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &terms=*denominator._SYMBptr->feuille._VECTptr;if(terms.size()!=2)return false;
    gen A=0,C=0;
    for(unsigned i=0;i<terms.size();++i){
      if(!contains(terms[i],x)){C+=terms[i];continue;}
      gen term=terms[i],coefficient=integration_coefficient(term,x,contextptr),base;
      if(!integration_power(term,base,2) || base!=x || contains(coefficient,x))return false;
      A+=coefficient;
    }
    if(!is_zero(im(A,contextptr)) || !is_zero(im(C,contextptr)) ||
       !is_strictly_positive(A,contextptr) || !is_strictly_positive(C,contextptr))return false;
    gen scale=sqrt(C/A,contextptr);
    res=gen(halves)*cst_pi*exp(-scale*abs(frequency,contextptr),contextptr)/(2*A*scale);return true;
  }

  // u=c*x^q maps the finite interval onto [0,1]. These two Catalan
  // measures and the circular atan measure share the same bounded parser.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_unit_log_arc(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !is_zero(lo) || !integration_resource_rational(hi) || !is_strictly_positive(hi,contextptr) ||
       !e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()<2 || v.size()>4)return false;
    gen arg,weight=1,den,rad;bool arc=false,logarithm=false,have_den=false,have_rad=false;
    for(unsigned i=0;i<v.size();++i){
      gen base;
      if(v[i].is_symb_of_sommet(at_atan) || v[i].is_symb_of_sommet(at_ln)){
        if(arc || logarithm)return false;
        arc=v[i].is_symb_of_sommet(at_atan);logarithm=!arc;arg=v[i]._SYMBptr->feuille;
      }
      else if(integration_power(v[i],base,-1) && base.is_symb_of_sommet(at_plus)){
        if(have_den)return false;den=base;have_den=true;
      }
      else if((integration_power(v[i],base,-1) && integration_square_root(base,rad)) ||
              (v[i].is_symb_of_sommet(at_pow) && v[i]._SYMBptr->feuille.type==_VECT && v[i]._SYMBptr->feuille._VECTptr->size()==2 &&
               v[i]._SYMBptr->feuille[1]==-gen(1)/2 && (rad=v[i]._SYMBptr->feuille[0],true))){
        if(have_rad)return false;have_rad=true;
      }
      else weight=weight*v[i];
    }
    gen c,q,w,n;
    if((!arc && !logarithm) || !integration_monomial(arg,x,c,q,contextptr) || !integration_monomial(weight,x,w,n,contextptr) ||
       !integration_resource_rational(c) || !integration_resource_rational(q) || !integration_resource_rational(w) ||
       is_zero(c) || !is_strictly_positive(q,contextptr) || is_strictly_greater(q,32,contextptr))return false;
    if(q.type==_FRAC && (q._FRACptr->den.type!=_INT_ || q._FRACptr->den.val>16))return false;
    if(arc && n==-1 && !have_den){
      if(have_rad){
        gen other,rc,rn;
        if(!integration_one_plus(rad,other) || !integration_monomial(other,x,rc,rn,contextptr) || rn!=2*q ||
           !is_strictly_positive(-rc,contextptr) || !is_zero(ratnormal(-rc*pow(hi,2*q,contextptr)-1,contextptr)))return false;
        gen b=c/sqrt(-rc,contextptr),value=asinh(abs(b,contextptr),contextptr);
        if(is_strictly_positive(-c,contextptr))value=-value;
        res=w*cst_pi*value/(2*q);return true;
      }
      gen end=ratnormal(c*pow(hi,q,contextptr),contextptr);
      if(end!=1 && end!=-1)return false;
      gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
      res=w*end*G/q;return true;
    }
    if(logarithm && have_den && !have_rad && n==q-1 && is_strictly_positive(c,contextptr)){
      gen other,dc,dn;
      if(!integration_one_plus(den,other) || !integration_monomial(other,x,dc,dn,contextptr) || dn!=2*q || dc!=c*c ||
         !is_zero(ratnormal(c*pow(hi,q,contextptr)-1,contextptr)))return false;
      gen G=(gen(symbolic(at_Psi,makesequence(gen(1)/4,1)))-gen(symbolic(at_Psi,makesequence(gen(3)/4,1))))/16;
      res=-w*G/(c*q);return true;
    }
    return false;
  }

  // Fourier coefficients of a positive cosine kernel, including the
  // Poisson form. Integer order is an actual assumption, not a name test.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_positive_cosine_kernel(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!angle_radian(contextptr) || !integration_period_real_bound(lo,contextptr) || !integration_period_real_bound(hi,contextptr) || taille(e,129)>128)return false;
    gen kernel=e,harmonic;bool have_harmonic=false;
    if(e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT){
      const vecteur &v=*e._SYMBptr->feuille._VECTptr;if(v.size()!=2)return false;
      unsigned j=v[0].is_symb_of_sommet(at_cos)?0:1;
      if(!v[j].is_symb_of_sommet(at_cos))return false;
      harmonic=v[j]._SYMBptr->feuille;have_harmonic=true;kernel=v[1-j];
    }
    gen den;bool logarithm=kernel.is_symb_of_sommet(at_ln);
    if(logarithm)den=kernel._SYMBptr->feuille;
    else if(!integration_power(kernel,den,-1))return false;
    if(!den.is_symb_of_sommet(at_plus) || den._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*den._SYMBptr->feuille._VECTptr;if(v.size()>4)return false;
    gen A=0,B=0,u;bool found=false;
    for(unsigned i=0;i<v.size();++i){
      if(!contains(v[i],x)){if(!is_zero(im(v[i],contextptr)))return false;A+=v[i];continue;}
      gen t=v[i],c=integration_coefficient(t,x,contextptr);
      if(found || !is_zero(im(c,contextptr)) || !t.is_symb_of_sommet(at_cos))return false;
      B=c;u=t._SYMBptr->feuille;found=true;
    }
    if(!found)return false;
    gen a,b,n=0;
    if(!is_linear_wrt(u,x,a,b,contextptr) || !integration_resource_rational(a) || is_zero(a) || !integration_period_real_bound(b,contextptr))return false;
    if(have_harmonic){
      gen c,d;
      if(!is_linear_wrt(harmonic,x,c,d,contextptr))return false;
      n=ratnormal(c/a,contextptr);
      if(taille(n,17)>16 || !is_assumed_integer(n,contextptr) || !is_zero(ratnormal(d-n*b,contextptr)))return false;
      n=abs(n,contextptr);
    }
    gen l=ratnormal((a*lo+b)/cst_pi,contextptr),r=ratnormal((a*hi+b)/cst_pi,contextptr),periods=ratnormal((r-l)/2,contextptr);
    bool whole=periods.type==_INT_ && periods.val>=-64 && periods.val<=64;
    bool half=l.type==_INT_ && r.type==_INT_ && l.val>=-128 && l.val<=128 && r.val>=-128 && r.val<=128;
    if(!whole && !half)return false;
    gen q=ratnormal(-B/2,contextptr),root,mean;
    bool poisson=is_zero(ratnormal(A-1-q*q,contextptr)) && is_strictly_positive(1-q,contextptr) && is_strictly_positive(1+q,contextptr);
    if(poisson){root=1-q*q;mean=0;}
    else {
      if(!is_strictly_positive(A-abs(B,contextptr),contextptr))return false;
      root=sqrt(A*A-B*B,contextptr);q=-B/(A+root);
      if(logarithm && is_zero(n))mean=ln((A+root)/2,contextptr);
    }
    if(logarithm && !is_zero(n) && !is_strictly_positive(n,contextptr))return false;
    if(logarithm)res=is_zero(n)?(hi-lo)*mean:-(hi-lo)*pow(q,n,contextptr)/n;
    else res=(hi-lo)*(is_zero(n)?gen(1):pow(q,n,contextptr))/root;
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_dilog_definite(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!integration_resource_rational(lo) || !integration_resource_rational(hi) || is_strictly_positive(-lo,contextptr) || is_strictly_positive(-hi,contextptr))return false;
    // log(1+d*x^q)*w*x^n ~ d*w*x^(q+n) at zero. Keep the
    // cancellation inside the logarithm; a nonintegrable one-sided power
    // has a signed infinite limit, without constructing a large primitive.
    if(is_zero(lo) && is_strictly_positive(hi,contextptr) && taille(e,97)<=96){
      vecteur terms=e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT?*e._SYMBptr->feuille._VECTptr:vecteur(1,e);
      gen argument,weight=1;unsigned logs=0;
      if(terms.size()<=4){
        for(unsigned j=0;j<terms.size();++j){
          if(terms[j].is_symb_of_sommet(at_ln)){argument=terms[j]._SYMBptr->feuille;++logs;}
          else weight=weight*terms[j];
        }
        gen other,d,q,w,n;
        if(logs==1 && integration_one_plus(argument,other) &&
           integration_monomial(other,x,d,q,contextptr) && integration_resource_rational(d) && !is_zero(d) &&
           integration_resource_rational(q) && is_strictly_positive(q,contextptr) && !is_strictly_greater(q,32,contextptr) &&
           integration_monomial(weight,x,w,n,contextptr) && integration_resource_rational(w) && !is_zero(w) &&
           integration_resource_rational(n) && !is_strictly_positive(q+n+1,contextptr)){
          gen endpoint=1+d*pow(hi,q,contextptr);
          if(is_zero(endpoint) || is_strictly_positive(endpoint,contextptr)){
            res=is_strictly_positive(w*d,contextptr)?plus_inf:minus_inf;return true;
          }
        }
      }
    }
    gen u,scale;bool extra;
    if(!integration_dilog_form(e,x,u,scale,extra,contextptr))return false;
    // A complex exponential can cross a principal branch cut between real
    // endpoints. The real definite shortcut does not prove that path safe.
    if(!is_zero(im(scale,contextptr)))return false;
    gen c,q;
    if(integration_monomial(u,x,c,q,contextptr) && !is_strictly_positive(q,contextptr))return false;
    gen l=subst(u,x,lo,false,contextptr).eval(1,contextptr),r=subst(u,x,hi,false,contextptr).eval(1,contextptr);
    if(!is_zero(im(l,contextptr)) || !is_zero(im(r,contextptr)) ||
       (!is_zero(1-l) && !is_strictly_positive(1-l,contextptr)) || (!is_zero(1-r) && !is_strictly_positive(1-r,contextptr)))return false;
    if(extra && (is_one(l) || is_one(r)))return false;
    res=scale*(_Li2(l,contextptr)-_Li2(r,contextptr));
    if(extra){gen L=ln(1-l,contextptr),R=ln(1-r,contextptr);res+=scale*(L*L-R*R)/2;}
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_compact_definite(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(integrate_dilog_definite(e,x,lo,hi,res,contextptr) ||
       integrate_positive_cosine_kernel(e,x,lo,hi,res,contextptr) ||
       integrate_unit_log_arc(e,x,lo,hi,res,contextptr) ||
       integrate_complementary_ratio(e,x,lo,hi,res,contextptr) ||
       integrate_cauchy_fourier(e,x,lo,hi,res,contextptr) ||
       integrate_exponential_beta(e,x,lo,hi,res,contextptr) ||
       integrate_monomial_gaussian_erf(e,x,lo,hi,res,contextptr) ||
       integrate_atan_cauchy_power(e,x,lo,hi,res,contextptr) ||
       integrate_atan_log_cauchy(e,x,lo,hi,res,contextptr) ||
       integrate_opposite_trig_logs(e,x,lo,hi,res,contextptr) ||
       integrate_gamma_log_moment(e,x,lo,hi,res,contextptr) ||
       integrate_mobius_cauchy_log(e,x,lo,hi,res,contextptr) ||
       integrate_log_sine_cosine_sum(e,x,lo,hi,res,contextptr) ||
       integrate_erf_tail_product(e,x,lo,hi,res,contextptr) ||
       integrate_reciprocal_cosh(e,x,lo,hi,res,contextptr) ||
       integrate_gaussian_atan_moment(e,x,lo,hi,res,contextptr) ||
       integrate_exponential_log_moment(e,x,lo,hi,res,contextptr) ||
       integrate_log_trig_quarters(e,x,lo,hi,res,contextptr) ||
       integrate_log_product_zeta(e,x,lo,hi,res,contextptr) ||
       integrate_log_trig_sum(e,x,lo,hi,res,contextptr) ||
       integrate_atan_log_measure(e,x,lo,hi,res,contextptr) ||
       integrate_mixed_mellin_log(e,x,lo,hi,res,contextptr) ||
       integrate_oscillatory_cancellation(e,x,lo,hi,res,contextptr))return true;

    // Whole-expression shortcuts must precede any linear sum decomposition.
    if (integrate_gaussian_erf_exp(e,x,lo,hi,res,contextptr) ||
        integrate_beta_affine_log(e,x,lo,hi,res,contextptr) ||
        integrate_acos_monomial_pullback(e,x,lo,hi,res,contextptr) ||
        integrate_erf_chain(e,x,lo,hi,res,contextptr) ||
        integrate_atan_rectangle_pair(e,x,lo,hi,res,contextptr)) return true;

    if (integrate_reciprocal_trig_period(e,x,lo,hi,res,contextptr)) return true;
    if (integrate_exp_difference(e,x,lo,hi,res,contextptr)) return true;
    if (integrate_mellin_two_binomials(e,x,lo,hi,res,contextptr)) return true;
    if (integrate_gaussian_erf(e,x,lo,hi,res,contextptr)) return true;
    if (integrate_arc_rational_circle(e,x,lo,hi,res,contextptr) ||
        integrate_loglog_frullani(e,x,lo,hi,res,contextptr) ||
        integrate_acos_circle(e,x,lo,hi,res,contextptr) ||
        integrate_atan_frullani(e,x,lo,hi,res,contextptr) ||
        integrate_loglog_mellin(e,x,lo,hi,res,contextptr)) return true;
    if (integrate_log_zeta(e,x,lo,hi,res,contextptr) || integrate_hyperbolic_log(e,x,lo,hi,res,contextptr) ||
        integrate_atan_log_moment(e,x,lo,hi,res,contextptr) ||
        integrate_atan_square(e,x,lo,hi,res,contextptr) || integrate_mellin_log(e,x,lo,hi,res,contextptr) || integrate_beta_log(e,x,lo,hi,res,contextptr) ||
        integrate_laplace_difference(e,x,lo,hi,res,contextptr) || integrate_inverse_gaussian(e,x,lo,hi,res,contextptr) ||
        integrate_decay_transform(e,x,lo,hi,res,contextptr) || integrate_log_trig(e,x,lo,hi,res,contextptr) ||
        integrate_thermal_moment(e,x,lo,hi,res,contextptr)) return true;
    gen primitive;
    if (!is_inf(lo) && !is_inf(hi) && integrate_binomial_chain(e,x,primitive,true,contextptr)){
      res=subst(primitive,x,hi,false,contextptr)-subst(primitive,x,lo,false,contextptr);return true;
    }
    if (lo==minus_inf && hi==plus_inf && e.is_symb_of_sommet(at_inv) &&
        integrate_logistic_moment(makevecteur(1,e),x,res,contextptr)) return true;
    if (!e.is_symb_of_sommet(at_prod) || e._SYMBptr->feuille.type!=_VECT) return false;
    const vecteur &v=*e._SYMBptr->feuille._VECTptr;
    return integrate_sine_dirichlet(v,x,lo,hi,res,contextptr) ||
      (lo==minus_inf && hi==plus_inf && integrate_logistic_moment(v,x,res,contextptr)) ||
      integrate_weighted_reflection(v,x,lo,hi,res,contextptr);
  }

  // Real odd-root substitution, without replacing real roots by principal
  // complex powers. The substitution is globally one-to-one on the real line.
  // Keep these bounded dispatch paths compact in the calculator ROM.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_real_root(const gen &e,const gen &x,gen &res,int mode,GIAC_CONTEXT){
    if (complex_mode(contextptr) || complex_variables(contextptr) || taille(e,129)>128 ||
        (!has_op(e,*at_surd) && !has_op(e,*at_NTHROOT))) return false;
    vecteur roots=mergevecteur(lop(e,at_surd),lop(e,at_NTHROOT));
    if (roots.empty() || roots.size()>8) return false;
    const gen &f=roots[0]._SYMBptr->feuille;
    if (f.type!=_VECT || f._VECTptr->size()!=2) return false;
    bool nth=roots[0].is_symb_of_sommet(at_NTHROOT);
    gen arg=f[nth?1:0],index=f[nth?0:1],a,b;
    if (index.type!=_INT_ || index.val < -9 || index.val>9) return false;
    int n=index.val<0?-index.val:index.val;
    if (n<3 || n%2==0 || !is_linear_wrt(arg,x,a,b,contextptr) ||
        !integration_rational(a) || !integration_rational(b) || is_zero(a)) return false;
    if(roots.size()==1 && index.val>0){
      gen base=integration_syntax(e,contextptr),coefficient=integration_syntax(integration_coefficient(base,x,contextptr),contextptr),den,A,B;
      if(integration_resource_rational(coefficient) && integration_power(base,den,-1) &&
         is_linear_wrt(den,roots[0],A,B,contextptr) && integration_resource_rational(A) &&
         integration_resource_rational(B) && !is_zero(A)){
        gen u=symbolic(at_NTHROOT,makesequence(n,arg)),q=-B/A,total=0;
        // Polynomial division in the real root variable. This avoids a
        // generic rational-integration call and its deep algebraic stack.
        for(int j=0;j<n-1;++j)total+=pow(q,j)*pow(u,n-1-j)/(n-1-j);
        if(!is_zero(B))total+=pow(q,n-1)*symbolic(at_ln,symbolic(at_abs,A*u+B));
        res=coefficient*gen(n)*total/(a*A);return true;
      }
    }
    vecteur powers=lop(e,at_pow);
    for (unsigned i=0;i<powers.size();++i){
      const gen &p=powers[i]._SYMBptr->feuille;
      if (p.type!=_VECT || p._VECTptr->size()!=2 || p[1].type!=_INT_ ||
          p[1].val < -16 || p[1].val>16) return false;
    }
    gen t(identificateur(" khicas_root"));
    if (contains(lidnt(e),t) || eval(t,1,contextptr)!=t) return false;
    vecteur replacements;
    for (unsigned i=0;i<roots.size();++i){
      const gen &r=roots[i]._SYMBptr->feuille;
      if (r.type!=_VECT || r._VECTptr->size()!=2) return false;
      bool nth=roots[i].is_symb_of_sommet(at_NTHROOT);
      const gen &ri=(*r._VECTptr)[nth?0:1];
      if (r[nth?1:0]!=arg || (ri!=n && ri!=-n)) return false;
      replacements.push_back(ri==n?t:inv(t,contextptr));
    }
    gen transformed=subst(e,roots,replacements,false,contextptr);
    transformed=subst(transformed,x,(pow(t,n)-b)/a,false,contextptr)*gen(n)*pow(t,n-1)/a;
    transformed=ratnormal(transformed,contextptr);
    gen remainder;
    gen primitive=linear_integrate_nostep(transformed,t,remainder,mode,contextptr);
    if (!is_zero(remainder) || is_undef(primitive) || has_op(primitive,*at_integrate)) return false;
    res=subst(primitive,t,symbolic(at_NTHROOT,makesequence(n,arg)),false,contextptr);
    return true;
  }

  // Full-line rational kernel: R=sum(a_i/(x-b_i)), with all residues of
  // one sign. For positive residues R is strictly decreasing between poles.
  // The roots of R(x)=t have sum sum(b_i)+sum(a_i)/t (Vieta); hence their
  // total Jacobian is sum(a_i)/t^2. Integrating t^2/(1+t^2) gives pi*sum(a_i).
  // Keep these bounded dispatch paths compact in the calculator ROM.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_residue_kernel(const gen &e,const gen &x,gen &res,GIAC_CONTEXT){
    gen den,r;
    if (!integration_power(e,den,-1) || !integration_one_plus(den,r) || !integration_power(r,r,-2)) return false;
    vecteur terms;
    if (r.is_symb_of_sommet(at_plus) && r._SYMBptr->feuille.type==_VECT) terms=*r._SYMBptr->feuille._VECTptr;
    else terms.push_back(r);
    if (terms.empty() || terms.size()>8) return false;
    gen total=0;int sign=0;
    for (unsigned i=0;i<terms.size();++i){
      gen term=terms[i],c=integration_coefficient(term,x,contextptr),base,a,b;
      if (!integration_rational(c) || !integration_power(term,base,-1) ||
          !is_linear_wrt(base,x,a,b,contextptr) || !integration_rational(a) ||
          !integration_rational(b) || is_zero(a)) return false;
      c=c/a;
      if (is_zero(c)) continue;
      int s=is_strictly_positive(c,contextptr)?1:-1;
      if (sign && sign!=s) return false;
      sign=s;total+=c;
    }
    if (!sign) return false;
    res=cst_pi*(sign*total);return true;
  }

  // Leading term at +infinity of bounded rational arithmetic. If leading
  // terms cancel, decline rather than guessing the next degree. This check
  // proves a nonzero 1/x tail without expanding a rational function.
  bool integration_rational_tail(const gen &g,const gen &x,int &degree,gen &leading,unsigned &budget,unsigned depth,GIAC_CONTEXT){
    if(!budget || depth>8)return false;--budget;
    if(integration_resource_rational(g)){if(is_zero(g))return false;degree=0;leading=g;return true;}
    if(g==x){degree=1;leading=1;return true;}
    if(g.type!=_SYMB)return false;
    const gen &f=g._SYMBptr->feuille;
    if(g.is_symb_of_sommet(at_neg) || g.is_symb_of_sommet(at_inv)){
      if(!integration_rational_tail(f,x,degree,leading,budget,depth+1,contextptr))return false;
      if(g.is_symb_of_sommet(at_neg))leading=-leading;
      else {degree=-degree;leading=inv(leading,contextptr);}
      return integration_resource_rational(leading);
    }
    if(f.type!=_VECT)return false;const vecteur &v=*f._VECTptr;
    if(g.is_symb_of_sommet(at_pow)){
      if(v.size()!=2 || v[1].type!=_INT_ || v[1].val < -64 || v[1].val>64 ||
         !integration_rational_tail(v[0],x,degree,leading,budget,depth+1,contextptr))return false;
      int n=v[1].val;
      if(n && (degree>128/(n<0?-n:n) || degree < -128/(n<0?-n:n)))return false;
      degree*=n;leading=pow(leading,n,contextptr);return integration_resource_rational(leading);
    }
    bool product=g.is_symb_of_sommet(at_prod),sum=g.is_symb_of_sommet(at_plus);
    if((!product && !sum) || v.empty() || v.size()>16)return false;
    degree=product?0:-129;leading=product?1:0;
    for(unsigned i=0;i<v.size();++i){
      if(is_zero(v[i])){if(product)return false;continue;}
      int d;gen c;if(!integration_rational_tail(v[i],x,d,c,budget,depth+1,contextptr))return false;
      if(product){degree+=d;leading=leading*c;}
      else if(d>degree){degree=d;leading=c;}
      else if(d==degree)leading+=c;
      if(degree>128 || degree < -128 || !integration_resource_rational(leading))return false;
    }
    return !is_zero(leading);
  }

  // Exact, small expression families with proved real-domain convergence or
  // divergence. Never infer convergence merely from an odd integrand.
  // Keep these bounded dispatch paths compact in the calculator ROM.
  static bool integration_finite_poly_add(sparse_poly1 &out,const gen &c,int degree,unsigned &budget){
    if(!budget || degree>64)return false;--budget;
    if(is_zero(c))return true;
    for(unsigned i=0;i<out.size();++i)if(out[i].exponent==degree){out[i].coeff+=c;return true;}
    if(out.size()>=32)return false;out.push_back(monome(c,degree));return true;
  }

  static bool integration_finite_poly_product(const sparse_poly1 &left,const sparse_poly1 &right,sparse_poly1 &out,unsigned &budget){
    if(left.size()*right.size()>budget)return false;
    for(unsigned i=0;i<left.size();++i)for(unsigned j=0;j<right.size();++j){
      int degree=left[i].exponent.val+right[j].exponent.val;
      if(degree>64 || !integration_finite_poly_add(out,left[i].coeff*right[j].coeff,degree,budget))return false;
    }
    return true;
  }

  static bool integration_finite_poly_terms(const gen &g,const gen &x,sparse_poly1 &out,unsigned &budget,unsigned depth,GIAC_CONTEXT){
    if(!budget || depth>8)return false;--budget;
    if(integration_rational(g))return integration_finite_poly_add(out,g,0,budget);
    if(g==x)return integration_finite_poly_add(out,1,1,budget);
    if(g.is_symb_of_sommet(at_inv) && integration_rational(g._SYMBptr->feuille) && !is_zero(g._SYMBptr->feuille))
      return integration_finite_poly_add(out,gen(1)/g._SYMBptr->feuille,0,budget);
    if(g.type==_FRAC && integration_rational(g._FRACptr->den) && !is_zero(g._FRACptr->den)){
      sparse_poly1 p;if(!integration_finite_poly_terms(g._FRACptr->num,x,p,budget,depth+1,contextptr))return false;
      for(unsigned i=0;i<p.size();++i)if(!integration_finite_poly_add(out,p[i].coeff/g._FRACptr->den,p[i].exponent.val,budget))return false;return true;
    }
    if(g.is_symb_of_sommet(at_neg)){
      sparse_poly1 p;if(!integration_finite_poly_terms(g._SYMBptr->feuille,x,p,budget,depth+1,contextptr))return false;
      for(unsigned i=0;i<p.size();++i)if(!integration_finite_poly_add(out,-p[i].coeff,p[i].exponent.val,budget))return false;return true;
    }
    if(g.type!=_SYMB || g._SYMBptr->feuille.type!=_VECT)return false;
    const vecteur &v=*g._SYMBptr->feuille._VECTptr;
    if(g.is_symb_of_sommet(at_plus)){
      if(v.size()>32)return false;
      for(unsigned i=0;i<v.size();++i)if(!integration_finite_poly_terms(v[i],x,out,budget,depth+1,contextptr))return false;return true;
    }
    bool power=g.is_symb_of_sommet(at_pow);
    if(power && (v.size()!=2 || v[1].type!=_INT_ || v[1].val<0 || v[1].val>64))return false;
    if(!power && (!g.is_symb_of_sommet(at_prod) || v.size()>8))return false;
    sparse_poly1 result(1,monome(1,0)),base;
    if(power){
      if(!integration_finite_poly_terms(v[0],x,base,budget,depth+1,contextptr))return false;
      unsigned n=v[1].val;
      while(n){
        if(n&1){sparse_poly1 next;if(!integration_finite_poly_product(result,base,next,budget))return false;result.swap(next);}
        n>>=1;if(n){sparse_poly1 next;if(!integration_finite_poly_product(base,base,next,budget))return false;base.swap(next);}
      }
    }
    else for(unsigned i=0;i<v.size();++i){
      base.clear();if(!integration_finite_poly_terms(v[i],x,base,budget,depth+1,contextptr))return false;
      sparse_poly1 next;if(!integration_finite_poly_product(result,base,next,budget))return false;result.swap(next);
    }
    for(unsigned i=0;i<result.size();++i)if(!integration_finite_poly_add(out,result[i].coeff,result[i].exponent.val,budget))return false;return true;
  }

// A rational polynomial is continuous at every finite rational endpoint.
// Direct endpoint powers therefore replace generic assumptions and limits.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_finite_polynomial(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(!integration_rational(lo) || !integration_rational(hi) || taille(e,129)>128)return false;
    sparse_poly1 polynomial;unsigned budget=512;
    if(!integration_finite_poly_terms(e,x,polynomial,budget,0,contextptr))return false;
    gen answer=0;
    for(unsigned i=0;i<polynomial.size();++i){
      if(is_zero(polynomial[i].coeff))continue;
      int degree=polynomial[i].exponent.val+1;
      answer+=polynomial[i].coeff*(pow(hi,degree,contextptr)-pow(lo,degree,contextptr))/gen(degree);
    }
    res=answer;return true;
  }

  static bool integration_finite_elementary_bound(const gen &g,GIAC_CONTEXT){
    if(taille(g,33)>32)return false;
    if(g.is_symb_of_sommet(at_exp) && integration_rational(g._SYMBptr->feuille))return true;
    return integration_period_real_bound(g,contextptr);
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_finite_elementary(const gen &e,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(taille(e,97)>96 || !integration_finite_elementary_bound(lo,contextptr) || !integration_finite_elementary_bound(hi,contextptr))return false;
    gen f=e,c=integration_syntax(integration_coefficient(f,x,contextptr),contextptr);
    if(!integration_rational(c))return false;
    gen a,b,arg,lower,upper;
    gen square;
    if(integration_square_root(f,arg) && integration_power(arg,square,2)){
      gen u=square;
      if(u.is_symb_of_sommet(at_sin) || u.is_symb_of_sommet(at_cos))u=gen(u._SYMBptr->feuille);
      if(is_linear_wrt(u,x,a,b,contextptr) && integration_rational(a) && integration_finite_elementary_bound(b,contextptr))
        f=symbolic(at_abs,square);
    }
    if(f.is_symb_of_sommet(at_abs)){
      const gen &u=f._SYMBptr->feuille;
      if((u.is_symb_of_sommet(at_sin) || u.is_symb_of_sommet(at_cos)) && angle_radian(contextptr) &&
         is_linear_wrt(u._SYMBptr->feuille,x,a,b,contextptr) && integration_rational(a) && !is_zero(a) && integration_finite_elementary_bound(b,contextptr)){
        gen periods=ratnormal(a*(hi-lo)/cst_pi,contextptr);
        if(periods.type==_INT_){res=ratnormal(2*c*(hi-lo)/cst_pi,contextptr);return true;}
      }
    }
    bool sine=f.is_symb_of_sommet(at_sin),cosine=f.is_symb_of_sommet(at_cos),exponential=f.is_symb_of_sommet(at_exp),logarithm=f.is_symb_of_sommet(at_ln),arctangent=f.is_symb_of_sommet(at_atan),absolute=f.is_symb_of_sommet(at_abs);
    if(sine || cosine || exponential || logarithm || arctangent || absolute){
      if((sine || cosine || arctangent) && !angle_radian(contextptr))return false;
      if(!is_linear_wrt(f._SYMBptr->feuille,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) || !integration_finite_elementary_bound(b,contextptr))return false;
      lower=ratnormal(a*lo+b,contextptr);upper=ratnormal(a*hi+b,contextptr);
      if(sine)res=c*(cos(lower,contextptr)-cos(upper,contextptr))/a;
      else if(cosine)res=c*(sin(upper,contextptr)-sin(lower,contextptr))/a;
      else if(exponential)res=c*(exp(upper,contextptr)-exp(lower,contextptr))/a;
      else if(absolute)res=c*(upper*abs(upper,contextptr)-lower*abs(lower,contextptr))/(2*a);
      else if(arctangent)res=c*(upper*atan(upper,contextptr)-lower*atan(lower,contextptr)-ln((1+upper*upper)/(1+lower*lower),contextptr)/2)/a;
      else {
        if((!is_zero(lower) && !is_strictly_positive(lower,contextptr)) || (!is_zero(upper) && !is_strictly_positive(upper,contextptr)))return false;
        // u*ln(u) has real limit0 at the positive-side endpoint u=0.
        gen L=is_zero(lower)?gen(0):lower*ln(lower,contextptr)-lower;
        gen U=is_zero(upper)?gen(0):upper*ln(upper,contextptr)-upper;
        res=c*(U-L)/a;
      }
      return true;
    }
    // The positive quadratic kernel is continuous on every real interval.
    if(integration_power(f,arg,-1) && integration_quadratic(arg,x,a,b,contextptr) && is_strictly_positive(a,contextptr) && is_strictly_positive(b,contextptr)){
      if(!angle_radian(contextptr))return false;
      gen scale=sqrt(a/b,contextptr);
      res=c*(atan(scale*hi,contextptr)-atan(scale*lo,contextptr))/sqrt(a*b,contextptr);return true;
    }
    // Integer outer powers can be combined; if any wrapper is fractional,
    // the underlying affine base must stay nonnegative on the interval.
    arg=f;gen p=1;bool wrapped=false,fractional=false;
    for(unsigned depth=0;depth<8;++depth){
      if(arg.is_symb_of_sommet(at_inv)){p=-p;arg=gen(arg._SYMBptr->feuille);wrapped=true;continue;}
      if(arg.is_symb_of_sommet(at_sqrt)){p=p/2;arg=gen(arg._SYMBptr->feuille);wrapped=true;fractional=true;continue;}
      if(arg.is_symb_of_sommet(at_pow) && arg._SYMBptr->feuille.type==_VECT){
        const vecteur &v=*arg._SYMBptr->feuille._VECTptr;
        if(v.size()!=2 || !integration_rational(v[1]))return false;
        const gen &n=v[1].type==_FRAC?v[1]._FRACptr->num:v[1];
        if(n.type!=_INT_ || n.val < -64 || n.val>64 || (v[1].type==_FRAC && (v[1]._FRACptr->den.type!=_INT_ || v[1]._FRACptr->den.val>16)))return false;
        if(v[1].type!=_INT_)fractional=true;
        p=p*v[1];arg=gen(v[0]);wrapped=true;continue;
      }
      break;
    }
    if(!wrapped || !integration_rational(p) || !is_linear_wrt(arg,x,a,b,contextptr) || !integration_rational(a) || is_zero(a) || !integration_rational(b))return false;
    const gen &numerator=p.type==_FRAC?p._FRACptr->num:p;
    if(numerator.type!=_INT_ || numerator.val < -64 || numerator.val>64 || (p.type==_FRAC && (p._FRACptr->den.type!=_INT_ || p._FRACptr->den.val>16)))return false;
    lower=ratnormal(a*lo+b,contextptr);upper=ratnormal(a*hi+b,contextptr);
    bool lp=is_strictly_positive(lower,contextptr),up=is_strictly_positive(upper,contextptr);
    if(fractional && ((!lp && !is_zero(lower)) || (!up && !is_zero(upper))))return false;
    if(!is_strictly_positive(p+1,contextptr)){
      bool negative=is_strictly_positive(-lower,contextptr) && is_strictly_positive(-upper,contextptr);
      if(!(lp && up) && (fractional || !negative))return false;
    }
    if(p==-1)res=c*ln(abs(upper/lower,contextptr),contextptr)/a;
    else res=c*(pow(upper,p+1,contextptr)-pow(lower,p+1,contextptr))/(a*(p+1));
    return true;
  }

  // Complex parameter kernels on a real integration interval. Preserve
  // unproved convergence as an explicit condition, rather than treating
  // analytic continuation as the value of an ordinary improper integral.
#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_parameter_kernel(const gen &input,const gen &x,const gen &lo,const gen &hi,gen &res,GIAC_CONTEXT){
    if(taille(input,97)>96)return false;
    gen e=input,coefficient=integration_coefficient(e,x,contextptr);
    if(contains(coefficient,x) || taille(coefficient,17)>16 || is_zero(coefficient))return false;
    if(is_zero(lo) && hi==plus_inf && e.is_symb_of_sommet(at_plus) && e._SYMBptr->feuille.type==_VECT){
      const vecteur &terms=*e._SYMBptr->feuille._VECTptr;
      if(terms.size()>4)return false;
      gen sum=0;bool complex_rate=false;
      for(unsigned i=0;i<terms.size();++i){
        gen term=terms[i],c=integration_coefficient(term,x,contextptr),a,b;
        if(!term.is_symb_of_sommet(at_exp) || !is_linear_wrt(term._SYMBptr->feuille,x,a,b,contextptr) || !is_zero(b) ||
           contains(a,x) || contains(c,x) || taille(a,17)>16 || taille(c,17)>16)return false;
        gen realrate=re(-a,contextptr);
        if(contains(realrate,*at_re) || !is_strictly_positive(realrate,contextptr))return false;
        complex_rate=complex_rate || !is_zero(im(a,contextptr));sum-=c/a;
      }
      if(!complex_rate)return false;
      res=ratnormal(coefficient*sum,contextptr);return true;
    }
    vecteur factors=e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT?*e._SYMBptr->feuille._VECTptr:vecteur(1,e);
    if(factors.size()>2)return false;
    gen shape=1,rate=0,den,answer,condition=1,fallback=undef;
    vecteur required;bool power=false,exponential=false,inverse=false,trig=false,sine=false;gen frequency;
    if(is_zero(lo) && hi==plus_inf){
      for(unsigned i=0;i<factors.size();++i){
        gen base;const gen &f=factors[i];
        if(f.is_symb_of_sommet(at_exp)){
          gen a,b;
          if(exponential || !is_linear_wrt(f._SYMBptr->feuille,x,a,b,contextptr) || !is_zero(b) || contains(a,x))return false;
          exponential=true;rate=-a;
        }
        else if(f.is_symb_of_sommet(at_sin) || f.is_symb_of_sommet(at_cos)){
          gen phase;
          if(trig || !angle_radian(contextptr) || !is_linear_wrt(f._SYMBptr->feuille,x,frequency,phase,contextptr) ||
             !is_zero(phase) || contains(frequency,x) || !is_zero(im(frequency,contextptr)))return false;
          trig=true;sine=f.is_symb_of_sommet(at_sin);
        }
        else if(integration_power(f,base,-1) && base.is_symb_of_sommet(at_plus)){
          if(inverse)return false;inverse=true;den=base;
        }
        else if(f.is_symb_of_sommet(at_pow) && f._SYMBptr->feuille.type==_VECT && f._SYMBptr->feuille._VECTptr->size()==2 && f._SYMBptr->feuille[0]==x){
          if(power || contains(f._SYMBptr->feuille[1],x))return false;power=true;shape=ratnormal(f._SYMBptr->feuille[1]+1,contextptr);
        }
        else if(f==x){if(power)return false;power=true;shape=2;}
        else return false;
      }
      if(taille(shape,17)>16 || taille(rate,17)>16)return false;
      // Leave the existing bounded rational-parameter families in charge of
      // their established exact and high-power behavior.
      if(integration_rational(shape) && integration_rational(rate))return false;
      if(exponential && !inverse){
        gen realrate=re(rate,contextptr);
        required.push_back(realrate);
        if(trig){
          if(power)return false;
          answer=(sine?frequency:rate)/(rate*rate+frequency*frequency);
        }
        else if(!power){answer=inv(rate,contextptr);}
        else {
          required.push_back(re(shape,contextptr));
          answer=symbolic(at_Gamma,shape)/(is_one(rate)?gen(1):gen(symbolic(at_pow,makesequence(rate,shape))));
          // Purely imaginary rates can have conditionally convergent Fourier
          // integrals. Outside the right half-plane retain that unsolved case.
          if(contains(realrate,*at_re) || !is_strictly_positive(realrate,contextptr))
            fallback=symb_quote(symbolic(at_integrate,makesequence(input,x,lo,hi)));
        }
      }
      else if(inverse && !exponential && !trig){
        if(!angle_radian(contextptr))return false;
        gen other,c,q;
        if(!integration_one_plus(den,other) || !integration_monomial(other,x,c,q,contextptr) || !is_one(c) ||
           !integration_resource_rational(q) || !is_strictly_positive(q,contextptr) || is_strictly_greater(q,16,contextptr))return false;
        if(q.type==_FRAC && (q._FRACptr->den.type!=_INT_ || q._FRACptr->den.val>16))return false;
        required.push_back(re(shape,contextptr));required.push_back(q-re(shape,contextptr));
        answer=cst_pi/(q*sin(cst_pi*shape/q,contextptr));
      }
      else return false;
    }
    else {
      if(factors.size()!=2 || !integration_resource_rational(lo) || !integration_resource_rational(hi) || !is_strictly_positive(hi-lo,contextptr))return false;
      gen alpha=0,beta=0,left=0,right=0;
      for(unsigned i=0;i<2;++i){
        const gen &f=factors[i];gen base=f,exponent=1;
        if(f.is_symb_of_sommet(at_pow) && f._SYMBptr->feuille.type==_VECT && f._SYMBptr->feuille._VECTptr->size()==2){base=f._SYMBptr->feuille[0];exponent=f._SYMBptr->feuille[1];}
        gen a,b;
        if(contains(exponent,x) || taille(exponent,17)>16 || !is_linear_wrt(base,x,a,b,contextptr) || !integration_resource_rational(a) || !integration_resource_rational(b))return false;
        if(is_strictly_positive(a,contextptr) && is_zero(a*lo+b) && is_zero(left)){left=a*(hi-lo);alpha=ratnormal(exponent+1,contextptr);}
        else if(is_strictly_positive(-a,contextptr) && is_zero(a*hi+b) && is_zero(right)){right=-a*(hi-lo);beta=ratnormal(exponent+1,contextptr);}
        else return false;
      }
      if(is_zero(left) || is_zero(right) || (integration_rational(alpha) && integration_rational(beta)))return false;
      required.push_back(re(alpha,contextptr));required.push_back(re(beta,contextptr));
      gen leftscale=is_one(left)?gen(1):gen(symbolic(at_pow,makesequence(left,alpha-1)));
      gen rightscale=is_one(right)?gen(1):gen(symbolic(at_pow,makesequence(right,beta-1)));
      answer=(hi-lo)*leftscale*rightscale*symbolic(at_Gamma,alpha)*symbolic(at_Gamma,beta)/symbolic(at_Gamma,alpha+beta);
    }
    for(unsigned i=0;i<required.size();++i){
      const gen &v=required[i];
      bool unknown_real_part=contains(v,*at_re);
      if(!unknown_real_part && is_strictly_positive(v,contextptr))continue;
      if(!unknown_real_part && (is_zero(v) || is_strictly_positive(-v,contextptr))){
        if(is_undef(fallback) || i>0){*logptr(contextptr)<<"Divergent improper integral (parameter convergence boundary)"<<'\n';res=undef;return true;}
        return false;
      }
      gen next=symb_superieur_strict(v,0);condition=is_one(condition)?next:symb_and(condition,next);
    }
    answer=coefficient*answer;
    res=is_one(condition)?answer:gen(symbolic(at_when,makesequence(condition,answer,fallback)));
    return true;
  }

#if defined(__GNUC__) && !defined(__clang__)
  __attribute__((noinline,optimize("Os")))
#endif
  static bool integrate_real_definite(const gen &input,const gen &x,gen lo,gen hi,gen &res,GIAC_CONTEXT){
    if (taille(input,129)>128) return false;
    bool reverse=(lo==plus_inf || hi==minus_inf) ||
      (!is_inf(lo) && !is_inf(hi) && is_strictly_greater(lo,hi,contextptr));
    if (reverse) swapgen(lo,hi);
    bool full=lo==minus_inf && hi==plus_inf;
    gen e=integration_syntax(input,contextptr);
    if(integrate_parameter_kernel(e,x,lo,hi,res,contextptr)){if(reverse)res=-res;return true;}
    if(complex_variables(contextptr))return false;
    if(full){
      unsigned budget=128;int degree;gen leading;
      if(integration_rational_tail(e,x,degree,leading,budget,0,contextptr) && degree==-1){
        *logptr(contextptr)<<"Divergent improper integral (nonzero 1/x tail)"<<'\n';
        res=undef;return true;
      }
    }
    if(integrate_finite_polynomial(e,x,lo,hi,res,contextptr) ||
       integrate_finite_elementary(e,x,lo,hi,res,contextptr)){
      if(reverse)res=-res;return true;
    }
    gen c=integration_syntax(integration_coefficient(e,x,contextptr),contextptr);
    if (!integration_rational(c) || is_zero(c)) return false;
    // Nondecaying real oscillations have no ordinary improper limit.
    if((is_inf(lo) || is_inf(hi)) && angle_radian(contextptr) &&
       (e.is_symb_of_sommet(at_cos) || e.is_symb_of_sommet(at_sin))){
      gen a,b;
      if(is_linear_wrt(e._SYMBptr->feuille,x,a,b,contextptr) && integration_resource_rational(a) && !is_zero(a) && integration_period_real_bound(b,contextptr)){
        *logptr(contextptr)<<"Divergent improper integral (nondecaying oscillation)"<<'\n';res=undef;return true;
      }
    }
    gen answer;
    if (integrate_compact_definite(e,x,lo,hi,answer,contextptr) ||
        (full && integrate_residue_kernel(e,x,answer,contextptr))){
      res=(reverse?-c:c)*answer;return true;
    }
    vecteur factors;
    if (e.is_symb_of_sommet(at_prod) && e._SYMBptr->feuille.type==_VECT) factors=*e._SYMBptr->feuille._VECTptr;
    else factors.push_back(e);
    if (factors.size()>3) return false;
    // exp(-a*x^2)/(1+exp(k*x)), exp(-a/x^2)/x^2, exp(q*x^2)/x^2.
    if (factors.size()==2){
      unsigned ei=factors[0].is_symb_of_sommet(at_exp)?0:1;
      if (factors[ei].is_symb_of_sommet(at_exp)){
        gen exponent=factors[ei]._SYMBptr->feuille;
        gen q=integration_coefficient(exponent,x,contextptr),base,den,h,a,b;
        if (integration_rational(q)){
          bool square=integration_power(exponent,base,2) && base==x;
          if (full && square && is_strictly_positive(-q,contextptr) &&
              integration_power(factors[1-ei],den,-1) && integration_one_plus(den,h) &&
              h.is_symb_of_sommet(at_exp) && is_linear_wrt(h._SYMBptr->feuille,x,a,b,contextptr) &&
              integration_rational(a) && is_zero(b)){
            res=(reverse?-c:c)*sqrt(cst_pi/(-q),contextptr)/2;return true;
          }
          if (integration_power(factors[1-ei],base,-2) && base==x && is_zero(lo) && hi==plus_inf){
            if (integration_power(exponent,base,-2) && base==x && is_strictly_positive(-q,contextptr)){
              res=(reverse?-c:c)*sqrt(cst_pi/(-q),contextptr)/2;return true;
            }
            if (square){res=(reverse?-c:c)*plus_inf;return true;}
          }
        }
      }
    }
    // 1/(x^2*ln(x)) on [1,b], b>1: a positive
    // endpoint pole. Bounds below 1 or across that pole are not this rule.
    if (factors.size()==2 && is_one(lo) && (hi==plus_inf ||
        (integration_rational(hi) && is_strictly_greater(hi,1,contextptr)))){
      for (unsigned i=0;i<2;++i){
        gen base;
        if (!integration_power(factors[i],base,-1) || !base.is_symb_of_sommet(at_ln) || base._SYMBptr->feuille!=x) continue;
        if (integration_power(factors[1-i],base,-2) && base==x){res=(reverse?-c:c)*plus_inf;return true;}
      }
    }
    if (!angle_radian(contextptr) || is_inf(lo) || is_inf(hi)) return false;
    // Reflection tan(u) -> 1/tan(u) on exactly one positive quadrant.
    gen den,h;
    if (integration_power(e,den,-1) && integration_one_plus(den,h) &&
        h.is_symb_of_sommet(at_pow) && h._SYMBptr->feuille.type==_VECT){
      const vecteur &p=*h._SYMBptr->feuille._VECTptr;
      gen a,b;
      if (p.size()==2 && p[0].is_symb_of_sommet(at_tan) && lidnt(p[1]).empty() &&
          is_zero(im(p[1],contextptr)) && !is_inf(p[1]) && !is_undef(p[1]) &&
          evalf_double(p[1],1,contextptr).type==_DOUBLE_ &&
          is_linear_wrt(p[0]._SYMBptr->feuille,x,a,b,contextptr) && integration_rational(a) && integration_rational(b) && !is_zero(a)){
        gen l=ratnormal(a*lo+b,contextptr),r=ratnormal(a*hi+b,contextptr);
        if ((is_zero(l) && is_zero(ratnormal(r-cst_pi/2,contextptr))) ||
            (is_zero(r) && is_zero(ratnormal(l-cst_pi/2,contextptr)))){
          res=(reverse?-c:c)*(hi-lo)/2;return true;
        }
      }
    }
    // u -> (1-u)/(1+u) preserves du/(1+u^2) up to orientation,
    // and log(1+u)+log(1+(1-u)/(1+u))=log(2).
    if (factors.size()==2){
      unsigned li=factors[0].is_symb_of_sommet(at_ln)?0:1;
      gen u,a,b;
      if (factors[li].is_symb_of_sommet(at_ln) && integration_power(factors[1-li],den,-1) &&
          integration_one_plus(den,h) && integration_power(h,u,2) &&
          is_linear_wrt(u,x,a,b,contextptr) && integration_rational(a) && integration_rational(b) && !is_zero(a)){
        gen l=ratnormal(a*lo+b,contextptr),r=ratnormal(a*hi+b,contextptr);
        if ((is_zero(l) && is_one(r)) || (is_one(l) && is_zero(r))){
          gen ka,kb;
          if (!is_linear_wrt(factors[li]._SYMBptr->feuille,x,ka,kb,contextptr) ||
              !integration_rational(ka) || !integration_rational(kb)) return false;
          gen k=ka/a;
          if (kb==k*(1+b) && is_strictly_positive(k,contextptr)){
            res=(reverse?-c:c)*(r-l)/a*cst_pi*(ln(k,contextptr)/4+ln(gen(2),contextptr)/8);return true;
          }
        }
      }
    }
    return false;
  }

  // intmode bit 0 is used for sqrt int control, bit 1 control step/step info
  // bit 2 = 1 to avoid Risch call
  gen integrate_id_rem(const gen & e_orig,const gen & gen_x,gen & remains_to_integrate,GIAC_CONTEXT,int intmode){
#if defined(FXCG) || defined(KHICAS_TEST_INTEGRATION_LIMITS)
    integration_guard guard(e_orig,gen_x,intmode,contextptr);
    if (!guard.allowed){
      remains_to_integrate=e_orig;
      return 0; // leave this part unevaluated rather than exhaust the stack
    }
#endif
#ifdef TIMEOUT
    control_c();
#endif
    if (ctrl_c || interrupted){
      remains_to_integrate=undef;
      return undef;
    }
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate id_rem ") << e_orig << '\n';
#endif
    remains_to_integrate=0;
    gen e(e_orig);
    // A bare logarithm of an affine sine/cosine has no elementary
    // antiderivative. Integration by parts only replaces it with a larger
    // unresolved integral, which then sends simplify through trig expansion.
    // Keep the original atom; definite log-trig rules run before this path.
    if (e.is_symb_of_sommet(at_ln) && !angle_mode(contextptr)){
      gen trig=e._SYMBptr->feuille,a,b;
      if (trig.is_symb_of_sommet(at_abs)) trig=gen(trig._SYMBptr->feuille);
      if ((trig.is_symb_of_sommet(at_sin) || trig.is_symb_of_sommet(at_cos)) &&
          is_linear_wrt(trig._SYMBptr->feuille,gen_x,a,b,contextptr) &&
          integration_rational(a) && !is_zero(a)){
        remains_to_integrate=e;
        return 0;
      }
    }
    gen power_primitive;
    if (integrate_large_power(e,gen_x,power_primitive,contextptr) ||
        integrate_sparse_atan(e,gen_x,power_primitive,contextptr))
      return power_primitive;
    if (integrate_compact_primitive(e,gen_x,power_primitive,contextptr) ||
        integrate_real_root(e,gen_x,power_primitive,intmode,contextptr))
      return power_primitive;
    // Additional check: atan/asin in degree/grad
    if (angle_mode(contextptr)){
      if (has_op(e,*at_asin)|| has_op(e,*at_atan) || has_op(e,*at_acos))
	return undeferr("Inverse trigonometric functions are supported in radian mode only.");
    }
    // Step -3: replace when by piecewise
    e=when2piecewise(e,contextptr);
    e=Heavisidetopiecewise(e,contextptr); // e=Heavisidetosign(e,contextptr);
    if (is_constant_wrt(e,gen_x,contextptr) && lop(e,at_sign).empty())
      return e*gen_x;
    if (e.type!=_SYMB) {
      remains_to_integrate=zero;
      if (e==gen_x)
	return rdiv(pow(e,2),plus_two,contextptr);
      else
	return e*gen_x;
    }
    // Step -2: piecewise
    vecteur lpiece(lop(e,at_piecewise));
    if (!lpiece.empty()) lpiece=lvarx(lpiece,gen_x);
    if (!lpiece.empty()){
      *logptr(contextptr) << gettext("Warning: piecewise indefinite integration does not return a continuous antiderivative") << '\n';
      gen piece=lpiece.front();
      if (piece.is_symb_of_sommet(at_piecewise))
	return integrate_piecewise(e,piece,gen_x,remains_to_integrate,contextptr,intmode);
    }
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate step -2 ") << e << '\n';
#endif
    // Step -1: replace ifte(a,b,c) by b+sign(a==0)*(c-b)
    // if a is A1>A2 or A1>=A2 condition, the sign(a==0) is replaced by (sign(A2-A1)+1)/2
    if (!when2sign(e,gen_x,contextptr))
      return gensizeerr(gettext("Bad when ")+e.print(contextptr));
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate step 0 ") << e << '\n';
#endif
    // Step 0: replace abs(var_dep_x) with sign*var_dep_x
    // and then sign() with a constant
    gen res;
    vecteur l1(lop(e,at_abs)),m1(lop(e,at_sign));
    if (!l1.empty()) l1=lvarx(l1,gen_x); 
    if (!m1.empty()) m1=lvarx(m1,gen_x);
    if (!l1.empty() || !m1.empty()){
      if (integrate_step0(e,gen_x,l1,m1,res,remains_to_integrate,contextptr,intmode))
	return res;
    }
    // Step1: detection of some unary_op[linear fcn]
    if (e.is_symb_of_sommet(at_inv) && e._SYMBptr->feuille.is_symb_of_sommet(at_pow)){
      gen f=e._SYMBptr->feuille._SYMBptr->feuille;
      if (f.type==_VECT && f._VECTptr->size()==2){
	gen b=f._VECTptr->back();
	if (!is_integer(b) && b.type!=_FRAC)
	  e=symbolic(at_pow,makevecteur(f._VECTptr->front(),-b));
      }
    }
    unary_function_ptr u=e._SYMBptr->sommet;
    gen f=e._SYMBptr->feuille,a,b;
    // particular case for ^, _FUNCnd arg must be constant
    if ( ((intmode & 4)==0) && u==at_pow && f[0].is_symb_of_sommet(at_pow)){
      e=symbolic(at_pow,makesequence(f[0]._SYMBptr->feuille[0],f[0]._SYMBptr->feuille[1]*f[1]));
      return integrate_id_rem(e,gen_x,remains_to_integrate,contextptr,intmode);
    }
    if ( (u==at_pow) && is_constant_wrt(f._VECTptr->back(),gen_x,contextptr) && is_linear_wrt(f._VECTptr->front(),gen_x,a,b,contextptr) ){
      if ( (intmode & 2)==0)
	gprintf(step_linear,gettext("Integrate %gen, a linear expression u=%gen to a constant power n=%gen,\nIf n=-1 then ln(u)/a else u^(n+1)/((n+1)*%gen)"),makevecteur(e,a*gen_x+b,f._VECTptr->back(),a),contextptr);
      b=f._VECTptr->back();
      if (is_minus_one(b))
	return rdiv(lnabs(f._VECTptr->front(),contextptr),a,contextptr);
      return rdiv(pow(f._VECTptr->front(),b+plus_one,contextptr),a*(b+plus_one),contextptr);
    }
    if ( (u==at_surd) && is_constant_wrt(f._VECTptr->back(),gen_x,contextptr) && is_linear_wrt(f._VECTptr->front(),gen_x,a,b,contextptr) ){
      if ( (intmode & 2)==0)
	gprintf(step_linear,gettext("Integrate %gen, a linear expression u=%gen to a constant power n=1/%gen,\nIf n=-1 then ln(u)/a else u^(n+1)/((n+1)*%gen)"),makevecteur(e,a*gen_x+b,f._VECTptr->front(),a),contextptr);
      b=f._VECTptr->back();
      if (is_minus_one(b))
	return rdiv(lnabs(f._VECTptr->front(),contextptr),a,contextptr);
      return f._VECTptr->front()*symbolic(at_surd,f)/(a+a/b);
    }
    if ( (u==at_NTHROOT) && is_constant_wrt(f._VECTptr->front(),gen_x,contextptr) && is_linear_wrt(f._VECTptr->back(),gen_x,a,b,contextptr) ){
      if ( (intmode & 2)==0)
	gprintf(step_linear,gettext("Integrate %gen, a linear expression u=%gen to a constant power n=1/%gen,\nIf n=-1 then ln(u)/a else u^(n+1)/((n+1)*%gen)"),makevecteur(e,a*gen_x+b,f._VECTptr->front(),a),contextptr);
      b=f._VECTptr->front();
      if (is_minus_one(b))
	return rdiv(lnabs(f._VECTptr->back(),contextptr),a,contextptr);
      return f._VECTptr->back()*symbolic(at_NTHROOT,f)/(a+a/b);
    }
#if 1 // ndef EMCC // re-enabled Aug. 2015 for integrate(1/surd(x^2,3),x,-1,1)
    if (has_op(e,*at_surd) || has_op(e,*at_NTHROOT)){
      vecteur subst1,subst2;
      surd2pow(e,subst1,subst2,contextptr);
      gen g=subst(e,subst1,subst2,false,contextptr);
      g=integrate_id_rem(g,gen_x,remains_to_integrate,contextptr,intmode | 4);
      remains_to_integrate=subst(remains_to_integrate,subst2,subst1,false,contextptr);
      g=subst(g,subst2,subst1,false,contextptr);
      return g;
    }
#endif
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate step 1 ") << e << '\n';
#endif
    if (u==at_sum && f.type==_VECT && f._VECTptr->size()==4){
      vecteur & fv=*f._VECTptr;
      if (!is_zero(derive(fv[1],gen_x,contextptr)))
	return gensizeerr("Mute variable of sum depends on integration variable");
      if (!is_zero(derive(fv[2],gen_x,contextptr)) || !is_zero(derive(fv[3],gen_x,contextptr)) )
	return gensizeerr("Boundaries of sum depends on integration variables");
      if (is_inf(fv[2])||is_inf(fv[3]))
	*logptr(contextptr) << "Warning: assuming integration and sum commutes" << '\n';
      gen res=integrate_id_rem(fv[0],gen_x,remains_to_integrate,contextptr,intmode);
      res=_sum(makesequence(res,fv[1],fv[2],fv[3]),contextptr);
      if (!is_zero(remains_to_integrate))
	remains_to_integrate=_sum(makesequence(remains_to_integrate,fv[1],fv[2],fv[3]),contextptr);
      return res;
    }
    // unary op only
    int s=equalposcomp(primitive_tab_op,u);
    if (s && is_linear_wrt(f,gen_x,a,b,contextptr) ){
      if ( (intmode & 2)==0)
	gprintf(step_funclinear,gettext("Integrate %gen: function %gen applied to a linear expression u=%gen, result %gen"),makevecteur(e,primitive_tab_op[s-1],a*gen_x+b,primitive_tab_primitive[s-1](a*gen_x+b,contextptr)/a),contextptr);      
      return rdiv(primitive_tab_primitive[s-1](f,contextptr),a,contextptr);
    }
    if (u==at_of && f.type==_VECT && f._VECTptr->size()==2 && is_linear_wrt(f._VECTptr->back(),gen_x,a,b,contextptr)){
      gen f0=f[0];
      if (f0.is_symb_of_sommet(at_function_diff) && f0._SYMBptr->feuille.type!=_VECT)
        return symb_of(f0._SYMBptr->feuille,f[1])/a;
    }
    // Step2: detection of f(u)*u' 
    vecteur v(1,gen_x);
    rlvarx(e,gen_x,v);
    // detect constants and gcd for linear args
    gen curgcd(v.size()<=2?1:0); bool allsame=true;
    for (int i=1;i<v.size();++i){
      if (v[i].type!=_SYMB)
	continue;
      gen vf=v[i]._SYMBptr->feuille;
      gen vf1=derive(vf,gen_x,contextptr);
      vf1=ratnormal(vf1,contextptr);
      if (is_zero(vf1) && gen_x.type==_IDNT){
	vf=limit(vf,*gen_x._IDNTptr,0,1,contextptr);
	if (!is_undef(vf)){
	  gen e1=complex_subst(e,v[i],v[i]._SYMBptr->sommet(vf,contextptr),contextptr);
	  vecteur w(1,gen_x);
	  rlvarx(e1,gen_x,w);
	  if (w.size()<v.size()){
	    v=w;
	    e=e1;
	  }
	}
      }
      if (!is_constant_wrt(vf1,gen_x,contextptr)){
	curgcd=1;
	break;
      }
      if (vf1.type==_VECT) 
	vf1=_gcd(vf1,contextptr);
      if (curgcd!=0 && vf1!=curgcd){
	allsame=false;
      }
      curgcd=gcd(vf1,curgcd);
    }
    if (!allsame && curgcd==1){ // translate if exists b!=0 with f(x+b)
      for (int i=1;i<v.size();++i){
	if (v[i].type!=_SYMB)
	  continue;
	gen vf=v[i]._SYMBptr->feuille,a,b;
	if (is_linear_wrt(vf,gen_x,a,b,contextptr) && (a==1||a==-1)){ 
	  if (b==0) break; // 
	  gen e1=complex_subst(e,gen_x,a*(gen_x-b),contextptr);
	  gen E1=integrate_id_rem(e1,gen_x,remains_to_integrate,contextptr,intmode);
	  remains_to_integrate=complex_subst(remains_to_integrate,gen_x,a*gen_x+b,contextptr);
	  E1=complex_subst(E1,gen_x,a*gen_x+b,contextptr);
	  return a*E1/curgcd;
	}
      }
    }
    if (!allsame && curgcd!=0 && curgcd!=1){
      gen e1=complex_subst(e,gen_x,inv(curgcd,contextptr)*gen_x,contextptr);
      v=vecteur(1,gen_x);
      rlvarx(e1,gen_x,v);
      vecteur vrep(v);
      for (int i=1;i<v.size();++i){
	if (v[i].type!=_SYMB)
	  continue;
	gen vf=expand(ratnormal(v[i]._SYMBptr->feuille,contextptr),contextptr);
	vrep[i]=symbolic(v[i]._SYMBptr->sommet,vf);
      }
      if (v!=vrep) e1=complex_subst(e1,v,vrep,contextptr);
      gen E1=integrate_id_rem(e1,gen_x,remains_to_integrate,contextptr,intmode);
      remains_to_integrate=complex_subst(remains_to_integrate,gen_x,curgcd*gen_x,contextptr);
      E1=complex_subst(E1,gen_x,curgcd*gen_x,contextptr);
      return E1/curgcd;
    }
    if (!lop(v,at_rootof).empty()){
      remains_to_integrate=e_orig;
      return 0;
    }
    int rvarsize=int(v.size());
    if (rvarsize>1){
      gen e2=_texpand(e,contextptr);
      if (is_undef(e2))
	e2=e;
      vecteur v2(1,gen_x);
      rlvarx(e2,gen_x,v2);
      if (v2.size()<rvarsize){
	e=e2;
	v=v2;
	rvarsize=int(v2.size());
      }
      v2=lop(lvar(e),*at_pow);
      if (v2.size()==1){
        // code added for integrate(x^n*ln(x))
        gen v20=v2.front()._SYMBptr->feuille;
        if (!lvar(v20[1]).empty()){
          e2=ratnormal(powexpand(e,contextptr),contextptr);
          v2.clear();
          rlvarx(e2,gen_x,v2);
          if (lvar(e2).size()<lvar(e).size()){
            e=e2;
            v=v2;
            rvarsize=int(v2.size());
          }
        }
      }
      if (rvarsize==2){
	e2=simplifier(e,contextptr);
	v2.clear();
	v2.push_back(gen_x);
	rlvarx(e2,gen_x,v2);
	if (v2.size()==1 || (v2.size()==2 && taille(v2[1],100)<taille(v[1],100))){
	  e=e2;
	  v=v2;
	}
      }
    }
    // gen_sort_f(v.begin(),v.end(),islesscomplexthanf);      
    vecteur rvar=v;
    gen fu,fx;
    bool chkevenodd=true; int evenodd=-1;
    if (rvarsize<=TRY_FU_UPRIME){ // otherwise no hope
      const_iterateur it=v.begin()+1,itend=v.end();
      for (int pos=1;pos<=v.size();++it,++pos){
        //confirm("integrate fuu'",it->print().c_str());
	gen u;
	if (pos==v.size()){
	  if (!chkevenodd || rvar.size()==1)
	    break;
	  if (evenodd==-1)
	    evenodd=is_even_odd(e,gen_x,contextptr); 
	  if (evenodd!=2)
	    break;
	  u=pow(gen_x,2);
	}
	else
	  u=*it;
	if (u.is_symb_of_sommet(at_fsolve) || u.is_symb_of_sommet(at_equal) || u.is_symb_of_sommet(at_of))
	  continue;
	if (!u.is_symb_of_sommet(at_pow))
	  chkevenodd=false;
	u=lvar_ratnormal(u,contextptr);
	gen df=derive(u,gen_x,contextptr);
	gen tmprem;
	fu=rdiv(e,df,contextptr);
	{
	  gen e2=_texpand(ratnormal(fu,contextptr),contextptr);
          //confirm("integrate texpand",e2.print().c_str());
	  if (!is_undef(e2)){
	    vecteur v2=lvarx(e2,gen_x),vf=lvarx(fu,gen_x); 
	    if (v2.size()<vf.size())
	      fu=e2;
	  }
	}
	fu=ratnormal(recursive_ratnormal(fu,contextptr),contextptr);
	fu=eval(fu,1,contextptr);
        //confirm("integrate fu",fu.print().c_str());
	if ((is_undef(fu) || is_inf(fu)) && is_zero(ratnormal(df,contextptr))){
	  // u is constant -> find the value
	  tmprem=subst(u,gen_x,zero,false,contextptr);
	  e=subst(e,u,tmprem,false,contextptr);
	  return integrate_id_rem(e,gen_x,remains_to_integrate,contextptr,intmode | 2);
	}
	if (is_undef(fu) || is_inf(fu))
	  continue;
	bool issin=u.is_symb_of_sommet(at_sin),iscos=u.is_symb_of_sommet(at_cos);
        // crash trigsin( (-cos(x)^2*sin(x)+sin(x))^(1/3)*4^(1/3) )
	if (iscos)
	  fu=_trigcos(tan2sincos(fu,contextptr),contextptr);
	if (issin)
	  fu=_trigsin(tan2sincos(fu,contextptr),contextptr);
        // confirm("integrate trigsin",fu.print().c_str());
	if (u.is_symb_of_sommet(at_tan))
	  fu=_trigtan(fu,contextptr);
	if (u.is_symb_of_sommet(at_atan) 
	    // ?additional check with contains to avoid recursion in int by part
	    && !equalposcomp(lvar(e),u) 	  
	    ){
	  // ? change of variable with argument of atan
	  gen argatan=u._SYMBptr->feuille,a,b;
	  if (is_linear_wrt(argatan,gen_x,a,b,contextptr)){
	    // t=atan(a*gen_x+b), gen_x=(tan(t)-b)/a
	    gen ck=subst(fu,makevecteur(u,gen_x,sqrt(pow(a*gen_x+b,2)+1,contextptr)),makevecteur(gen_x,(symbolic(at_sin,gen_x)/symbolic(at_cos,gen_x)-b)/a,symb_inv(symb_cos(gen_x))),false,contextptr);
	    // gen xval=assumeeval(gen_x,contextptr);
	    // giac_assume(symb_and(symb_superieur_egal(gen_x,0),symb_inferieur_egal(gen_x,cst_pi_over_2)),contextptr);
	    ck=linear_integrate_nostep(ck,gen_x,tmprem,intmode,contextptr);
	    // restorepurge(xval,gen_x,contextptr);
	    if (is_zero(tmprem)){
	      ck=complex_subst(ck,gen_x,u,contextptr);
	      return ck;
	    }
	  }
	}
	bool tst=is_rewritable_as_f_of(fu,u,fx,gen_x,contextptr);
	if (tst && (issin || iscos)){
	  gen fx1=ratnormal(fx/2/gen_x),fx2,fx21;
	  if (is_rewritable_as_f_of(fx1,pow(gen_x,2),fx2,gen_x,contextptr)){ // attempt with cos^2 or sin^2
	    fx2=recursive_ratnormal(fx2,contextptr);
	    gen fx21=recursive_ratnormal(subst(-fx2,gen_x,1-gen_x,false,contextptr),contextptr);
	    int t1=taille(fx2,256),t2=taille(fx21,256);
	    if (t2<0.7*t1){
	      if (issin) u=symb_cos(u._SYMBptr->feuille); else u=symb_sin(u._SYMBptr->feuille);
	      fx2=fx21;
	    }
	    u=pow(u,2); fx=fx2;
	  }
	}
	if (tst){
	  if (taille(fx,256)>taille(e,255)){
	    vecteur fxv=lvarx(fx,gen_x);
	    if (has_op(fxv,*at_ln) || has_op(fxv,*at_atan))
	      tst=false;
	  }
	}
	if (tst){
	  if ( (intmode & 2)==0)
	    gprintf(step_fuuprime,gettext("Integration of %gen: f(u)*u' where f=%gen->%gen and u=%gen"),makevecteur(e,gen_x,fx,u),contextptr);
#if 0
	  // no abs, for integrate(cot(ln(x))/x,x), but has side effect...
	  // would be better to add implicit assumptions
	  bool save_do_lnabs=do_lnabs(contextptr);
	  do_lnabs(false,contextptr);
	  e=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	  do_lnabs(save_do_lnabs,contextptr);
	  remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,*it,contextptr)*df;
	  e=complex_subst(e,gen_x,u,contextptr);
	  if (save_do_lnabs){
	    vector<const unary_function_ptr *> ln_tab(1,at_ln);
	    vector<gen_op_context> lnabs_tab(1,add_lnabs);
	    e=subst(e,ln_tab,lnabs_tab,true,contextptr);
	  }
	  return e;
#else
	  // ln() in integration should not be ln(abs()) if complex change of variable, example a:=-2/(2*i*exp(2*i*x)+2*i)*exp(2*i*x); b:=int(a); simplify(diff(b)-a);
	  bool b=do_lnabs(contextptr);
	  if (has_i(u)) do_lnabs(false,contextptr);
	  e=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	  do_lnabs(b,contextptr);
	  remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,u,contextptr)*df;
	  bool batan=atan_tan_no_floor(contextptr);
	  atan_tan_no_floor(true,contextptr);
	  e=complex_subst(e,gen_x,u,contextptr);
	  atan_tan_no_floor(batan,contextptr);
	  // additional check for integrals like
	  // int(sqrt (1+x^(-2/3)),x,-1,0)
	  if (u.is_symb_of_sommet(at_pow)){
	    gen powarg=u[1],powa,powb;
	    if (is_linear_wrt(powarg,gen_x,powa,powb,contextptr) && !is_zero(powa)){
	      gen powx=-powb/powa;
	      // check derivative at powx+-1
	      gen check=derive(e,gen_x,contextptr)/e_orig;
	      check=ratnormal(check,contextptr);
	      gen chkplus=subst(check,gen_x,powx+1.0,false,contextptr);
	      gen chkminus=subst(check,gen_x,powx-1.0,false,contextptr);
	      bool tstplus=is_zero(chkplus+1,contextptr);		
	      bool tstminus=is_zero(chkminus+1,contextptr);		
	      if (tstplus){
		if (tstminus)
		  e=-e;
		else
		  e=-sign(gen_x,contextptr)*e;
	      }
	      else {
		if (tstminus)
		  e=sign(gen_x,contextptr)*e;
	      }
	    }
	  }
	  if (!lop(lvarx(remains_to_integrate,gen_x),at_rootof).empty()){
	    remains_to_integrate=e_orig;
	    return 0;
	  }
	  return e;
#endif
	}
	if (pos<v.size() && u.is_symb_of_sommet(at_pow)){ // no check with u=gen_x^2
	  v[it-v.begin()]=powexpand(*it,contextptr);
	  gen uit=lvar_ratnormal(*it,contextptr);
	  bool tst=is_rewritable_as_f_of(powexpand(fu,contextptr),uit,fx,gen_x,contextptr);
	  if (tst){
	    if (taille(fx,256)>taille(e,255)){
	      vecteur fxv=lvarx(fx,gen_x);
	      if (has_op(fxv,*at_ln) || has_op(fxv,*at_atan))
		tst=false;
	    }
	  }
	  if (tst){
	    if ( (intmode & 2)==0)
	      gprintf(step_fuuprime,gettext("Integration of %gen: f(u)*u' where f=%gen->%gen and u=%gen"),makevecteur(e,gen_x,fx,uit),contextptr);
	    e=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	    remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,*it,contextptr)*df;
	    return complex_subst(e,gen_x,uit,contextptr);
	  }
	}
	if (u.type!=_SYMB)
	  continue;
	f=ratnormal(u._SYMBptr->feuille,contextptr); 
	// ratnormal added otherwise infinite recursion for int(1/sin(x^-1))
	if ( (f.type==_VECT) && (!f._VECTptr->empty()) )
	  f=f._VECTptr->front();
	if (f.type!=_SYMB)
	  continue;
	if (is_linear_wrt(f,gen_x,a,b,contextptr))
	  continue;
	df=derive(f,gen_x,contextptr);
	// if rvarsize==2 and f=(a*gen_x+b)/(c*gen_x+d), make the change of var
	// inf recurs will not happen, e=RAT(gen_x,function(RAT[gen_x]))
	// will be replaced with RAT(RAT[f],function(f))*RAT[f]
	// a simpler integral  
	gen A,B,C;
	if (rvarsize==2 && is_quadratic_wrt(inv(df,contextptr),gen_x,A,B,C,contextptr)&&is_zero(ratnormal(B*B-4*A*C,contextptr))){
	  A=2*A;
	  C=ratnormal(f*(A*gen_x+B),contextptr);
	  // f=C/(2*A*gen_x+B)
	  gen a,b;
	  if (is_linear_wrt(C,gen_x,a,b,contextptr)){ // always true
	    // f=(a*gen_x+b)/(A*gen_x+B)
	    C=gcd(gcd(a,b),gcd(A,B));
	    if (is_positive(-A,contextptr))
	      C=-C;
	    a=ratnormal(a/C); b=ratnormal(b/C); A=ratnormal(A/C); B=ratnormal(B/C);
	    // let f=x, gen_x=(B*x-b)/(-A*x+a)
	    e=subst(e,makevecteur(gen_x,f),makevecteur((B*gen_x-b)/(-A*gen_x+a),gen_x),false,contextptr)*(-A*b+B*a)/pow(gen_x*A-a,2);
	    e=linear_integrate_nostep(e,gen_x,tmprem,intmode,contextptr);
	    remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,f,contextptr);
	    return complex_subst(e,gen_x,f,contextptr);
	    
	  }
	}
	fu=recursive_ratnormal(rdiv(e,df,contextptr),contextptr); // changed from ratnormal, made 30/06/2021 for int((4/x^5)*cos((1/x^4)-6) );
	if (is_rewritable_as_f_of(fu,f,fx,gen_x,contextptr) && !is_undef(fx)){
	  if ( (intmode & 2)==0)
	    gprintf(step_fuuprime,gettext("Integration of %gen: f(u)*u' where f=%gen->%gen and u=%gen"),makevecteur(e,gen_x,fx,f),contextptr);
	  e=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	  remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,f,contextptr)*df;
	  return complex_subst(e,gen_x,f,contextptr);
	}	  
      }
    }
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate step 2 ") << e << '\n';
#endif
    //confirm("integrate step 2",e.print().c_str());
    if (e.type!=_SYMB){
      if (e==gen_x)
	return pow(gen_x,2,contextptr)/2;
      else
	return e*gen_x;
    }
    // try with argument of the product or of a power
    v.clear();
    bool est_puissance;
    if (e._SYMBptr->sommet==at_pow){
      v=vecteur(1,e._SYMBptr->feuille._VECTptr->front());
      fu=pow(e._SYMBptr->feuille._VECTptr->front(),e._SYMBptr->feuille._VECTptr->back()-plus_one,contextptr);
      est_puissance=true;
    }
    else {
      if ( (e._SYMBptr->sommet==at_prod) && (e._SYMBptr->feuille.type==_VECT))
	v=*e._SYMBptr->feuille._VECTptr;
      est_puissance=false;
    }
    const_iterateur vt=v.begin(),vtend=v.end();
    for (int i=0;(i<TRY_FU_UPRIME) && (vt!=vtend);++vt,++i){
      gen tmprem,u=linear_integrate_nostep(*vt,gen_x,tmprem,intmode|2,contextptr);
      if (is_undef(u) || !is_zero(tmprem)){
	gen tst=*vt;
	if ((intmode&8)==0 && tst.is_symb_of_sommet(at_pow)){
	  gen vtbase=tst._SYMBptr->feuille[0],vtexpo=inv(tst._SYMBptr->feuille[1],contextptr);
	  if (vtexpo.type==_INT_ && vtexpo.val==4){
	    if (evenodd==-1)
	      evenodd=is_even_odd(e,gen_x,contextptr); 
	    if (evenodd==1){
	      gen tmp=complex_subst(e,gen_x,inv(gen_x,contextptr),contextptr);
	      gen root=complex_subst(tst,gen_x,inv(gen_x,contextptr),contextptr);
	      gen sroot=simplify(root,contextptr);
	      if (is_even_odd(sroot,gen_x,contextptr)){
		tmp=complex_subst(tmp,root,sroot,contextptr);
		tmp=-linear_integrate_nostep(tmp*pow(gen_x,-2,contextptr),gen_x,tmprem,intmode|2|8,contextptr);
		if (!is_undef(tmp) && is_zero(tmprem)){
		  tmp=simplifier(tmp,contextptr);
		  tmp=complex_subst(tmp,gen_x,inv(gen_x,contextptr),contextptr);
		  vecteur v=lop(tmp,at_pow);
		  vecteur w=gen2vecteur(simplify(v,contextptr));
		  tmp=complex_subst(tmp,v,w,contextptr);
		  return tmp;
		}
	      }
	    }
	  } 
	}
	continue;
      }
      if (!est_puissance){
	vecteur vv(v);
	vv.erase(vv.begin()+i,vv.begin()+i+1);
	fu=symbolic(at_prod,gen(vv,_SEQ__VECT));
      }
      gen cst=extract_cst(u,gen_x,contextptr);
      bool recur=rvarsize>2 && gen_x.type==_IDNT && strcmp(gen_x._IDNTptr->id_name,"t_nostep")==0 && u.is_symb_of_sommet(at_pow); // workaround for some stupid integrals like integrate(sin((d*x + c)^(2/3)*b + a)/(f*x + E),x);
      if (!recur && 
	  (is_rewritable_as_f_of(fu,u,fx,gen_x,contextptr) || is_rewritable_as_f_of(simplifier(fu,contextptr),simplifier(u,contextptr),fx,gen_x,contextptr))){
	fx=cst*fx;
	if ( (intmode & 2)==0)
	  gprintf(step_fuuprime,gettext("Integration of %gen: f(u)*u' where f=%gen->%gen and u=%gen"),makevecteur(e,gen_x,fx,u),contextptr);
	gen e1=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	//*logptr(contextptr) << "idrem " << e1 << " " << gen_x << " " << u << '\n';
#if 1 // changed 2020 dec 13 for integrate(exp(t)*(t+1)^-2,t);
	if (is_zero(tmprem))
	  return complex_subst(e1,gen_x,u,contextptr);
#else
	remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,u,contextptr)*derive(u,gen_x,contextptr);
	return complex_subst(e1,gen_x,u,contextptr);
#endif
      }
      if (vt->is_symb_of_sommet(at_pow)){
	gen vtbase=vt->_SYMBptr->feuille[0],vtexpo=vt->_SYMBptr->feuille[1];
	if (vtexpo.type==_INT_ && vtexpo.val %2){ 
	  // for example *vt=x^9, retry with *vt=x^4
	  u=linear_integrate_nostep(pow(vtbase,vtexpo.val/2,contextptr),gen_x,tmprem,intmode|2,contextptr);
	  if (is_undef(u) || !is_zero(tmprem))
	    continue;
	  if (!est_puissance){
	    vecteur vv(v);
	    vv.erase(vv.begin()+i,vv.begin()+i+1);
	    fu=symbolic(at_prod,gen(vv,_SEQ__VECT));
	  }
	  cst=extract_cst(u,gen_x,contextptr);
	  fu=fu*pow(vtbase,vtexpo.val-vtexpo.val/2,contextptr);
	  if (is_rewritable_as_f_of(fu,u,fx,gen_x,contextptr)){
	    fx=cst*fx;
	    if ( (intmode & 2)==0)
	      gprintf(step_fuuprime,gettext("Integration of %gen: f(u)*u' where f=%gen->%gen and u=%gen"),makevecteur(e,gen_x,fx,u),contextptr);
	    e=linear_integrate_nostep(fx,gen_x,tmprem,intmode,contextptr);
	    remains_to_integrate=remains_to_integrate+complex_subst(tmprem,gen_x,u,contextptr)*derive(u,gen_x,contextptr);
	    return complex_subst(e,gen_x,u,contextptr);
	  }	  
	}
      }
    }
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate step 3 ") << e << '\n';
#endif
    //confirm("integrate step 3",e.print().c_str());
    // Step3: rational fraction?
    if (rvarsize==1){
      gen xvar(gen_x);
      return integrate_rational(e,gen_x,remains_to_integrate,xvar,intmode,contextptr);
    }
    bool do_risch=true;
    if (intmode & 4) do_risch=false;
    for (size_t i=0;i<rvar.size();++i){
      if (rvar[i].is_symb_of_sommet(at_pow)){
	do_risch=false;
	break;
      }
    }
    // minimal support for LambertW
    if (has_op(rvar,*at_LambertW)){
      vecteur vw(lop(rvar,at_LambertW));
      gen a,b;
      if (vw.size()==1 && is_linear_wrt(vw[0]._SYMBptr->feuille,gen_x,a,b,contextptr)){
	// W(ax+b) inside, change of variables ax+b=z*exp(z)
	// W(ax+b)=z, dx=(z+1)*exp(z)*dz/a
	vecteur substin(makevecteur(vw[0],gen_x));
	vecteur substout(makevecteur(gen_x,(gen_x*symbolic(at_exp,gen_x))));
	gen tmpe=complex_subst(e,substin,substout,contextptr)*(gen_x+1)*symbolic(at_exp,gen_x)/a,tmprem;
	gen tmpres=linear_integrate_nostep(tmpe,gen_x,tmprem,intmode,contextptr);
	substout[1]=symbolic(at_exp,gen_x); substin[1]=(a*gen_x+b)/vw[0];
	remains_to_integrate=complex_subst(tmprem,substout,substin,contextptr);
	res=complex_subst(tmpres,substout,substin,contextptr);
	return res;
      }
    }
    // square roots
    if ( (rvarsize==2) && (rvar.back().type==_SYMB) && (rvar.back()._SYMBptr->sommet==at_pow) ){
      // FIXME remove ==2, requires adding intmode parameter everywhere...
      if (integrate_sqrt(e,gen_x,rvar,res,remains_to_integrate,intmode,contextptr)==2){
	//*logptr(contextptr) << "intsqrt "  << e << " " << res << '\n';
	if ( (intmode & 1)==0 && is_zero(res)){
	  // try again with x->1/x?
	  gen e2=normal(-complex_subst(e,gen_x,inv(gen_x,contextptr),contextptr)/gen_x/gen_x,contextptr);
	  gen remains_to_integrate2,res2=integrate_id_rem(e2,gen_x,remains_to_integrate2,contextptr,1);
	  if (!is_zero(res2)){
	    res=complex_subst(res2,gen_x,inv(gen_x,contextptr),contextptr);
	    remains_to_integrate=-complex_subst(remains_to_integrate2,gen_x,inv(gen_x,contextptr),contextptr)/gen_x/gen_x;
	    return res;
	  }
	  remains_to_integrate=e;
	}
	return res;
      }
    }
    // detection of inv of trig or ln of a linear expression
    if (detect_inv_trigln(e,rvar,gen_x,res,remains_to_integrate,true,intmode,contextptr))
      return res;

    // integration by part?
    if ( (e._SYMBptr->sommet==at_prod) && (e._SYMBptr->feuille.type==_VECT)){
      const_iterateur ibp=e._SYMBptr->feuille._VECTptr->begin(),ibpend=e._SYMBptr->feuille._VECTptr->end();
      for (int j=0;ibp!=ibpend;++ibp,++j){
	int test;
	if (ibp->type!=_SYMB)
	  continue;
	if ( (ibp->_SYMBptr->sommet==at_pow) &&
	     (ibp->_SYMBptr->feuille._VECTptr->front().type==_SYMB) &&  
	     (ibp->_SYMBptr->feuille._VECTptr->back().type==_INT_) && 
	     (ibp->_SYMBptr->feuille._VECTptr->back().val>0) )
	  test=equalposcomp(inverse_tab_op,ibp->_SYMBptr->feuille._VECTptr->front()._SYMBptr->sommet);
	else
	  test=equalposcomp(inverse_tab_op,ibp->_SYMBptr->sommet);
	if (!test)
	  continue;
	vecteur ibpv(*e._SYMBptr->feuille._VECTptr);
	ibpv.erase(ibpv.begin()+j);
	gen ibpe=_prod(ibpv,contextptr);
#if 1
	gen tmpres,tmprem,tmpprimitive,tmp;
	tmpprimitive=linear_integrate_nostep(ibpe,gen_x,tmp,intmode|2,contextptr);
	if (is_zero(tmp)){ 
	  vecteur tmpv=rlvarx(tmpprimitive,gen_x);
	  unsigned tmpi=0;
	  for (;tmpi<tmpv.size();++tmpi){
	    if (tmpv[tmpi].type==_SYMB && equalposcomp(inverse_tab_op,tmpv[tmpi]._SYMBptr->sommet))
	      break;
	  }
	  if (tmpi==tmpv.size()){
	    if ( (intmode & 2)==0)
	      gprintf(step_bypart,gettext("Integration of %gen: by part, u*v'=%gen*(%gen)'"),makevecteur(e,*ibp,tmpprimitive),contextptr);
	    tmpres=tmpprimitive*derive(*ibp,gen_x,contextptr);
	    tmpres=recursive_normal(tmpres,true,contextptr);
	    tmpres=linear_integrate_nostep(tmpres,gen_x,tmprem,intmode,contextptr);
	    remains_to_integrate=-tmprem;
	    return tmpprimitive*(*ibp)-tmpres;
	  }
	}
#else
	vecteur tmpv(1,gen_x);
	lvar(ibpe,tmpv);
	tmpv.erase(tmpv.begin());
	if (lvarx(tmpv,gen_x).empty()){
	  gen tmpres,tmprem,tmpprimitive,tmp,xvar(gen_x);
	  tmpprimitive=integrate_rational(ibpe,gen_x,tmp,xvar,intmode,contextptr);
	  if (is_zero(tmp) && lvarx(tmpprimitive,gen_x)==vecteur(1,gen_x)){
	    tmpres=tmpprimitive*derive(*ibp,gen_x,contextptr);
	    tmpres=recursive_normal(tmpres,true,contextptr);
	    tmpres=linear_integrate_nostep(tmpres,gen_x,tmprem,intmode,contextptr);
	    remains_to_integrate=-tmprem;
	    return tmpprimitive*(*ibp)-tmpres;
	  }
	}
#endif
      }
    }
    else { // check for u'=1
      int test;
      if ( (e._SYMBptr->sommet==at_pow) && (e._SYMBptr->feuille._VECTptr->front().type==_SYMB) && (e._SYMBptr->feuille._VECTptr->back().type==_INT_) && (e._SYMBptr->feuille._VECTptr->back().val>0) )
	test=equalposcomp(inverse_tab_op,e._SYMBptr->feuille._VECTptr->front()._SYMBptr->sommet);
      else
	test=equalposcomp(inverse_tab_op,e._SYMBptr->sommet);
      if (test){
	if ( (intmode & 2)==0)
	  gprintf(step_bypart1,gettext("Integration of %gen by part of u*v' where u=1 and v=%gen'"),makevecteur(e,e),contextptr);
	gen tmpres,tmprem;
	tmpres=normal(derive(e,gen_x,contextptr),contextptr);
	tmpres=linear_integrate_nostep(gen_x*tmpres,gen_x,tmprem,intmode,contextptr);
	if (!has_i(e) && has_i(tmpres)){
	  remains_to_integrate=e;
	  return 0;
	}
	remains_to_integrate=-tmprem;
	return gen_x*e-tmpres;
      }
    }
    // additional check on e for f:= x*(x + 1)*(2*x*(x - (2*x**3 + 2*x**2 + x + 1)*log(x + 1))*exp(3*x**2) + (x**2*exp(2*x**2) - log(x + 1)**2)**2)/((x + 1)*log(x + 1)**2 - (x**3 + x**2)*exp(2*x**2))**2
    if (!is_elementary(rvar,gen_x) && detect_inv_trigln(e,rvar,gen_x,res,remains_to_integrate,false,intmode,contextptr))
      return res;

    // rewrite inv(exp)
    vector<const unary_function_ptr *> vsubstin(1,at_inv);
    vector<gen_op_context> vsubstout(1,invexptoexpneg);
    e=subst(e,vsubstin,vsubstout,true,contextptr); // changed to true for numint of programs (otherwise e becomes undef)
    // detection of denominator=independent of x
    v=lvarxwithinv(e,gen_x,contextptr);
    // search for nop (for nop[inv])
    if (!has_nop_var(v)){
      // additional check for non integer powers
      v=lop(lvar(e),at_pow);
      vecteur vx=lvarx(v,gen_x);
      if (vx.empty() || vx==vecteur(1,gen_x))
	return integrate_linearizable(e,gen_x,remains_to_integrate,intmode,true,contextptr);
      // second try with ^ rewritten as exp(ln)
      gen etmp=pow2expln(e,contextptr);
      v=lvarxwithinv(etmp,gen_x,contextptr);
      // search for nop (for nop[inv])
      if (!has_nop_var(v)){
	// additional check for non integer powers
	v=lop(lvar(etmp),at_pow);
	vecteur vx=lvarx(v,gen_x);
	if (vx.empty() || vx==vecteur(1,gen_x))
	  return integrate_linearizable(etmp,gen_x,remains_to_integrate,intmode,true,contextptr);
      }
    }
    // trigonometric fraction (or exp _FRAC), rewrite all elemnts of rvar as
    // tan of the common half angle, i.e. tan([coeff_trig*x+b]/2)
    int trig_fraction=-1; 
    gen coeff_trig;
    vecteur var(lvarx(e,gen_x));
    const_iterateur vart=var.begin(),vartend=var.end();
    for (;vart!=vartend;++vart){
      if (vart->type!=_SYMB){
	trig_fraction=false;
	continue;
      }
      int vartt=equalposcomp(primitive_tab_op,vart->_SYMBptr->sommet);
      if ( (!vartt) || (vartt>4) )
	trig_fraction=0;
      if (trig_fraction==-1){
          trig_fraction=vartt;
      }
      else {
          if (trig_fraction==4){
              if (vartt!=4)
                  trig_fraction=0;
          }
          else {
              if (vartt==4)
                  trig_fraction=0;
          }
      }
      if (trig_fraction ){ // trig of linear?
	gen a,b;
	if (!is_linear_wrt(vart->_SYMBptr->feuille,gen_x,a,b,contextptr)){
	  trig_fraction=false;
	  continue;
	}
	if (is_zero(coeff_trig))
	  coeff_trig=a;
	else {
	  gen quotient=ratnormal(rdiv(a,coeff_trig,contextptr),contextptr);
	  if (quotient.type==_INT_)
	    continue;
	  if ( (quotient.type==_FRAC) && (quotient._FRACptr->num.type==_INT_) && (quotient._FRACptr->den.type==_INT_) ){
	    coeff_trig=ratnormal(rdiv(coeff_trig,quotient._FRACptr->den,contextptr),contextptr);
	    continue;
	  }
	  if ( (quotient.type==_SYMB) && (quotient._SYMBptr->sommet==at_inv) && (quotient._SYMBptr->feuille.type==_INT_)){
	    coeff_trig=ratnormal(rdiv(coeff_trig,quotient._SYMBptr->feuille,contextptr),contextptr);
	    continue;
	  }
	  trig_fraction=false;
	}
      } // end if (trig_fraction)
    }
    if (trig_fraction){
      bool b=do_lnabs(contextptr);
      if (has_i(e))
	do_lnabs(false,contextptr);
      res=integrate_trig_fraction(e,gen_x,var,coeff_trig,trig_fraction,remains_to_integrate,intmode,contextptr);
      do_lnabs(b,contextptr);
      return res;
    }
    if (!do_risch){
      // Propfrac step
      gen nd=_fxnd(e,contextptr);
      if (nd.type==_VECT && nd._VECTptr->size()==2){
	gen num=nd[0],den=nd[1];
	vecteur propf=lvarx(den,gen_x);
        if (propf.empty()){
          remains_to_integrate=e_orig;
          return 0;
        }
	gen_sort_f(propf.begin(),propf.end(),islesscomplexthanf);
	nd=_quorem(makesequence(num,den,propf.back()),contextptr);
	if (nd.type==_VECT && nd._VECTptr->size()==2){
	  gen q=nd[0],r=nd[1];
	  if (!is_zero(q) && !is_zero(r)){
	    gen tmprem=0,tmpres;
	    tmpres = integrate_id_rem(q,*gen_x._IDNTptr,tmprem,contextptr,1);
	    // remains_to_integrate += tmprem; tmprem=0;
	    if (is_zero(tmprem)){
	      tmpres += integrate_id_rem(r/den,*gen_x._IDNTptr,tmprem,contextptr,1);	    
	      // remains_to_integrate += tmprem;
	      if (is_zero(tmprem))
		return res+tmpres;
	    }
	  }
	}
      }
      remains_to_integrate+=e;
      return 0;
    }
    // finish by calling the Risch algorithm
    if ( (intmode & 2)==0)
      gprintf(step_risch,gettext("Integrate %gen, no heuristic found, running Risch algorithm"),makevecteur(e),contextptr);
    res=risch(e,*gen_x._IDNTptr,remains_to_integrate,contextptr);
    if (!is_zero(remains_to_integrate) && taille(e,100)>taille(remains_to_integrate,100)){
      e=remains_to_integrate;
      res += integrate_id_rem(e,*gen_x._IDNTptr,remains_to_integrate,contextptr,0);
    }
    return res;
  }

  gen linear_integrate(const gen & e,const gen & x,gen & remains_to_integrate,int intmode,GIAC_CONTEXT){
    gen ee(normalize_sqrt(e,contextptr));
    return linear_apply(ee,x,remains_to_integrate,intmode,contextptr,integrate_gen_rem);
  }

  gen linear_integrate_nostep(const gen & e,const gen & x,gen & remains_to_integrate,int intmode,GIAC_CONTEXT){
    int step_infolevelsave=step_infolevel(contextptr);
    if ((intmode & 2)==2) 
      step_infolevel(contextptr)=0;
    // temporarily remove assumptions by changing integration variable
    gen t(identificateur("t_nostep"));
    gen tt(t);
    gen ee=quotesubst(e,x,tt,contextptr);
    ee=normalize_sqrt(ee,contextptr);
    gen res=linear_apply(ee,tt,remains_to_integrate,intmode,contextptr,integrate_gen_rem);
    step_infolevel(contextptr)=step_infolevelsave;
    //*logptr(contextptr) << "nostep " << res << " " << tt << " " << x << '\n';
    res=quotesubst(res,tt,x,contextptr);
    remains_to_integrate=quotesubst(remains_to_integrate,tt,x,contextptr);
    return res;
  }

  gen min2abs(const gen & g,GIAC_CONTEXT){
    if (g.type!=_VECT || g._VECTptr->size()!=2)
      return symbolic(at_min,g);
    gen a=g._VECTptr->front(),b=g._VECTptr->back();
    return (a+b-abs(a-b,contextptr))/2;
  }

  gen max2abs(const gen & g,GIAC_CONTEXT){
    if (g.type!=_VECT || g._VECTptr->size()!=2)
      return symbolic(at_min,g);
    gen a=g._VECTptr->front(),b=g._VECTptr->back();
    return (a+b+abs(a-b,contextptr))/2;
  }

  gen rewrite_minmax(const gen & e,bool quotesubst,GIAC_CONTEXT){
    vector<const unary_function_ptr *> vu;
    vu.push_back(at_min); 
    vu.push_back(at_max); 
    vector <gen_op_context> vv;
    vv.push_back(min2abs);
    vv.push_back(max2abs);
    return subst(e,vu,vv,quotesubst,contextptr);
  }

  gen integrate_id(const gen & e,const identificateur & x,GIAC_CONTEXT){
    if (e.type==_VECT){
      vecteur w;
      vecteur::const_iterator it=e._VECTptr->begin(),itend=e._VECTptr->end();
      for (;it!=itend;++it)
	w.push_back(integrate_id(*it,x,contextptr));
      return w;
    }
    gen remains_to_integrate;
    gen ee=rewrite_hyper(e,contextptr);
    ee=rewrite_minmax(ee,true,contextptr);
    gen res=_simplifier(linear_integrate(ee,x,remains_to_integrate,0,contextptr),contextptr);
    if (is_zero(remains_to_integrate))
      return res;
    else
      return res+symbolic(at_integrate,gen(makevecteur(remains_to_integrate,x),_SEQ__VECT));
  }

  static gen integrate0_(const gen & e,const identificateur & x,gen & remains_to_integrate,GIAC_CONTEXT){
    if (step_infolevel(contextptr))
      gprintf(step_integrate_header,gettext("===== Step/step primitive of %gen with respect to %gen ====="),makevecteur(e,x),contextptr);
    if (e.type==_VECT){
      vecteur w;
      vecteur::const_iterator it=e._VECTptr->begin(),itend=e._VECTptr->end();
      for (;it!=itend;++it)
	w.push_back(integrate_id(*it,x,contextptr));
      return w;
    }
    gen ee=rewrite_hyper(e,contextptr),tmprem;
    ee=rewrite_minmax(ee,true,contextptr);
    gen res=linear_integrate(ee,x,tmprem,0,contextptr);
    if (!is_zero(tmprem)){
      ee = tmprem;
      gen k=extract_cst(ee,x,contextptr);
      if (ee.is_symb_of_sommet(at_plus)){
	res += k*integrate_gen_rem(ee,x,tmprem,0,contextptr);
	tmprem = k*tmprem;
      }
    }
    remains_to_integrate=remains_to_integrate+tmprem;
    if (step_infolevel(contextptr) && is_zero(remains_to_integrate))
      gprintf(gettext("Hence primitive of %gen with respect to %gen is %gen"),makevecteur(e,x,res),contextptr);
    return res;
  }

  static gen integrate0(const gen & e,const identificateur & x,gen & remains_to_integrate,GIAC_CONTEXT){
    bool b_acosh=keep_acosh_asinh(contextptr);
    keep_acosh_asinh(true,contextptr);
    gen res=integrate0_(e,x,remains_to_integrate,contextptr);
    keep_acosh_asinh(b_acosh,contextptr);
    return res;
  }

  gen integrate_gen(const gen & e,const gen & f,GIAC_CONTEXT){
    if (f.type!=_IDNT){
      gen x(identificateur("tmpx"));
      gen e1=subst(e,f,x,false,contextptr);
      return quotesubst(integrate_id(e1,*x._IDNTptr,contextptr),x,f,contextptr);
    }
    return integrate_id(e,*f._IDNTptr,contextptr);
  }

  bool adjust_int_sum_arg(vecteur & v,int & s){
    if (s<2)
      return false; // setsizeerr(contextptr);
    if ( (s==2) && (v[1].type==_SYMB) && (v[1]._SYMBptr->sommet==at_equal || v[1]._SYMBptr->sommet==at_equal2 || v[1]._SYMBptr->sommet==at_same)){
      v.push_back(v[1]._SYMBptr->feuille._VECTptr->back());
      v[1]=v[1]._SYMBptr->feuille._VECTptr->front();
      if ( (v[2].type!=_SYMB) || (v[2]._SYMBptr->sommet!=at_interval) )
	return false; // settypeerr(contextptr);
      v.push_back(v[2]._SYMBptr->feuille._VECTptr->back());
      v[2]=v[2]._SYMBptr->feuille._VECTptr->front();
      s=4;
    }
    return true;
  }

  static int ggb_intcounter=0;

  gen ck_int_numerically(const gen & f,const gen & x,const gen & a,const gen &b,const gen & exactvalue,GIAC_CONTEXT){
    if (is_inf(a) || is_inf(b))
      return exactvalue;
    gen tmp=evalf_double(exactvalue,1,contextptr);
#if defined HAVE_LIBMPFR && !defined NO_STDEXCEPT
    if ( (tmp.type==_DOUBLE_ || tmp.type==_CPLX) 
	 && !has_i(lop(exactvalue,at_erf)) // otherwise it's slow
	 ){
      try {
	tmp=evalf_double(accurate_evalf(exactvalue,256),1,contextptr);
      } catch (std::runtime_error & err){
	last_evaled_argptr(contextptr)=NULL;
      }
    }
#endif
    if (tmp.type!=_DOUBLE_ && tmp.type!=_CPLX)
      return exactvalue;
    if (debug_infolevel)
      *logptr(contextptr) << gettext("Checking exact value of integral with numeric approximation")<<'\n';
    gen tmp2;
    if (!tegral(f,x,a,b,1e-6,(1<<10),tmp2,true,contextptr))
      return exactvalue;
    tmp2=evalf_double(tmp2,1,contextptr);
    if ( (tmp2.type!=_DOUBLE_ && tmp2.type!=_CPLX) || 
	 (abs(tmp,contextptr)._DOUBLE_val<1e-8 && abs(tmp2,contextptr)._DOUBLE_val<1e-8) || 
	 abs(tmp-tmp2,contextptr)._DOUBLE_val<=1e-3*abs(tmp2,contextptr)._DOUBLE_val
	 )
      return simplifier(exactvalue,contextptr);
    *logptr(contextptr) << gettext("Error while checking exact value with approximate value, returning both!") << '\n';
    return makevecteur(exactvalue,tmp2);
  }

  void comprim(vecteur & v){
    vecteur w;
    for (unsigned i=0;i<v.size();++i){
      if (!equalposcomp(w,v[i]))
	w.push_back(v[i]);
    }
    v=w;
  }

  gen assumeeval(const gen & x,GIAC_CONTEXT){
    // if (contextptr && contextptr->globalcontextptr!=contextptr) return assumeeval(x,contextptr->globalcontextptr); 
    if (x.type!=_IDNT)
      return x.eval(1,contextptr);
    gen evaled;
    if (x._IDNTptr->in_eval(1,x,evaled,contextptr))
      return evaled;
    return x;
  }

  void restorepurge(const gen & xval,const gen & x,GIAC_CONTEXT){
    // if (contextptr && contextptr->globalcontextptr!=contextptr) restorepurge(xval,x,contextptr->globalcontextptr);
    if (xval==x 
	// || (xval.type==_VECT && xval.subtype==_ASSUME__VECT && xval._VECTptr->size()==1 && xval._VECTptr->front().val==_SYMB)
	)
      purgenoassume(x,contextptr);
    else
      sto(xval,x,contextptr);
  }

#if !defined USE_GMP_REPLACEMENTS && !defined BF2GMP_H
  // small utility for ggb floats looking like fractions
  void ggb_num_coeff(gen & g){
    if (g.type!=_FRAC || g._FRACptr->den.type!=_ZINT)
      return;
    mpz_t t; mpz_init_set(t,*g._FRACptr->den._ZINTptr);
    while (mpz_divisible_ui_p(t,2)){
      mpz_divexact_ui(t,t,2);
      continue;
    }
    while (mpz_divisible_ui_p(t,5)){
      mpz_divexact_ui(t,t,5);
      continue;
    }
    if (mpz_cmp_ui(t,1)==0)
      g=evalf(g,1,context0);
    mpz_clear(t);
  }
#endif

#ifdef NO_STDEXCEPT
  inline gen protect_integrate(const gen & args,GIAC_CONTEXT){
    return _integrate(args,contextptr);
  }
#else
  gen protect_integrate(const gen & args,GIAC_CONTEXT){
    gen res;
    try {
      res=_integrate(args,contextptr);
    } catch (std::runtime_error & err){
      last_evaled_argptr(contextptr)=NULL;
      res=string2gen(err.what(),false);
      res.subtype=-1;
    }
    return res;
  }
#endif
  gen integrate_chknum(const gen & v0,const gen & x,gen & rem,GIAC_CONTEXT){
    gen primitive;
    if (has_num_coeff(v0)){
      primitive=integrate0(exact(v0,contextptr),*x._IDNTptr,rem,contextptr);
      primitive=evalf(primitive,1,contextptr);
      rem=evalf(rem,1,contextptr);
    }
    else 
      primitive=integrate0(v0,*x._IDNTptr,rem,contextptr);
    return primitive;
  }

  // auto-assumptions assuming g is real-defined
  // if an assumption is already made on a variable, it is ignored
  vecteur autoassume(const gen & g_,const gen & x_,GIAC_CONTEXT){
    gen g=eval(g_,1,contextptr),x=eval(x_,1,contextptr);
    vecteur v(rlvar(g,false));
    vecteur ass,res,bases; // list of assumptions and assumed idnt
    for (int i=0;i<v.size();++i){
      if (v[i].type!=_SYMB)
	continue;
      gen f=v[i]._SYMBptr->feuille;
      const unary_function_ptr & u=v[i]._SYMBptr->sommet;
      gen base,expo;
      if (u==at_pow && f.type==_VECT && f._VECTptr->size()==2){
	base=f[0];
	expo=f[1];
      }
      if (u==at_sqrt || u==at_ln){
	base=f;
	expo=plus_one_half;
      }
      if (expo!=0){
	if (equalposcomp(bases,base))
	  continue;
	bases.push_back(base);
	if (is_assumed_integer(expo,contextptr))
	  continue;
	if (expo.type==_FRAC && expo._FRACptr->den.type==_INT_ && (expo._FRACptr->den.val%2==1))
	  continue;
	vecteur varbase(lvar(base));
	if (varbase.size()>=1){
	  vecteur lid=lidnt(base);
	  if (!lid.empty()){
	    gen var=lid[0],a,b,c,hyp,varval;
	    if (equalposcomp(lid,x))
	      var=x;
	    bool addi=equalposcomp(res,var); // additional hyp?
	    if (!addi)
	      varval=assumeeval(var,contextptr);
	    if (addi || varval==var){	 
	      if (var.type==_IDNT && is_linear_wrt(base,var,a,b,contextptr) && !is_zero(a)){
		int as=fastsign(a,contextptr);
		gen avar,aa,ab; vecteur av;
		if (as==0){
		  av=lidnt(a);
		  if (av.size()==1 && !equalposcomp(res,av[0]) && is_linear_wrt(a,av[0],aa,ab,contextptr)){
		    as=fastsign(aa,contextptr);
		    if (as==1)
		      hyp=symb_superieur_strict(av[0],-ab/aa);
		    else if (as==-1)
		      hyp=symb_inferieur_strict(av[0],-ab/aa);
		    if (as){
		      res.push_back(av[0]);
		      ass.push_back(hyp);
		      giac_assume(hyp,contextptr);
		      as=1;
		    }
		  }
		}
		if (as){
		  av=lidnt(b);
		  if (av.size()==1 && !equalposcomp(res,av[0]) && is_linear_wrt(b,av[0],aa,ab,contextptr)){
		    as=fastsign(aa,contextptr);
		    if (as==1)
		      hyp=symb_superieur_strict(av[0],-ab/aa);
		    else if (as==-1)
		      hyp=symb_inferieur_strict(av[0],-ab/aa);
		    if (as){
		      res.push_back(av[0]);
		      ass.push_back(hyp);
		      giac_assume(hyp,contextptr);
		      as=1;
		      b=0;
		    }
		  }
		}
		if (as==1)
		  hyp=symb_superieur_strict(var,-b/a);
		else if (as==-1)
		  hyp=symb_inferieur_strict(var,-b/a);
	      } // end linear case
	      else {
		if (var.type==_IDNT && is_quadratic_wrt(base,var,a,b,c,contextptr) && !is_zero(a)){
		  int as=fastsign(a,contextptr);
		  gen avar,aa,ab; vecteur av;
		  if (as==0){
		    av=lidnt(a);
		    if (av.size()==1 && !equalposcomp(res,av[0]) && is_linear_wrt(a,av[0],aa,ab,contextptr)){
		      as=fastsign(aa,contextptr);
		      if (as==1)
			hyp=symb_superieur_strict(av[0],-ab/aa);
		      else if (as==-1)
			hyp=symb_inferieur_strict(av[0],-ab/aa);
		      if (as){
			res.push_back(av[0]);
			ass.push_back(hyp);
			giac_assume(hyp,contextptr);
		      }
		    }
		  } // end as==0
		} // end quadratic
		varbase=lvarx(base,var);
		if (varbase.size()==1 && lidnt(base).size()==1){
		  gen var0=varbase[0],addhyp;
		  bool dosolve=false;
		  if (var0.type==_IDNT)
		    dosolve=true;
		  if (var0.type==_SYMB){
		    const unary_function_ptr & u=var0._SYMBptr->sommet;
		    gen varf=var0._SYMBptr->feuille;
		    if (varf.type!=_VECT && is_linear_wrt(varf,var,a,b,contextptr)){ // f(a*x+b), if f is trig assume in a*x+b in a period
		      int as=fastsign(a,contextptr);
		      if (as){
			if (u==at_sin || u==at_cos)
			  addhyp=cst_pi;
			else if (u==at_tan)
			  addhyp=cst_pi/2;
			else
			  addhyp=0;
			if (addhyp!=0){
			  if (as==1)
			    addhyp=symb_and(symb_superieur_strict(var,(-addhyp-b)/a),symb_inferieur_strict(var,(addhyp-b)/a));
			  else if (as==-1)
			    addhyp=symb_and(symb_inferieur_strict(var,(-addhyp-b)/a),symb_superieur_strict(var,(addhyp-b)/a));
			}
			dosolve=true;
		      }
		    }
		  }
		  if (dosolve){
		    if (!is_zero(addhyp)){
		      ass.push_back(addhyp);
		      if (addi)
			giac_additionally(addhyp,contextptr);
		      else {
			res.push_back(var);
			giac_assume(addhyp,contextptr);
			addi=true;
		      }
		    }
		    hyp=symbolic(at_solve,makesequence(symb_superieur_strict(base,0),var));
		    hyp=protecteval(hyp,1,contextptr);
		  }
		}
	      }
	    }
	    if (hyp.type==_SYMB){
	      ass.push_back(hyp);
	      if (addi)
		giac_additionally(hyp,contextptr);
	      else {
		res.push_back(var);
		giac_assume(hyp,contextptr);
	      }
	    }
	    if (hyp.type==_VECT){
	      vecteur hypv=*hyp._VECTptr;
	      for (int j=hypv.size()-1;j>=0;--j){
		// j decreasing will give "simpler" auto-assumptions for trig
		gen curhyp=hypv[j];
		if (curhyp.type!=_SYMB)
		  continue;
		const unary_function_ptr & u=curhyp._SYMBptr->sommet;
		if (u!=at_and && u!=at_ou && 
		    u!=at_superieur_strict && u!=at_superieur_egal &&
		    u!=at_inferieur_strict && u!=at_inferieur_egal)
		  continue;
		ass.push_back(curhyp);
		if (addi || j)
		  giac_additionally(curhyp,contextptr);
		else {
		  res.push_back(var);
		  giac_assume(curhyp,contextptr);
		  addi=true;
		}
		break; // solve will return different intervals, we select one
	      }
	    } // end hyp.type==_VECT
	  } // end if lidnt(base) not empty
	} // end if lvar(base).size()>=1
      } // end if expo!=0
    }
    if (!ass.empty())
      *logptr(contextptr) << "Auto-assuming " << ass << "\n";
    return res;
  }

  gen abs2piecewise(const gen & x,GIAC_CONTEXT){
    return symbolic(at_piecewise,makesequence(symbolic(at_inferieur_strict,x,0),-x,x));
  }

  gen min2piecewise(const gen & g,GIAC_CONTEXT){
    if (g.type!=_VECT || g._VECTptr->size()!=2)
      return symbolic(at_min,g);
    gen a=g._VECTptr->front(),b=g._VECTptr->back();
    return symbolic(at_piecewise,makesequence(symbolic(at_inferieur_strict,a,b),a,b));
  }

  gen max2piecewise(const gen & g,GIAC_CONTEXT){
    if (g.type!=_VECT || g._VECTptr->size()!=2)
      return symbolic(at_min,g);
    gen a=g._VECTptr->front(),b=g._VECTptr->back();
    return symbolic(at_piecewise,makesequence(symbolic(at_inferieur_strict,a,b),b,a));
  }

  gen whenmaxmin2piecewise(const gen & g,GIAC_CONTEXT){
    vector<const unary_function_ptr *> vu;
    vu.push_back(at_min); 
    vu.push_back(at_max); 
    vector <gen_op_context> vv;
    vv.push_back(min2piecewise);
    vv.push_back(max2piecewise);
    gen r=subst(g,vu,vv,true,contextptr);
    r=when2piecewise(r,contextptr);
    return r;
  }

  bool has_undef(const gen & g){
    if (is_undef(g))
      return true;
    if (g.type==_VECT){
      unsigned s=unsigned(g._VECTptr->size());
      for (unsigned i=0;i<s;++i){
	if (has_undef((*g._VECTptr)[i]))
	  return true;
      }
      return false;
    }
    if (g.type==_POLY){
      unsigned s=unsigned(g._POLYptr->coord.size());
      for (unsigned i=0;i<s;++i){
	if (has_undef(g._POLYptr->coord[i].value))
	  return true;
      }
      return false;
    }
    if (g.type==_SYMB)
      return has_undef(g._SYMBptr->feuille);
    return false;
  }

  gen _integrate_(const gen &args,GIAC_CONTEXT);
  
  // integrate w=[M,N]=Mdx+Ndy along curve, v=[x,y]
  // or more generally w vector field along curve, v=[x1,..,xdim]
  gen curviligne(const vecteur & w,const vecteur & v,const gen & curve,const gen & V,const gen & tmin_,const gen & tmax_,GIAC_CONTEXT){
    if (curve.type==_VECT){
      gen S=0;
      vecteur c=*curve._VECTptr;
      for (int i=0;i<c.size();++i)
        S += curviligne(w,v,c[i],V,tmin_,tmax_,contextptr);
      return S;
    }
    if (curve.is_symb_of_sommet(at_union)){
      vecteur f=gen2vecteur(curve._SYMBptr->feuille);
      gen res=0;
      for (int i=0;i<f.size();++i){
        res += curviligne(w,v,f[i],V,tmin_,tmax_,contextptr);
      }
      return res;
    }
    // handle curve, segment and arc of circle
    gen c=curve;
    if (!c.is_symb_of_sommet(at_pnt))
      c=eval(c,1,contextptr);
    c=remove_at_pnt(c);
    gen eq,t,tmin(tmin_),tmax(tmax_);
    if (c.is_symb_of_sommet(at_curve)){
      c=c[1];
      eq=c[0];t=c[1];
      if (is_undef(tmin))
        tmin=c[2];
      if (is_undef(tmax))
        tmax=c[3];
    }
    else {
      if (c.type==_VECT && c._VECTptr->size()==2){
        eq=c[0]+vx_var*(c[1]-c[0]);
        t=vx_var;
        if (is_undef(tmin))
          tmin=0;
        if (is_undef(tmax))
          tmax=1;
      }
      else if (c.is_symb_of_sommet(at_cercle)){
        vecteur cv=gen2vecteur(c._SYMBptr->feuille);
        if (cv.size()<3)
          return undef;
        if (is_undef(tmin))
          tmin=cv[1];
        if (is_undef(tmax))
          tmax=cv[2];
        cv=gen2vecteur(cv[0]);
        eq=(cv[0]+cv[1])/2+(cv[1]-cv[0])/2*symb_exp(cst_i*vx_var);
        t=vx_var;
      }
      else return undef;
    }
    vecteur vt;
    if (v.size()==2){
      gen xt,yt;
      reim(eq,xt,yt,contextptr);
      vt=makevecteur(xt,yt);
    }
    else {
      if (eq.type!=_VECT || eq._VECTptr->size()!=v.size())
        return gendimerr(contextptr);
      vt=*eq._VECTptr;
    }
    if (!is_undef(V))
      return subst(V,v,subst(vt,t,tmax,false,contextptr),false,contextptr)-subst(V,v,subst(vt,t,tmin,false,contextptr),false,contextptr);
    gen M=subst(w,v,vt,false,contextptr);
    gen g=dotvecteur(M,derive(vt,t,contextptr),contextptr);
    return _integrate_(makesequence(g,t,tmin,tmax),contextptr);
  }

  gen curviligne(const vecteur & w,const vecteur & v,const gen & curve,const gen & tmin,const gen &tmax,GIAC_CONTEXT){
    gen V;
    if (!is_potential(w,v,V,contextptr))
      V=undef;
    return curviligne(w,v,curve,V,tmin,tmax,contextptr);
  }

  // Split a bounded number of affine absolute values at exact rational
  // breakpoints. Each open segment then has a fixed sign, and the normal
  // definite integrator still checks its endpoint limits and singularities.
  static bool integrate_affine_abs(const gen &f,const gen &x,gen lo,gen hi,gen &res,GIAC_CONTEXT){
    unsigned budget=3;
    if (!is_constant_wrt(lo,x,contextptr) || !small_polynomial(lo,x,budget)) return false;
    budget=3;
    if (!is_constant_wrt(hi,x,contextptr) || !small_polynomial(hi,x,budget) || taille(f,128)>127) return false;
    vecteur terms=lop(f,at_abs);
    if (terms.empty() || terms.size()>8) return false;
    bool reverse=is_strictly_greater(lo,hi,contextptr);
    if (reverse) swapgen(lo,hi);
    vecteur points=makevecteur(lo,hi),arguments;
    for (unsigned i=0;i<terms.size();++i){
      gen arg=terms[i]._SYMBptr->feuille,a,b;
      if (!is_linear_wrt(arg,x,a,b,contextptr)) return false;
      budget=3;
      if (!small_polynomial(a,x,budget)) return false;
      budget=3;
      if (!small_polynomial(b,x,budget)) return false;
      arguments.push_back(arg);
      if (is_zero(a)) continue;
      gen r=rdiv(-b,a,contextptr);
      if (!is_strictly_greater(r,lo,contextptr) || !is_strictly_greater(hi,r,contextptr)) continue;
      for (unsigned j=1;j<points.size();++j){
        if (r==points[j]) break;
        if (is_strictly_greater(points[j],r,contextptr)){
          points.insert(points.begin()+j,r);break;
        }
      }
    }
    if (points.size()==2) return false;
    res=0;
    for (unsigned i=1;i<points.size();++i){
      gen mid=(points[i-1]+points[i])/2;
      vecteur replacements;
      for (unsigned j=0;j<arguments.size();++j){
        gen value=subst(arguments[j],x,mid,false,contextptr);
        replacements.push_back(is_strictly_positive(value,contextptr)?arguments[j]:-arguments[j]);
      }
      gen segment=subst(f,terms,replacements,false,contextptr);
      gen part=_integrate_(makesequence(segment,x,points[i-1],points[i]),contextptr);
      if (is_undef(part)){res=part;return true;}
      res+=part;
#ifdef TIMEOUT
      control_c();
#endif
      if (ctrl_c || interrupted){res=undef;return true;}
    }
    if (reverse) res=-res;
    return true;
  }

  gen _integrate_(const gen &args,GIAC_CONTEXT){
#ifdef LOGINT
    *logptr(contextptr) << gettext("integrate begin") << '\n';
#endif
    if (has_undef(args))
      return undef;
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    vecteur v(gen2vecteur(args));
    if (v.size()==1){
      gen a,b,c=eval(args,1,contextptr);
      if (c.type==_SPOL1){
	sparse_poly1 res=*c._SPOL1ptr;
	sparse_poly1::iterator it=res.begin(),itend=res.end();
	for (;it!=itend;++it){
	  gen e=it->exponent+1;
	  if (e==0)
	    return sparse_poly1(1,monome(undef,undef));
	  it->coeff=it->coeff/e;
	  it->exponent=e;
	}
	return res;
      }
      if (c.type==_VECT && c.subtype==_POLY1__VECT){
	vecteur v=*c._VECTptr;
	reverse(v.begin(),v.end());
	v=integrate(v,1);
	reverse(v.begin(),v.end());      
	v.push_back(0);
	return gen(v,_POLY1__VECT);
      }
      if (is_algebraic_program(c,a,b) && a.type!=_VECT)
	return symbolic(at_program,makesequence(a,0,_integrate(gen(makevecteur(b,a),_SEQ__VECT),contextptr)));
      if (calc_mode(contextptr)==1)
	v.push_back(ggb_var(v.front()));
      else
	v.push_back(vx_var);
    }
    int s=int(v.size());
    if (!adjust_int_sum_arg(v,s))
      return gensizeerr(contextptr);
    if (s>=4 && complex_mode(contextptr)){
      complex_mode(false,contextptr);
      gen res=_integrate(args,contextptr);
      complex_mode(true,contextptr);
      return res;
    }
    if (s==1)
      return gentoofewargs("integrate");
    if (s==2){
      gen v0=eval(v[0],1,contextptr);
      gen v1=eval(v[1],1,contextptr);
      int t1=graph_output_type(v1);
      if (v0.type==_VECT && v0._VECTptr->size()==2 && (t1==2 || v1.is_symb_of_sommet(at_union))) 
        return curviligne(*v0._VECTptr,makevecteur(x__IDNT_e,y__IDNT_e),v1,undef,undef,contextptr);
    }
    if (s==7){
      for (int i=0;i<s;++i)
        v[i]=eval(v[i],1,contextptr);
      gen M,N,x,y;
      // integrate([M,N],[x,y],x(t),y(t),t,tmin,tmax)
      // integrate(M,N,x(t),y(t),t,tmin,tmax)
      if (v[0].type==_VECT && v[1].type==_VECT && v[0]._VECTptr->size()==2 && v[1]._VECTptr->size()==2){
        x=v[1]._VECTptr->front();
        y=v[1]._VECTptr->back();
        M=v[0]._VECTptr->front();
        N=v[0]._VECTptr->back();        
      }
      else {
        x=x__IDNT_e;
        y=y__IDNT_e;
        M=v[0];
        N=v[1];
      }
      gen xt=v[2],yt=v[3],t=v[4],tmin=v[5],tmax=v[6];
      gen xy=makevecteur(x,y); gen xyt=makevecteur(xt,yt),V;
      if (is_potential(makevecteur(M,N),*xy._VECTptr,V,contextptr))
        return subst(V,xy,subst(xyt,t,tmax,false,contextptr),false,contextptr)-subst(V,xy,subst(xyt,t,tmin,false,contextptr),false,contextptr);
      M=subst(M,xy,xyt,false,contextptr);
      N=subst(N,xy,xyt,false,contextptr);
      gen g=M*derive(xt,t,contextptr)+N*derive(yt,t,contextptr);
      return _integrate_(makesequence(g,t,tmin,tmax),contextptr);
    }
    if (v.back()!=at_assume && (s==3 || s==5)){
      if (v[0].type==_IDNT && v[0]!=v[1])
        v[0]=eval(v[0],1,contextptr);
      if (v[0].type==_VECT){
        if (v[1].type==_IDNT)
          v[1]=eval(v[1],1,contextptr);
        if (v[1].type==_VECT && v[0]._VECTptr->size()==v[1]._VECTptr->size()){
          if (v[2].type==_IDNT)
            v[2]=eval(v[2],1,contextptr);
          // integrate([M,N],[x,y],G)
          return curviligne(*v[0]._VECTptr,*v[1]._VECTptr,v[2],s==3?undef:v[3],s==3?undef:v[4],contextptr);
        }
      }
      if (s==3 && calc_mode(contextptr)!=1)
	// indefinite integration with constant of integration
	return _integrate(gen(makevecteur(v[0],v[1]),_SEQ__VECT),contextptr)+v[2];
      v.insert(v.begin()+1,ggb_var(eval(v.front(),1,contextptr)));
      ++s;
    }
    if (s>6)
      return gentoomanyargs("integrate");
    gen x=v[1];
    if (x.is_symb_of_sommet(at_unquote))
      x=eval(x,1,contextptr);
    if (storcl_38 && x.type==_IDNT && storcl_38(x,0,x._IDNTptr->id_name,undef,false,contextptr,NULL,false)){
      gen t(identificateur("t_"));
      x=v[1];
      v[0]=quotesubst(v[0],x,t,contextptr);
      v[1]=t;
      gen res=_integrate(gen(v,_SEQ__VECT),contextptr);
      return quotesubst(res,t,x,contextptr);
    }
    if (x.type!=_IDNT){
      if (x.type<_IDNT)
	return gensizeerr(contextptr);
      if (abs_calc_mode(contextptr)==38 && x.type!=_SYMB)
	return gensizeerr(contextptr);
      if (x.type==_SYMB && x._SYMBptr->sommet!=at_of && x._SYMBptr->sommet!=at_at)
	return gensizeerr(contextptr);
      gen t(identificateur("tmpt"));
      v[0]=quotesubst(v[0],x,t,contextptr);
      v[1]=t;
      gen res=_integrate(gen(v,_SEQ__VECT),contextptr);
      return quotesubst(res,t,x,contextptr);
    }
    // Do this before interval assumptions evaluate abs(sin(...)): their
    // sign analysis may enumerate millions of zeros over a long interval.
    gen period_result;
    if (s==4 && !approx_mode(contextptr) && !has_num_coeff(v[0]) &&
        !has_num_coeff(v[2]) && !has_num_coeff(v[3]) &&
        (integrate_real_definite(v[0],x,eval(v[2],1,contextptr),eval(v[3],1,contextptr),period_result,contextptr) ||
         integrate_trig_periods(v[0],x,eval(v[2],1,contextptr),eval(v[3],1,contextptr),period_result,contextptr) ||
         integrate_affine_abs(v[0],x,eval(v[2],1,contextptr),eval(v[3],1,contextptr),period_result,contextptr)))
      return period_result;
    int quoted=0;
    if (x._IDNTptr->quoted){
      quoted=*x._IDNTptr->quoted;
      *x._IDNTptr->quoted=1;
    }
    for (int i=2;i<s;++i){
      v[i]=eval(v[i],eval_level(contextptr),contextptr);
      if (v[i].is_symb_of_sommet(at_pnt))
	return gensizeerr(contextptr);
    }
#if 0 // ndef USE_GMP_REPLACEMENTS
    if (calc_mode(contextptr)==1){
      if (s>2) 
	ggb_num_coeff(v[2]);
      if (s>3) 
	ggb_num_coeff(v[3]);
    }
#endif
    if (s>=4){ // take care of boundaries when evaluating
      if (v.back()==at_assume){
	--s;
	v.pop_back();
      }
      else {
	gen xval=assumeeval(x,contextptr);
	gen a(v[2]),b(v[3]);
	if (evalf_double(a,1,contextptr).type==_DOUBLE_ && evalf_double(b,1,contextptr).type==_DOUBLE_){
	  bool neg=false;
	  if (is_greater(v[2],v[3],contextptr)){
	    a=v[3]; b=v[2];
	    neg=true;
	    v[2]=a; v[3]=b;
	  }
	  vecteur lv=lop(lvarx(v[0],v[1]),at_pow);
	  lv=mergevecteur(lv,lop(lvarx(v[0],v[1]),at_surd));
	  lv=mergevecteur(lv,lop(lvarx(v[0],v[1]),at_NTHROOT));
	  if (lv.size()==1 && v[1].type==_IDNT){
	    gen powarg=lv[0][1];
	    if (lv[0][0]==at_NTHROOT)
	      powarg=lv[0][2];
	    lv=protect_solve(powarg,*v[1]._IDNTptr,0,contextptr);
	    for (int i=0;i<int(lv.size());++i){
	      if (is_strictly_greater(lv[i],a,contextptr) && is_strictly_greater(b,lv[i],contextptr)){
		v[3]=lv[i];
		gen res1=_integrate(v,contextptr);
		v[3]=b;
		v[2]=lv[i];
		gen res2=_integrate(v,contextptr);
		res2=res1+res2;
		return neg?-res2:res2;
	      }
	    }
	  }
	  giac_assume(symb_and(symb_superieur_egal(x,a),symb_inferieur_egal(x,b)),contextptr);
	  v.push_back(at_assume);
	  gen res=protect_integrate(gen(v,_SEQ__VECT),contextptr);
	  restorepurge(xval,x,contextptr);
	  return neg?-res:res;
	}
	if (is_greater(b,a,contextptr)){
	  if (x==b)
	    giac_assume(symb_superieur_egal(x,a),contextptr);
	  else
	    giac_assume(symb_and(symb_superieur_egal(x,a),symb_inferieur_egal(x,b)),contextptr);
	  v.push_back(at_assume);
	  gen res=protect_integrate(gen(v,_SEQ__VECT),contextptr);
	  restorepurge(xval,x,contextptr);
	  return res;
	}
	else {
	  if (is_greater(a,b,contextptr)){
	    giac_assume(symb_and(symb_superieur_egal(x,b),symb_inferieur_egal(x,a)),contextptr);
	    v.push_back(at_assume);
	    gen res=protect_integrate(gen(v,_SEQ__VECT),contextptr);
	    restorepurge(xval,x,contextptr);
	    return res;
	  }
	}
      }
    }
    bool b_acosh=keep_acosh_asinh(contextptr);
    keep_acosh_asinh(true,contextptr);
#ifdef NO_STDEXCEPT
    if (contextptr && contextptr->quoted_global_vars && !is_assumed_real(x,contextptr)){
      contextptr->quoted_global_vars->push_back(x);
      gen tmp=eval(v[0],eval_level(contextptr),contextptr); 
      tmp=Heavisidetopiecewise(tmp,contextptr);
      if (!is_undef(tmp)) v[0]=tmp;
      contextptr->quoted_global_vars->pop_back();
    }
    else {
      gen tmp=eval(v[0],eval_level(contextptr),contextptr); 
      tmp=Heavisidetopiecewise(tmp,contextptr);
      if (!is_undef(tmp)) v[0]=tmp;
    }
#else
    try {
      if (contextptr && contextptr->quoted_global_vars && !is_assumed_real(x,contextptr)){
	contextptr->quoted_global_vars->push_back(x);
	gen tmp=eval(v[0],eval_level(contextptr),contextptr); 
	tmp=Heavisidetopiecewise(tmp,contextptr);
	if (!is_undef(tmp)) v[0]=tmp;
	contextptr->quoted_global_vars->pop_back();
      }
      else {
	gen tmp=eval(v[0],eval_level(contextptr),contextptr); 
	tmp=Heavisidetopiecewise(tmp,contextptr);
	if (!is_undef(tmp)) v[0]=tmp;
      }
    } catch (std::runtime_error & err){
      last_evaled_argptr(contextptr)=NULL;
      CERR << "Unable to eval " << v[0] << ": " << err.what() << '\n';
    }
#endif
    keep_acosh_asinh(b_acosh,contextptr);
    if (x._IDNTptr->quoted)
      *x._IDNTptr->quoted=quoted;    
    if (s>4 || (approx_mode(contextptr) && (s==4)) ){
      v[1]=x;
      return intnum(gen(v,_SEQ__VECT),false,contextptr,true);
    }
    gen rem,borne_inf,borne_sup,res,v0orig,aorig,borig;
    if (s==4){
#ifndef POCKETCAS
      if ( (has_num_coeff(v[0]) ||
	    v[2].type==_FLOAT_ || v[2].type==_DOUBLE_ || v[2].type==_REAL ||
	    v[3].type==_FLOAT_ || v[3].type==_DOUBLE_ || v[3].type==_REAL)){
	vecteur ld=makevecteur(unsigned_inf,cst_pi);
	// should first remove mute variables inside embedded sum/int/fsolve
	lidnt(makevecteur(true_lidnt(v[0]),evalf_double(v[2],1,contextptr),evalf_double(v[3],1,contextptr)),ld,false);
	ld.erase(ld.begin());
	ld.erase(ld.begin());
	if (ld==vecteur(1,v[1]) || ld.empty())
	  return intnum(gen(makevecteur(v[0],v[1],v[2],v[3]),_SEQ__VECT),false,contextptr,true);
      }
#endif
      v0orig=v[0];
      aorig=borne_inf=v[2];
      borig=borne_sup=v[3];
      if (borne_inf==borne_sup)
	return 0;
      if (integrate_trig_periods(v[0],x,borne_inf,borne_sup,res,contextptr))
        return res;
      unsigned parity_budget=64;
      if (!is_inf(borne_inf) && !is_inf(borne_sup) &&
          is_zero(borne_inf+borne_sup) &&
          continuous_parity(v[0],x,parity_budget)==1)
        return 0;
      gen power_primitive;
      if (!is_inf(borne_inf) && !is_inf(borne_sup) &&
          (integrate_large_power(v[0],x,power_primitive,contextptr) ||
           integrate_sparse_atan(v[0],x,power_primitive,contextptr)))
        return subst(power_primitive,x,borne_sup,false,contextptr)-
          subst(power_primitive,x,borne_inf,false,contextptr);
      v[0]=ceil2floor(v[0],contextptr,true);
      vecteur lfloor(lop(v[0],at_floor));
      lfloor=lvarx(lfloor,x);
      if (!lfloor.empty()){
	gen a,b,l,cond=lfloor.front()._SYMBptr->feuille,tmp;
	if (lvarx(cond,x).size()>1 || !is_linear_wrt(cond,x,a,b,contextptr) ){
	  *logptr(contextptr) << gettext("Floor definite integration: can only handle linear < or > condition") << '\n';
	  if (!tegral(v0orig,x,aorig,borig,1e-12,(1<<10),res,true,contextptr))
	    return undef;
	  return res;
	}
	if (is_inf(borne_inf) || is_inf(borne_sup)){
	  *logptr(contextptr) << gettext("Floor definite integration: unable to handle infinite boundaries") << '\n';
	}
	else {
	  // find integers of the form a*x+b in [borne_inf,borne_sup]
	  gen n1=_floor(a*borne_inf+b,contextptr);
	  // n1=a*x+b -> x=(n1-b)/a
	  gen stepx,stepn;
	  if (is_positive(a,contextptr)){
	    stepx=inv(a,contextptr);
	    stepn=1;
	  }
	  else {
	    stepx=-inv(a,contextptr);
	    stepn=-1;
	  }
	  gen cur=borne_inf,next=(n1+stepn-b)/a,res=0;
	  if (stepn==-1 && n1==a*borne_inf+b)
	    n1 -= 1;
	  for (;is_greater(borne_sup,next,contextptr); cur=next,next+=stepx,n1+=stepn){
	    tmp=quotesubst(v[0],lfloor.front(),n1,contextptr);
	    res += _integrate(makesequence(tmp,x,cur,next),contextptr);
#ifdef TIMEOUT
	    control_c();
#endif
	    if (ctrl_c || interrupted) { 
	      interrupted = true; ctrl_c=false;
	      gensizeerr(gettext("Stopped by user interruption."),res);
	      return res;
	    }
	    if (is_undef(res))
	      return res;
	  }
	  tmp=quotesubst(v[0],lfloor.front(),n1,contextptr);
	  res += _integrate(makesequence(tmp,x,cur,borne_sup),contextptr);
	  return ck_int_numerically(v0orig,x,aorig,borig,res,contextptr);
	}
      }
      v[0]=whenmaxmin2piecewise(v[0],contextptr);
      vecteur lpiece(lop(v[0],at_piecewise));
      lpiece=lvarx(lpiece,x);
      if (!lpiece.empty()){
	bool chsign=is_strictly_greater(borne_inf,borne_sup,contextptr);
	if (chsign)
	  swapgen(borne_inf,borne_sup);
	res=0;
	gen piece=lpiece.front();
	if (!piece.is_symb_of_sommet(at_piecewise))
	  return gensizeerr(contextptr);
	gen piecef=piece._SYMBptr->feuille;
	if (piecef.type!=_VECT || piecef._VECTptr->size()<2)
	  return gensizeerr(contextptr);
	vecteur & piecev = *piecef._VECTptr;
	// check conditions: they must be linear wrt x
	int vs=int(piecev.size());
	for (int i=0;i<vs/2;++i){
	  bool unable=true;
	  gen cond=piecev[2*i];
	  if (is_zero(cond))
	    continue;
	  if (is_one(cond)){
	    gen tmp=quotesubst(v[0],piece,piecev[2*i+1],contextptr);
	    res += _integrate(gen(makevecteur(tmp,x,borne_inf,borne_sup),_SEQ__VECT),contextptr);
	    return ck_int_numerically(v0orig,x,aorig,borig,(chsign?-res:res),contextptr);
	  }
	  if (is_equal(cond) || cond.is_symb_of_sommet(at_same)){
	    *logptr(contextptr) << gettext("Assuming false condition ") << cond << '\n';
	    continue;
	  }
	  if (cond.is_symb_of_sommet(at_different)){
	    *logptr(contextptr) << gettext("Assuming true condition ") << cond << '\n';
	    v[0]=quotesubst(v[0],piece,piecev[2*i+1],contextptr);
	    res += _integrate(gen(makevecteur(v[0],x,borne_inf,borne_sup),_SEQ__VECT),contextptr);
	    return ck_int_numerically(v0orig,x,aorig,borig,(chsign?-res:res),contextptr);
	  }
	  if (cond.is_symb_of_sommet(at_superieur_strict) || cond.is_symb_of_sommet(at_superieur_egal)){
	    cond=cond._SYMBptr->feuille[0]-cond._SYMBptr->feuille[1];
	    unable=false;
	  }
	  if (cond.is_symb_of_sommet(at_inferieur_strict) || cond.is_symb_of_sommet(at_inferieur_egal)){
	    cond=cond._SYMBptr->feuille[1]-cond._SYMBptr->feuille[0];
	    unable=false;
	  }
	  gen a,b,l;
	  if (unable || !is_linear_wrt(cond,x,a,b,contextptr)){
	    *logptr(contextptr) << gettext("Piecewise definite integration: can only handle linear < or > condition") << '\n';
	    if (!tegral(v0orig,x,aorig,borig,1e-12,(1<<10),res,true,contextptr))
	      return undef;
	    return res;
	  }
	  // check if a*x+b>0 on [borne_inf,borne_sup]
	  l=-b/a;
	  bool positif=ck_is_greater(a,0,contextptr);
	  gen tmp=quotesubst(v[0],piece,piecev[2*i+1],contextptr);
	  if (ck_is_greater(l,borne_sup,contextptr)){
	    // borne_inf < borne_sup <= l
	    if (positif) // test is false, continue
	      continue;
	    // test is true we can compute the integral
	    res += _integrate(gen(makevecteur(tmp,x,borne_inf,borne_sup),_SEQ__VECT),contextptr);
	    return ck_int_numerically(v0orig,x,aorig,borig,(chsign?-res:res),contextptr);
	  }
	  if (ck_is_greater(borne_inf,l,contextptr)){
	    // l <= borne_inf < borne_sup
	    if (!positif) // test is false, continue
	      continue;
	    // test is true we can compute the integral
	    res += _integrate(gen(makevecteur(tmp,x,borne_inf,borne_sup),_SEQ__VECT),contextptr);
	    return ck_int_numerically(v0orig,x,aorig,borig,(chsign?-res:res),contextptr);
	  }
	  // borne_inf<l<borne_sup
	  if (positif){
	    // compute integral between l and borne_sup
	    res += _integrate(gen(makevecteur(tmp,x,l,borne_sup),_SEQ__VECT),contextptr);
	    borne_sup=l; // continue with integral from borne_inf to l
	    continue;
	  }
	  // compute integral between borne_inf and l
	  res += _integrate(gen(makevecteur(tmp,x,borne_inf,l),_SEQ__VECT),contextptr);
	  borne_inf=l; // continue with integral from l to borne_sup
	}
	if (vs%2){
	  v[0]=quotesubst(v[0],piece,piecev[vs-1],contextptr);
	  res += _integrate(gen(makevecteur(v[0],x,borne_inf,borne_sup),_SEQ__VECT),contextptr);
	}
	return ck_int_numerically(v0orig,x,aorig,borig,(chsign?-res:res),contextptr); // return chsign?-res:res;
      } // end piecewise
      if (intgab(v[0],x,borne_inf,borne_sup,res,contextptr)){
	// additional check for singularities in ggb mode
	if (calc_mode(contextptr)==1 || abs_calc_mode(contextptr)==38){
	  bool ordonne=is_greater(borne_sup,borne_inf,contextptr);
	  vecteur sp=protect_find_singularities(v[0],*x._IDNTptr,false,contextptr);
	  int sps=int(sp.size());
	  for (int i=0;i<sps;i++){
	    if ( (ordonne && is_strictly_greater(sp[i],borne_inf,contextptr) && is_strictly_greater(borne_sup,sp[i],contextptr) ) || 
		 (!ordonne && is_strictly_greater(sp[i],borne_sup,contextptr) && is_strictly_greater(borne_inf,sp[i],contextptr) )
		 ){
	      if (!is_zero(limit(v[0]*(sp[i]-x),*x._IDNTptr,sp[i],0,contextptr)))
		return undef;
	    }   
	  }
	}
      	return res;
      }
    }
    gen primitive;
    // fast check if we are integrating over a period
    // if so we can shift integration to
    // simplify one of the functions
    if (s==4 && !is_inf(borne_sup) && !is_inf(borne_inf)){
      gen v0ab=subst(v[0],x,x+borne_sup-borne_inf,false,contextptr)-v[0];
      gen tmpv0ab=recursive_ratnormal(v0ab,contextptr);
      if (!contains(tmpv0ab,undef)) 
	v0ab=tmpv0ab;
      if (is_zero(v0ab)){
	vecteur l(rlvarx(v[0],x));
	unsigned i=0; gen a,b;
	for (;i<l.size();++i){
	  if (l[i].type==_SYMB && is_linear_wrt(l[i]._SYMBptr->feuille,x,a,b,contextptr) && !is_zero(a) && !is_zero(b))
	    break;
	}
	if (i<l.size()){
	  vecteur vin=makevecteur(x,l[i]),vout=makevecteur(x-b/a,l[i]._SYMBptr->sommet(a*x,contextptr));
	  v[0]=subst(v[0],vin,vout,false,contextptr);
	}
      }
    }
    if (s==2){
      if (calc_mode(contextptr)!=1 &&
          (integrate_large_power(v[0],x,primitive,contextptr) ||
           integrate_sparse_atan(v[0],x,primitive,contextptr) ||
           integrate_compact_primitive(v[0],x,primitive,contextptr) ||
           integrate_real_root(v[0],x,primitive,0,contextptr)))
        return primitive;
      primitive=integrate_chknum(v[0],x,rem,contextptr);
      if (calc_mode(contextptr)==1){
	++ggb_intcounter;
	primitive += diffeq_constante(ggb_intcounter,contextptr);
      }
      if (is_zero(rem))
	return primitive;
      return primitive + symbolic(at_integrate,gen(makevecteur(rem,x),_SEQ__VECT));
    }
    // here s==4
    bool ordonne=is_greater(borne_sup,borne_inf,contextptr);
    bool desordonne=false;
#ifdef NO_STDEXCEPT
    if (ordonne){
      gen xval=assumeeval(x,contextptr);
      giac_assume(symb_and(symb_superieur_egal(x,borne_inf),symb_inferieur_egal(x,borne_sup)),contextptr);
      primitive=integrate_chknum(v[0],x,rem,contextptr);
      primitive=eval(primitive,1,contextptr);
      restorepurge(xval,x,contextptr);
      res=limit(primitive,*x._IDNTptr,borne_sup,-1,contextptr)-limit(primitive,*x._IDNTptr,borne_inf,1,contextptr);
    }
    else {
      if ( (desordonne=is_greater(borne_inf,borne_sup,contextptr) )){
	gen xval=assumeeval(x,contextptr);
	giac_assume(symb_and(symb_superieur_egal(x,borne_sup),symb_inferieur_egal(x,borne_inf)),contextptr);
	primitive=integrate_chknum(v[0],x,rem,contextptr);
	primitive=eval(primitive,1,contextptr);
	restorepurge(xval,x,contextptr);
	res=limit(primitive,*x._IDNTptr,borne_sup,1,contextptr)-limit(primitive,*x._IDNTptr,borne_inf,-1,contextptr) ;
      }
      else {
	primitive=integrate_chknum(v[0],x,rem,contextptr);
	res=limit(primitive,*x._IDNTptr,borne_sup,0,contextptr)-limit(primitive,*x._IDNTptr,borne_inf,0,contextptr);
      }
    }
#else
    try {
      if (ordonne){
	gen xval=assumeeval(x,contextptr);
	giac_assume(symb_and(symb_superieur_egal(x,borne_inf),symb_inferieur_egal(x,borne_sup)),contextptr);
	primitive=integrate_chknum(v[0],x,rem,contextptr);
	primitive=eval(primitive,1,contextptr);
	restorepurge(xval,x,contextptr);
	gen ri=limit(primitive,*x._IDNTptr,borne_inf,1,contextptr);
	gen rs=limit(primitive,*x._IDNTptr,borne_sup,-1,contextptr);
	res=rs-ri;
      }
      else {
	if ( (desordonne=is_greater(borne_inf,borne_sup,contextptr) )){
	  gen xval=assumeeval(x,contextptr);
	  giac_assume(symb_and(symb_superieur_egal(x,borne_sup),symb_inferieur_egal(x,borne_inf)),contextptr);
	  primitive=integrate_chknum(v[0],x,rem,contextptr);
	  primitive=eval(primitive,1,contextptr);
	  restorepurge(xval,x,contextptr);
	  res=limit(primitive,*x._IDNTptr,borne_sup,1,contextptr)-limit(primitive,*x._IDNTptr,borne_inf,-1,contextptr) ;
	}
	else {
	  primitive=integrate_chknum(v[0],x,rem,contextptr);
	  res=limit(primitive,*x._IDNTptr,borne_sup,0,contextptr)-limit(primitive,*x._IDNTptr,borne_inf,0,contextptr);
	}
      }
    } catch (std::runtime_error & e){
      last_evaled_argptr(contextptr)=NULL;
      *logptr(contextptr) << "Error trying to find limit of " << primitive << '\n';
      return symbolic(at_integrate,makesequence(v[0],x,borne_inf,borne_sup));
    }
#endif
    if (!lop(res,at_bounded_function).empty())
      res=undef;
    if (is_undef(res)){
      if (res.type==_STRNG && abs_calc_mode(contextptr)==38)
	return res;
      res=subst(primitive,*x._IDNTptr,borne_sup,false,contextptr)-subst(primitive,*x._IDNTptr,borne_inf,false,contextptr);
    }
    vecteur sp;
    gen prim2(primitive);
    // remove multiplicative constants to compute sp
    if (prim2.is_symb_of_sommet(at_prod)){
      gen primf=prim2._SYMBptr->feuille;
      if (primf.type==_VECT){
	vecteur primv=*primf._VECTptr,primv2;
	for (int i=0;i<primv.size();++i){
	  if (contains(lidnt(primv[i]),x))
	    primv2.push_back(primv[i]);
	}
	prim2=symbolic(at_prod,gen(primv2,_SEQ__VECT));
      }
    }
    sp=lidnt(evalf(makevecteur(prim2,borne_inf,borne_sup),1,contextptr));
    if (sp.size()>1){
      *logptr(contextptr) << gettext("No checks were made for singular points of antiderivative ")+primitive.print(contextptr)+gettext(" for definite integration in [")+borne_inf.print(contextptr)+","+borne_sup.print(contextptr)+"]" << '\n' ;
      sp.clear();
    }
    else {
      if ((is_inf(borne_inf) || evalf_double(borne_inf,1,contextptr).type==_DOUBLE_)
	  && (is_inf(borne_sup) || evalf_double(borne_sup,1,contextptr).type==_DOUBLE_)){
	gen xval=assumeeval(x,contextptr);
	if (is_greater(borne_sup,borne_inf,contextptr))
	  giac_assume(symb_and(symb_superieur_egal(x,borne_inf),symb_inferieur_egal(x,borne_sup)),contextptr);
	else
	  giac_assume(symb_and(symb_superieur_egal(x,borne_sup),symb_inferieur_egal(x,borne_inf)),contextptr);
	sp=protect_find_singularities(primitive,*x._IDNTptr,2,contextptr);
	restorepurge(xval,x,contextptr);
	if (!lidnt(evalf_double(sp,1,contextptr)).empty())
	  return gensizeerr("Unable to handle singularities of "+ primitive.print(contextptr)+" at "+gen(sp).print(contextptr));
      }
      else
	sp=protect_find_singularities(primitive,*x._IDNTptr,0,contextptr);
      if (is_undef(sp)){
	*logptr(contextptr) << gettext("Unable to find singular points of antiderivative") << '\n' ;
	if (!tegral(v0orig,x,aorig,borig,1e-12,(1<<10),res,true,contextptr))
	  return undef;
	return res;
      }
    }
    // FIXME if v depends on an integer parameter, find values in inf,sup
    comprim(sp);
    int sps=int(sp.size());
    for (int i=0;i<sps;i++){
      if (sp[i].type==_DOUBLE_ || sp[i].type==_REAL || has_op(sp[i],*at_rootof)){
	*logptr(contextptr) << gettext("Unable to handle approx. or algebraic extension singular point ")+sp[i].print(contextptr)+gettext(" of antiderivative");
	if (!tegral(v0orig,x,aorig,borig,1e-12,(1<<10),res,true,contextptr))
	  return undef;
	return res;
      }
      if ( (ordonne && is_strictly_greater(sp[i],borne_inf,contextptr) && is_strictly_greater(borne_sup,sp[i],contextptr) ) || 
	   (desordonne && is_strictly_greater(sp[i],borne_sup,contextptr) && is_strictly_greater(borne_inf,sp[i],contextptr) )
	   )
	res += limit(primitive,*x._IDNTptr,sp[i],-1,contextptr)-limit(primitive,*x._IDNTptr,sp[i],1,contextptr);
    }
    if (!is_zero(rem)){
      if (is_inf(res))
	return symbolic(at_integrate,gen(makevecteur(v[0],x,v[2],v[3]),_SEQ__VECT));
      if (ordonne || !desordonne)
	res = res + symbolic(at_integrate,gen(makevecteur(rem,x,v[2],v[3]),_SEQ__VECT));
      else
	res = res - symbolic(at_integrate,gen(makevecteur(rem,x,v[3],v[2]),_SEQ__VECT));
      return res;
    }
    return ck_int_numerically(v0orig,x,aorig,borig,res,contextptr);
  }

  // for inputs like integrate(sqrt(x^2.),x,-1,0);
  gen exactify_pow(const gen & g){
    if (g.type==_VECT){
      vecteur v=*g._VECTptr;
      for (int i=0;i<v.size();++i)
        v[i]=exactify_pow(v[i]);
      return gen(v,g.subtype);
    }
    if (g.type!=_SYMB)
      return g;
    gen f=exactify_pow(g._SYMBptr->feuille);
    if (g._SYMBptr->sommet!=at_pow)
      return symbolic(g._SYMBptr->sommet,f);
    if (f.type!=_VECT || f._VECTptr->size()!=2)
      return symbolic(at_pow,f);
    gen f1=f._VECTptr->back();
    if (f1.type==_DOUBLE_ && f1._DOUBLE_val==int(f1._DOUBLE_val))
      f1=int(f1._DOUBLE_val);
    else if (f1.type==_FLOAT_ && f1._FLOAT_val==int(get_double(f1._FLOAT_val)))
      f1=int(get_double(f1._FLOAT_val));
    else return symbolic(at_pow,f);;
    return symbolic(at_pow,makesequence(f._VECTptr->front(),f1));
  }
  // "unary" version
  gen _integrate(const gen & args_,GIAC_CONTEXT){
    gen args(exactify_pow(args_));
    if (complex_variables(contextptr))
      *logptr(contextptr) << gettext("Warning, complex variables is set, this can lead to fairly complex answers. It is recommended to switch off complex variables in the settings or by complex_variables:=0; and declare individual variables to be complex by e.g. assume(a,complex).") << '\n';
    vecteur ass;
    if (auto_assume(contextptr)){
      if (args.type==_VECT && args._VECTptr->size()>=2)
	ass=autoassume(args._VECTptr->front(),(*args._VECTptr)[1],contextptr);
      else if (args.type==_SYMB || args.type==_IDNT)
	ass=autoassume(args,vx_var,contextptr);
    }
    if (!ass.empty()){
      *logptr(contextptr) << "Run purge(" << ass << "); or purge(unquote(assumptions)) to clear auto-assumptions\n" ;
      sto(ass,gen("assumptions",contextptr),contextptr);
    }
    if (args.type==_VECT && args._VECTptr->size()==4
#if defined(FXCG) || defined(KHICAS_TEST_INTEGRATION_LIMITS)
        && step_infolevel(contextptr) // optional log preview, not result validation
#endif
        ){
      const vecteur &v = *args._VECTptr;
      gen x=v[1],a=v[2],b=v[3];
      if (x.type==_IDNT && a.type!=_DOUBLE_ && b.type!=_DOUBLE_){
        gen v0=evalf_double(v[0],1,contextptr),resapprox,tmp;
        vecteur lv=lidnt(v0);
        if (lv.size()==1 && lv[0]==x && has_evalf(a,tmp,1,contextptr) && has_evalf(b,tmp,1,contextptr) && tegral(v[0],x,a,b,1e-6,(1<<10),resapprox,false,contextptr)){
          *logptr(contextptr) << "// ~= " << resapprox << "\n";
        }
      }
    }
    gen res=_integrate_(args,contextptr);
    if (0){
      for (int i=0;i<ass.size();++i){
        purgenoassume(ass[i],contextptr);
      }
    }
    return res;
  }
  static const char _integrate_s []="integrate";
  static string texprintasintegrate(const gen & g,const char * s_orig,GIAC_CONTEXT){
    string s("\\int ");
    if (g.type!=_VECT)
      return s+gen2tex(g,contextptr);
    const vecteur &v=*g._VECTptr;
    int l(int(v.size()));
    if (!l)
      return s;
    if (l==1)
      return s+gen2tex(v.front(),contextptr);
    if (l==2)
      return s+gen2tex(v.front(),contextptr)+"\\, d"+gen2tex(v.back(),contextptr);
    if (l==4)
      return s+"_{"+gen2tex(v[2],contextptr)+"}^{"+gen2tex(v[3],contextptr)+"}"+gen2tex(v.front(),contextptr)+"\\, d"+gen2tex(v[1],contextptr);
    return s;
  }
  static define_unary_function_eval4_quoted (__integrate,&_integrate,_integrate_s,0,&texprintasintegrate);
  define_unary_function_ptr5( at_integrate ,alias_at_integrate,&__integrate,_QUOTE_ARGUMENTS,true);

  // called by approx_area
  double rombergo(const gen & f0,const gen & x, const gen & a_orig, const gen & b_orig, int n,GIAC_CONTEXT){
    //f est l'expression a integrer, x le nom de la variable, a et b les bornes
    // n si on veut faire 2^n subdivisions
    gen f=eval(f0,1,context0); // otherwise int(1/sqrt(x),x,0,1) fails on 38
    vector<double> T(n+1);
    //ligne du triangle de romberg avec T(n)=aire avec "pts du milieu" 
    //avec 2^n subdivisions
    double  h;
    gen at;
    //at sert a faire les substitutions c'est un gen = au debut a f((a+b)/2)
    //et en cours de prog egal a f(am)
    gen a=a_orig,b=b_orig;
    a=a.evalf(1,contextptr).evalf_double(1,contextptr);
    b=b.evalf(1,contextptr).evalf_double(1,contextptr);
    h=b._DOUBLE_val-a._DOUBLE_val;
    if (h==0)
      return 0;
    //h est la longueur de la subdivision
    //T[0] est l'aire du premier rectangle "pt milieu" f((a+b)/2)*(b-a)
    //puis T[j] = aire des rectangles "pt milieu" pour 2^j subdivisions
    double pui4;
    for (int j=0;j<=n;j++){
      //chaque fois que j augmente de 1 on double le nombre de subdivisions
    
      double ss;
      //ss est la somme provenant des valeurs de f aux points am ainsi rajoutes
      ss=0;
      gen am;
    
      am=a+gen(h/2);
      if (is_exactly_zero(am-a)){
	n=j-1;
	break;
      }
      while (is_greater(b,am,contextptr)){
	at=subst(f,x,am,false,contextptr).evalf(1,contextptr);
	ss=ss+at._DOUBLE_val;
	am=am+gen(h);
      }
      //T[j] est la nouvelle valeur de l'aire calculee avec les "pts  milieu"
      T[j]=ss*h;
    
      h=h/2;
    }
    //pui4 est la valeur de 4^k
    pui4=1;
    for (int j=1;j<=n;j++){ 
      pui4=pui4*4;
      for (int k=0;k<=n-j;k++){
	//on calcule T[k] en appliquant la formule de rec. de romberg
	//avec T[j] qui contient a chaque etape l'integrale par les pts milieu
	T[k]=(pui4*T[k+1]-T[k])/(pui4-1);
     
      }
      //on vient de remplir la kieme ligne on recommence avec j=j+1
      //on doit calculer les "pts du milieu" pour le nouv. j et le mettre ds T[j]
    }
 

    //c'est donc T[0] la meilleur approx de l'integrale
    return(T[0]);
  }

  // Not linked currently
  double rombergt(const gen & f,const gen & x, const gen & a, const gen & b, int n,GIAC_CONTEXT){
    //f est l'expression a integrer, x le nom de la variable, a et b les bornes
    // n si on veut faire 2^n subdivisions
    vector<double> T(n+1);
    //ligne du triangle de romberg avec T(n)=trapezes avec 2^n subdivisions
    double  h;
    gen at;
    //at sert a faire les substitutions c'est un gen egal au debut a f(a)+f(b)
    //et en cours de prog egal a f(am)
    h=b.evalf(1,contextptr)._DOUBLE_val-a.evalf(1,contextptr)._DOUBLE_val;
    if (h==0)
      return 0;
    //h est la longueur de la subdivision
    at=subst(f,x,b,false,contextptr).evalf(1,contextptr)+subst(f,x,a,false,contextptr).evalf(1,contextptr);
    T[0]=at._DOUBLE_val*h/2;
    //T[0] est l'aire du premier trapeze (f(a)+f(b))*(b-a)/2
    //puis T[j] = aire des trapezes pour 2^j subdivisions
    double pui4;
    for (int j=1;j<=n;j++){
      //chaque fois que j augmente de 1 on double le nombre de subdivisions
      h=h/2;
      double ss;
      //ss est la somme provenant des valeurs de f aux points am ainsi rajoutes
      ss=0;
      gen am;
      am=a+gen(h); 
      if (is_exactly_zero(am-a)){
	n=j-1;
	break;
      }
      while (is_greater(b,am,contextptr)){
	at=subst(f,x,am,false,contextptr).evalf(1,contextptr);
	ss=ss+at._DOUBLE_val;
	am=am+gen(2*h);
      }
      //T[j] est la nouvelle valeur de l'aire des trapezes
      T[j]=T[j-1]/2+ss*h;
      //pui4 est la valeur de 4^k
      pui4=1;
      for (int k=j-1;k>=0;k--){
	pui4=pui4*4;
	//on calcule T[k] en appliquant la formule de rec. de romberg
	//avec T[j] qui contient a chaque etape l'integrale par les trapezes
	T[k]=(pui4*T[k+1]-T[k])/(pui4-1);
      }
      //on vient de remplir la kieme ligne on recommence avec j=j+1
      //on doit calculer les trapezes pour le nouveau j et le mettre ds T[j]
    }
    //c'est donc T[0] la meilleur approx de l'integrale
    return(T[0]);
  }

  // find approx value of int(f) using Gauss quadrature with s=15 (order 30)
  // returns approx value of int(f), of int(abs(f)) and error estimate
  // error estimated using embedded order 14 and 6 method as
  // err1=abs(i30-i14); err2=abs(i30-i6); err1*(err1/err2)^2
#if 0
  static bool tegral_util(const gen & f,const gen &x, const gen &a,const gen &b,gen & i30,gen & i30abs, gen &err,GIAC_CONTEXT){
    gen h=evalf_double(b-a,1,contextptr),i14,i6;
    int s30=15,s14=14,s6=6;
    long_double c30[]={0.60037409897572857552e-2,0.31363303799647047846e-1,0.75896708294786391900e-1,0.13779113431991497629,0.21451391369573057623,0.30292432646121831505,0.39940295300128273885,0.50000000000000000000,0.60059704699871726115,0.69707567353878168495,0.78548608630426942377,0.86220886568008502371,0.92410329170521360810,0.96863669620035295215,0.99399625901024271424};
    long_double b30[]={0.15376620998058634177e-1,0.35183023744054062355e-1,0.53579610233585967506e-1,0.69785338963077157224e-1,0.83134602908496966777e-1,0.93080500007781105513e-1,0.99215742663555788228e-1,0.10128912096278063644,0.99215742663555788228e-1,0.93080500007781105514e-1,0.83134602908496966777e-1,0.69785338963077157224e-1,0.53579610233585967507e-1,0.35183023744054062355e-1,0.15376620998058634177e-1};
    long_double b14[]={0.21474028217339757006e-1,0.14373155100418764102e-1,0.92599218105237092609e-1,0.11827741709315709983e-1,0.15847003639679458478,0.38429189419875016111e-2,0.19741290152890658991,0.19741290152890658991,0.38429189419875016111e-2,0.15847003639679458478,0.11827741709315709983e-1,0.92599218105237092608e-1,0.14373155100418764102e-1,0.21474028217339757006e-1};
    long_double b6[]={0.10715760948621577132,0.31130901929813818033e-1,0.36171148858397041065,0,0.36171148858397041065,0.31130901929813818033e-1,0.10715760948621577132};
    vecteur v30(15),v30abs(15);
    for (int i=0;i<15;i++){
      v30[i]=evalf_double(subst(f,x,a+double(c30[i])*h,false,contextptr),1,contextptr);
      v30abs[i]=_l2norm(v30[i],contextptr);
      if (v30abs[i].type!=_DOUBLE_)
	return false;
    }
    i30abs=i30=i14=i6=0;
    for (int i=0;i<15;i++){
      i30 += double(b30[i])*v30[i];
      i30abs += double(b30[i])*v30abs[i];
    }
    for (int i=0;i<=6;i++){
      i14 += double(b14[i])*v30[i];
    }
    for (int i=8;i<=14;i++){
      i14 += double(b14[i-1])*v30[i];
    }
    for (int i=1;i<15;i+=2){
      if (i==7)
	continue;
      i6 += double(b6[(i-1)/2])*v30[i];
    }
    i30 = i30*h;
    i30abs = i30abs*h;
    i14 = i14*h;
    i6 = i6*h;
    gen err1=_l2norm(i30-i14,contextptr);
    gen err2=_l2norm(i30-i6,contextptr);
    // check if err1 and err2 corresponds to errors in h^14 and h^6
    if (is_greater(abs(14./6.-ln(err1,contextptr)/ln(err2,contextptr)),.1,contextptr))
      err=err1;
    else {
      err=err1/err2;
      err=err1*(err*err);
    }
    return true;
  }
#else // using -1..1 scaling instead of 0..1
  static bool tegral_util(const gen & f,const gen &x, const gen &a,const gen &b,gen & i30,gen & i30abs, gen &err,GIAC_CONTEXT){
    gen h=evalf_double(b-a,1,contextptr),i14,i6;
    //int s30=15,s14=14,s6=6;
    long_double c30[]={-0.98799251802048542849,-0.93727339240070590430,-0.84820658341042721620,-0.72441773136017004742,-0.57097217260853884754,-0.39415134707756336990,-0.20119409399743452230,0.00000000000000000000,0.20119409399743452230,0.39415134707756336990,0.57097217260853884754,0.72441773136017004742,0.84820658341042721620,0.93727339240070590430,0.98799251802048542849};
    long_double b30[]={0.15376620998058634177e-1,0.35183023744054062355e-1,0.53579610233585967506e-1,0.69785338963077157224e-1,0.83134602908496966777e-1,0.93080500007781105513e-1,0.99215742663555788228e-1,0.10128912096278063644,0.99215742663555788228e-1,0.93080500007781105514e-1,0.83134602908496966777e-1,0.69785338963077157224e-1,0.53579610233585967507e-1,0.35183023744054062355e-1,0.15376620998058634177e-1};
    long_double b14[]={0.21474028217339757006e-1,0.14373155100418764102e-1,0.92599218105237092609e-1,0.11827741709315709983e-1,0.15847003639679458478,0.38429189419875016111e-2,0.19741290152890658991,0.19741290152890658991,0.38429189419875016111e-2,0.15847003639679458478,0.11827741709315709983e-1,0.92599218105237092608e-1,0.14373155100418764102e-1,0.21474028217339757006e-1};
    long_double b6[]={0.10715760948621577132,0.31130901929813818033e-1,0.36171148858397041065,0,0.36171148858397041065,0.31130901929813818033e-1,0.10715760948621577132};
    vecteur v30(15),v30abs(15);
    for (int i=0;i<15;i++){
      v30[i]=evalf_double(eval(subst(f,x,((a+b)+double(c30[i])*h)/2,false,contextptr),1,contextptr),1,contextptr);
      v30abs[i]=_l2norm(v30[i],contextptr);
      if (v30abs[i].type!=_DOUBLE_)
	return false;
    }
    i30abs=i30=i14=i6=0;
    for (int i=0;i<=7;i++){
      i30 += double(b30[i])*v30[i];
      if (i<7)
	i30 += double(b30[14-i])*v30[14-i];
      i30abs += double(b30[i])*v30abs[i];
      if (i<7)
	i30abs += double(b30[14-i])*v30abs[14-i];
    }
    for (int i=0;i<=6;i++){
      i14 += double(b14[i])*v30[i];
    }
    for (int i=8;i<=14;i++){
      i14 += double(b14[i-1])*v30[i];
    }
    for (int i=1;i<15;i+=2){
      if (i==7)
	continue;
      i6 += double(b6[(i-1)/2])*v30[i];
    }
    i30 = i30*h;
    i30abs = i30abs*h;
    i14 = i14*h;
    i6 = i6*h;
    gen err1=_l2norm(i30-i14,contextptr);
    gen err2=_l2norm(i30-i6,contextptr);
    if (is_exactly_zero(err1) || is_exactly_zero(err2))
      err=0;
    else {
      // check if err1 and err2 corresponds to errors in h^14 and h^6
      if (is_greater(abs(14./6.-ln(err1,contextptr)/ln(err2,contextptr)),.1,contextptr))
	err=err1;
      else {
	err=err1/err2;
	err=err1*(err*err);
      }
    }
    return true;
  }
#endif

  bool approxint_exact(const gen &f,const gen &x,GIAC_CONTEXT){
    if (!lop(f,at_when).empty() || !lop(f,at_piecewise).empty())
      return false;
    if (!loptab(Heavisidetosign(f,contextptr),sign_floor_ceil_round_tab).empty() )
      return false;
    if (f.type!=_SYMB || is_constant_wrt(f,x,contextptr))
      return true;
    unary_function_ptr & u=f._SYMBptr->sommet;
    gen g=f._SYMBptr->feuille,a,b,c;
    if (u==at_exp)
      return is_quadratic_wrt(g,x,a,b,c,contextptr);
    if (u==at_sin || u==at_cos)
      return is_linear_wrt(g,x,a,b,contextptr);
    if (g.type!=_VECT) return false;
    const_iterateur it=g._VECTptr->begin(),itend=g._VECTptr->end();
    if (u==at_plus){
      for (;it!=itend;++it){
	if (!approxint_exact(*it,x,contextptr))
	  return false;
      }
      return true;
    }
    if (u==at_prod){
      for (;it!=itend;++it){
	if (is_constant_wrt(*it,x,contextptr))
	  continue;
	if (!is_zero(a))
	  return false;
	a=*it;
      }
      return approxint_exact(a,x,contextptr);
    }
    return false;
  }

  // nmax=max number of subdivisions (may be 1000 or more...)
  bool tegral(const gen & f,const gen & x,const gen & a_,const gen &b_,const gen & eps,int nmax,gen & value,bool exactcheck,GIAC_CONTEXT){
    gen a=evalf(a_,1,contextptr),b=evalf(b_,1,contextptr);
    if (a==b){
      value=0.0;
      return true;
    }
    if (exactcheck){
      vecteur vf(1,x);
      rlvarx(f,x,vf);
      if (0 && vf.size()<=1){ // dangerous
	gen r,F=linear_integrate(exact(f,contextptr),x,r,0,contextptr);
	value=_limit(makesequence(F,x,exact(b,contextptr),-1),contextptr)-_limit(makesequence(F,x,exact(a,contextptr),1),contextptr);
	value=evalf(value,1,contextptr);
	return true;
      }
      if (approxint_exact(f,x,contextptr)){
	gen r,F=linear_integrate(f,x,r,0,contextptr);
	if (is_zero(r)){
	  value=subst(F,x,b,false,contextptr)-subst(F,x,a,false,contextptr);
	  return true;
	}
      }
    }
    // adaptive integration, cf. Hairer
    gen i30,i30abs,err,maxerr,ERR,I30ABS;
    int maxerrpos;
    if (!tegral_util(f,x,a,b,i30,i30abs,err,contextptr))
      return false;
    vecteur v(1,makevecteur(a,b,i30,i30abs,err));
    for (;int(v.size())<nmax;){
      // sum of errors, check for end
      i30=I30ABS=ERR=maxerr=0;
      maxerrpos=0;
      for (unsigned i=0;i<v.size();++i){
	if (v[i].type!=_VECT || v[i]._VECTptr->size()<5)
	  return false;
	vecteur w=*v[i]._VECTptr;
	i30 = i30+w[2]; // += does not work in emscripten
	I30ABS = I30ABS+w[3];
	ERR = ERR+w[4];
	if (is_strictly_greater(w[4],maxerr,contextptr)){
	  maxerrpos=i;
	  maxerr=w[4];
	}
      }
      value=i30;
      // could add a minimal number of intervals for integrals like
      // integrate(when(x > 2, 1,2),x,0,2.01) or int(frac(x),x,0,6.01)
      // but one will always find intervals where this would fail
      if (!is_undef(ERR) && is_greater(eps,ERR/I30ABS,contextptr)) 
	return true;
      // cut interval at maxerrpos in 2 parts
      vecteur & w = *v[maxerrpos]._VECTptr;
      gen A=w[0],B=w[1],C=(A+B)/2;
      if (A==C || B==C){
	// can not subdivise anymore
	if (is_greater(1e-4,ERR/I30ABS,contextptr)){
	  *logptr(contextptr) << "Low accuracy, error estimate " << ERR/I30ABS << "\nError might be underestimated if initial boundary was +/-infinity" << '\n';
	  return true;
	}
	return false; 
      }
      if (!tegral_util(f,x,A,C,i30,i30abs,err,contextptr)){
	if (is_greater(1e-4,ERR/I30ABS,contextptr)){
	  *logptr(contextptr) << "Low accuracy, error estimate " << ERR/I30ABS << "\nError might be underestimated if initial boundary was +/-infinity" << '\n';
	  return true;
	}
	return false;
      }
      v[maxerrpos]=makevecteur(A,C,i30,i30abs,err);
      if (!tegral_util(f,x,C,B,i30,i30abs,err,contextptr)){
	if (is_greater(1e-4,ERR/I30ABS,contextptr)){
	  *logptr(contextptr) << "Low accuracy, error estimate " << ERR/I30ABS << "\nError might be underestimated if initial boundary was +/-infinity" << '\n';
	  return true;
	}
	return false;
      }
      v.push_back(makevecteur(C,B,i30,i30abs,err));
    }
    return false; // too many iterations
  }

  gen romberg(const gen & f0,const gen & x0,const gen & a,const gen &b,const gen & eps,int nmax,GIAC_CONTEXT){
    return evalf_int(f0,x0,a,b,eps,nmax,true,contextptr,false);
  }
  gen evalf_int(const gen & f0,const gen & x0,const gen & a,const gen &b,const gen & eps,int nmax,bool romberg_method,GIAC_CONTEXT,bool exactcheck){
    gen x(x0),f(f0);
    if (x.type!=_IDNT){
      x=gen(identificateur("tmpx"));
      f=subst(f,x0,x,false,contextptr);
    }
    gen value=undef;
    if (!romberg_method && tegral(f,x,a,b,eps,(1 << nmax),value,exactcheck,contextptr))
      return value;
    if (!romberg_method)
      *logptr(contextptr) << "Adaptive method failure, will try with Romberg, last approximation was " << value << '\n';
    // a, b and eps should be evalf-ed, and eps>0
    gen h=b-a;
    vecteur old_line,cur_line;
#ifdef NO_STDEXCEPT
    old_line.push_back(evalf(h*(limit(f,*x._IDNTptr,a,1,contextptr)+limit(f,*x._IDNTptr,b,-1,contextptr))/2,eval_level(contextptr),contextptr));
#else
    try {
      old_line.push_back(evalf(h*(limit(f,*x._IDNTptr,a,1,contextptr)+limit(f,*x._IDNTptr,b,-1,contextptr))/2,eval_level(contextptr),contextptr));
    } catch (std::runtime_error & ){
      last_evaled_argptr(contextptr)=NULL;
      old_line=vecteur(1,undef);
    }
#endif
    if (is_inf(old_line[0])|| is_undef(old_line[0]) || !lop(old_line[0],at_bounded_function).empty()){
      // FIXME middle point in arbitrary precision
      *logptr(contextptr) << gettext("Infinity or undefined limit at bounds.\nUsing middle point Romberg method") << '\n';
      gen y=(a+b)/2;
      gen fy=subst(f,x,y,false,contextptr);
      // Workaround for undefined middle point
      if (is_undef(fy) || is_inf(fy)){
	fy=limit(f,*x._IDNTptr,y,0,contextptr);
	if (is_undef(fy) || is_inf(fy))
	  return undef;
      }
      old_line=vecteur(1,fy*h);
      // At the i-th step of the loop compute the middle approx of the integral
      // and use old_line to compute cur_line
      nmax=int(2.*nmax/3.+0.5);
      int n=3;
      h=(b-a)/3;
      for (int i=0;i<nmax;++i){
	cur_line.clear();
	// compute trapeze
	gen y=a+h/2,sum;
	if (is_exactly_zero(y-a))
	  return old_line;
	for (int j=0;j<n;++j){
	  if (j%3==1){
	    y=y+h; // skip, already computed
	    continue;
	  }
	  gen fy=subst(f,x,y,false,contextptr);
	  // Workaround if fy undefined
	  if (is_undef(fy) || is_inf(fy)){
	    fy=limit(f,*x._IDNTptr,y,0,contextptr);
	    if (is_undef(fy) || is_inf(fy))
	      return undef;
	  }
	  sum=sum+evalf(fy,eval_level(contextptr),contextptr);
	  y=y+h;
	}
	cur_line.push_back(old_line[0]/3+sum*h); 
	h=h/3;
	n = 3*n ;
	gen pui9=1;
	for (int j=0;j<=i;++j){
	  pui9=9*pui9;
	  cur_line.push_back((pui9*cur_line[j]-old_line[j])/(pui9-1));
	}
	gen err=abs(old_line[i]-cur_line[i+1],contextptr);
	if (i>nmax/2 && (ck_is_greater(eps,err,contextptr)
			 || ck_is_greater(eps*abs(cur_line[i+1],contextptr),err,contextptr)) )
	  return (old_line[i]+cur_line[i+1])/2;
	if (i!=nmax-1)
	  old_line=cur_line;
      }
      if (calc_mode(contextptr)==1)
	return undef;
      *logptr(contextptr) << gettext("Unable to find numeric integral using Romberg method, returning the last approximations") << '\n';
      cur_line=is_undef(value)?makevecteur(old_line.back(),cur_line.back()):makevecteur(cur_line.back(),value);
      return cur_line;
      // return rombergo(f,x,a,b,nmax,contextptr);
    }
    int n=1;
    // At the i-th step of the loop compute the trapeze approx of the integral
    // and use old_line to compute cur_line
    for (int i=0;i<nmax;++i){
      cur_line.clear();
      // compute trapeze
      gen y=a+h/2,sum;
      if (is_exactly_zero(y-a))
	return old_line;
      for (int j=0;j<n;++j){
	gen fy=subst(f,x,y,false,contextptr);
	// Workaround for romberg((1-cos(x))/x^2,x,-1,1)?
	if (is_undef(fy) || is_inf(fy)){
	  fy=limit(f,*x._IDNTptr,y,0,contextptr);
	  if (is_undef(fy) || is_inf(fy))
	    return undef;
	}
	sum=sum+evalf(fy,eval_level(contextptr),contextptr);
	y=y+h;
      }
      h=h/2;
      cur_line.push_back(old_line[0]/2+sum*h); 
      n = 2*n ;
      gen pui4=1;
      for (int j=0;j<=i;++j){
	pui4=4*pui4;
	cur_line.push_back((pui4*cur_line[j]-old_line[j])/(pui4-1));
      }
      gen err=abs(old_line[i]-cur_line[i+1],contextptr);
      if (i>nmax/2 && (ck_is_greater(eps,err,contextptr)
		       || ck_is_greater(eps*abs(cur_line[i+1],contextptr),err,contextptr)) )
	return (old_line[i]+cur_line[i+1])/2;
      if (i!=nmax-1)
	old_line=cur_line;
    }
    if (calc_mode(contextptr)==1)
      return undef;
    *logptr(contextptr) << gettext("Unable to find numeric integral using Romberg method, returning the last approximations") << '\n';
    cur_line=is_undef(value)?makevecteur(old_line.back(),cur_line.back()):makevecteur(cur_line.back(),value);
    return cur_line;
  }
  gen ggb_var(const gen & f){
    vecteur l=lidnt(makevecteur(cst_pi,unsigned_inf,undef,f));
    l=vecteur(l.begin()+3,l.end());
    if (l.empty() || equalposcomp(l,vx_var))
      return vx_var;
    const_iterateur it=l.begin(),itend=l.end();
    for (;it!=itend;++it){
      string s=it->print(context0);
      if (s[s.size()-1]=='x')
	return *it;
    }
    return l.front();
  }
  gen intnum(const gen & args,bool romberg_method,GIAC_CONTEXT,bool exactcheck){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()<2) )
      return gensizeerr(contextptr);
    const_iterateur it=args._VECTptr->begin(),itend=args._VECTptr->end();
    gen f=*it;
    ++it;
    gen x=*it,a,b;
    if (it<itend-1){
      ++it;
      a=*it;
      ++it;
      if (it<itend)
	b=*it;
      else {
	b=a;
	a=x;
	x=ggb_var(f);
	--it;
      }
    }
    else {
      bool ok=false;
      if (is_equal(x)){
	a=x._SYMBptr->feuille;
	if (a.type==_VECT && a._VECTptr->size()==2){
	  x=a._VECTptr->front();
	  a=a._VECTptr->back();
	  if (a.is_symb_of_sommet(at_interval)){
	    a=a._SYMBptr->feuille;
	    if (a.type==_VECT && a._VECTptr->size()==2){
	      b=a._VECTptr->back();
	      a=a._VECTptr->front();
	      ok=true;
	    }
	  }
	} 
      }
      if (!ok)
	return symbolic(at_integrate,args);
    }
    if (is_inf(a) || is_inf(b)){ // change of variables x=tan(t), t=atan(x)
      gen tanx(tan(x,contextptr));
      f=subst(f,x,tanx,false,contextptr)*(1+pow(tanx,2));
      a=atan(a,contextptr);
      b=atan(b,contextptr);
      gen res=intnum(makesequence(f,x,a,b),romberg_method,contextptr,exactcheck);
      if (!angle_radian(contextptr))
      {
	if(angle_degree(contextptr))
          res=deg2rad_d*res; 
        //grad
        else 
          res = grad2rad_d*res;
      }
      return res;
    }
    a=a.evalf(1,contextptr);
    b=b.evalf(1,contextptr);
    if (a.type==_FLOAT_) a=evalf_double(a,1,contextptr);
    if (b.type==_FLOAT_) b=evalf_double(b,1,contextptr);
    ++it;
    gen eps(epsilon(contextptr));
    int n=11;
    if (it!=itend){
      eps=evalf(abs(*it,contextptr),1,contextptr);
      ++it;
      if (it!=itend && it->type==_INT_)
	n=it->val;
    }
    if (eps.type!=_DOUBLE_ && eps.type!=_FLOAT_ && eps.type!=_REAL)
      eps=epsilon(contextptr);
    if ( x.type!=_IDNT || 
	 (a.type!=_DOUBLE_ && a.type!=_REAL) 
	 || (b.type!=_DOUBLE_ && b.type!=_REAL) 
	 )
      return symbolic(at_integrate,args);
    return evalf_int(f,x,a,b,eps,n,romberg_method,contextptr,exactcheck);
  }
  gen _romberg(const gen & args,GIAC_CONTEXT) {
    return intnum(args,true,contextptr,false);
  }
  static const char _romberg_s []="romberg";
  static define_unary_function_eval (__romberg,&_romberg,_romberg_s);
  define_unary_function_ptr5( at_romberg ,alias_at_romberg,&__romberg,0,true);

  gen _gaussquad(const gen & args,GIAC_CONTEXT) {
    return intnum(args,false,contextptr,false);
  }
  static const char _gaussquad_s []="gaussquad";
  static define_unary_function_eval (__gaussquad,&_gaussquad,_gaussquad_s);
  define_unary_function_ptr5( at_gaussquad ,alias_at_gaussquad,&__gaussquad,0,true);

/**********************************************************************
* Desc:		Solve P(x+1)-P(x)=Q(x)
* Algo:		degree of P=degree of Q+1, constant coeff 0
*		If P=Sigma a_k x^k then write linear system for a_k
*		Columns of the matrix of the system are lines
*		of the Pascal triangle without the first element
*		(since we must subtract identity matrix to the triangle)
*		a_1 a_2 a_3 ... a_n+1		coeff of Q
*		1   1   1	1	X^0
*		0   2	3	n+1	X^1
*		0   0	3	...	X^2
*		0   0	0	n+1     X^n
**********************************************************************/
  static vecteur solveP_x_plus_1_minus_P_x(const vecteur & Q){
      vecteur v(1,plus_one);
      matrice m;
      int n=int(Q.size());
      for (int i=0;i<n;++i){
          v=pascal_next_line(v);
          vecteur v_copy(v);
	  if (i!=n-1){
	    v_copy[i+1]=zero;
	    for (int j=0;j<n-i-2;j++)
	      v_copy.push_back(zero);
	  }
	  else
	    v_copy.pop_back();
          m.push_back(v_copy);
      }
      vecteur Q_copy(Q);
      reverse(Q_copy.begin(),Q_copy.end());
      m.push_back(Q_copy);
      m=mtran(m);
          // reduce matrix, solution P are diag coeff in reverse order
      m=mrref(m,context0); // ok
      vecteur res(n+1);
      for (int i=0;i<n;++i)
          res[n-i-1]=rdiv(m[i][n],m[i][i],context0);
      return res;
      
  }

  // true if v2[x]=v1[x-n], false otherwise
  static bool is_shift_of(const vecteur & v1,const vecteur &v2,int & n){
    int s1=int(v1.size()),s2=int(v2.size());
    if (s1!=s2 || v1[0]!=v2[0])
      return false;
    if (s1<2)
      return false; // setsizeerr(contextptr);
    gen e=(v1[1]-v2[1])/v1[0];
    if (e.type!=_INT_)
      return false;
    if (e.val % (s1-1))
      return false;
    n=e.val / (s1-1);
    return v1==taylor(v2,n);
  }

  bool rational_sum(const gen & e,const gen & x,gen & res,gen& remains_to_sum,bool allow_psi,GIAC_CONTEXT){
    // first detect rational fraction
    vecteur v(lvarxpow(e,x));
    if (v.empty()){
      res = e*x;
      remains_to_sum = 0;
      return true;
    }
    if ( (v.size()!=1) || (v.front()!=x) ){
      remains_to_sum=e;
      return false;
    }
    lvar(e,v);
    gen r=e2r(e,v,contextptr),r_num,r_den;
    fxnd(r,r_num,r_den);
    if (r_num.type==_EXT){
      remains_to_sum=e;
      return false;
    }
    if ((r_den.type!=_POLY) || (!r_den._POLYptr->lexsorted_degree())){ // polynomial w.r.t. x
      vecteur Q;
      if (r_num.type!=_POLY){
	res=r_num*v.front()/r2e(r,v,contextptr);
	return true;
      }
      Q=polynome2poly1(*r_num._POLYptr,1);
      vecteur P(solveP_x_plus_1_minus_P_x(Q));
      gen den;
      lcmdeno(P,den,contextptr); // lcmdeno_converted?
      r_num=poly12polynome(P,1,r_num._POLYptr->dim);
      r=rdiv(r_num,r_den*den,contextptr);
      res=r2e(r,v,contextptr);
      return true;
    }
    // rational fraction wrt x
    int s=r_den._POLYptr->dim;
    polynome den(*r_den._POLYptr),num(s);
    if (r_num.type==_POLY)
      num=*r_num._POLYptr;
    else
      num=polynome(r_num,s);
    polynome p_content(s);
    // partial fraction decomposition
    factorization vden;
    gen extra_div=1;
    factor(den,p_content,vden,false,/* withsqrt */false,/* complex */ true,1,extra_div); 
    vector< pf<gen> > pfdecomp;
    polynome ipnum(s),ipden(s);
    partfrac(num,den,vden,pfdecomp,ipnum,ipden);
    // discrete antiderivative of integral part
    vecteur Q(polynome2poly1(ipnum,1));
    vecteur P(solveP_x_plus_1_minus_P_x(Q));
    ipnum=poly12polynome(P,1,ipnum.dim);
    r=rdiv(ipnum,ipden,contextptr);
    res=r2e(r,v,contextptr);
    int vdim=int(v.size());
    // detect integral shifted denominators
    vector<pf1> shiftfree_pfdecomp;
    vector< pf<gen> >::iterator it=pfdecomp.begin();
    vector< pf<gen> >::const_iterator itend=pfdecomp.end();
    for (;it!=itend;++it){
      vecteur it_fact(polynome2poly1(it->fact,1));
      vector<pf1>::iterator jt=shiftfree_pfdecomp.begin();
      vector<pf1>::const_iterator jtend=shiftfree_pfdecomp.end();
      int k;
      for (;jt!=jtend;++jt){
	if (is_shift_of(it_fact,jt->fact,k))
	  break;
      }
      if (jt==jtend)
	shiftfree_pfdecomp.push_back(pf1(it->num,it->den,it->fact,it->mult));
      else { // it_fact is the shift of jt->fact
	vecteur it_num(polynome2poly1(it->num,1)),it_den(polynome2poly1(it->den,1));
	// check which one has the highest multiplicity
	if (jt->mult<it->mult){ // we must swap to keep highest mult in *jt
	  std::swap(jt->num,it_num);
	  std::swap(jt->den,it_den);
	  std::swap(jt->fact,it_fact);
	  std::swap(jt->mult,it->mult);
	  k=-k;
	}
	// do the shift (this will modify jt->num and the result res)
	int decal;
	if (k<0){
	  decal=1;
	  k=-k;
	}
	else
	  decal=-1;
	for (int j=0;j<k;++j){
	  if (decal>0)
	    res=res-rdiv(r2e(poly12polynome(it_num,1,vdim),v,contextptr),r2e(poly12polynome(it_den,1,vdim),v,contextptr),contextptr);
	  it_num=taylor(it_num,decal);
	  it_den=taylor(it_den,decal);
	  if (decal<0)
	    res=res+rdiv(r2e(poly12polynome(it_num,1,vdim),v,contextptr),r2e(poly12polynome(it_den,1,vdim),v,contextptr),contextptr);
	}
	modpoly constante=jt->den/it_den;
	gen const_den;
	lcmdeno(constante,const_den,contextptr); // lcmdeno_converted?
	// should check if constante is a fraction
	jt->num=constante*it_num+const_den*jt->num;
	jt->den=const_den*jt->den;
      } // end else
    } // end for (;it!=itend;++it)
    // now add psi parts for every element of the shiftfree decomposition
    vector<pf1>::iterator jt=shiftfree_pfdecomp.begin();
    vector<pf1>::const_iterator jtend=shiftfree_pfdecomp.end();
    for (;jt!=jtend;++jt){
      vecteur & jtfact = jt->fact;
      // vecteur & jtnum = jt->num;
      vecteur & jtden = jt->den;
      if (jtfact.size()!=2){ // add to remains_to_sum
	remains_to_sum=remains_to_sum+rdiv(r2e(poly12polynome(jt->num,1,vdim),v,contextptr),r2e(poly12polynome(jt->den,1,vdim),v,contextptr),contextptr);
      }
      else {
	gen racine=-rdiv(jtfact.back(),jtfact.front(),contextptr);
	vecteur dec=taylor(jt->num,racine);
	vecteur vv=cdr_VECT(v);
	gen coeff(plus_one);
	int decal=int(jt->mult-dec.size());
	for (int i=0;i<jt->mult;++i){
	  if (i>=decal){
	    if (!allow_psi)
	      return false;
	    res=res+r2e(rdiv(dec[i-decal],coeff*jtden.front(),contextptr),vv,contextptr)*Psi(x-r2e(racine,vv,contextptr),i,contextptr);
	  }
	  coeff=gen(-i-1)*coeff;
	}
      }
    } // end for(;jt!=jtend;++jt)
    return true; // end non constant denominator
  } // end rational fraction or polynomial

  polynome taylor(const polynome & P,const gen & g){
    vecteur v(polynome2poly1(P,1));
    v=taylor(v,g);
    return poly12polynome(v,1,P.dim);
  }


  vecteur decalage_(const polynome & A,const polynome & B){
    int s=A.dim;
    // find integer roots of resultant of A(x),B(x+t) with respect to x
    vecteur l(s);
    for (int i=0;i<s;++i)
      l[i]=gen(identificateur("x"+print_INT_(i)));
    int adeg=A.lexsorted_degree();
    int bdeg=B.lexsorted_degree(); // total degree of B(x+t) is bdeg, total degree of A is adeg
    // therefore total degree of resultant is <= adeg*bdeg
    vecteur y(adeg*bdeg+1),x(adeg*bdeg+1);
    vecteur bb(polynome2poly1(B,1));
    for (int i=0;i<=adeg*bdeg;++i){
      x[i]=i;
      polynome b=poly12polynome(bb,1,s);
      y[i]=r2e(Tresultant<gen>(A,b),l,context0);
      bb=taylor(bb,1);
    }
    gen resu=_lagrange(makesequence(x,y,l.front()),context0);
    resu=e2r(resu,l,context0);
    if (resu.type!=_POLY)
      return vecteur(0);
    polynome pres=*resu._POLYptr;
    // Make the list of the positive integer roots k in t of the resultant
    return iroots(pres);
  }
  // IMPROVE: eval A and B at other variables to detect possible integer roots
  // then try gcd(A(x),B(x+t))
  vecteur decalage(const polynome & A,const polynome & B){
    int s=A.dim;
    if (s==1)
      return decalage_(A,B);
    vecteur l(s),L(s);
    for (int i=0;i<s;++i)
      l[i]=gen(identificateur("x"+print_INT_(i)));
    gen a=r2e(A,l,context0);
    gen b=r2e(B,l,context0);
    gen t(identificateur("t"));
    gen r=_sylvester(makesequence(a,subst(b,l[0],l[0]+t,false,context0),l[0]),context0);
    L[0]=l[0];
    gen r0=_det(subst(r,l,L,false,context0),context0);
    if (is_zero(derive(r0,t,context0))){
      int essai=0;
      for (;essai<s;++essai){
	L=vranm(s,0,0); // find random evaluation
	L[0]=l[0];
	r0=_det(subst(r,l,L,false,context0),context0);
	if (!is_zero(derive(r0,t,context0)))
	  break;
      }
      if (essai==s)
	return decalage_(A,B);
    }
    r0=e2r(r0,vecteur(1,t),context0);
    if (r0.type!=_POLY)
      return decalage_(A,B);
    vecteur v=iroots(*r0._POLYptr);
    vecteur res;
    for (int i=0;i<v.size();++i){
      gen ti=v[i];
      gen bti=subst(b,l[0],l[0]+ti,false,context0);
      gen g=gcd(a,bti,context0);
      if (!is_zero(derive(g,l[0],context0)))
	res.push_back(ti);
    }
    return res;
  }

  // Write a fraction A/B as E[P]/P*Q/E[R] where E[P]=subst(P,x,x+1)
  // and Q and all positive shifts of R are prime together
  void AB2PQR(const polynome & A,const polynome & B,polynome & P,polynome & Q, polynome & R){
    int s=A.dim;
    // First find integer roots of resultant of A(x),B(x+t) with respect to x
#if 1
    std::vector< facteur< tensor<gen> > > vA(Tsqff_char0(A)),vB(Tsqff_char0(B));
    std::vector< facteur< tensor<gen> > >::const_iterator itA=vA.begin(),itAend=vA.end(),itB=vB.begin(),itBend=vB.end();
    vecteur racines;
    for (;itA!=itAend;++itA){
      for (;itB!=itBend;++itB){
	racines=mergeset(racines,decalage(itA->fact,itB->fact));
      }
    }
#else    
    polynome a(A.untrunc1()); // add the t parameter
    polynome b(B.untrunc1());
    // exchange var 1 (parameter t) and 2 (x variable)
    vector<int> i=transposition(0,1,s+1);
    a.reorder(i);
    b.reorder(i);
    // now translate b by t
    vecteur bb(polynome2poly1(b,1));
    polynome t(monomial<gen>(plus_one,1,1,s));
    bb=taylor(bb,t);
    b=poly12polynome(bb,1,s+1);
    polynome pres=Tresultant<gen>(a,b);
    pres=pres.trunc1();
    // Make the list of the positive integer roots k in t of the resultant
    vecteur racines(iroots(pres));
#endif
    // The algorithm begins with P0=1 Q0=A R0=B
    P=polynome(monomial<gen>(plus_one,s));
    Q=A;
    R=B;
    int d=int(racines.size());
    for (int i=0;i<d;++i){
      gen k=racines[i];
      if (k.type!=_INT_ || k.val<=0)
	continue;
      // Then compute Pi Qi Ri so that E[Pi]/Pi*Qi/Ri=A/B
      // for each positive integer root k 
      // gcd[Qi,E^k[Ri]]=Y!=1 then Qi=Y*Q_{i+1}, Ri=E^-k[Y]*R_{i+1}
      // hence Qi/Ri=Q_{i+1}/R_{i+1}* Y/E^[-k]Y 
      polynome Y=gcd(Q,taylor(R,k));
      Q=Q/Y;
      polynome Yk=taylor(Y,-k);
      R=R/Yk;
      // therefore P_{i+1}=Pi*E^[-k]Y*...*E^[-1]Y 
      for (int j=-k.val;j<0;++j){
	P=P*Yk;
	Yk=taylor(Yk,plus_one);
      }
    }
    // At the end R=E^[-1] R_i
    R=taylor(R,minus_one);
  }

  // Solve P = Q E[Y] - R Y for Y
  // return true if there is a solution Y (solution is more precisely Y/deno)
  bool gosper(const polynome & P,const polynome & Q,const polynome & R,polynome & Y,gen & deno,GIAC_CONTEXT){
    // First find degree of Y
    // if q>r then y=p-q, if q<r then y=p-r, (if y<0 return false)
    // if q==r then y=p-q or p (p only if same leading coeff in Q,R)
    int p=P.lexsorted_degree(),q=Q.lexsorted_degree(),r=R.lexsorted_degree(),y;
    vecteur vP(polynome2poly1(P,1)),vQ(polynome2poly1(Q,1)),vR(polynome2poly1(R,1));
    gen qq=Tfirstcoeff<gen>(Q),rr=Tfirstcoeff<gen>(R);
    if (q==r && qq==rr){ // cancellation
      ++p;
      y=p-giacmax(q,r);
      if (q>0){
	vecteur vq=polynome2poly1(Q,1),vr=polynome2poly1(R,1);
	gen ydeg=(vr[1]-vq[1])/qq;//gen ydeg=(vr[q-1]-vq[q-1])/qq;
	if (ydeg.type==_INT_ && ydeg.val>y){
	  y=ydeg.val;
	  p=y+q-1;
	}
      }
    }
    else
      y=p-giacmax(q,r);
    if (y<0)
      return false;
    // Then solve a linear system with p+1 equations and y+1 unknowns
    // (p+1 rows, y+1 columns)
    // built the matrix of the system column by column
    // the column i is (X+1)^i*Q-X^i*R
    vecteur v(1,plus_one); // this will contain (X+1)^i using pascal_next_line
    vecteur w(v); // this is X^i
    matrice m;
    for (int i=0;i<=y;++i){
      vecteur current=v*vQ-w*vR;
      // adjust current size to p
      lrdm(current,p);
      m.push_back(current);
      v=pascal_next_line(v);
      w.push_back(zero);
    }
    reverse(m.begin(),m.end()); // higher coeff at the beginning
    // last column is P
    lrdm(vP,p);
    m.push_back(vP);
    m=mtran(m);
    int st=step_infolevel(contextptr);
    step_infolevel(contextptr)=0;
    m=mrref(m,contextptr);
    step_infolevel(contextptr)=st;
    vecteur res(y+1);
    for (int i=0;i<=y;++i){
      if (is_zero(m[i][i]))
	return false;
      res[i]=m[i][y+1]/m[i][i];
    }
    lcmdeno(res,deno,contextptr); // lcmdeno_converted?
    Y=poly12polynome(res,1,P.dim);
    return p==y || is_zero(m[y+1]);
    // Or alternatively do a Rothstein-Trager like method if Q non constant
    // Let P = Q U + R V with deg(V)<deg(Q)
    // then Q(E(Y)-U)=R(Y+V)
    // hence Q divides Y+V
    // If we know that deg(Y)<deg(Q) then check that Y=-V is solution
    // Otherwise let Y+V=Qy
    // Then QE(Qy-V) -R(Qy-V)=P=QU+RV
    // hence E(Qy-V)-Ry=U
    // we are reduced to solve E(Q)y-Ry=U+E(V) with deg(y)=deg(Y)-deg(Q)
  }

  // Check for hypergeometric e, if true
  // write e(x+1)/e(x) as P(n+1)/P(n)*Q(x)/R(x+1) 
  bool is_hypergeometric(const gen & e,const identificateur &x,vecteur &v,polynome & P,polynome & Q,polynome & R,GIAC_CONTEXT){
    v=lvarx(e,x);
    if (!loptab(v,sincostan_tab).empty() || !loptab(v,asinacosatan_tab).empty() || !lop(v,at_Psi).empty())
      return false;
    // if v contains a non linear exp abort
    int vs=int(v.size());
    gen a,b;
    for (int i=0;i<vs;++i){
      if (v[i].is_symb_of_sommet(at_exp) && !is_linear_wrt(v[i]._SYMBptr->feuille,x,a,b,contextptr))
	return false;
    }
    gen ratio=subst(e,x,x+1,false,contextptr)/e;
    ratio=simplify(ratio,contextptr);
    if (is_undef(ratio))
      return false;
    v=lvarx(makevecteur(ratio,x),x);
    if ( (v.size()!=1) || (v.front()!=x) ){
      ratio=simplify(_texpand(ratio,contextptr),contextptr);
      v=lvarx(makevecteur(ratio,x),x);
      if ( (v.size()!=1) || (v.front()!=x) )
	return false;
    }
    lvar(ratio,v);
    for (unsigned i=1;i<v.size();++i){
      if (!is_zero(derive(v[i],x,contextptr)))
	return false;
    }
    int s=int(v.size());
    gen f=e2r(ratio,v,contextptr);
    polynome A(s),B(s);
    if (f.type==_FRAC){
      A=gen2poly(f._FRACptr->num,s);
      B=gen2poly(f._FRACptr->den,s);
    }
    else {
      A=gen2poly(f,s);
      B=gen2poly(plus_one,s);
    }
    AB2PQR(A,B,P,Q,R); // A/B as E[P]/P*Q/E[R]
    return true;
  }

  static gen inner_sum(const gen & e,const gen & x,gen & remains_to_sum,int intmode,GIAC_CONTEXT){
    gen res;
    if (rational_sum(e,x,res,remains_to_sum,true,contextptr))
      return res;
    polynome P,Q,R;
    vecteur v;
    if (!is_hypergeometric(e,*x._IDNTptr,v,P,Q,R,contextptr)){
      remains_to_sum=e;
      return zero;
    }
    int s=int(v.size());
    gen deno;
    polynome Y(s);
    if (!gosper(P,Q,R,Y,deno,contextptr)){
      remains_to_sum=e;
      return zero;
    }
    remains_to_sum=zero;
    gen facteur=r2e(Y*R,v,contextptr)/r2e(P,v,contextptr)/r2e(deno,vecteur(v.begin()+1,v.end()),contextptr);
    return simplify(e*facteur,contextptr);
  }

  // discrete antiderivative
  gen sum(const gen & e,const gen & x,gen & remains_to_sum,GIAC_CONTEXT){
    if (x.type!=_IDNT)
      return gensizeerr(contextptr);
    vecteur v=lvarx(e,x);
    v=loptab(v,sincostan_tab);
    // keep only sincostan which are linear wrt x
    vecteur newv(v);
    v.clear();
    int s=int(newv.size());
    for (int i=0;i<s;++i){
      gen a,b;
      if (is_linear_wrt(newv[i]._SYMBptr->feuille,x,a,b,contextptr))
	v.push_back(newv[i]);
    }
    if (!v.empty()){
      gen w=trig2exp(v,contextptr);
      gen e1=_lin(subst(e,v,*w._VECTptr,true,contextptr),contextptr);
      return _simplify(_evalc(linear_apply(e1,x,remains_to_sum,0,contextptr,inner_sum),contextptr),contextptr); 
    }
    else
      return linear_apply(e,x,remains_to_sum,0,contextptr,inner_sum); 
  }
  
  // discrete antiderivative evaluated
  gen sum_loop(const gen & e,const gen & x,int i,int j,GIAC_CONTEXT){
    gen f(e),res;
    if (i>j){
      int tmp=j;
      j=i-1;
      i=tmp+1;
      f=-e;
    }
    for (;i<=j;++i){
      res=res+subst(f,x,i,false,contextptr).eval(eval_level(contextptr),contextptr);
    }
    return res;
  }

  gen sum(const gen & e,const gen & x,const gen & a,const gen &b,GIAC_CONTEXT){
    if ( (a.type==_INT_) && (b.type==_INT_) && (absint(b.val-a.val)<100) )
      return sum_loop(e,x,a.val,b.val,contextptr);
    gen res;
    if ( sumab(e,x,a,b,res,true,contextptr) )
      return res;
    gen remains_to_sum;
#if defined EMCC || defined GIAC_HAS_STO_38
    res=sum(e,x,remains_to_sum,contextptr);
#else
    gen oldx=eval(x,1,contextptr),X(x);
    if (!assume_t_in_ab(X,a,b,false,false,contextptr))
      return gensizeerr(contextptr);
    res=sum(e,x,remains_to_sum,contextptr);
    sto(oldx,X,contextptr);
#endif
    gen tmp1=( (is_inf(b) && x.type==_IDNT)?limit(res,*x._IDNTptr,b,0,contextptr):subst(res,x,b+1,false,contextptr));
    gen tmp2=(is_inf(a) && x.type==_IDNT)?limit(res,*x._IDNTptr,a,0,contextptr):subst(res,x,a,false,contextptr);
    res=tmp1-tmp2;
    if (is_zero(remains_to_sum))
      return res;
    if ( (a.type==_INT_) && (b.type==_INT_) && (absint(b.val-a.val)<max_sum_add(contextptr)) )
      return res+sum_loop(remains_to_sum,x,a.val,b.val,contextptr);
    return symbolic(at_sum,gen(makevecteur(e,x,a,b),_SEQ__VECT));
  }
  
  gen prodsum(const gen & g,bool isprod){
    if (g.type!=_VECT)
      return gensizeerr(gettext("prodsum"));
    vecteur v=*g._VECTptr;
    int s=int(v.size());
    if (!s)
      return isprod?1:0;
    int debut=1,fin=s;
    if (v[0].type==_VECT && g.subtype==_SEQ__VECT && s>1 && v[1].type==_INT_){
      debut=giacmax(1,v[1].val);
      if (s>2 && v[2].type==_INT_)
	fin=v[2].val;
      v=*v[0]._VECTptr;
      s=int(v.size());
      fin=giacmin(s,fin);
    }
    gen res;
    if (isprod){
      res=plus_one;
      for (--debut;debut<fin;++debut){
	res=matrix_apply(res,v[debut],prod);
      }
    }
    else {
      if (v[0].type==_STRNG) res=string2gen("",false); // fix for string joining (L.Marohnić)
      for (--debut;debut<fin;++debut){
	res=matrix_apply(res,v[debut],somme);
      }
    }
    return res;
  }

#if 0
  static void local_sto(const gen & value,const identificateur & i,GIAC_CONTEXT){
    if (contextptr)
      (*contextptr->tabptr)[i.id_name]=value;
    else 
      i.localvalue->back()=value;
  }

  static void local_sto_increment(const gen & value,const identificateur & i,GIAC_CONTEXT){
    if (contextptr)
      (*contextptr->tabptr)[i.id_name] += value;
    else
      i.localvalue->back() += value;
  }

  static void local_sto_int(int value,const identificateur & i,GIAC_CONTEXT){
    if (contextptr)
      (*contextptr->tabptr)[i.id_name].val=value;
    else
      i.localvalue->back().val=value;
  }

  static void local_sto_int_increment(int value,const identificateur & i,GIAC_CONTEXT){
    if (contextptr)
      (*contextptr->tabptr)[i.id_name].val += value;
    else
      i.localvalue->back().val += value;
  }
#endif

  // type=0 for seq, 1 for prod, 2 for sum
  gen seqprod(const gen & g,int type,GIAC_CONTEXT){
    vecteur v(gen2vecteur(g));
    if (v.size()==1)
      v=gen2vecteur(eval(g,contextptr));
    if (v.size()<4){
      gen v2;
      if (v.size()==3)
	v2=eval(v[2],1,contextptr);
      if (type==0 && v.size()==3 && v2.type==_VECT){
	// for example seq(2^k,k,[1,2,5])
	gen f=_unapply(makesequence(v[0],v[1]),contextptr);
	return _map(makesequence(v2,f),contextptr);
      }
      if (v.size()==3 && v[1].is_symb_of_sommet(at_equal) && v[1]._SYMBptr->feuille[1].is_symb_of_sommet(at_interval)){
	gen f=v[1]._SYMBptr->feuille;
	gen v1=f[0];
	gen v2=f[1]._SYMBptr->feuille[0],v3=f[1]._SYMBptr->feuille[1];
	return change_subtype(seqprod(makevecteur(v[0],v1,v2,v3,v[2]),type,contextptr),_SEQ__VECT);
      }
      if (v.size()==3 && !v[1].is_symb_of_sommet(at_equal) && g.subtype==_SEQ__VECT)
	return change_subtype(seqprod(gen(makevecteur(symb_interval(v[0],v[1]),v[2]),_SEQ__VECT),type,contextptr),0);
      if (type==0)
	return _dollar(g,contextptr);
      if (type==1)
	return prodsum(v,true);	
      if (type==2)
	return prodsum(v,false);
      return gentoofewargs("");
    }
    // v[1]=eval(v[1]);
    v[2]=eval(v[2],contextptr);
    v[3]=eval(v[3],contextptr);
    gen step=1;
    gen tmp;
    if (v.size()==5)
      step=eval(v[4],contextptr);
    if (is_zero(step))
      return gensizeerr(contextptr);
    if (!is_integral(v[3]) || !is_integral(v[2])){
      if (v.size()==4 && g.subtype==_SEQ__VECT){
        if (type==1)
          return symbolic(at_product,g);
	return gentypeerr(contextptr);
      }
      if (type==1 && (g.subtype!=_SEQ__VECT || v.size()!=5))
	return prodsum(v,true);
      if (type==2 && (g.subtype!=_SEQ__VECT || v.size()!=5))
	return prodsum(v,false);
    }
    // This will not work if v[0] has auto-quoting functions inside
    // because arguments are not evaled, hence replacement of v[1] by value
    // is not done inside arguments.
    // Example Ya:=desolve([y'+x*y=0,y(0)=a]); seq(plot(Ya),a,1,3); 
    gen debut=v[2],fin=v[3];
    if (is_greater(abs(fin-debut),type?max_sum_add(contextptr):LIST_SIZE_LIMIT,contextptr))
      return gendimerr(contextptr);
    vecteur res;
    if (is_strictly_greater(debut,fin,contextptr)){
      if (is_positive(step,contextptr))
	step=-step;
      for (;!ctrl_c && !interrupted && is_greater(debut,fin,contextptr);debut=debut+step){
#ifdef TIMEOUT
	control_c();
#endif
	tmp=quotesubst(v[0],v[1],debut,contextptr);
	tmp=eval(tmp,contextptr);
	tmp=quotesubst(tmp,v[1],debut,contextptr);
#ifdef RTOS_THREADX
	tmp=evalf(tmp,1,contextptr);
#endif
        if (!res.empty() && res.back().type<_POLY ){
          if (type==1){
            res.back() = res.back()*tmp;
            continue;
          }
          if (type==2){
            res.back() += tmp;
            continue;
          }
        }
	res.push_back(tmp);
      } // for
    }
    else {
      if (!is_greater(fin,debut,contextptr))
	return gensizeerr((gettext("Unable to sort boundaries ")+debut.print(contextptr))+(","+fin.print(contextptr)));
      if (is_positive(-step,contextptr))
	step=-step;
      for (;!ctrl_c && !interrupted && is_greater(fin,debut,contextptr);debut=debut+step){
#ifdef TIMEOUT
	control_c();
#endif
	tmp=quotesubst(v[0],v[1],debut,contextptr);
	tmp=eval(tmp,contextptr);
	tmp=quotesubst(tmp,v[1],debut,contextptr);
#ifdef RTOS_THREADX
	tmp=evalf(tmp,1,contextptr);
#endif
        if (!res.empty() && res.back().type<_POLY){
          if (type==1){
            res.back() = res.back()*tmp;
            continue;
          }
          if (type==2){
            res.back() += tmp;
            continue;
          }
        }
	res.push_back(tmp);
      } //for
    }
    if (type==1)
      return _prod(res,contextptr);
    if (type==2)
      return _plus(res,contextptr);
    return res;// return gen(res,_SEQ__VECT);
  }

#if 0
  static identificateur independant_identificateur(const gen & g){
    string xname(" x"+g.print(context0));
    gen x(xname,contextptr);
    return x;
  }

  // type=0 for seq, 1 for prod, 2 for sum
  static gen seqprod2(const gen & g,int type,GIAC_CONTEXT){
    vecteur v(gen2vecteur(g));
    if (v.size()==1)
      v=gen2vecteur(eval(g,eval_level(contextptr),contextptr));
    if (v.size()<4){
      if (type==0)
	return _dollar(g,contextptr);
      if (type==1)
	return prodsum(v,true);	
      if (type==2)
	return prodsum(v,false);
      return gentoofewargs("");
    }
    // v[1]=eval(v[1]);
    v[2]=eval(v[2],eval_level(contextptr),contextptr);
    v[3]=eval(v[3],eval_level(contextptr),contextptr);
    gen step=1;
    gen tmp;
    if (v.size()==5)
      step=eval(v[4],eval_level(contextptr),contextptr);
    if (is_zero(step))
      return gensizeerr(contextptr);
    if (v[3].type!=_INT_ || v[2].type!=_INT_){
      if (type==1)
	return prodsum(v,true);
      if (type==2)
	return prodsum(v,false);
    }
    gen debut=v[2],fin=v[3];
    vecteur res;
    gen nstep=evalf_double((fin-debut)/step,1,contextptr);
    if (nstep.type!=_DOUBLE_)
      return gensizeerr(gettext("Bad step"));
    res.reserve(int(absdouble(nstep._DOUBLE_val))+1);
    identificateur x=independant_identificateur(v[0]);
    tmp=quotesubst(v[0],v[1],x,contextptr);
    gen tmpev=eval(tmp,eval_level(contextptr),contextptr);
    gen a,b;
    if (is_linear_wrt(tmpev,x,a,b,contextptr)){
      if (is_strictly_greater(debut,fin,contextptr)){
	if (is_positive(step,contextptr)) // correct pos step to -
	  step=-step;
	for (;is_greater(debut,fin,contextptr);debut+=step){
	  res.push_back(a*debut+b);
	}
      }
      else {
	if (is_positive(-step,contextptr)) // correct negative step to +
	  step=-step;
	if (step.type==_INT_){
	  int D=debut.val,F=fin.val,S=step.val;
	  for (;D<=F;D+=S){
	    res.push_back(D*a+b);	    
	  }
	}
	else {
	  for (;is_greater(fin,debut,contextptr);debut+=step){
	    res.push_back(a*debut+b);
	  }
	}
      }
    }
    else {
      int level=eval_level(contextptr);
      context * newcontextptr= (context *) contextptr;
      vecteur localvar(1,x);
      int protect=bind(vecteur(1,debut),localvar,newcontextptr);
      if (is_strictly_greater(debut,fin,newcontextptr)){
	if (is_positive(step,newcontextptr)) // correct pos step to -
	  step=-step;
	if (step.type==_INT_){
	  int D=debut.val,F=fin.val,S=step.val;
	  for (;D>=F;D+=S){
	    res.push_back(tmp.eval(level,newcontextptr));
	    local_sto_int_increment(S,x,newcontextptr);
	  }
	}
	else {
	  for (;is_greater(debut,fin,newcontextptr);debut+=step){
	    local_sto(debut,x,newcontextptr);
	    res.push_back(tmp.eval(level,newcontextptr));
	  }
	}
      }
      else {
	if (is_positive(-step,newcontextptr)) // correct negative step to +
	  step=-step;
	if (step.type==_INT_){
	  int D=debut.val,F=fin.val,S=step.val;
	  for (;D<=F;D+=S){
	    res.push_back(tmp.eval(level,newcontextptr));
	    local_sto_int_increment(S,x,newcontextptr);
	  }
	}
	else {
	  for (;is_greater(fin,debut,newcontextptr);debut+=step){
	    local_sto(debut,x,newcontextptr);
	    res.push_back(tmp.eval(level,newcontextptr));
	  }
	}
      }
      leave(protect,localvar,newcontextptr);
    } // end is_linear
    if (type==1)
      return _prod(res,contextptr);
    if (type==2)
      return _plus(res,contextptr);
    return res;// return gen(res,_SEQ__VECT);
  }
#endif

  bool maple_sum_product_unquote(vecteur & v,GIAC_CONTEXT){
    bool res=false;
    int s=int(v.size());
    if (s<2)
      return false; // setsizeerr(contextptr);
    if (v[0].is_symb_of_sommet(at_quote))
      v[0]=v[0]._SYMBptr->feuille;
    if (v[1].type!=_IDNT){
      if (is_equal(v[1]) && v[1]._SYMBptr->feuille.type==_VECT){
	res=true;
	vecteur tmp =*v[1]._SYMBptr->feuille._VECTptr;
	if (tmp.size()==2){
	  if (tmp[0].is_symb_of_sommet(at_quote))
	    tmp[0]=tmp[0]._SYMBptr->feuille;
	  v[1]=symbolic(at_equal,gen(makevecteur(tmp[0],eval(tmp[1],eval_level(contextptr),contextptr)),_SEQ__VECT));
	}
      }
      else
	v[1]=eval(v[1],eval_level(contextptr),contextptr);
    }
    for (int i=2;i<s;++i)
      v[i]=eval(v[i],eval_level(contextptr),contextptr);
    return res;
  }

  gen _sum(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT && args.subtype!=_SEQ__VECT)
      return prodsum(args.eval(eval_level(contextptr),contextptr),false);
    if ( (args.type!=_VECT) || (args._VECTptr->size()<2) )
      return prodsum(args.eval(eval_level(contextptr),contextptr),false);
    vecteur v(*args._VECTptr);
    if (v.size()>1 && v[1].is_symb_of_sommet(at_unquote))
      v[1]=eval(v[1],1,contextptr);
    maple_sum_product_unquote(v,contextptr);
    int s=int(v.size());
    if (is_zero(ratnormal(v[0],contextptr)))
      return 0;
    if (!adjust_int_sum_arg(v,s))
      return gensizeerr(contextptr);
    if (v[1].type==_INT_){
      v[0]=eval(v[0],eval_level(contextptr),contextptr);
      if (v[0].type==_VECT && s==2 && args.subtype==_SEQ__VECT && v[1].val>0){
        const vecteur & l=*v[0]._VECTptr; int step=v[1].val;
        gen res;
        for (int pos=0;pos<l.size();pos+=step)
          res += l[pos];
        return res;
      }
      return prodsum(v,false);
    }
    if (s==5)
      return seqprod(gen(v,_SEQ__VECT),2,contextptr);
    if (s==4) {
      if (v[1]==cst_i)
	return gensizeerr(gettext("i=sqrt(-1), please use a valid identifier name"));
      gen af=evalf_double(v[2],1,contextptr),bf=evalf_double(v[3],1,contextptr);
      if (v[1].type==_IDNT && (is_inf(af) || af.type==_DOUBLE_) && (is_inf(bf) || bf.type==_DOUBLE_)){
	vecteur w;
#if !defined FXCG && !defined NSPIRE
	my_ostream * ptr=logptr(contextptr);
	logptr(0,contextptr);
#endif
#ifdef NO_STDEXCEPT
	gen v0=eval(v[0],1,contextptr);
	if (is_undef(v0)) 
	  v0=v[0];
	if (!has_num_coeff(v0))
	  w=protect_find_singularities(v0,*v[1]._IDNTptr,0,contextptr);
#else
	  gen v0=v[0];
	try {
#ifndef EMCC
	  v0=eval(v[0],1,contextptr);
#endif
	  if (!has_num_coeff(v0))
	    w=protect_find_singularities(v0,*v[1]._IDNTptr,0,contextptr);
	} catch (std::runtime_error & e){
	  last_evaled_argptr(contextptr)=NULL;
	  v0=v[0];
	}
#endif
#if !defined FXCG && !defined NSPIRE
	logptr(ptr,contextptr);
#endif
	for (unsigned i=0;i<w.size();++i){
	  if (is_integer(w[i]) && is_greater((v[3]-w[i])*(w[i]-v[2]),0,contextptr)) {
	    gen v0w=limit(v0,*v[1]._IDNTptr,w[i],0,contextptr);// gen v0w=subst(v0,v[1],w[i],false,contextptr);
	    if (is_undef(v0w) || is_inf(v0w))
	      return gensizeerr("Pole at "+w[i].print(contextptr));
	  }
	}
      }
      // test must be done twice for example for sum(sin(k),k,1,0)
      if (is_zero(v[2]-v[3]-1))
	return zero;
      bool numeval=(!is_integer(v[2]) && v[2].type!=_FRAC) || (!is_integer(v[3]) && v[3].type!=_FRAC) || approx_mode(contextptr);
      if (is_integral(v[2])){
	while (is_exactly_zero(subst(v[0],v[1],v[2],false,contextptr))){
	  if (v[2]==v[3])
	    return 0;
	  v[2]+=1;
	}
      }
      else {
	gen tmp;
	if (has_evalf(v[2],tmp,1,contextptr))
	    v[2]=_ceil(v[2],contextptr);
      }
      if (is_integral(v[3])){
	while (is_exactly_zero(subst(v[0],v[1],v[3],false,contextptr))){
	  if (v[2]==v[3])
	    return 0;
	  v[3]-=1;
	}
      }
      else {
	gen tmp;
	if (has_evalf(v[3],tmp,1,contextptr))
	    v[3]=_floor(v[3],contextptr);
      }
      if (is_zero(v[2]-v[3]-1))
	return zero;
      if (is_positive(v[2]-v[3]-1,contextptr))
	return -_sum(gen(makevecteur(v[0],v[1],v[3]+(numeval?gen(1.0):plus_one),v[2]-1),_SEQ__VECT),contextptr);
      if (is_strictly_positive(-v[2],contextptr) && is_positive(-v[3],contextptr)){
	gen tmp=quotesubst(v[0],v[1],-v[1],contextptr);
	return _sum(gen(makevecteur(tmp,v[1],-v[3],(numeval?evalf_double(-v[2],1,contextptr):-v[2])),args.subtype),contextptr);
      }
      if (v[2].type==_INT_ && v[3].type==_INT_ && absint(v[3].val-v[2].val)<max_sum_add(contextptr)){
	gen res=seqprod(v,2,contextptr);
	return (numeval || has_num_coeff(res))?evalf(res,1,contextptr):ratnormal(res,contextptr);
      }
    } // end if s==4
    const_iterateur it=v.begin(),itend=v.end();
    gen f=*it;
    ++it;
    gen x=*it;
    if (x.type==_IDNT){ 
      // quote x for evaluation of f
      if (contextptr && contextptr->quoted_global_vars){
	contextptr->quoted_global_vars->push_back(x);
	f=eval(f,eval_level(contextptr),contextptr);
	contextptr->quoted_global_vars->pop_back();
      }
      else {
	if (it->_IDNTptr->quoted){
	  int savequote=*it->_IDNTptr->quoted;
	  *it->_IDNTptr->quoted=1;
	  f=eval(f,eval_level(contextptr),contextptr);    
	  *it->_IDNTptr->quoted=savequote;
	}
	else
	  f=eval(f,eval_level(contextptr),contextptr);    
      }
    }
    ++it;
    if (it==itend){
      gen rem,res;
      res=sum(f,x,rem,contextptr);
      if (is_zero(rem))
	return res;
      else
	return res+symbolic(at_sum,makesequence(rem,x));
    }
    gen a=*it;
    ++it;
    if (it==itend)
      return prodsum(gen(v).eval(eval_level(contextptr),contextptr),false);
    gen b=*it;
    ++it;
    if ( (x.type!=_IDNT) )
      return prodsum(gen(v).eval(eval_level(contextptr),contextptr),false);
    return sum(f,x,a,b,contextptr);
  }

  static const char _somme_s []="somme";
  static define_unary_function_eval_quoted (__somme,&_sum,_somme_s);
  define_unary_function_ptr5( at_somme ,alias_at_somme,&__somme,_QUOTE_ARGUMENTS,true);

  // innert form
  gen _Sum(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    return symbolic(at_sum,args);
  }  
  static const char _Sum_s []="Sum";
  static define_unary_function_eval_quoted (__Sum,&_Sum,_Sum_s);
  define_unary_function_ptr5( at_Sum ,alias_at_Sum,&__Sum,_QUOTE_ARGUMENTS,true);

  void fourier_assume(const gen &n,GIAC_CONTEXT){
    if (n.type==_IDNT && eval(n,1,contextptr)==n){
      *logptr(contextptr) << "Running assume(" << n << ",integer)" << '\n';
      sto(gen(makevecteur(change_subtype(2,1)),_ASSUME__VECT),n,contextptr);
    }
  }

  gen _wz_certificate(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    gen F,dF,G,n(n__IDNT_e),k(k__IDNT_e);
    if (args.type==_VECT){
      int s=args._VECTptr->size();
      const vecteur & v=*args._VECTptr;
      if (s==0 || s>4) return gensizeerr(contextptr);
      if (s==1) F=v[0];
      if (s==2) F=v[0]/v[1];
      if (s==3){ F=v[0]; n=v[1]; k=v[2]; }
      if (s==4){ F=v[0]/v[1]; n=v[2]; k=v[3]; }
    }
    else
      F=args;
    fourier_assume(n,contextptr);
    fourier_assume(k,contextptr);
    dF=simplify(subst(F,n,n+1,false,contextptr)-F,contextptr);
    G=_sum(makesequence(dF,k),contextptr);
    if (lop(G,at_sum).empty()){
      gen R=G/subst(F,k,k-1,false,contextptr);
      R=_eval(simplify(R,contextptr),contextptr);
      return _factor(R,contextptr);
    }
    return 0;
  }  
  static const char _wz_certificate_s []="wz_certificate";
  static define_unary_function_eval_quoted (__wz_certificate,&_wz_certificate,_wz_certificate_s);
  define_unary_function_ptr5( at_wz_certificate ,alias_at_wz_certificate,&__wz_certificate,0,true);

  // sum does also what maple add does
  /*
  gen _add(const gen & args,GIAC_CONTEXT) {
  if ( args.type==_STRNG && args.subtype==-1) return  args;
    int & elevel =eval_level(contextptr);
    int el=elevel;
    elevel=1;
    gen res;
    try {
      res=_sum(args,contextptr);
    }
    catch (std::runtime_error & e){
		  last_evaled_argptr(contextptr)=NULL;
      elevel=el;
      throw(e);
    }
    elevel=el;
    return res;
  } 
  */ 
  static const char _add_s []="add";
  static define_unary_function_eval_quoted (__add,&_sum //&_add
			    ,_add_s);
  define_unary_function_ptr5( at_add ,alias_at_add,&__add,_QUOTE_ARGUMENTS,true);

  gen bernoulli(const gen & x){
    if (x.type==_VECT && x._VECTptr->size()==2){
      gen a=x._VECTptr->front(),y=x._VECTptr->back();
      if (a.type!=_INT_)
	return gensizeerr(gettext("bernoulli"));
      bool all=a.val<0;
      int n=absint(a.val);
      if (n==0)
	return plus_one;
      if (n==1)
	return y+minus_one_half;
      gen bi=bernoulli(-n);
      if (bi.type!=_VECT)
	return gensizeerr(gettext("bernoulli"));
      vecteur biv=*bi._VECTptr;
      if (biv.size()<=n)
	biv.push_back(0);
      // bernoulli polynomials B_n=n*int(B_n-1)+bi[n]
      vecteur allv;
      vecteur cur(1,1);
      if (all)
	allv.push_back((y.type==_VECT?cur:plus_one));
      for (int i=1;i<=n;++i){
	cur=multvecteur(i,integrate(cur,1));
	cur.insert(cur.begin(),biv[i]);
	if (all){
	  vecteur tmp(cur);
	  reverse(tmp.begin(),tmp.end());
	  if (y.type==_VECT)
	    allv.push_back(tmp);
	  else
	    allv.push_back(symb_horner(tmp,y));
	}
      }
      reverse(cur.begin(),cur.end());
      return all?allv:(y.type==_VECT?cur:symb_horner(cur,y));
    }
    if (x.type!=_INT_)
      return gensizeerr(gettext("bernoulli"));
    bool all=x.val<0;
    int n=absint(x.val);
    if (!n)
      return plus_one;
    if (n==1)
      return all?vecteur(1,minus_one_half):minus_one_half;
    if (n%2){
      if (!all)
	return zero;
      --n;
    }
    if (!all){
      if (n==2)
	return inv(6,context0);
#ifndef WIN32 // otherwise wrong for n>=28??
      if (0)
#endif
	return bernoulli_rat(n);
#if defined HAVE_LIBBERNMM && !defined BF2GMP_H
      if (n>=
#ifdef HAVE_LIBPARI
	  1e5
#else
	  0
#endif
	  ){
	mpq_t resq;
	mpq_init(resq);
	bernmm::bern_rat(resq,x.val,threads);
	mpz_t num,den;
	mpz_init(num); mpz_init(den);
	mpq_get_num(num,resq);
	mpq_get_den(den,resq);
	mpq_clear(resq);
	gen numer(num),denom(den);
	mpz_clear(num); mpz_clear(den);
	return numer/denom;
      }
#endif
#ifdef HAVE_LIBPARI
      return _pari(makesequence(string2gen("bernfrac",false),n),context0);
#endif
      return bernoulli_rat(n); 
    }
    gen a(plus_one);
    gen b(rdiv(1-n,plus_two,context0));
    vecteur bi(makevecteur(plus_one,minus_one_half));
    int i=2;
    for (; i< n-1; i+=2){
      // compute bernoulli(i)
      gen A=1;
      gen B=gen(1-i)/2;
      for (int j=2; j<i-1;j+=2){
	A=iquo( A*gen(i+3-j)*gen(i+2-j),(j-1)*j);
	B=B+A* bi[j];
      }
      bi.push_back(-B/gen(i+1));
      bi.push_back(0);
      a=iquo( (a*gen(n+3-i)*gen(n+2-i)),((i-1)*i));
      b=b+a* bi[i];
    }
    if (all){
      bi.push_back(rdiv(-b,n+1,context0));
      return bi;
    }
    return rdiv(-b,n+1,context0);
  }
  gen _bernoulli(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type==_VECT && args._VECTptr->size()==2 && args._VECTptr->back().type!=_INT_)
      return bernoulli(args);
    return apply(args,bernoulli);
  }
  static const char _bernoulli_s []="bernoulli";
  static define_unary_function_eval (__bernoulli,&_bernoulli,_bernoulli_s);
  define_unary_function_ptr5( at_bernoulli ,alias_at_bernoulli,&__bernoulli,0,true);

  vecteur double2vecteur(const double * y,int dim){
    vecteur ye;
    ye.reserve(dim);
    for (int i=0;i<dim;++i)
      ye.push_back(y[i]);
    return ye;
  }

#ifdef HAVE_LIBGSL
  struct odesolve_param {
    gen odesolve_t;
    vecteur odesolve_f,odesolve_ft,odesolve_y;
    matrice odesolve_fy;
    gsl_odeiv_system odesolve_system;
    const context * contextptr;
  };

  double m_undef = numeric_limits<double>::infinity();

  static int gsl_odesolve_function(double t, const double y[], double dydt[], void * params){
    odesolve_param * par =(odesolve_param *) params;
#ifndef NO_STDEXCEPT
    try{
#endif
      gen res=subst(par->odesolve_f,par->odesolve_t,t,false,par->contextptr);
      vecteur vtmp=double2vecteur(y,par->odesolve_system.dimension);
      res=subst(res,par->odesolve_y,vtmp,false,par->contextptr);
      res=res.evalf(1,par->contextptr);
      // store result
      if ( (res.type!=_VECT) || (res._VECTptr->size()!=par->odesolve_system.dimension)){
#ifdef NO_STDEXCEPT
	return 1; 
#else
	setsizeerr(par->contextptr);
#endif
      }
      const_iterateur it=res._VECTptr->begin(),itend=res._VECTptr->end();
      for (double * dydt_it=dydt;it!=itend;++it,++dydt_it){
	if (it->type==_DOUBLE_)
	  *dydt_it=it->_DOUBLE_val;
	else {
#ifdef NO_STDEXCEPT
	  return 1; 
#else
	  setsizeerr(par->contextptr);
#endif
	}
      }
#ifndef NO_STDEXCEPT
    }
    catch (std::runtime_error & err){
      CERR << err.what() << '\n';
      int n=par->odesolve_system.dimension,i=0;
      for (double * dydt_it=dydt;i<n;++i,++dydt_it){
	*dydt_it=m_undef;
      }
    }
#endif
    return 0;
  }


  static int gsl_odesolve_jacobian (double t, const double y[], double * dfdy, double dfdt[], void * params){
    // compute
    odesolve_param * par =(odesolve_param *) params;
#ifndef NO_STDEXCEPT
    try {
#endif
      vecteur yv(double2vecteur(y,par->odesolve_system.dimension));
      gen res=subst(par->odesolve_ft,par->odesolve_t,t,false,par->contextptr);
      res=subst(res,par->odesolve_y,yv,false,par->contextptr).evalf(1,par->contextptr);
      // store result
      if ( (res.type!=_VECT) || (res._VECTptr->size()!=par->odesolve_system.dimension)){
#ifdef NO_STDEXCEPT
	return 1; 
#else
	setsizeerr(par->contextptr);
#endif
      }
      const_iterateur it=res._VECTptr->begin(),itend=res._VECTptr->end();
      for (double * dfdt_it=dfdt;it!=itend;++it,++dfdt_it){
	if (it->type==_DOUBLE_)
	  *dfdt_it=it->_DOUBLE_val;
	else {
#ifdef NO_STDEXCEPT
	  return 1; 
#else
	  setsizeerr(par->contextptr);
#endif
	}
      }
      res=subst(par->odesolve_fy,par->odesolve_t,t,false,par->contextptr);
      res=subst(res,par->odesolve_y,yv,false,par->contextptr).evalf(1,par->contextptr);
      // store result
      if ( (res.type!=_VECT) || (res._VECTptr->size()!=par->odesolve_system.dimension)){
#ifdef NO_STDEXCEPT
	return 1; 
#else
	setsizeerr(par->contextptr);
#endif
      }
      it=res._VECTptr->begin();
      itend=res._VECTptr->end();
      for (double * dfdy_it=dfdy;it!=itend;++it){
	if (it->type!=_VECT){
#ifdef NO_STDEXCEPT
	  return 1; 
#else
	  setsizeerr(par->contextptr);
#endif
      }
	const_iterateur jt=it->_VECTptr->begin(),jtend=it->_VECTptr->end();
	for (;jt!=jtend;++jt,++dfdy_it){
	  if (jt->type==_DOUBLE_)
	    *dfdy_it=it->_DOUBLE_val;
	  else {
#ifdef NO_STDEXCEPT
	    return 1; 
#else
	    setsizeerr(par->contextptr);
#endif
	  }
	}
      }    
#ifndef NO_STDEXCEPT
    }
    catch (std::runtime_error & err){
      CERR << err.what() << '\n';
      int n=par->odesolve_system.dimension,i=0;
      for (double * dydt_it=dfdt;i<n;++i,++dydt_it){
	*dydt_it=m_undef;
      }
      i=0;
      for (double * dydt_it=dfdy;i<n;++i,++dydt_it){
	*dydt_it=m_undef;
      }
    }
#endif
    return 0;
  }
#endif // HAVE_LIBGSL

  double rk_error(const vecteur & v,const vecteur & w_final,const vecteur & w_init,GIAC_CONTEXT){
    double err=0,derr;
    unsigned dim=unsigned(v.size());
    for (unsigned i=0;i<dim;++i){
      gen wf=w_final[i],wi=w_init[i];
      double wfa=abs(wf,contextptr)._DOUBLE_val; 
      double wia=abs(wi,contextptr)._DOUBLE_val;
      double sci=1+((wfa<wia)?wia:wfa);
      derr = abs(wf-v[i],contextptr)._DOUBLE_val/sci;
      derr *= derr;
      err += derr;
    }
    err = std::sqrt(err/dim);
    return err;
  }

  // solve dy/dt=f(t,y) with initial value y(t0)=y0 to final value t1
  // returns by default y[t1] or a vector of [t,y[t]]
  // if return_curve is true stop as soon as y is outside ymin,ymax
  // f is eitheir a prog (t,y) -> f(t,y) or a _VECT [f(t,y) t y]
  gen odesolve(const gen & t0orig,const gen & t1orig,const gen & f,const gen & y0orig,double tstep,bool return_curve,double * ymin,double * ymax,int maxstep,GIAC_CONTEXT){
    bool iscomplex=false; 
    // switch to false if GSL is installed or true to force using giac code for real ode
    gen t0_e=evalf_double(t0orig.evalf(1,contextptr),1,contextptr);
    gen t1_e=evalf_double(t1orig.evalf(1,contextptr),1,contextptr);
    // Now accept t0 and t1 complex!
    if ( (t0_e.type!=_DOUBLE_ && t0_e.type!=_CPLX)|| (t1_e.type!=_DOUBLE_ && t1_e.type!=_CPLX))
      return gensizeerr(contextptr);
    gen y0=evalf_double(y0orig.evalf(1,contextptr),1,contextptr);
    if (y0.type!=_VECT)
      y0=vecteur(1,y0);
    vecteur y0v=*y0._VECTptr;
    int dim=int(y0v.size());
    if (tstep==0){
      if (dim==2)
	tstep=(gnuplot_xmax-gnuplot_xmin)/100;
      else {
	if (return_curve && abs(t1_e,contextptr)._DOUBLE_val>1e300)
	  tstep=abs(t0_e,contextptr)._DOUBLE_val/100;
	else
	  tstep=abs(t1_e-t0_e,contextptr)._DOUBLE_val/100;
      }
    }
    if (tstep>abs(t1_e-t0_e,contextptr)._DOUBLE_val)
      tstep=abs(t1_e-t0_e,contextptr)._DOUBLE_val;
#if 1
    ALLOCA(double, y, dim*sizeof(double));// double * y =(double *)alloca(dim*sizeof(double));
#else
    double * y=new double[dim];
#endif
    for (int i=0;i<dim;i++){
      if (y0v[i].type!=_DOUBLE_ && y0v[i].type!=_CPLX)
	return gensizeerr(contextptr);
      if (y0v[i].type==_DOUBLE_)
	y[i]=y0v[i]._DOUBLE_val;
      else
	iscomplex=true;
    }
    gen t_id(identificateur("odesolve_t"));
    vecteur yv;
    gen tmp;
    if (f.type==_VECT){
      vecteur tmpv(*f._VECTptr);
      if (tmpv.size()!=3)
	return gensizeerr(contextptr);
      tmp=tmpv[0];
      if (tmpv[1].type!=_IDNT)
	return gensizeerr(contextptr);
      t_id=*tmpv[1]._IDNTptr;
      if (tmpv[2].type!=_VECT){
	yv=vecteur(1,tmpv[2]);
      }
      else
	yv=*tmpv[2]._VECTptr;
      if (signed(yv.size())!=dim)
	return gendimerr(contextptr);
    }
    else {
      for (int i=0;i<dim;++i)
	yv.push_back(gen(identificateur("y"+print_INT_(i))));
      tmp=f(gen(makevecteur(t_id,yv),_SEQ__VECT),contextptr);
    }
    vecteur resv; // contains the curve
    if (return_curve)
      resv.push_back(makevecteur(t0_e,y0v));
#if 0 //def HAVE_LIBGSL
    if (!iscomplex && t0_e.type==_DOUBLE_ && t1_e.type==_DOUBLE_ && is_zero(im(tmp,contextptr))){
      double t0=t0_e._DOUBLE_val;
      double t1=t1_e._DOUBLE_val;
      bool time_reverse=(t1<t0);
      if (time_reverse){
	t0=-t0;
	t1=-t1;
      }
      double t=t0;
      if (time_reverse)
	tmp=-subst(tmp,t_id,-t_id,false,contextptr);
      vecteur odesolve_f;
      if (tmp.type!=_VECT) 
	odesolve_f=vecteur(1,tmp);
      else
	odesolve_f=*tmp._VECTptr;
      if (signed(odesolve_f.size())!=dim)
	return gendimerr(contextptr);
      // N.B.: GSL implementation uses Dormand-Prince method of orders 8/9
      // which is explicit and does not require Jacobian...
      gen diff1=derive(odesolve_f,yv,contextptr),diff2=derive(odesolve_f,t_id,contextptr);
      if (is_undef(diff1) || diff1.type!=_VECT || is_undef(diff2) || diff2.type!=_VECT)
	return diff1+diff2;
      odesolve_param * par=new odesolve_param;
      par->odesolve_t=t_id;
      par->odesolve_y=yv;
      par->contextptr=contextptr;
      par->odesolve_f=odesolve_f;
      par->odesolve_fy=*diff1._VECTptr;
      par->odesolve_ft=*diff2._VECTptr;
      par->odesolve_system.function=gsl_odesolve_function;
      par->odesolve_system.dimension=dim;
      par->odesolve_system.jacobian=gsl_odesolve_jacobian;
      par->odesolve_system.params=par;
      // GSL call
      const gsl_odeiv_step_type * T = gsl_odeiv_step_rk8pd;    
      gsl_odeiv_step * s   = gsl_odeiv_step_alloc (T, dim);
      gsl_odeiv_control * c = gsl_odeiv_control_y_new (1e-7, 1e-7);
      gsl_odeiv_evolve * e = gsl_odeiv_evolve_alloc (dim);
      double h;
      if (return_curve){
	h=fabs(t);
	if (h<1e-4)
	  h=1e-4;
      }
      else
	h=(t1-t)/1e4;
      double oldt=t0;
      bool do_while=true;
      for (int nstep=0;nstep<maxstep && do_while && t<t1;++nstep) {
	if (h>tstep)
	  h=tstep;
	int status = gsl_odeiv_evolve_apply (e, c, s,
					     &par->odesolve_system,
					     &t, t1, &h,
					     y);
	if (status != GSL_SUCCESS)
	  return gensizeerr(gettext("RK8 evolve not successful"));
	if (debug_infolevel>5)
	  CERR << nstep << ":" << t << ",y5=" << double2vecteur(y,dim) << '\n';
	if (return_curve)  {
	  if ( (t-oldt)> tstep/2 || t==t1){
	    oldt=t;
	    if (time_reverse)
	      resv.push_back(makevecteur(-t,double2vecteur(y,dim)));
	    else
	      resv.push_back(makevecteur(t,double2vecteur(y,dim)));
	  }
	  for (int i=0;i<dim;++i){
	    // CERR << y[i] << '\n';
	    if ( ymin && ymax && ( y[i]<ymin[i] || y[i]>ymax[i]) )
	      do_while=false;
	  }
	}
      }
      gsl_odeiv_evolve_free(e);
      gsl_odeiv_control_free(c);
      gsl_odeiv_step_free(s);
      delete par;
      if (return_curve){
        gen res(vecteur(0));
        res._VECTptr->swap(resv);
        return res;
      }
      else {
	if (t!=t1)
	  return makevecteur(t,double2vecteur(y,dim));
	return double2vecteur(y,dim);
      }
    }
#endif // HAVE_LIBGSL
    vecteur odesolve_f;
    if (tmp.type!=_VECT) 
      odesolve_f=vecteur(1,tmp);
    else
      odesolve_f=*tmp._VECTptr;
    if (signed(odesolve_f.size())!=dim)
      return gendimerr(contextptr);
    // solve vector ode y'=f(t,y) with respect to time variable t_id in t0..t1
    // f is stored in odesolve_f, symbolic y in yv, initial value in a double array y
    /* Butcher tableau for Dormand/Prince 4/5
       0      |
       1/5    | 1/5
       3/10   | 3/40 	      9/40
       4/5    | 44/45 	      −56/15 	  32/9
       8/9    | 19372/6561  −25360/2187  64448/6561 	−212/729
       1      | 9017/3168    −355/33 	 46732/5247 	49/176 	       −5103/18656
       1      | 35/384 	        0 	  500/1113 	125/192 	−2187/6784 	11/84 
       ===============================================================================	
       RK5    |  35/384 	0 	500/1113 	125/192 	−2187/6784 	11/84 	0
       RK4      5179/57600 	0 	7571/16695 	393/640 	−92097/339200 	187/2100 	1/40
       RK4 is used for computation of the tstep variable 
       RK4 error being estimated by |RK5-RK4|
       Step is determined by the following algorithm 
       (cf. Ernst Hairer http://www.unige.ch/~hairer/poly/chap3.pdf, p.67 in French)
       initialization: use h=tstep
       compute RK5_final and RK4_final, then 
       err=|| RK5-RK4 || = sqrt(1/dim*sum(((RK5[i]-RK4[i])/(1+max(RK5[i]_init,RK5[i]_final)))^2,i=1..dim))
       and hoptimal = 0.9*h*(tolerance/||RK5-RK4||)^(1/5)
       if (err<=hoptimal) then time += h; y_init=RK5_final; h=min(hoptimal,t_final-t_current)
       else h=hoptimal
     */
#ifdef KHICAS
    gen tolerance=epsilon(contextptr)>1e-9?epsilon(contextptr):1e-9;
#else
    gen tolerance=epsilon(contextptr)>1e-12?epsilon(contextptr):1e-12;
#endif
    vecteur yt(dim+1),ytvar(yv);
    for (int i=0;i<dim;++i)
      yt[i]=y0v[i];
    gen t_e(t0_e);
    yt[dim]=t_e;
    vecteur yt1(dim+1);
    ytvar.push_back(t_id);
    bool do_while=true;
    double butcher_c[]={0,0.2,0.3,4./5,8./9,1.,1.};
    double butcher_a[]={1./5,
			3./40,9./40,
			44./45,-56./15,32./9,
			19372./6561,-25360./2187,64448./6561,-212./729,
			9017./3168,-355./33,46732./5247,49./176,-5103./18656,
			35./384,0,500./1113,125./192,-2187./6784,11./84};
    // double butcher_b5[]={35./384,0,500./1113,125./192,-2187./6784,11./84,0};
    double butcher_b4[]={5179./57600,0,7571./16695,393./640,-92097./339200,187./2100,1./40};
    vecteur y_final5(yt.begin(),yt.begin()+dim),y_final4(dim);
    vecteur butcher_k(7);
    for (int i=0;i<7;++i)
      butcher_k[i]=vecteur(dim);
    vecteur firsteval=subst(odesolve_f,ytvar,yt,false,contextptr),lasteval;
    gen direction=t1_e-t0_e;
    double temps_total=abs(direction,contextptr)._DOUBLE_val,temps=0;
    direction=direction/temps_total;
    for (int nstep=0;do_while && nstep<maxstep && temps<temps_total;++nstep) {
      gen dt=tstep*direction;
      // compute next step
      vecteur & bk0=*butcher_k[0]._VECTptr;
      bk0=firsteval;
      if (is_undef(bk0))
	return bk0;
      multvecteur(dt,bk0,bk0);
      int butcher_a_shift=0;
      for (int j=1;j<=6;j++){
	// compute butcher_k[j]
	for (int i=0;i<dim;++i){
	  yt1[i]=yt[i];
	}
	if (dim==1){
	  gen & yt10=yt1[0];
	  for (int k=0;k<j;k++){
	    type_operator_plus_times(butcher_a[butcher_a_shift+k],butcher_k[k]._VECTptr->front(),yt10); 
	  }
	}
	else {
	  for (int k=0;k<j;k++){
	    gen bak=butcher_a[butcher_a_shift+k];
	    const vecteur & bkk=(*butcher_k[k]._VECTptr);
	    for (int i=0;i<dim;++i){
	      type_operator_plus_times(bak,bkk[i],yt1[i]); //yt1[i] += bak*bkk[i];
	    }
	  }
	}
	butcher_a_shift += j;
	yt1[dim]=yt[dim];
	type_operator_plus_times(butcher_c[j],dt,yt1[dim]);
	vecteur & bkj = *butcher_k[j]._VECTptr;
	if (j<6)
	  bkj=subst(odesolve_f,ytvar,yt1,false,contextptr);
	else
	  bkj=lasteval=subst(odesolve_f,ytvar,yt1,false,contextptr);
	if (is_undef(bkj))
	  return bkj;
	multvecteur(dt,bkj,bkj);
      }
      for (int i=0;i<dim;++i){
	y_final5[i]=yt1[i];
	y_final4[i]=yt[i];
      }
      for (int j=0;j<7;++j){
	vecteur & bkj=*butcher_k[j]._VECTptr;
	// gen bb5j=butcher_b5[j];
	gen bb4j=butcher_b4[j];
	for (int i=0;i<dim;i++){
	  // y_final5[i] += bb5j*bkj[i];
	  type_operator_plus_times(bb4j,bkj[i],y_final4[i]);//y_final4[i] += bb4j*bkj[i];
	}
      }
      // accept or reject current step and compute dt
      double err=rk_error(y_final4,y_final5,yt,contextptr);
      gen hopt=err==0?tstep:.9*tstep*pow(tolerance/err,.2,contextptr);
      if (is_undef(hopt))
	break;
      if (debug_infolevel>5)
	CERR << nstep << ":" << t_e << ",y5=" << y_final5 << ",y4=" << y_final4 << " " << tstep << " tstep (optimal)=" << hopt << " err=" << err << '\n';
      if (is_strictly_greater(err,tolerance,contextptr)){
	// reject step
	tstep=hopt._DOUBLE_val;
      }
      else { // accept
	swap(firsteval,lasteval);
	for (int i=0;i<dim;++i)
	  yt[i]=y_final5[i];
	t_e += dt;
	yt[dim]=t_e;
	temps += tstep;
	tstep=abs(t1_e-t_e,contextptr)._DOUBLE_val;
	if (hopt._DOUBLE_val<tstep)
	  tstep=hopt._DOUBLE_val;
	if (return_curve)
	  resv.push_back(makevecteur(t_e,y_final5));
	if (!iscomplex){
	  // check boundaries for y_final5
	  for (int i=0;i<dim;++i){
	    // CERR << y[i] << '\n';
	    if ( ymin && ymax && ( y_final5[i]._DOUBLE_val< ymin[i] || y_final5[i]._DOUBLE_val>ymax[i]) )
	      do_while=false;
	  }
	}
      }
    } // end integration loop
    if (return_curve){
      gen res(vecteur(0));
      res._VECTptr->swap(resv);
      return res;
    }
    else {
      if (t_e!=t1_e)
	return makevecteur(t_e,y_final5);
      return y_final5;
    }    
  }
  // note that params is not used

  // standard format is expression,t=t0..t1,vars,init_values
  // also accepted 
  // t0..t1,function,init_values
  // expression,t,vars,init_values,t=tmin..tmax
  // expression,[t,vars],[t0,init_values],t1
  static gen odesolve(const vecteur & w,GIAC_CONTEXT){
    vecteur v(w);
    int vs=int(v.size());
    if (vs<3)
      return gendimerr(contextptr);
    // convert expression,[t,vars],[t0,init_values],t1
    gen t0t=v[0],t0,t1,f,t,y0;
    if (v[1].type==_VECT && v[2].type==_VECT && v[2]._VECTptr->size()==v[1]._VECTptr->size() && vs>3){
      if (v[1]._VECTptr->size()<2)
	return gendimerr(contextptr);
      t0=v[2]._VECTptr->front();
      t1=v[3];
      gen newv1=symbolic(at_equal,v[1]._VECTptr->front(),symb_interval(v[2]._VECTptr->front(),v[3]));
      gen newv2=vecteur(v[1]._VECTptr->begin()+1,v[1]._VECTptr->end());
      gen newv3=vecteur(v[2]._VECTptr->begin()+1,v[2]._VECTptr->end());
      v[1]=newv1;
      v[2]=newv2;
      v[3]=newv3;
    }
    int maxstep=1000,vstart=0;
    double tstep=0;
    if ( t0t.is_symb_of_sommet(at_interval)){ // functional form
      t0=t0t._SYMBptr->feuille._VECTptr->front(); 
      t1=t0t._SYMBptr->feuille._VECTptr->back(); 
      f=v[1];
      y0=v[2];
      vstart=3;
    }
    else { // expression,t=tmin..tmax,y,y0
      if (vs<4)
	return gentypeerr(contextptr);
      y0=v[3];
      gen t=readvar(v[1]);
      f=makevecteur(v[0],t,v[2]);
      bool tminmax_defined,tstep_defined;
      double tmin(-1e300),tmax(1e300);
      vstart=1;
      read_tmintmaxtstep(v,t,vstart,tmin,tmax,tstep,tminmax_defined,tstep_defined,contextptr);
      if (t0!=t1){
	if (tstep==0)
	  tstep=evalf_double(abs(t1-t0,contextptr),1,contextptr)._DOUBLE_val/30;
      }
      else {
	if (tmin>0 || tmax<0 || tmin>tmax || tstep<=0)
	  *logptr(contextptr) << gettext("Warning time reversal") << '\n';
	t0=tmin;
	t1=tmax;
      }
      // if (tminmax_defined && tstep_defined) maxstep=2*int((tmax-tmin)/tstep)+1;
      // commented since the real step is used is smaller than tstep most of the time!
      vstart=3;
    }
    double ym[2]={gnuplot_xmin,gnuplot_ymin},yM[2]={gnuplot_xmin,gnuplot_ymin};
    double *ymin=0,*ymax=0;
    vs=int(v.size());
    bool curve=false;
    for (int i=vstart;i<vs;++i){
      if (readvar(v[i])==x__IDNT_e){
	if (readrange(v[i],gnuplot_xmin,gnuplot_xmax,v[i],ym[0],ym[1],contextptr)){
	  ymin=ym;
	  ymax=yM;
	  v.erase(v.begin()+i);
	  --vs;
	}
      }
      if (readvar(v[i])==y__IDNT_e){
	if (readrange(v[i],gnuplot_xmin,gnuplot_xmax,v[i],yM[0],yM[1],contextptr)){
	  ymin=ym;
	  ymax=yM;
	  v.erase(v.begin()+i);
	  --vs;
	}
      }
      if (v[i]==at_curve)
	curve=true;
    }
    return odesolve(t0,t1,f,y0,tstep,curve,ymin,ymax,maxstep,contextptr);
  }
  // odesolve(t0..t1,f,y0) or odesolve(f(t,y),t,y,t0,y0,t1)
  gen _odesolve(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()<3 ) )
      return symbolic(at_odesolve,args);
    vecteur v(*args._VECTptr);
    return odesolve(v,contextptr);
  }
  static const char _odesolve_s []="odesolve";
  static define_unary_function_eval (__odesolve,&_odesolve,_odesolve_s);
  define_unary_function_ptr5( at_odesolve ,alias_at_odesolve,&__odesolve,0,true);

  gen preval(const gen & f,const gen & x,const gen & a,const gen & b,GIAC_CONTEXT){
    if (x.type!=_IDNT)
      return gentypeerr(contextptr);
    gen res;
    if (is_greater(b,a,contextptr))
      res=limit(f,*x._IDNTptr,b,-1,contextptr)-limit(f,*x._IDNTptr,a,1,contextptr);
    else {
      if (is_greater(a,b,contextptr))
	res=limit(f,*x._IDNTptr,b,1,contextptr)-limit(f,*x._IDNTptr,a,-1,contextptr) ;
      else
	res=limit(f,*x._IDNTptr,b,0,contextptr)-limit(f,*x._IDNTptr,a,0,contextptr);
    }
    return res;
  }

  // args=[u'*v,u] or [[F,u'*v],u] -> [F+u*v,-u*v']
  // a third argument would be the integration var
  // if u=cste returns F+integrate(u'*v,x)
  gen _ibpdv(const gen & args,GIAC_CONTEXT) {
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if ( (args.type!=_VECT) || (args._VECTptr->size()<2) )
      return symbolic(at_ibpdv,args);
    vecteur & w=*args._VECTptr;
    gen X(vx_var),x(vx_var),a,b;
    bool bound=false;
    if (w.size()>=3)
      x=X=w[2];
    if (is_equal(x))
      x=x._SYMBptr->feuille[0];
    if (w.size()>=5)
      X=symb_equal(x,symb_interval(w[3],w[4]));
    if (is_equal(X) && X._SYMBptr->feuille[1].is_symb_of_sommet(at_interval)){
      a=X._SYMBptr->feuille[1]._SYMBptr->feuille[0];
      b=X._SYMBptr->feuille[1]._SYMBptr->feuille[1];
      bound=true;
    }
    gen u(w[1]),v,uprimev,F;
    if (w.front().type==_VECT){
      vecteur & ww=*w.front()._VECTptr;
      if (ww.size()!=2)
	return gensizeerr(contextptr);
      F=ww.front();
      uprimev=ww.back();
    }
    else 
      uprimev=w.front();
    gen uprime(derive(u,x,contextptr));
    if (is_zero(uprime)){
      gen tmp=integrate_gen(uprimev,x,contextptr);
      if (bound)
	tmp=preval(tmp,x,a,b,contextptr);      
      return tmp+F;
    }
    v=normal(rdiv(uprimev,derive(u,x,contextptr),contextptr),contextptr);
    if (bound)
      F += preval(u*v,x,a,b,contextptr);    
    else
      F += u*v;
    return makevecteur(F,normal(-u*derive(v,x,contextptr),contextptr));
  }
  static const char _ibpdv_s []="ibpdv";
  static define_unary_function_eval (__ibpdv,&_ibpdv,_ibpdv_s);
  define_unary_function_ptr5( at_ibpdv ,alias_at_ibpdv,&__ibpdv,0,true);

  gen fourier_an(const gen & f,const gen & x,const gen & T,const gen & n,const gen & a,GIAC_CONTEXT){
    gen primi,iT=inv(T,contextptr);
    gen omega=ratnormal(2*cst_pi*iT,contextptr);
    fourier_assume(n,contextptr);
    primi=_integrate(gen(makevecteur(f*cos(omega*n*x,contextptr),x,a,ratnormal(a+T,contextptr)),_SEQ__VECT),contextptr);
    gen an=iT*primi;
    if (n!=0) 
      an=2*an;
    return has_num_coeff(an)?an:recursive_normal(an,contextptr);
  }
  bool get_fourier(vecteur & v){
    if (v.size()<2) return false;
    if (v.size()==2) 
      v=makevecteur(v[0],vx_var,cst_two_pi,v[1],-cst_pi);
    if (v.size()==3) 
      v=makevecteur(v[0],v[1],cst_two_pi,v[2],-cst_pi);
    if (v.size()==4) v.push_back(0);
    if (equalposcomp(lidnt(v[3]),v[1]))
      return false;
    return v.size()==5;
  }
  gen _fourier_an(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type!=_VECT) return gensizeerr(contextptr);
    vecteur v(*args._VECTptr);
    if (!get_fourier(v)) return gensizeerr(contextptr);
    return fourier_an(v[0],v[1],v[2],v[3],v[4],contextptr);
    //gen f=v[0],x=v[1],T=v[2],n=v[3],a=v[4];
    //return fourier_an(f,x,T,n,a,contextptr);
  }
  static const char _fourier_an_s []="fourier_an";
  static define_unary_function_eval (__fourier_an,&_fourier_an,_fourier_an_s);
  define_unary_function_ptr5( at_fourier_an ,alias_at_fourier_an,&__fourier_an,0,true);


  gen fourier_bn(const gen & f,const gen & x,const gen & T,const gen & n,const gen & a,GIAC_CONTEXT){
    fourier_assume(n,contextptr);
    gen primi,iT=inv(T,contextptr);
    gen omega=ratnormal(2*cst_pi*iT,contextptr);
    primi=_integrate(gen(makevecteur(f*sin(omega*n*x,contextptr),x,a,ratnormal(a+T,contextptr)),_SEQ__VECT),contextptr);
    gen an=2*iT*primi;
    return has_num_coeff(an)?an:recursive_normal(an,contextptr);
  }
  gen _fourier_bn(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type!=_VECT) return gensizeerr(contextptr);
    vecteur v(*args._VECTptr);
    if (!get_fourier(v)) return gensizeerr(contextptr);
    return fourier_bn(v[0],v[1],v[2],v[3],v[4],contextptr);
    // gen f=v[0],x=v[1],T=v[2],n=v[3],a=v[4];
    // return fourier_bn(f,x,T,n,a,contextptr);
  } 
  static const char _fourier_bn_s []="fourier_bn";
  static define_unary_function_eval (__fourier_bn,&_fourier_bn,_fourier_bn_s);
  define_unary_function_ptr5( at_fourier_bn ,alias_at_fourier_bn,&__fourier_bn,0,true);
  
  gen fourier_cn(const gen & f,const gen & x,const gen & T,const gen & n,const gen & a,GIAC_CONTEXT){
    fourier_assume(n,contextptr);
    gen primi,iT=inv(T,contextptr);
    gen omega=ratnormal(2*cst_pi*iT,contextptr);
    primi=_integrate(gen(makevecteur(f*exp(-cst_i*omega*n*x,contextptr),x,a,ratnormal(a+T,contextptr)),_SEQ__VECT),contextptr);
    gen cn=iT*primi;
    return has_num_coeff(cn)?cn:recursive_normal(cn,contextptr);
  }
  gen _fourier_cn(const gen & args,GIAC_CONTEXT){
    if ( args.type==_STRNG && args.subtype==-1) return  args;
    if (args.type!=_VECT) return gensizeerr(contextptr);
    vecteur v(*args._VECTptr);
    if (!get_fourier(v)) return gensizeerr(contextptr);
    return fourier_cn(v[0],v[1],v[2],v[3],v[4],contextptr);
    // gen f=v[0],x=v[1],T=v[2],n=v[3],a=v[4];
    // return fourier_cn(f,x,T,n,a,contextptr);
  } 

  static const char _fourier_cn_s []="fourier_cn";
  static define_unary_function_eval (__fourier_cn,&_fourier_cn,_fourier_cn_s);
  define_unary_function_ptr5( at_fourier_cn ,alias_at_fourier_cn,&__fourier_cn,0,true);

#if defined FXCG || !defined USE_GMP_REPLACEMENTS
  // periodic by Luka Marohnić
  // example f:=periodic(x^2,x,-1,1); plot(f,x=-5..5)
  gen _periodic(const gen & g,GIAC_CONTEXT) {
    if (g.type==_STRNG && g.subtype==-1) return g;
    if (g.type!=_VECT || g.subtype!=_SEQ__VECT)
      return gentypeerr(contextptr);
    vecteur & gv = *g._VECTptr;
    if (gv.size()!=4 && gv.size()!=2)
      return gensizeerr(contextptr);
    gen & e=gv[0],x,a,b;
    //if (e.type!=_SYMB && e.type!=_IDNT) return gentypeerr(contextptr);
    vecteur vars(*_lname(e,contextptr)._VECTptr);
    if (vars.empty())
      return e;
    if (gv.size()==2) {
      if (!gv[1].is_symb_of_sommet(at_equal))
	return gentypeerr(contextptr);
      vecteur & fl=*gv[1]._SYMBptr->feuille._VECTptr;
      if ((x=fl[0]).type!=_IDNT || !fl[1].is_symb_of_sommet(at_interval))
	return gentypeerr(contextptr);
      vecteur & ab=*fl[1]._SYMBptr->feuille._VECTptr;
      a=ab[0];
      b=ab[1];
    }
    else {
      x=gv[1];
      if (x.type!=_IDNT)
	return gentypeerr(contextptr);
      if (find(vars.begin(),vars.end(),x)==vars.end())
	return e;
      a=gv[2];
      b=gv[3];
    }
    gen T(b-a);
    if (!is_strictly_positive(T,contextptr))
      return gentypeerr(contextptr);
    gen p(subst(e,x,x-T*_floor((x-a)/T,contextptr),false,contextptr));
    return p;// _unapply(makesequence(p,x),contextptr);
  }
  static const char _periodic_s []="periodic";
  static define_unary_function_eval (__periodic,&_periodic,_periodic_s);
  define_unary_function_ptr5(at_periodic,alias_at_periodic,&__periodic,0,true);  
#endif

#ifndef NO_NAMESPACE_GIAC
} // namespace giac
#endif // ndef NO_NAMESPACE_GIAC
