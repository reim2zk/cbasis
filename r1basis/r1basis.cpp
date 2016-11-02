#include <stdexcept>
#include <iostream>
#include "../src_cpp/fact.hpp"
#include "../src_cpp/macros.hpp"
#include "r1basis.hpp"
#include "r1_lc.hpp"

using namespace std;
using namespace Eigen;

namespace cbasis {

  // ==== create =====
  template<int m>
  boost::shared_ptr<_EXPs<m> > Create_EXPs() {

    typedef typename _EXPs<m>::EXPs EXPs;
    EXPs ptr(new _EXPs<m>);
    return ptr;

  }
  STOs Create_STOs() {

    return Create_EXPs<1>();
    
  }
  GTOs Create_GTOs() {

    return Create_EXPs<2>();
    
  }

  // ==== Calculation ====
  dcomplex STOInt(int n, dcomplex a) {

    if(n < 0) {
      string msg; SUB_LOCATION(msg);
      msg += "\nn must be bigger than 0";
      throw runtime_error(msg);
    }

    if(n == 0)
      return 1.0/a;
    
    return STOInt(n-1, a) * (1.0*n) / a;
    
    //    return DFactorial(n)/pow(a, n+1)

  }
  dcomplex GTOInt(int n, dcomplex a) {

    if(n < 0) {
      std::string msg; SUB_LOCATION(msg);
      msg += "\nn must be bigger than 0";
      throw std::runtime_error(msg);
    }

    if(n == 0)
      return sqrt(M_PI)/(2.0*sqrt(a));
    if(n == 1)
      return 0.5/a;

    return dcomplex(n-1)/(2.0*a) * GTOInt(n-2, a);

  }
  template<int m1, int m2>
  dcomplex EXPInt(int n, dcomplex a, dcomplex b);
  template<>
  dcomplex EXPInt<1,1>(int n, dcomplex a, dcomplex b) {
    return STOInt(n, a+b);
  }
  template<>
  dcomplex EXPInt<2,2>(int n, dcomplex a, dcomplex b) {
    return GTOInt(n, a+b);
  }
  template<>
  dcomplex EXPInt<1,2>(int n, dcomplex a, dcomplex b) {

    string msg; SUB_LOCATION(msg);
    throw runtime_error(msg);
    return 1.0;

  }
  template<>
  dcomplex EXPInt<2,1>(int n, dcomplex a, dcomplex b) {

    string msg; SUB_LOCATION(msg);
    throw runtime_error(msg);
    return 1.0;

  }

  template<int m1, int m2>
  dcomplex EXPIntD2(int na, dcomplex a, int nb, dcomplex b);
  template<>
  dcomplex EXPIntD2<1, 1>(int ni, dcomplex zi, int nj, dcomplex zj) {

    //
    // dr2 r^n exp(-zr) = { zz -2nzr^{-1} + (nn-n)r^{-2}} r^n exp[-zr]
    //

    dcomplex acc(0);
    acc += zj*zj      * EXPInt<1,1>(ni+nj,   zi, zj);
    acc += -2.0*nj*zj * EXPInt<1,1>(ni+nj-1, zi, zj);
    if(nj > 1)
      acc += dcomplex(nj*nj-nj) * EXPInt<1,1>(ni+nj-2, zi, zj);

    return acc;

  }
  template<>
  dcomplex EXPIntD2<2, 1>(int ni, dcomplex zi, int nj, dcomplex zj) {

    //
    // dr2 r^n exp(-zr) = { zz -2nzr^{-1} + (nn-n)r^{-2}} r^n exp[-zr]
    //

    dcomplex acc(0);
    acc += zj*zj      * EXPInt<2,1>(ni+nj,   zi, zj);
    acc += -2.0*nj*zj * EXPInt<2,1>(ni+nj-1, zi, zj);
    if(nj > 1)
      acc += dcomplex(nj*nj-nj) * EXPInt<2,1>(ni+nj-2, zi, zj);

    return acc;

  }  
  template<>
  dcomplex EXPIntD2<1, 2>(int ni, dcomplex zi, int nj, dcomplex zj) {
    
    //
    // dr2 r^n exp(-zr^2) = { 4zz r^{2} -2z(2n+1) + (nn-n)r^{-2}} r^n exp[-zr^2]
    //
    
    dcomplex acc(0);
    acc += ( +4.0*zj*zj * EXPInt<1,2>(ni+nj+2, zi, zj)
	     -2.0*(2*nj+1)*zj* EXPInt<1,2>(ni+nj, zi, zj));
    if(nj > 1)
      acc += dcomplex(nj*nj-nj) * EXPInt<1,2>(ni+nj-2, zi, zj);
    return acc;
    
  }  
  template<>
  dcomplex EXPIntD2<2, 2>(int ni, dcomplex zi, int nj, dcomplex zj) {
    
    //
    // dr2 r^n exp(-zr^2) = { 4zz r^{2} -2z(2n+1) + (nn-n)r^{-2}} r^n exp[-zr^2]
    //
    
    dcomplex acc(0);
    acc += ( +4.0*zj*zj * EXPInt<2,2>(ni+nj+2, zi, zj)
	     -2.0*(2*nj+1)*zj* EXPInt<2,2>(ni+nj, zi, zj));
    if(nj > 1)
      acc += dcomplex(nj*nj-nj) * EXPInt<2,2>(ni+nj-2, zi, zj);
    return acc;
    
  }

