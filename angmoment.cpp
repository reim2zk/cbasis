#include "angmoment.hpp"
#include "macros.hpp"
#include <iostream>

#include <gsl/gsl_sf_coupling.h>
#include <math.h>

namespace l2func {

  // ==== Exception class ====
  ExceptionBadYlm::ExceptionBadYlm(int L, int M, std::string msg) :std::exception() {
      std::stringstream ss;
      ss << "\nUnphysical (L, M) pair. (L, M) = (" << L << ", " << M << ")";
      msg_ = msg + ss.str();
  }
  ExceptionBadYlm::~ExceptionBadYlm() throw() {}
  const char* ExceptionBadYlm::what() const throw() {
    return msg_.c_str();
  }

  // ==== Utilities ====
  double cg_coef(int j1, int j2, int m1, int m2, int j3, int m3) {
    return pow(-1, j1-j2+m3) * sqrt(2*j3+1.0) *
      gsl_sf_coupling_3j(2*j1, 2*j2, 2*j3, 2*m1, 2*m2, -2*m3);
  }
  int lm_index(int L, int M) {
    return L * L + (M+L);
  }
  int num_lm_pair(int max_l) {

    return lm_index(max_l+1, -max_l-1);

  }
  double GTOExpansionCoef(int l, int m, int lp, int lppp,
			  int Jp, int Mp, int Jpp, int Mpp) {

    // See
    // APPLICATION OF THE SCHWINGER VARIATIONAL PRINCIPLE TO ELECTRON-MOLECULE COLLISIONS AND MOLECULAR PHOTOIONIZATION
    // R.Lucchese, K.Takatsuka, V.McKoy
    // Phys. Rep. 131, (1986), 147

    double cumsum(0);

    for(int mp = -lp; mp <= lp; mp++)
      for(int mpp = -lp; mpp <= lp; mpp++)
	for(int J = l-lp; J <= l+lp; J++)
	  for(int M = -J; M <= J; M++)
	    for(int mppp = -lppp; mppp <= lppp; mppp++) {
	      double t1 = pow(-1, l+m-(lp+mp)) * (2.0*lppp+1) / DFactorial(l-lp);
	      double t2 = sqrt((4.0 * M_PI * (2*l+1)*DFactorial(l-mp)*DFactorial(l+mp)) /
			       ((2.0*Jp+1)*(2.0*Jpp+1)*DFactorial(lp-mp)*DFactorial(lp+mp)));
	      double t3 = (cg_coef(lp,l,mpp,-m,J,M) *
			   cg_coef(lp,l,mp,-mp,J,0) * 
			   cg_coef(J,lppp,0,0,Jp,0) *
			   cg_coef(J,lppp,M,mppp,Jp,Mp) * 
			   cg_coef(lp,lppp,0,0,Jpp,0) *
			   cg_coef(lp,lppp,mpp,mppp,Jpp,Mpp));
		
	      cumsum += t1*t2*t3;
	    }
    return cumsum;
  }

  // ==== Special functions ====
  void ModSphericalBessel(dcomplex x, int max_n, dcomplex* res) {

    double eps(0.0001);

    if(abs(x) < eps) {
      dcomplex xx(0.5*x*x);
      dcomplex xn(1);
      for(int n = 0; n < max_n + 1; n++) {
	res[n] = (xn / (1.0*DoubleFactorial(2*n+1)) *
		  (1.0 + xx/(2.0*n+3.0) +
		   pow(xx,2)/(2.0*(2.0*n+3)*(2.0*n+5))));
	xn *= x;
      }
    } else {
      res[0] = sinh(x) / x;
      res[1] = (x*cosh(x)-sinh(x)) / (x*x);
      for(int n = 1; n < max_n; n++) 
	res[n+1] = res[n-1] - (2.0*n+1.0)/x*res[n];
    }
  }
  void AssociatedLegendre(dcomplex x, int max_l, dcomplex* res) {

    /*
      before calling this function, pointer res must allocate:
      // res = new dcomplex[num_lm_pair(max_l)]
     */
    
    res[0] = 1.0;
    for(int L = 0; L < max_l; L++) {
      dcomplex val = -(2.0*L+1.0) * sqrt(1.0-x*x) * res[lm_index(L, L)];
      res[lm_index(L+1, L+1)] = val;
      res[lm_index(L+1, -L-1)] = pow(-1.0, L+1) / DFactorial(2*(L+1)) * val;
    }

    for(int L = 1; L <= max_l; L++)
      for(int M = -L; M < L-1; M++) {
	dcomplex t1 = (L-M)*1.0*x*res[lm_index(L, M)];
	dcomplex t2 = (L+M)*1.0*res[lm_index(L-1, M)];
	res[lm_index(L, M+1)] =  (t1 - t2) / sqrt(1.0-x*x);
      }

  }
  void RealSphericalHarmonics(dcomplex theta, dcomplex phi, int max_l, dcomplex* res) {

    // WARNING
    // The expression used for Real valued Spherical Harmonics is different from
    // one in Wiki.
    // 
    // res must be allocate before this function:
    // // res = new dcomplex[num_lm_pair(max_l)];

    AssociatedLegendre(cos(theta), max_l, res);
    for(int l = 0; l <= max_l; l++) {
      
      res[lm_index(l, 0)] = sqrt((2*l+1)/(4.0*M_PI)) * res[lm_index(l, 0)];
      for(int m = 1; m <= l; m++) {
	int am = abs(m);
	dcomplex plm = res[lm_index(l, am)];
	dcomplex t0 = pow(-1, m)*sqrt(2.0)*
	  sqrt((2*l+1) / (4.0*M_PI)*DFactorial(l-am)*1.0/DFactorial(l+am));
	res[lm_index(l, -m)] = t0 * plm * sin(am*1.0*phi);
	
	dcomplex t1 = (pow(-1, m) * sqrt(2.0) *
		       sqrt((2.0*l+1)/(4.0*M_PI) * DFactorial(l-m)/DFactorial(l+m)));
	res[lm_index(l,m)] = t1 * plm * cos(m*1.0*phi);
      }
    }
  }

