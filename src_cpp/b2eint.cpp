#include "b2eint.hpp"

namespace l2func {

  // ==== Interface ====
  IB2EInt::~IB2EInt() {}


  // ==== Mem ====
  // ---- Constructors ----
  B2EIntMem::B2EIntMem(int n):
    capacity_(n), size_(0), idx_(0),
    ibs(n), jbs(n), kbs(n), lbs(n),
    is(n), js(n), ks(n), ls(n),
    ts(n) , vs(n) {}
  B2EIntMem::~B2EIntMem() {}

  // ---- Main ----
  bool B2EIntMem::Get(int *ib, int *jb, int *kb, int *lb,
		      int *i, int *j, int *k, int *l,
		      int *type, dcomplex *val) {
    if(this->idx_ >= this->size_ ) {
      return false;
    }
    *ib = this->ibs[this->idx_];
    *jb =  this->jbs[this->idx_];
    *kb =  this->kbs[this->idx_];
    *lb =  this->lbs[this->idx_];
    *i  = this->is[this->idx_];
    *j  = this->js[this->idx_];
    *k  = this->ks[this->idx_];
    *l  = this->ls[this->idx_];
    *type = this->ts[this->idx_];
    *val  = this->vs[this->idx_];
    this->idx_++;
    return true;
  }
  bool B2EIntMem::Set(int ib, int jb, int kb, int lb,
		      int i, int j, int k, int l,
		      dcomplex val) {
    this->ibs[this->size_] = ib;
    this->jbs[this->size_] = jb;
    this->kbs[this->size_] = kb;
    this->lbs[this->size_] = lb;
    this->is[this->size_]  = i;
    this->js[this->size_]  = j;
    this->ks[this->size_]  = k;
    this->ls[this->size_]  = l;
    this->ts[this->size_]  = 0;
    this->vs[this->size_]  = val;
    this->size_++;
    return true;
  }
  void B2EIntMem::Reset() {
    idx_ = 0;
  }
  

}