  dcomplex EXPIntLC(LC_STOs a, int m, LC_STOs b) {
    dcomplex acc(0);
    for(int i = 0; i < a->size(); i++)
      for(int j = 0; j < b->size(); j++) {
	dcomplex c(a->c(i) * b->c(j));
	int      n(a->n(i) + b->n(j));
	dcomplex z(a->z(i) + b->z(j));
	acc +=  c * STOInt(n+m, z);
      }
    return acc;    
  }
  dcomplex EXPIntLC(LC_GTOs a, int m, LC_GTOs b) {
    dcomplex acc(0);
    for(int i = 0; i < a->size(); i++)
      for(int j = 0; j < b->size(); j++) {
	dcomplex c(a->c(i) * b->c(j));
	int      n(a->n(i) + b->n(j));
	dcomplex z(a->z(i) + b->z(j));
	acc +=  c * GTOInt(n+m, z);
      }
    return acc;    
  }

  // ==== vector ====
  template<int m1, int m2>
  VectorXcd CalcVec(_EXPs<m1>& a, _LC_EXPs<m2>& b) {
    
    if(!a.HasCoef()) {
      string msg; SUB_LOCATION(msg);
      msg = "\n" + msg + "EXP does not have coef. Call SetUp or use no_normal method only";
      throw runtime_error(msg);
    }
    
    int numa(a.size());
    VectorXcd vec(numa);
    
    for(int i = 0; i < numa; i++) {
      
	typename _EXPs<m1>::LC_EXPs bi = a.basis(i);
	dcomplex acc(0);
	for(int jj = 0; jj < b.size(); jj++) {
	  dcomplex c(bi->c(i) * b.c(jj));
	  int      n(bi->n(i) + b.n(jj));
	  dcomplex zi(bi->z(i));
	  dcomplex zj(b.z(jj));
	  acc +=  c * EXPInt<m1, m2>(n, zi, zj);
	}
	vec(i) = acc;
    }
    return vec;
  }
  template VectorXcd CalcVec<1,1>(_EXPs<1>& a, _LC_EXPs<1>& b);
  template VectorXcd CalcVec<1,2>(_EXPs<1>& a, _LC_EXPs<2>& b);
  template VectorXcd CalcVec<2,1>(_EXPs<2>& a, _LC_EXPs<1>& b);
  template VectorXcd CalcVec<2,2>(_EXPs<2>& a, _LC_EXPs<2>& b);
  
  // ==== matrix ====
  template<int m1, int m2>
  MatrixXcd CalcRmMat(_EXPs<m1>& a, int M, _EXPs<m2>& b) {
    
    if(!a.HasCoef() || !b.HasCoef()) {
      string msg; SUB_LOCATION(msg);
      msg = "\n" + msg + "EXP does not have coef. Call SetUp or use no_normal method only";
      throw runtime_error(msg);
    }
    
    int numa(a.size());
    int numb(b.size());
    MatrixXcd mat(numa, numb);
    
    for(int i = 0; i < numa; i++)
      for(int j = 0; j < numb; j++) {

	typename _EXPs<m1>::LC_EXPs bi = a.basis(i);
	typename _EXPs<m2>::LC_EXPs bj = b.basis(j);

	dcomplex acc(0);
	for(int ii = 0; ii < bi->size(); ii++) {
	  for(int jj = 0; jj < bj->size(); jj++) {
	    dcomplex c(bi->c(ii) * bj->c(jj));
	    int      n(bi->n(ii) + bj->n(jj));
	    dcomplex zi(bi->z(ii));
	    dcomplex zj(bj->z(jj));
	    acc +=  c * EXPInt<m1, m2>(n+M, zi, zj);
	  }
	}
	mat(i, j) = acc;
      }

    return mat;
  }
  template MatrixXcd CalcRmMat<1,1>(_EXPs<1>& a, int M, _EXPs<1>& b);
  template MatrixXcd CalcRmMat<1,2>(_EXPs<1>& a, int M, _EXPs<2>& b);
  template MatrixXcd CalcRmMat<2,1>(_EXPs<2>& a, int M, _EXPs<1>& b);
  template MatrixXcd CalcRmMat<2,2>(_EXPs<2>& a, int M, _EXPs<2>& b);
  