  /*
  dcomplex* SphericalHarmonics(dcomplex phi, dcomplex theta, int max_l) {

    std::string msg;
    SUB_LOCATION(msg);
    msg += "Not tested";
    throw std::runtime_error(msg);

    dcomplex* ps = AssociatedLegendre(cos(theta), max_l);
    dcomplex* vs = new dcomplex[num_lm_pair(max_l)];
    for(int L = 0; L <= max_l; L++)
      for(int M = -L; M <= L; M++) {
	vs[lm_index(L, M)] = pow(-1.0, (M+abs(M))/2) *
	  sqrt((2*L+1)/(4.0*M_PI) * DFactorial(L-abs(M))/DFactorial(L+abs(M))) *
	       ps[lm_index(L, abs(M))] * exp(dcomplex(0.0, 1.0)*(1.0*M)*phi);
      }
    delete ps;
    return vs;
  }
  */
  
  void gto_00_r(dcomplex x, dcomplex y, dcomplex z, int Jpp, int Mpp,dcomplex zeta,
		dcomplex* rs, int num_r,  dcomplex* work, dcomplex* res) {
    /*
      gives one center expansion values of (Jpp,Mpp) wave for s-GTO at (x,y,z).

      Inputs
      ------
      x,y,z : location of s-GTO to expand
      Jpp,Mpp: partial wave to obtain 
      zeta   : orbital exponents
      *rs    : location array for evaluation. (new dcomplex[num_r])
      num_r  : number of location rs.
      *work  : working space (new dcomplex[num_lm(Jpp) + Jpp +1])
     */

    dcomplex a2= x*x+y*y+z*z;
    dcomplex a = sqrt(a2);
    dcomplex theta = acos(z / a);
    dcomplex phi   = acos(x/sqrt(x*x+y*y));

    dcomplex* ylm = &work[0];
    dcomplex* il =  &work[num_r];

    RealSphericalHarmonics(theta, phi, Jpp, ylm);

    for(int i = 0; i < num_r; i++) {
      dcomplex r(rs[i]);
      ModSphericalBessel(2.0*zeta*a*r, Jpp, il);
      res[i] = (4.0*M_PI * exp(-zeta*(r*r+a2)) * il[Jpp] *
		pow(-1.0, Mpp) * ylm[lm_index(Jpp, -Mpp)]);
    }
  }
  void gto_lm_r_center(int l, int m, int Jpp, int Mpp, dcomplex zeta,
			    dcomplex* rs, int num_r, dcomplex* res) {

    for(int i = 0; i < num_r; i++) {
      dcomplex r(rs[i]);
      if(l != Jpp || m != Mpp) {
	res[i] = dcomplex(0.0, 0.0);
      } else {
	res[i] = pow(r, l) * exp(-zeta*r*r);
      }
    }
  }

  void gto_lm_r_general(int l, int m,
			     dcomplex x, dcomplex y, dcomplex z,
			     int Jpp, int Mpp,
			     dcomplex zeta,
			     int lppp_max,
			     dcomplex* rs, int num_r,
			     dcomplex* work,
			     dcomplex* res) {
    std::string msg;
    SUB_LOCATION(msg);
    msg += "not implemented for general case";
    throw std::runtime_error(msg);


    dcomplex *ylm = &work[0];
    dcomplex *il  = &work[num_r];
    dcomplex theta = acos(z / sqrt(x*x+y*y+z*z));
    dcomplex phi = acos(x / sqrt(x*x+y*y));
    dcomplex A = sqrt(x*x + y*y + z*z);
    RealSphericalHarmonics(theta, phi, 2*l+lppp_max, ylm);

    for(int i = 0; i < num_r; i++) {
      dcomplex r(rs[i]);
      ModSphericalBessel(2.0*A * r, lppp_max, il);
      dcomplex e_term = exp(-zeta*(r*r+A*A));

      dcomplex cumsum(0);
      for(int lp = 0; lp <= l; lp++)
	for(int lppp = 0; lppp <= lppp_max; lppp++)
	  for(int Jp = abs(l-lp-lppp); Jp <= l+lp+lppp; Jp++) 
	    for(int Mp = -Jp; Mp <= Jp; Mp++)
	      cumsum += GTOExpansionCoef(l, m, lp, lppp, Jp, Mp, Jpp, Mpp) *
		pow(A, l) * pow(r/A, lp) * e_term * il[lppp] *
		ylm[lm_index(Jp, Mp)];
      res[i] = cumsum;
    }

  }
  void gto_lm_r(int l, int m,
		     dcomplex x, dcomplex y, dcomplex z,
		     int Jpp, int Mpp,
		     dcomplex zeta,
		     int lppp_max,
		     dcomplex* rs, int num_r,
		     dcomplex* work, dcomplex* res) {

    double eps = pow(10.0, -10.0);
    if(l == 0 && m == 0) {
      gto_00_r(x, y, z, Jpp, Mpp, zeta, rs, num_r, work, res);
    } else if(abs(x) < eps && abs(y) < eps && abs(z) < eps) {
      gto_lm_r_center(l, m, Jpp, Mpp, zeta, rs, num_r, res);
    } else {
      gto_lm_r_general(l, m, x, y, z, Jpp, Mpp, zeta, lppp_max,
		       rs, num_r, work, res);
    }
  }


}
