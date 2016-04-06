#ifndef R1GTOINT_H
#define R1GTOINT_H

#include <vector>
#include <map>
#include <Eigen/Core>
#include "typedef.hpp"

// TODO
// CalcVecの引数をR1GTOsから、std::vector<R1GTO>に変更。
// それぞれのメッソドをAccessor等に分けて書く。


namespace l2func {

  struct R1GTO {
    R1GTO(dcomplex _c, int _n, dcomplex _z);
    dcomplex c;
    int n;
    dcomplex z;
    bool operator=(const R1GTO& o) const { return this==&o; }
    bool operator!=(const R1GTO& o) const { return this!=&o; }
  };
  struct R1STO {
    R1STO(dcomplex _c, int _n, dcomplex _z);
    dcomplex c;
    int n;
    dcomplex z;  
    bool operator=( const R1STO& o) const { return this==&o; }
    bool operator!=(const R1STO& o) const { return this!=&o; }  
    bool operator=( const R1STO& o) { return this==&o; }
    bool operator!=(const R1STO& o) { return this!=&o; }  
  };

  std::ostream& operator<<(std::ostream& out, const R1GTO& basis);
  std::ostream& operator<<(std::ostream& out, const R1STO& basis);

  typedef std::map<std::string, Eigen::MatrixXcd> MatMap;
  typedef std::vector<R1GTO>::const_iterator CIt;
  typedef std::map<std::string, Eigen::VectorXcd> VecMap;
  class R1GTOs;
  class R1STOs;  

  void CalcGTOInt(int maxn, dcomplex a, dcomplex* res);
  
  class R1GTOs {
    struct Prim {
      int n;
      dcomplex z;
      Prim(int _n, dcomplex _z) : n(_n), z(_z) {}
    };
    struct Contraction {
      std::vector<Prim> prim;
      Eigen::MatrixXcd  coef;
      int offset;
      int size_prim() const { return coef.cols(); }
      int size_basis() const { return coef.rows(); }
    };
    typedef std::vector<Prim>::iterator ItPrim;
    typedef std::vector<Prim>::const_iterator cItPrim;
    typedef std::vector<Contraction>::iterator ItCont;
    typedef std::vector<Contraction>::const_iterator cItCont;
  public:
    bool normalized_q_;       // basis is normalized or not
    bool calc_mat_q_;         // matrix is calculated with current setting
    bool calc_vec_q_;         // vector is calculated with current setting
    std::vector<Contraction> conts_;
    int L_;      // angular quantum number
    MatMap mat_; // store calculation results
    VecMap vec_; // store calculation results

  public:

    // ---- Constructors ----
    R1GTOs(int _L);
    void swap(R1GTOs& o);

    // ---- Utils ----
    int max_n() const;

    // ---- Accessors -----
    int L() const { return L_; }
    int size_basis() const;
    int size_prim() const;
    const Prim& prim(int i) const;    
    Prim& prim(int i);
    dcomplex z_prim(int i) { return this->prim(i).z; }
    int      n_prim(int i) { return this->prim(i).n; }
    Eigen::MatrixXcd& mat(std::string label);
    Eigen::VectorXcd& vec(std::string label);
    bool normalized_q() const { return normalized_q_; }
    //    void Add(dcomplex c, int n, dcomplex zeta);
    void Add(int n, dcomplex zeta);
    void Add(int n, const Eigen::VectorXcd& zs);    
    void Add(int n, const Eigen::VectorXcd& zs, const Eigen::MatrixXcd& coef);
    void Set(int n, const Eigen::VectorXcd& zs);

    // ---- Calculation ----
    void Normalize();
    void CalcMat();
    void CalcVec(const R1GTOs&);
    void CalcVec(const R1STOs&);
    void AtR(const Eigen::VectorXcd&,
	     const Eigen::VectorXcd&, Eigen::VectorXcd*);
    Eigen::VectorXcd* AtR(const Eigen::VectorXcd&,
			  const Eigen::VectorXcd&);
    
  };
  class R1STOs {
  public:
    bool normalized_q_;
    std::vector<R1STO> stos_;
  public:
    R1STOs(): normalized_q_(false) {}
    int size_basis() const { return stos_.size();}
    const R1STO& basis(int i) const { return stos_[i]; }
    bool normalized_q() const { return normalized_q_; }
    void Add(dcomplex c, int n, dcomplex zeta);
    void Add(int n, dcomplex zeta);
  };

  std::ostream& operator<<(std::ostream& out, const R1GTOs& basis);
  std::ostream& operator<<(std::ostream& out, const R1STOs& basis);
  
}

#endif