  template<int m1, int m2>
  Eigen::MatrixXcd CalcD2Mat(_EXPs<m1>& a, _EXPs<m2>& b) {

    if(!a.HasCoef() || !b.HasCoef()) {
      string msg; SUB_LOCATION(msg);
      msg = "\n" + msg + "No coefficient.";
      throw runtime_error(msg);
    }

    int numa(a.size());
    int numb(b.size());
    MatrixXcd mat(numa, numb);
    
    for(int i = 0; i < numa; i++) {
      for(int j = 0; j < numb; j++) {
	
	typename _EXPs<m1>::LC_EXPs bi = a.basis(i);
	typename _EXPs<m2>::LC_EXPs bj = b.basis(j);
	
	dcomplex acc(0);
	for(int ii = 0; ii < bi->size(); ii++)
	  for(int jj = 0; jj < bj->size(); jj++) {
	    dcomplex c(bi->c(ii) * bj->c(jj));
	    int      ni(bi->n(ii));
	    int      nj(bj->n(jj));
	    dcomplex zi(bi->z(ii));
	    dcomplex zj(bj->z(jj));
	    acc += c * EXPIntD2<m1, m2>(ni, zi, nj, zj);
	  }
	mat(i, j) = acc;
      }
    }
    
    return mat;    

  }
  template MatrixXcd CalcD2Mat<1,1>(_EXPs<1>& a, _EXPs<1>& b);
  template MatrixXcd CalcD2Mat<1,2>(_EXPs<1>& a, _EXPs<2>& b);
  template MatrixXcd CalcD2Mat<2,1>(_EXPs<2>& a, _EXPs<1>& b);
  template MatrixXcd CalcD2Mat<2,2>(_EXPs<2>& a, _EXPs<2>& b);
				  
  /*
  template<int m1, int m2>
  MatrixXcd CalcRMMat(shared_ptr<_EXPs<m1> > a, int M,
		      shared_ptr<_EXPs<m2> >b) {
    return MatrixXcd::Zero(2, 2);
  }
  template<int m1, int m2>
  MatrixXcd CalcD2Mat(boost::shared_ptr<_EXPs<m1> > a,
		      boost::shared_ptr<_EXPs<m2> > b) {
    return MatrixXcd::Zero(2, 2);
  }
  template MatrixXcd CalcD2Mat<1,1>(boost::shared_ptr<_EXPs<1> > a,
				    boost::shared_ptr<_EXPs<1> > b);
  */
  /*
  MatrixXcd CalcRMMatSG(STOs a, int M, GTO b) {return }
  MatrixXcd CalcRMMatGS(GTOs a, int M, STO b);
  MatrixXcd CalcRMMatGG(GTOs a, int M, GTO b);

  MatrixXcd CalcD2MatSS(STOs a, STO b);
  MatrixXcd CalcD2MatSG(STOs a, GTO b);
  MatrixXcd CalcD2MatGS(GTOs a, STO b);
  MatrixXcd CalcD2MatGG(GTOs a, GTO b);

  VectorXcd CalcVecSS(STOs a, LC_STOs b);
  VectorXcd CalcVecSG(STOs a, LC_GTOs b);
  VectorXcd CalcVecGS(GTOs a, LC_STOs b);
  VectorXcd CalcVecGG(GTOs a, LC_GTOs b);    
  */

