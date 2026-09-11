#ifndef KHICAS_DILOGARITHM_H
#define KHICAS_DILOGARITHM_H
#include <complex>
#include <cmath>
#ifndef NO_NAMESPACE_GIAC
namespace giac {
#endif
extern const unary_function_ptr * const at_Li2;
typedef std::complex<double> khicas_dilog_complex;
// Principal complex branch, with the real cut itself using Log(negative)=+i*pi.
// Inversion followed by reflection needs at most two reductions. In the
// reduced disk the Bernoulli expansion has |log(1-z)| <= pi/3; its fixed
// 12 even terms have truncation below double rounding. Tiny z uses a series
// directly to avoid losing z in the subtraction 1-z.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static std::complex<double> khicas_dilog_log(khicas_dilog_complex z){
  double a=std::abs(z.real()),b=std::abs(z.imag());
  if(a<b){double t=a;a=b;b=t;}
  double ratio=a==0?0:b/a;
  return khicas_dilog_complex(std::log(a)+std::log(1+ratio*ratio)/2,
                             std::atan2(z.imag()==0?0.:z.imag(),z.real()));
}
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static std::complex<double> khicas_dilog_numeric(khicas_dilog_complex z){
 const double pi=3.141592653589793238462643383279502884;
 if(z==khicas_dilog_complex(1,0))return khicas_dilog_complex(pi*pi/6,0);
 double radius=std::abs(z);
 if(radius<=0.25){
  khicas_dilog_complex sum=z,power=z;
  for(int n=2;n<=48;++n){power*=z;khicas_dilog_complex term=power/double(n*n);sum+=term;if(std::abs(term)<=2e-17*std::abs(sum))break;}
  return sum;
 }
 if(radius>1){khicas_dilog_complex L=khicas_dilog_log(-z);return -khicas_dilog_numeric(khicas_dilog_complex(1,0)/z)-pi*pi/6.-L*L/2.;}
 if(z.real()>0.5)return pi*pi/6.-khicas_dilog_log(z)*khicas_dilog_log(khicas_dilog_complex(1,0)-z)-khicas_dilog_numeric(khicas_dilog_complex(1,0)-z);
 static const double coefficients[]={0.027777777777777776,-0.00027777777777777778,4.7241118669690098e-06,-9.1857730746619641e-08,1.8978869988971001e-09,-4.0647616451442256e-11,8.9216910204564523e-13,-1.9939295860721074e-14,4.5189800296199183e-16,-1.0356517612181247e-17,2.395218621026187e-19,-5.581785874325009e-21};
 khicas_dilog_complex w=-khicas_dilog_log(khicas_dilog_complex(1,0)-z),w2=w*w,power=w*w2,sum=w-w2/4.;
 for(unsigned i=0;i<12;++i){sum+=coefficients[i]*power;power*=w2;}
 return sum;
}

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static gen d_Li2(const gen &z,GIAC_CONTEXT){
  if(is_zero(z))return 1;
  gen regular=-ln(1-z,contextptr)/z;
  if(z.type<=_REAL || z.is_symb_of_sommet(at_exp))return regular;
  // Li2 is analytic at zero. Preserve its removable derivative value in
  // the expression itself, so substitution after a chain rule is valid.
  return symbolic(at_when,makesequence(symbolic(at_equal,makesequence(z,0)),1,regular));
}
define_partial_derivative_onearg_genop(D_at_Li2,"D_at_Li2",&d_Li2);
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
gen _Li2(const gen &argument,GIAC_CONTEXT){
  if(argument.type==_STRNG && argument.subtype==-1)return argument;
  // Evaluate a symbolic unit-circle phase without expanding its exponential
  // into cyclotomic roots. Quoting is limited to this exact bounded form;
  // ordinary arguments (including assigned names and numeric input) evaluate
  // normally below.
  if(argument.is_symb_of_sommet(at_exp) && taille(argument,65)<=64){
    gen phase=argument._SYMBptr->feuille.eval(1,contextptr);
    vecteur names=lidnt(phase);bool variable=false;
    for(unsigned j=0;j<names.size();++j)if(names[j]!=cst_pi){variable=true;break;}
    if(variable && !has_num_coeff(phase) && is_zero(re(phase,contextptr)))
      return symbolic(at_Li2,symbolic(at_exp,phase));
  }
  gen z=argument.eval(1,contextptr);
  if(z.type==_VECT)return apply(z,_Li2,contextptr);
  if(is_undef(z))return z;
  if(is_zero(z))return z;
  if(taille(z,129)<=128 && has_num_coeff(z)){
    gen value=evalf_double(z,1,contextptr);khicas_dilog_complex input;
    if(value.type==_DOUBLE_)input=khicas_dilog_complex(value._DOUBLE_val,0);
    else if(value.type==_CPLX){
      gen r=evalf_double(re(value,contextptr),1,contextptr),i=evalf_double(im(value,contextptr),1,contextptr);
      if(r.type!=_DOUBLE_ || i.type!=_DOUBLE_)return symbolic(at_Li2,z);
      input=khicas_dilog_complex(r._DOUBLE_val,i._DOUBLE_val);
    }
    else return symbolic(at_Li2,z);
    if(!(std::abs(input.real())<=1.7976931348623157e308) || !(std::abs(input.imag())<=1.7976931348623157e308))return undef;
    khicas_dilog_complex result=khicas_dilog_numeric(input);
    if(input.imag()==0 && input.real()<=1)return gen(result.real());
    return gen(result.real(),result.imag());
  }
  if(is_one(z))return cst_pi*cst_pi/6;
  if(z==-1)return -cst_pi*cst_pi/12;
  if(z==gen(1)/2){gen L=ln(gen(2),contextptr);return cst_pi*cst_pi/12-L*L/2;}
  return symbolic(at_Li2,z);
}
static const char _Li2_s[]="Li2";
#ifdef GIAC_HAS_STO_38
static define_unary_function_eval3_quoted(__Li2,&_Li2,(size_t)&D_at_Li2unary_function_ptr,_Li2_s);
#else
static define_unary_function_eval3_quoted(__Li2,&_Li2,D_at_Li2,_Li2_s);
#endif
define_unary_function_ptr5(at_Li2,alias_at_Li2,&__Li2,_QUOTE_ARGUMENTS,true);
#ifndef NO_NAMESPACE_GIAC
}
#endif
#endif
