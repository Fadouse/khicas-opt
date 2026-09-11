#ifndef KHICAS_ELLIPTIC_FIRST_H
#define KHICAS_ELLIPTIC_FIRST_H
#include <cmath>
namespace giac {
extern const unary_function_ptr * const at_EllipticF;
// Carlson duplication and the degree-seven local polynomial, DLMF 19.36.1.
// https://dlmf.nist.gov/19.36 and https://dlmf.nist.gov/19.25
// Real nonnegative arguments only; scaling prevents intermediate overflow.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
static bool elliptic_first_rf(double x,double y,double z,double &value){
  double scale=x>y?x:y;if(z>scale)scale=z;
  if(!(x>=0 && y>=0 && z>=0 && scale>0 && scale<=1.7976931348623157e308))return false;
  if((x==0 && y==0) || (x==0 && z==0) || (y==0 && z==0))return false;
  x/=scale;y/=scale;z/=scale;
  for(unsigned n=0;n<64;++n){
    double mean=(x+y+z)/3,dx=(mean-x)/mean,dy=(mean-y)/mean,dz=-dx-dy;
    if(std::abs(dx)<0.002 && std::abs(dy)<0.002 && std::abs(dz)<0.002){
      double e2=dx*dy+dy*dz+dz*dx,e3=dx*dy*dz;
      double p=1-e2/10+e3/14+e2*e2/24-3*e2*e3/44-5*e2*e2*e2/208+3*e3*e3/104+e2*e2*e3/16;
      value=p/std::sqrt(mean)/std::sqrt(scale);return true;
    }
    double sx=std::sqrt(x),sy=std::sqrt(y),sz=std::sqrt(z),lambda=sx*sy+sy*sz+sz*sx;
    x=(x+lambda)/4;y=(y+lambda)/4;z=(z+lambda)/4;
  }
  return false; // Exhausting a budget is not permission to return an approximation.
}

#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noinline,optimize("Os")))
#endif
gen _EllipticF(const gen &argument,GIAC_CONTEXT){
  if(argument.type==_STRNG && argument.subtype==-1)return argument;
  if(argument.type!=_VECT || argument._VECTptr->size()!=2)return gensizeerr("EllipticF(phi,m): parameter m, radians");
  if(!angle_radian(contextptr))return gensizeerr("EllipticF requires radians");
  const gen &phi=argument[0],&m=argument[1];
  if(is_undef(phi) || is_undef(m))return undef;
  if(is_zero(phi))return 0;
  if(is_zero(m))return phi;
  if(taille(argument,65)<=64 && has_num_coeff(argument)){
    // Preserve supplied doubles; evalf_double may round them again.
    // Form the complementary parameter exactly before conversion when
    // m is exact, avoiding cancellation near m=1.
    gen p=phi.type==_DOUBLE_?phi:evalf_double(phi,1,contextptr),complement=1-m;
    gen q=complement.type==_DOUBLE_?complement:evalf_double(complement,1,contextptr);
    if(p.type==_DOUBLE_ && q.type==_DOUBLE_ && std::abs(p._DOUBLE_val)<=1000000 && q._DOUBLE_val>0){
      const double pi=3.141592653589793238462643383279502884;
      double turns=std::floor((p._DOUBLE_val+pi/2)/pi),u=p._DOUBLE_val-turns*pi;
      double si=std::sin(u),co=std::cos(u),rf,complete=0;
      if(elliptic_first_rf(co*co,co*co+q._DOUBLE_val*si*si,1,rf) &&
         (turns==0 || elliptic_first_rf(0,q._DOUBLE_val,1,complete)))
        return gen(si*rf+2*turns*complete);
    }
  }
  // Real exact special values. Other exact arguments retain a registered
  // mathematical function, not a numerically guessed or unevaluated integral.
  if(m==-1 && (phi==cst_pi/2 || phi==-cst_pi/2)){
    gen k=sqrt(cst_pi,contextptr)*Gamma(gen(1)/4,contextptr)/(4*Gamma(gen(3)/4,contextptr));
    return phi==cst_pi/2?k:-k;
  }
  return symbolic(at_EllipticF,argument);
}
static const char _EllipticF_s[]="EllipticF";
static define_unary_function_eval(__EllipticF,&_EllipticF,_EllipticF_s);
define_unary_function_ptr5(at_EllipticF,alias_at_EllipticF,&__EllipticF,0,true);
}
#endif