  // ==== member field ====
  template<int m>
  _EXPs<m>::_EXPs() {
  }
  template<int m>
  bool _EXPs<m>::OnlyPrim() const {

    bool acc(true);

    typedef typename vector<LC_EXPs>::const_iterator It;
    for(It it = this->basis_.begin();
	it != this->basis_.end();
	++it) {
      acc &= (*it)->size() == 1;
    }

    return acc;
    
  }
  template<int m>
  VectorXcd _EXPs<m>::AtR(const VectorXcd& rs, const VectorXcd& cs) const {

    if(cs.size() != this->size()) {
      string msg;
      SUB_LOCATION(msg);
      msg += " : size mismatch.";
      throw runtime_error(msg);
    }

    int num(this->size());
    VectorXcd ys = VectorXcd::Zero(rs.size());

    for(int i = 0; i < num; i++) {
      VectorXcd ys0 = this->basis(i)->AtR(rs);
      ys += cs[i] * ys0;
    }

    return ys;

  }
  template<int m>
  VectorXcd _EXPs<m>::DAtR(const VectorXcd& rs, const VectorXcd& cs) const {

    if(cs.size() != this->size()) {
      string msg;
      SUB_LOCATION(msg);
      msg += " : size mismatch.";
      throw runtime_error(msg);
    }

    int num(this->size());
    VectorXcd ys = VectorXcd::Zero(rs.size());

    for(int i = 0; i < num; i++) {
      VectorXcd ys0 = this->basis(i)->DAtR(rs);
      ys += ys0;
    }

    return ys;

  }
  template<int m>
  string _EXPs<m>::str() const {
    ostringstream oss; 
    if(m == 1) {
      oss << "==== STOs ====" << endl;
      oss << "STO basis set" << endl;
    } else if (m == 2) {
      oss << "==== GTOs ====" << endl;
      oss << "GTO basis set" << endl;
    }

    for(int i = 0; i < this->size(); i++) {
      oss << i << " : " << this->basis(i)->str() << endl;
    }
    
    oss << "==============" << endl;
    
    return oss.str();
  }
  template<int m>
  bool _EXPs<m>::IsNormal() const {
    bool acc = true;
    typedef vector<int>::const_iterator It;
    for(It it = coef_type_.begin(); it != coef_type_.end(); ++it) {
      acc = acc && (*it == COEF_NORMAL);
    }
    return acc;
  }
  template<int m> 
  bool _EXPs<m>::HasCoef() const {
    bool acc = true;
    typedef vector<int>::const_iterator It;
    for(It it = coef_type_.begin(); it != coef_type_.end(); ++it) {
      acc = acc && (*it == COEF_NORMAL || *it == COEF_NOT_NORMAL);
    }
    return acc;
  }

  template<int m>
  _EXPs<m>* _EXPs<m>::AddPrim(int n, dcomplex z) {

    LC_EXPs g = Create_LC_EXPs<m>();
    g->Add(1.0, n, z);
    this->basis_.push_back(g);
    this->coef_type_.push_back(COEF_NO);
    return this;
    
  }
  template<int m>
  _EXPs<m>* _EXPs<m>::AddPrims(int n, Eigen::VectorXcd zs) {

    for(int i = 0; i < zs.size(); i++) 
      this->AddPrim(n, zs[i]);    
    return this;
    
  }
  template<int m>
  _EXPs<m>* _EXPs<m>::AddLC(LC_EXPs lc) {
    
    this->basis_.push_back(lc);
    this->coef_type_.push_back(COEF_NO);
    return this;
    
  }
  template<int m>
  _EXPs<m>* _EXPs<m>::AddNotNormalPrim(dcomplex c, int n, dcomplex z) {

    LC_EXPs g = Create_LC_EXPs<m>();
    g->Add(c, n, z);
    this->basis_.push_back(g);
    this->coef_type_.push_back(COEF_NOT_NORMAL);
    return this;
    
  }
  template<int m>
  _EXPs<m>* _EXPs<m>::AddNotNormalLC(LC_EXPs lc) {

    this->basis_.push_back(lc);
    this->coef_type_.push_back(COEF_NOT_NORMAL);
    return this;
    
  }    

  template<int m>
  _EXPs<m>* _EXPs<m>::SetUp() {

    if(this->HasCoef())
      return this;

    int num(this->size());
    
    for(int i = 0; i < num; i++) {

      if(this->coef_type_[i] == COEF_NO) {
	LC_EXPs bi = this->basis(i);
	dcomplex nterm(1.0/sqrt(EXPIntLC(bi, 0, bi)));
	for(int ii = 0; ii < bi->size(); ii++) {
	  bi->c(ii) *= nterm;
	}
	this->coef_type_[i] = COEF_NORMAL;
      }
    }

    return this;
    
  }

