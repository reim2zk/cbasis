#ifndef EIGNN_PLUS_H
#define EIGNN_PLUS_H

#include <Eigen/Core>
#include "typedef.hpp"

template<class F>
Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>
m33(F m00, F m01,
    F m10, F m11) {
  Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic> M(2, 2);
  M << m00, m01, m10, m11;
  return M;
}

template<class F>
Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>
m13(F m00, F m01, F m02) {
  Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic> M(1, 3);
  M << m00, m01, m02;
  return M;
}
Eigen::MatrixXcd
m13cd(dcomplex m00, dcomplex m01, dcomplex m02);
Eigen::MatrixXcd
m33cd(dcomplex m00, dcomplex m01, dcomplex m02,
      dcomplex m10, dcomplex m11, dcomplex m12,
      dcomplex m20, dcomplex m21, dcomplex m22);
Eigen::VectorXi v1i(int i);
Eigen::VectorXi v3i(int,int,int);
Eigen::VectorXcd v1cd(dcomplex v);
Eigen::VectorXcd v3cd(dcomplex v1, dcomplex v2, dcomplex v4);

template<class F>
Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic>
m33(F m00, F m01, F m02,
    F m10, F m11, F m12,
    F m20, F m21, F m22) {
  Eigen::Matrix<F, Eigen::Dynamic, Eigen::Dynamic> M(3, 3);
  M << m00, m01, m02, m10, m11, m12, m20, m21, m22;
  return M;
}

namespace {
  typedef Eigen::MatrixXcd CM;
  typedef Eigen::VectorXcd CV;
}

double TakeReal(dcomplex x);
double TakeAbs(dcomplex x);
std::complex<double> cnorm(const CV& v);
void complex_normalize(CV& v);
void col_cnormalize(CM& c);
void matrix_inv_sqrt(const CM& s, CM* s_inv_sqrt);
void SortEigs(Eigen::VectorXcd& eigs, Eigen::MatrixXcd& eigvecs,
	      double (*to_real)(dcomplex), bool reverse=false);
void generalizedComplexEigenSolve(const CM& f, const CM& s, CM* c, CV* eig);
void CanonicalMatrix(const CM& S, double eps, CM* res);
void CEigenSolveCanonical(const CM& f, const CM& s, double eps, CM* c, CV* eig);
void CEigenSolveCanonicalNum(const CM& F, const CM& S, int num0,
			     CM* c, CV* eig);

class SymGenComplexEigenSolver {
private:
  CM eigenvectors_;
  CV eigenvalues_;
  
public:
  SymGenComplexEigenSolver();
  SymGenComplexEigenSolver(const CM& _A, const CM& _B);
  void compute(const CM& _A, const CM& B);
  CV eigenvalues() const;
  CM eigenvectors() const;
};




#endif