  template<int m>
  typename _EXPs<m>::EXPs _EXPs<m>::Clone() const {
    
    EXPs ptr = Create_EXPs<m>();

    int num(this->size());
    for(int i = 0; i < num; i++) {
      LC_EXPs lc = this->basis_[i]->Clone();
      ptr->basis_.push_back(lc);
      ptr->coef_type_.push_back(this->coef_type_[i]);
    }
    ptr->SetUp();
    return ptr;
  }
  template<int m>
  typename _EXPs<m>::EXPs _EXPs<m>::Conj() const {

    EXPs ptr = Create_EXPs<m>();

    int num(this->size());
    for(int i = 0; i < num; i++) {
      LC_EXPs lc = this->basis_[i]->Conj();
      ptr->basis_.push_back(lc);
      ptr->coef_type_.push_back(this->coef_type_[i]);
    }
    ptr->SetUp();
    return ptr;
    
  }

  template<int m>
  MatrixXcd _EXPs<m>::CalcRmMat(int M) const {
    
    int num(this->size());
    MatrixXcd mat(num, num);
    
    for(int i = 0; i < num; i++)
      for(int j = 0; j < num; j++) {

	LC_EXPs bi = this->basis(i);
	LC_EXPs bj = this->basis(j);

	dcomplex acc(0);
	for(int ii = 0; ii < bi->size(); ii++) {
	  for(int jj = 0; jj < bj->size(); jj++) {
	    dcomplex c(bi->c(ii) * bj->c(jj));
	    int      n(bi->n(ii) + bj->n(jj));
	    dcomplex zi(bi->z(ii));
	    dcomplex zj(bj->z(jj));
	    acc +=  c * EXPInt<m,m>(n+M, zi, zj);
	  }
	}
	mat(i, j) = acc;
	//mat(i, j) = EXPIntLC<m>(bi, M, bj);
      }

    return mat;

  }
  template<int m>
  MatrixXcd _EXPs<m>::CalcD2Mat() const {

    int num(this->size());
    MatrixXcd mat(num, num);
    
    for(int i = 0; i < num; i++)
      for(int j = 0; j < num; j++) {

	LC_EXPs bi = this->basis(i);
	LC_EXPs bj = this->basis(j);
	dcomplex acc(0);
	for(int ii = 0; ii < bi->size(); ii++)
	  for(int jj = 0; jj < bj->size(); jj++) {
	    dcomplex c(bi->c(ii) * bj->c(jj));
	    int      ni(bi->n(ii));
	    int      nj(bj->n(jj));
	    dcomplex zi(bi->z(ii));
	    dcomplex zj(bj->z(jj));
	    acc += c * EXPIntD2<m,m>(ni, zi, nj, zj);
	  }
	mat(i, j) = acc;
      }

    return mat;
  }
  template<int m>
  VectorXcd _EXPs<m>::CalcVecSTO(LC_STOs o) const {
    
    int num(this->size());
    int numo(o->size());

    VectorXcd vec(num);
        
    for(int i = 0; i < num; i++) {
      _EXPs<m>::LC_EXPs bi = this->basis(i);
      dcomplex acc;
      for(int oo = 0; oo < numo; oo++) {
	for(int ii = 0; ii < bi->size(); ii++) {
	  dcomplex c(bi->c(ii) * o->c(oo));
	  int      n(bi->n(ii) + o->n(oo));
	  acc += c * EXPInt<m, 1>(n, bi->z(ii), o->z(oo));
	}
      }
      
      vec(i) = acc;
      
    }
    
    return vec;

  }
  template<int m>
  VectorXcd _EXPs<m>::CalcVecGTO(LC_GTOs gtos) const {

    
    int num(this->size());
    int numo(gtos->size());

    VectorXcd vec(num);
        
    for(int i = 0; i < num; i++) {
      _EXPs<m>::LC_EXPs bi = this->basis(i);
      dcomplex acc;
      for(int oo = 0; oo < numo; oo++) {
	for(int ii = 0; ii < bi->size(); ii++) {
	  dcomplex c(bi->c(ii) * gtos->c(oo));
	  int      n(bi->n(ii) + gtos->n(oo));
	  acc += c * EXPInt<m, 2>(n, bi->z(ii), gtos->z(oo));
	}
      }
      
      vec(i) = acc;
      
    }
    
    return vec;

  }

  // ==== realize ====
  template class _EXPs<1>;
  template class _EXPs<2>;


}
