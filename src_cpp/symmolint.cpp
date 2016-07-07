#include <iostream>
#include <numeric>
#include "angmoment.hpp"
#include "mol_func.hpp"

#include "symmolint.hpp"

namespace l2func {

  using namespace std;
  using namespace Eigen;
  typedef vector<SubSymGTOs>::iterator SubIt;
  typedef vector<Reduction>::iterator RdsIt;
  typedef vector<SubSymGTOs>::const_iterator cSubIt;
  typedef vector<Reduction>::const_iterator cRdsIt;
  typedef MultArray<dcomplex, 1> A1dc;
  typedef MultArray<dcomplex, 2> A2dc;
  typedef MultArray<dcomplex, 3> A3dc;
  typedef MultArray<dcomplex, 4> A4dc;

  // ==== coef R ====
  // -- more efficient code can be used! --
  void calc_R_coef(dcomplex zetaP,
		   dcomplex wPx, dcomplex wPy, dcomplex wPz,		   
		   const MatrixXcd& xyzq_kat, A2dc& Fjs_kat,
		   dcomplex mult_coef,
		   int mx, int my, int mz, int mat, A4dc& res) {
    
    res.SetRange(0, mx, 0, my, 0, mz, 0, mat);
    for(int nx = 0; nx <= mx; nx++)
      for(int ny = 0; ny <= my; ny++)
	for(int nz = 0; nz <= mz; nz++)
	  for(int kat = 0; kat < mat; kat++) {	    
	    dcomplex v = coef_R(zetaP,
				wPx, wPy, wPz,
				xyzq_kat(0, kat),
				xyzq_kat(1, kat),
				xyzq_kat(2, kat),
				nx, ny, nz, 0, &Fjs_kat(kat, 0));
	    res(nx, ny, nz, kat) = mult_coef * v;
	  }

  }
  void calc_R_coef_eri(dcomplex zarg,
		       dcomplex wPx,  dcomplex wPy,  dcomplex wPz,
		       dcomplex wPpx, dcomplex wPpy, dcomplex wPpz,
		       int max_n, dcomplex *Fjs, dcomplex mult_coef,
		       A3dc& res) {
    res.SetRange(0, max_n, 0, max_n, 0, max_n);
    for(int nx = 0; nx <= max_n; nx++)
      for(int ny = 0; ny <= max_n; ny++)
	for(int nz = 0; nz <= max_n; nz++) {
	  if(nx + ny + nz <= max_n) {
	    dcomplex v = coef_R(zarg,
			      wPx, wPy, wPz,
			      wPpx, wPpy, wPpz,
			      nx, ny, nz, 0, Fjs);
	    res(nx, ny, nz) = mult_coef * v;
	  }
	}
  }

  // ==== Reduction Sets ====
  string Reduction::str() const {
    ostringstream oss;
    oss << "==== RedcutionSets ====" << endl;
    oss << "irrep : " << irrep << endl;
    oss << "coef_iat_ipn: " << endl << coef_iat_ipn << endl;
    oss << "coef_iz: " << endl <<  coef_iz << endl;
    oss << "offset:  " << offset << endl;
    return oss.str();
  }
  void Reduction::Display() const {
    cout << this->str() ;
  }
  
  // ==== Sub ====
  SubSymGTOs::SubSymGTOs(pSymmetryGroup g) {
    sym_group = g;
    zeta_iz = VectorXcd::Zero(0);
    setupq = false;
  }
  void SubSymGTOs::SetUp() {

    // ---- check values ----
    if(this->size_at() * this->size_pn() == 0) {
      string msg; SUB_LOCATION(msg);
      msg += ": primitive GTO is not set.";
      throw runtime_error(msg);
    }
    if(this->zeta_iz.size() == 0) {
      string msg; SUB_LOCATION(msg);
      msg += ": zeta is not set.";
      throw runtime_error(msg);
    }
    if(this->rds.size() == 0) {
      string msg; SUB_LOCATION(msg);
      msg += ": ReductionSet is not set.";
      throw runtime_error(msg);
    }
    for(RdsIt it = rds.begin(); it != rds.end(); ++it) {
      if(it->size_at() != this->size_at() |
	 it->size_pn() != this->size_pn()) {
	string msg; SUB_LOCATION(msg);
	ostringstream oss; oss << msg;
	oss << ": size mismatch.\n"
	    << "size_at (ReductionSets, SubSymGTOs) = "
	    << it->size_at() << this->size_at()
	    << "size_pn (ReductionSets, SubSymGTOs) = "
	    << it->size_pn() << this->size_pn() << endl;
	throw runtime_error(oss.str());
      }
    }

    // ---- compute internal values ----
    ip_iat_ipn = MatrixXi::Zero(this->size_at(), this->size_pn());
    int ip(0);
    for(int iat = 0; iat < this->size_at(); ++iat)
      for(int ipn = 0; ipn <  this->size_pn(); ++ipn) {
	ip_iat_ipn(iat, ipn) = ip;
	ip++;
      }
    
    maxn = 0;
    for(int ipn = 0; ipn < size_pn(); ipn++) {
      if(maxn < nx(ipn) + ny(ipn) + nz(ipn))
	maxn = nx(ipn) + ny(ipn) + nz(ipn);
    }
    for(RdsIt it = rds.begin(); it != rds.end(); ++it) {
      it->set_zs_size(zeta_iz.size());
    }

    // -- build PrimGTO set.
    int nat(this->size_at());
    int npn(this->size_pn());
    vector<PrimGTO> gtos(nat * npn);
    for(int iat = 0; iat < nat; iat++) 
      for(int ipn = 0; ipn < npn; ipn++)  {
	int ip = this->ip_iat_ipn(iat, ipn);
	gtos[ip] = PrimGTO(this->nx(ipn),
			   this->ny(ipn),
			   this->nz(ipn),
			   this->x(iat),
			   this->y(iat),
			   this->z(iat));
      }
    // -- compute symmetry operation and primitive GTO relations --
    sym_group->CalcSymMatrix(gtos, this->ip_jg_kp, this->sign_ip_jg_kp);

    // -- set flag --
    setupq = true;
  }
  void SubSymGTOs::AddXyz(Vector3cd xyz) {

    setupq = false;
    this->x_iat.push_back(xyz[0]);
    this->y_iat.push_back(xyz[1]);
    this->z_iat.push_back(xyz[2]);

  }
  void SubSymGTOs::AddNs(Vector3i ns) {
    setupq = false;
    this->nx_ipn.push_back(ns[0]);
    this->ny_ipn.push_back(ns[1]);
    this->nz_ipn.push_back(ns[2]);
  }
  void SubSymGTOs::AddZeta(const VectorXcd& zs) {
    setupq = false;
    VectorXcd res(zeta_iz.size() + zs.size());
    for(int i = 0; i < zeta_iz.size(); i++)
      res(i) = zeta_iz(i);
    for(int i = 0; i < zs.size(); i++)
      res(i+zeta_iz.size()) = zs(i);
    zeta_iz.swap(res);
  }
  void SubSymGTOs::AddRds(const Reduction& _rds) {
    setupq = false;
    rds.push_back(_rds);
  }
  string SubSymGTOs::str() const {
    ostringstream oss;
    oss << "==== SubSymGTOs ====" << endl;
    //oss << "sym : " << sym_group->str() << endl;
    oss << "xyz : " << endl;
    for(int iat = 0; iat < size_at(); iat++)
      oss <<  x(iat) << y(iat) << z(iat) << endl;
    oss << "ns  : " << endl;
    for(int ipn = 0; ipn < size_pn(); ipn++) 
      oss <<  nx(ipn) << ny(ipn) << nz(ipn) << endl;
    oss << "zeta: " << endl <<  zeta_iz << endl;
    oss << "maxn: " << maxn << endl;
    for(cRdsIt it = rds.begin(); it != rds.end(); ++it)
      oss << it->str();
    oss << "ip_iat_ipn: " << endl << ip_iat_ipn << endl;
    oss << "ip_jg_kp: " << endl << ip_jg_kp << endl;
    oss << "sign_ip_jg_kp: " << endl << sign_ip_jg_kp << endl;
    return oss.str();
  }
  void SubSymGTOs::Display() const {
    cout << this->str();
  }
  SubSymGTOs Sub_s(pSymmetryGroup sym, Irrep irrep, Vector3cd xyz, VectorXcd zs) {

    MatrixXcd cs = MatrixXcd::Ones(1,1);
    Reduction rds(irrep, cs);
    MatrixXi symmat = MatrixXi::Ones(1, 1);
    MatrixXi signmat= MatrixXi::Ones(1, 1);

    SubSymGTOs sub(sym);
    sub.AddXyz(xyz);
    sub.AddNs(Vector3i(0, 0, 0));
    sub.AddZeta(zs);
    sub.AddRds(rds);
    sub.SetUp();
    return sub;
  }
  SubSymGTOs Sub_pz(pSymmetryGroup sym, Irrep irrep, Vector3cd xyz, VectorXcd zs) {

    MatrixXcd cs = MatrixXcd::Ones(1,1);
    Reduction rds(irrep, cs);

    SubSymGTOs sub(sym);
    sub.AddXyz(xyz);
    sub.AddNs(Vector3i(0, 0, 1));
    sub.AddRds(rds);
    sub.AddZeta(zs);
    //sub.SetSym(MatrixXi::Ones(1, 1), MatrixXi::Ones(1, 1));
    sub.SetUp();
    return sub;    
  }
  SubSymGTOs Sub_TwoSGTO(pSymmetryGroup sym, Irrep irrep,
			 Vector3cd xyz, VectorXcd zs) {

    sym->CheckIrrep(irrep);
    SubSymGTOs sub(sym);

    if(sym->name() == "Cs") {
      dcomplex x = xyz[0];
      dcomplex y = xyz[1];
      dcomplex z = xyz[2];
      sub.AddXyz(xyz);
      sub.AddXyz(Vector3cd(x, y, -z));
      sub.AddNs(Vector3i(0, 0, 0));
      sub.AddZeta(zs);

      MatrixXcd cs(2, 1);
      if(irrep == sym->GetIrrep("A'")) {
	cs << 1.0, 1.0;
      } else if (irrep == sym->GetIrrep("A''")){
	cs << 1.0, -1.0;
      }
      Reduction rds(irrep, cs);
      sub.AddRds(rds);
      
      sub.SetUp();
      return sub;

    } else {
      string msg; SUB_LOCATION(msg);
      msg += "only Cs symmetry is implemented now";
      throw runtime_error(msg);
    }
  }
  SubSymGTOs Sub_mono(pSymmetryGroup sym, Irrep irrep,
		      Vector3cd xyz, Vector3i ns, VectorXcd zs) {

    sym->CheckIrrep(irrep);
    SubSymGTOs sub(sym);

    sub.AddXyz(xyz);
    sub.AddNs(ns);
    sub.AddZeta(zs);
    sub.AddRds(Reduction(irrep, MatrixXcd::Ones(1, 1)));
    sub.SetUp();
    return sub;
  }
  
  // ==== SymGTOs ====
  // ---- Constructors ----
  SymGTOs::SymGTOs(pSymmetryGroup _sym_group):
    sym_group(_sym_group), setupq(false)  {
    xyzq_iat = MatrixXcd::Zero(4, 0);
  }

  // ---- Accessors ----
  int SymGTOs::size_atom() const {return xyzq_iat.cols(); }
  int SymGTOs::size_basis() const {
    int cumsum(0);
    for(cSubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      cumsum += isub->size_zeta() * isub->rds.size();
    }
    return cumsum;
  }
  int SymGTOs::size_basis_isym(Irrep isym) const {

    int cumsum(0);
    for(cSubIt isub = subs.begin(); isub != subs.end(); ++isub) {

      for(cRdsIt irds = isub->rds.begin(); irds != isub->rds.end(); 
	  ++irds) {

	if(irds->irrep == isym) 
	  cumsum += isub->size_zeta();
      }
    }
    return cumsum;

  }
  string SymGTOs::str() const {
    ostringstream oss;
    oss << "==== SymGTOs ====" << endl;
    oss << "Set Up?" << (setupq ? "Yes" : "No") << endl;
    oss << sym_group->str();
    for(cSubIt it = subs.begin(); it != subs.end(); ++it) 
      oss << it->str();
    oss << "xyzq:" << endl;
    oss << xyzq_iat << endl;
    return oss.str();
  }

  // ---- Add ----
  void SymGTOs::SetAtoms(MatrixXcd _xyzq_iat) {

    if(_xyzq_iat.rows() != 4) {
      string msg; SUB_LOCATION(msg);
      msg += "xyzq_iat.rows() must be 4 ";
      throw runtime_error(msg);
    }

    xyzq_iat = _xyzq_iat;

  }
  void SymGTOs::AddAtom(Eigen::Vector3cd _xyz, dcomplex q) {

    int num_atom = xyzq_iat.cols();
    MatrixXcd res(4, num_atom + 1);

    for(int i = 0; i < num_atom; i++) {
      for(int j = 0; j < 4; j++)
	res(j, i) = xyzq_iat(j, i);
    }

    res(0, num_atom) = _xyz(0);
    res(1, num_atom) = _xyz(1);
    res(2, num_atom) = _xyz(2);
    res(3, num_atom) = q;

    xyzq_iat.swap(res);

  }
  void SymGTOs::AddSub(SubSymGTOs sub) {

    subs.push_back(sub);

  }
  void SymGTOs::SetComplexConj(SymGTOs& o) {
    if(!o.setupq) {
      string msg; SUB_LOCATION(msg); 
      msg += ": o is not setup";
      throw runtime_error(msg);
    }

    this->sym_group = o.sym_group;
    this->xyzq_iat = o.xyzq_iat;
    this->subs = o.subs;
    for(SubIt isub = this->subs.begin(); isub != this->subs.end(); ++isub) {
      isub->zeta_iz = isub->zeta_iz.conjugate();
      for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds) {
	irds->coef_iat_ipn = irds->coef_iat_ipn.conjugate();
	irds->coef_iz = irds->coef_iz.conjugate();
      }
      isub->SetUp();
    }
    this->SetUp();
  }

  // ---- SetUp ----
  void SymGTOs::SetUp() {
    
    for(SubIt it = subs.begin(); it != subs.end(); ++it) {

      // -- setup sub --
      if(it->setupq == false)
	it->SetUp();

      // -- check each sub --
      if(it->ip_jg_kp.rows() != sym_group->order()) {
	string msg; SUB_LOCATION(msg); 
	msg += ": size of symmetry transformation matrix and num_class of symmetry group is not matched.";
	cout << "ip_jg_kp:" << endl;
	cout << it->ip_jg_kp << endl;
	throw runtime_error(msg);
      }
      
      
      // -- init offset --
      for(RdsIt irds = it->begin_rds(); irds != it->end_rds(); ++irds) {
	sym_group->CheckIrrep(irds->irrep);
	irds->offset = 0;
      }
    }

    this->SetOffset();
    this->Normalize();
    setupq = true;
  }
  void SymGTOs::SetOffset() {
    
    vector<int> num_irrep(10, 0);
    
    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      for(RdsIt irds = isub->rds.begin(), end = isub->rds.end();
	  irds != end; ++irds) {
	irds->offset = num_irrep[irds->irrep];
	num_irrep[irds->irrep] += isub->size_zeta();
      }
    }

  }
  void SymGTOs::Normalize() {

    /*
  A3dc dxmap(100), dymap(100), dzmap(100);
    for(SubIt isub = subs.begin(), end = subs.end(); isub != end; ++isub) {
      for(int iz = 0; iz < isub->size_zeta(); iz++) {

	dcomplex zetai = isub->zeta_iz[iz];
	dcomplex zetaP = zetai + zetai;

	int mi = isub->maxn;
	calc_d_coef(mi,mi,0, zetaP, 0.0, 0.0, 0.0, dxmap);
	calc_d_coef(mi,mi,0, zetaP, 0.0, 0.0, 0.0, dymap);
	calc_d_coef(mi,mi,0, zetaP, 0.0, 0.0, 0.0, dzmap);

	int nipn(isub->size_pn());
	for(int ipn = 0; ipn < nipn; ipn++) {
	  int nxi, nyi, nzi;
	  nxi = isub->nx(ipn);
	  nyi = isub->ny(ipn);
	  nzi = isub->nz(ipn);
	  dcomplex dx00, dy00, dz00;
	  dx00 = dxmap(nxi, nxi ,0);
	  dy00 = dymap(nyi, nyi ,0);
	  dz00 = dzmap(nzi, nzi ,0);
	  dcomplex s_ele = dx00 * dy00 * dz00;
	  dcomplex ce = pow(M_PI/zetaP, 1.5);	  
	  for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds) {
	    irds->coef_iz(iz) = 1.0/sqrt(ce*s_ele);
	  }
	}
      }
    }
    */

    A3dc dxmap(100), dymap(100), dzmap(100);
    // >>> Irrep Adapted GTOs >>>
    for(SubIt isub = subs.begin(), end = subs.end(); isub != end; ++isub) {
      for(int iz = 0; iz < isub->size_zeta(); iz++) {
      for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end();
	    ++irds) {
	  dcomplex zetai = isub->zeta_iz[iz];
	  dcomplex zetaP = zetai + zetai;
	  int niat(isub->size_at()); int nipn(isub->size_pn());
	  dcomplex norm2(0.0);

	  // >>> Primitive GTOs >>>
	  for(int iat = 0; iat < niat; iat++) {
	    for(int jat = 0; jat < niat; jat++) {
	      dcomplex xi, xj, yi, yj, zi, zj, wPx, wPy, wPz;
	      xi = isub->x(iat); xj = isub->x(jat);
	      yi = isub->y(iat); yj = isub->y(jat);
	      zi = isub->z(iat); zj = isub->z(jat);
	      wPx = (zetai*xi+zetai*xj)/zetaP;
	      wPy = (zetai*yi+zetai*yj)/zetaP;
	      wPz = (zetai*zi+zetai*zj)/zetaP;
	      dcomplex d2 = pow(xi-xj,2) + pow(yi-yj,2) + pow(zi-zj,2);	
	      dcomplex eAB = exp(-zetai*zetai/zetaP*d2);
	      dcomplex ce = eAB * pow(M_PI/zetaP, 1.5);
	      int mi = isub->maxn;
	      calc_d_coef(mi,mi,0, zetaP,wPx,xi,xj,dxmap);
	      calc_d_coef(mi,mi,0, zetaP,wPy,yi,yj,dymap);
	      calc_d_coef(mi,mi,0, zetaP,wPz,zi,zj,dzmap);
	      
	      for(int ipn = 0; ipn < nipn; ipn++) {
		for(int jpn = 0; jpn < nipn; jpn++) {		    
		  int nxi, nxj, nyi, nyj, nzi, nzj;
		  nxi = isub->nx(ipn); nxj = isub->nx(jpn);
		  nyi = isub->ny(ipn); nyj = isub->ny(jpn);
		  nzi = isub->nz(ipn); nzj = isub->nz(jpn);
		  dcomplex dx00, dy00, dz00;
		  dx00 = dxmap(nxi, nxj ,0);
		  dy00 = dymap(nyi, nyj ,0);
		  dz00 = dzmap(nzi, nzj ,0);
		  dcomplex s_ele = dx00 * dy00 * dz00;
		  norm2 += ce * s_ele *
		    irds->coef_iat_ipn(iat, ipn) *
		    irds->coef_iat_ipn(jat, jpn);
		}
	      }
	    }
	  }

	  if(abs(norm2) < pow(10.0, -14.0)) {
	    string msg; SUB_LOCATION(msg);
	    ostringstream oss; oss << msg << ": " << endl;
	    oss << "norm is too small" << endl;
	    oss << "isub:"  << distance(subs.begin(), isub) << endl;
	    oss << "iz  : " << iz << endl;
	    oss << "zeta: " << zetai << endl;
	    oss << "irds:" << distance(isub->rds.begin(), irds) << endl;
	    oss << "nipn: " << nipn << endl;
	    oss << "niat: " << niat << endl;
	    oss << "norm2: " << norm2 << endl;	    
	    throw runtime_error(oss.str());
	  }

	  // <<< Primitive GTOs <<<
	  irds->coef_iz(iz) = 1.0/sqrt(norm2);
	}
      }
    }
    // <<< Irrep Adapted GTOs <<<

  }

  // ---- utils ----
  // -- not used now --
  int SymGTOs::max_n() const {
    int max_n(0);
    for(cSubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      for(int ipn = 0; ipn < isub->size_pn(); ipn++) {
	int nx(isub->nx(ipn));
	int ny(isub->ny(ipn));
	int nz(isub->nz(ipn));
	if(max_n < nx)
	  max_n = nx;
	if(max_n < ny)
	  max_n = ny;
	if(max_n < nz)
	  max_n = nz;
      }
    }    
    return max_n;
  }
  
  // ---- CalcMat ----
  struct PrimBasis {
    A4dc s, t, v, x, y, z;
    PrimBasis(int num):
      s(num), t(num), v(num), x(num), y(num), z(num) {}
    void SetRange(int niat, int nipn, int njat, int njpn) {
      s.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
      t.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
      v.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
      x.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
      y.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
      z.SetRange(0, niat, 0, nipn, 0, njat, 0, njpn);
    }
  };
  dcomplex calc_tele(SubIt isub, SubIt jsub, dcomplex zetaj, 
		     int ipn, int jpn,
		     A3dc& dxmap, A3dc& dymap, A3dc& dzmap) {
    int nxi, nxj, nyi, nyj, nzi, nzj;
    nxi = isub->nx(ipn); nxj = jsub->nx(jpn);
    nyi = isub->ny(ipn); nyj = jsub->ny(jpn);
    nzi = isub->nz(ipn); nzj = jsub->nz(jpn);
    dcomplex dx00, dy00, dz00, dx02, dy02, dz02;
    dx00 = dxmap(nxi, nxj ,0);
    dy00 = dymap(nyi, nyj ,0);
    dz00 = dzmap(nzi, nzj ,0);
    dx02 = dxmap(nxi, nxj+2 ,0);
    dy02 = dymap(nyi, nyj+2 ,0);
    dz02 = dzmap(nzi, nzj+2 ,0);

    dcomplex t_ele(0.0);		    
    t_ele += -2.0*(2*nxj+2*nyj+2*nzj+3)*zetaj*dx00*dy00*dz00;
    t_ele += 4.0*zetaj*zetaj*(dx02*dy00*dz00+dx00*dy02*dz00+dx00*dy00*dz02);
    if(nxj > 1) {
      dcomplex dx = dxmap(nxi, nxj-2, 0);
      t_ele += 1.0*nxj*(nxj-1) * dx * dy00 * dz00;
    }
    if(nyj > 1) {
      dcomplex dy = dymap(nyi, nyj-2, 0);
      t_ele += 1.0*nyj*(nyj-1) * dx00 * dy * dz00;
    }
    if(nzj > 1) {
      dcomplex dz = dzmap(nzi, nzj-2, 0);
      t_ele += 1.0*nzj*(nzj-1) * dx00 * dy00 * dz;
    }
    return t_ele;

  }
  dcomplex calc_vele(const SymGTOs& gtos, SubIt isub, SubIt jsub, int ipn, int jpn,
		     A3dc& dxmap, A3dc& dymap, A3dc& dzmap, A4dc& rmap) {

    int nxi, nxj, nyi, nyj, nzi, nzj;
    nxi = isub->nx(ipn); nxj = jsub->nx(jpn);
    nyi = isub->ny(ipn); nyj = jsub->ny(jpn);
    nzi = isub->nz(ipn); nzj = jsub->nz(jpn);

    dcomplex v_ele(0.0);
    for(int nx = 0; nx <= nxi + nxj; nx++)
      for(int ny = 0; ny <= nyi + nyj; ny++)
	for(int nz = 0; nz <= nzi + nzj; nz++)
	  for(int kat = 0; kat < gtos.size_atom(); kat++) {
	    /*
	    cout << "symmolint.cpp: " << nx << ny << nz << 
	      dxmap(nxi, nxj, nx) <<
	      dymap(nyi, nyj, ny) <<
	      dzmap(nzi, nzj, nz) <<
	      rmap(nx, ny, nz, kat) << endl;
	      */
	    v_ele += (gtos.q_at(kat) *
		      dxmap(nxi, nxj, nx) *
		      dymap(nyi, nyj, ny) *
		      dzmap(nzi, nzj, nz) *
		      rmap(nx, ny, nz, kat));
		  }
    return v_ele;
  }

  void CalcPrim(const SymGTOs gtos, SubIt isub, SubIt jsub, int iz, int jz,
		A3dc& dxmap, A3dc& dymap, A3dc& dzmap, A4dc& rmap, A2dc& Fjs_iat,
		PrimBasis& prim, bool calc_coulomb) {
    dcomplex zetai, zetaj;
    zetai = isub->zeta_iz[iz]; zetaj = jsub->zeta_iz[jz];
    dcomplex zetaP = zetai + zetaj;
    int niat(isub->size_at()); int njat(jsub->size_at());
    int nipn(isub->size_pn()); int njpn(jsub->size_pn());

    prim.SetRange(niat, nipn, njat, njpn);
    Fjs_iat.SetRange(0, gtos.size_atom(), 0, isub->maxn+jsub->maxn);

    for(int iat = 0; iat < niat; iat++) {
      // int jat0 = (isub == jsub && iz == jz ? iat : 0);
      for(int jat = 0; jat < njat; jat++) { 
	dcomplex xi, xj, yi, yj, zi, zj, wPx, wPy, wPz;
	xi = isub->x(iat); yi = isub->y(iat); zi = isub->z(iat);
	xj = jsub->x(jat); yj = jsub->y(jat); zj = jsub->z(jat);
	
	wPx = (zetai*xi+zetaj*xj)/zetaP;
	wPy = (zetai*yi+zetaj*yj)/zetaP;
	wPz = (zetai*zi+zetaj*zj)/zetaP;
	dcomplex d2 = dist2(xi-xj, yi-yj, zi-zj);
	dcomplex eAB = exp(-zetai*zetaj/zetaP*d2);
	dcomplex ce = eAB * pow(M_PI/zetaP, 1.5);
	int mi = isub->maxn; int mj = jsub->maxn;
	if(calc_coulomb) {
	  calc_d_coef(mi,mj+2,mi+mj,zetaP,wPx,xi,xj,dxmap);
	  calc_d_coef(mi,mj+2,mi+mj,zetaP,wPy,yi,yj,dymap);
	  calc_d_coef(mi,mj+2,mi+mj,zetaP,wPz,zi,zj,dzmap);
	  for(int kat = 0; kat < gtos.size_atom(); kat++) {
	    dcomplex d2p = dist2(wPx-gtos.x_at(kat), wPy-gtos.y_at(kat),
				 wPz-gtos.z_at(kat));
	    dcomplex arg = zetaP * d2p;
	    double delta(0.0000000000001);
	    if(real(arg)+delta > 0.0) {
	      IncompleteGamma(isub->maxn+jsub->maxn, arg, &Fjs_iat(kat, 0));
	    } else {
	      ExpIncompleteGamma(isub->maxn+jsub->maxn, -arg, &Fjs_iat(kat, 0));
	    }
	  }
	} else {
	  calc_d_coef(mi,mj+2,0,zetaP,wPx,xi,xj,dxmap);
	  calc_d_coef(mi,mj+2,0,zetaP,wPy,yi,yj,dymap);
	  calc_d_coef(mi,mj+2,0,zetaP,wPz,zi,zj,dzmap);	  
	}
	
	for(int ipn = 0; ipn < nipn; ipn++) {
	  for(int jpn = 0; jpn < njpn; jpn++) {
	    int nxi, nxj, nyi, nyj, nzi, nzj;
	    nxi = isub->nx(ipn); nxj = jsub->nx(jpn);
	    nyi = isub->ny(ipn); nyj = jsub->ny(jpn);
	    nzi = isub->nz(ipn); nzj = jsub->nz(jpn);

	    dcomplex s_ele = dxmap(nxi,nxj,0) * dymap(nyi,nyj,0) * dzmap(nzi,nzj,0);
	    dcomplex x_ele = dymap(nyi,nyj,0)*dzmap(nzi,nzj,0)*
	      (dxmap(nxi,nxj+1,0)+xj*dxmap(nxi,nxj,0));
	    dcomplex y_ele = dzmap(nzi,nzj,0)*dxmap(nxi,nxj,0)*
	      (dymap(nyi,nyj+1,0)+yj*dymap(nyi,nyj,0));
	    dcomplex z_ele = dxmap(nxi,nxj,0)*dymap(nyi,nyj,0)*
	      (dzmap(nzi,nzj+1,0)+zj*dzmap(nzi,nzj,0));
	    dcomplex t_ele = calc_tele(isub, jsub, zetaj, ipn, jpn,
				       dxmap, dymap, dzmap);	    
	    prim.s(iat, ipn, jat, jpn) =  ce * s_ele;
	    prim.t(iat, ipn, jat, jpn) =  -0.5* ce * t_ele;
	    prim.x(iat, ipn, jat, jpn) =  ce*x_ele;
	    prim.y(iat, ipn, jat, jpn) =  ce*y_ele;
	    prim.z(iat, ipn, jat, jpn) =  ce*z_ele;
	    rmap.SetRange(0, nxi+nxj, 0, nyi+nyj, 0, nzi+nzj, 0, gtos.size_atom());

	    if(calc_coulomb) {	      
	      for(int kat = 0; kat < gtos.size_atom(); kat++) {
		dcomplex d2p = dist2(wPx-gtos.x_at(kat), wPy-gtos.y_at(kat),
				     wPz-gtos.z_at(kat));
		dcomplex arg = zetaP * d2p;
		double delta(0.0000000000001);
		dcomplex wKx = gtos.x_at(kat);
		dcomplex wKy = gtos.y_at(kat);
		dcomplex wKz = gtos.z_at(kat);
		if(real(arg)+delta > 0.0) {
		  for(int nx = 0; nx <= nxi + nxj; nx++)
		    for(int ny = 0; ny <= nyi + nyj; ny++)
		      for(int nz = 0; nz <= nzi + nzj; nz++)
			rmap(nx, ny, nz, kat) =
			  -2.0*M_PI/zetaP*eAB * coef_R(zetaP, wPx, wPy, wPz,
						       wKx, wKy, wKz, nx, ny, nz,
						       0, &Fjs_iat(kat, 0));
		} else {
		  dcomplex arg_other = -zetai * zetaj / zetaP * d2 - arg;
		  for(int nx = 0; nx <= nxi + nxj; nx++)
		    for(int ny = 0; ny <= nyi + nyj; ny++)
		      for(int nz = 0; nz <= nzi + nzj; nz++) {
			rmap(nx, ny, nz, kat) =
			  -2.0*M_PI/zetaP*exp(arg_other) *
			  coef_R(zetaP, wPx, wPy, wPz,
				 wKx, wKy, wKz, nx, ny, nz,
				 0, &Fjs_iat(kat, 0)); 
			/*
			cout << "sym: " << nx << ny << nz <<
			  rmap(nx, ny, nz, kat)<<endl <<
			  coef_R(zetaP, wPx, wPy, wPz,
				 wKx, wKy, wKz, nx, ny, nz,
				 0, &Fjs_iat(kat, 0)) << endl;
				 */
		      }
		}
		
	      }
	      prim.v(iat, ipn, jat, jpn) =  
		    calc_vele(gtos, isub, jsub, ipn, jpn, dxmap, dymap, dzmap, rmap);
	    }
	  }}}}
  }
  void CalcTrans(SubIt isub, SubIt jsub, int iz, int jz,
		 PrimBasis& prim, BMatSet& mat_map) {

    int niat(isub->size_at()); int njat(jsub->size_at());
    int nipn(isub->size_pn()); int njpn(jsub->size_pn());
    //    cout << "tras before set : " << niat << nipn << njat << njpn << endl;

    for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds) {
      for(RdsIt jrds = jsub->rds.begin(); jrds != jsub->rds.end();++jrds) {
	dcomplex cumsum_s(0.0), cumsum_t(0.0), cumsum_v(0.0);
	dcomplex cumsum_x(0.0), cumsum_y(0.0), cumsum_z(0.0);
	for(int iat = 0; iat < niat; iat++) {
	  for(int ipn = 0; ipn < nipn; ipn++) {
	    for(int jat = 0; jat < njat; jat++) { 
	      for(int jpn = 0; jpn < njpn; jpn++) {
		dcomplex cc = 
		  irds->coef_iat_ipn(iat, ipn) *
		  jrds->coef_iat_ipn(jat, jpn) * 
		  irds->coef_iz(iz) * 
		  jrds->coef_iz(jz);
		cumsum_s += cc*prim.s(iat, ipn, jat, jpn);
		cumsum_t += cc*prim.t(iat, ipn, jat, jpn);
		cumsum_v += cc*prim.v(iat, ipn, jat, jpn);
		cumsum_x += cc*prim.x(iat, ipn, jat, jpn);
		cumsum_y += cc*prim.y(iat, ipn, jat, jpn);
		cumsum_z += cc*prim.z(iat, ipn, jat, jpn);
	      }}}}
	int i(irds->offset + iz); int j(jrds->offset + jz);
	int isym(irds->irrep); int jsym(jrds->irrep);
	
	mat_map.SelfAdd("s", isym, jsym, i, j, cumsum_s);
	mat_map.SelfAdd("t", isym, jsym, i, j, cumsum_t);
	mat_map.SelfAdd("v", isym, jsym, i, j, cumsum_v);
	mat_map.SelfAdd("x", isym, jsym, i, j, cumsum_x);
	mat_map.SelfAdd("y", isym, jsym, i, j, cumsum_y);
	mat_map.SelfAdd("z", isym, jsym, i, j, cumsum_z);
      }
    }
  }
  dcomplex OneERI(dcomplex xi, dcomplex yi, dcomplex zi,
		  int nxi, int nyi, int nzi, dcomplex zetai,
		  dcomplex xj, dcomplex yj, dcomplex zj,
		  int nxj, int nyj, int nzj, dcomplex& zetaj,
		  dcomplex xk, dcomplex yk, dcomplex zk,
		  int nxk, int nyk, int nzk, dcomplex& zetak,
		  dcomplex xl, dcomplex yl, dcomplex zl,
		  int nxl, int nyl, int nzl, dcomplex zetal) {
    
    static dcomplex Fjs[100];
    static A3dc Rrs(100);
    static int n(1000);
    static A3dc dxmap(n), dymap(n), dzmap(n), dxmap_p(n), dymap_p(n), dzmap_p(n);    

    dcomplex zetaP = zetai + zetaj; dcomplex zetaPp= zetak + zetal;
    dcomplex lambda = 2.0*pow(M_PI, 2.5)/(zetaP * zetaPp * sqrt(zetaP + zetaPp));

    dcomplex wPx((zetai*xi + zetaj*xj)/zetaP);
    dcomplex wPy((zetai*yi + zetaj*yj)/zetaP);
    dcomplex wPz((zetai*zi + zetaj*zj)/zetaP);
    dcomplex eij(exp(-zetai * zetaj / zetaP *  dist2(xi-xj, yi-yj, zi-zj)));
    int mi = nxi + nyi + nzi;
    int mj = nxj + nyj + nzj;
    int mk = nxk + nyk + nzk;
    int ml = nxl + nyl + nzl;

    calc_d_coef(mi, mj, mi+mj, zetaP,  wPx,  xi, xj, dxmap);
    calc_d_coef(mi, mj, mi+mj, zetaP,  wPy,  yi, yj, dymap);
    calc_d_coef(mi, mj, mi+mj, zetaP,  wPz,  zi, zj, dzmap);

    dcomplex wPpx((zetak*xk + zetal*xl)/zetaPp);
    dcomplex wPpy((zetak*yk + zetal*yl)/zetaPp);
    dcomplex wPpz((zetak*zk + zetal*zl)/zetaPp);
    dcomplex ekl(exp(-zetak * zetal / zetaPp * dist2(xk-xl, yk-yl, zk-zl)));
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpx, xk, xl, dxmap_p);
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpy, yk, yl, dymap_p);
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpz, zk, zl, dzmap_p);
      
    dcomplex zarg(zetaP * zetaPp / (zetaP + zetaPp));
    dcomplex argIncGamma(zarg * dist2(wPx-wPpx, wPy-wPpy, wPz-wPpz));
    IncompleteGamma(mi+mj+mk+ml, argIncGamma, Fjs);      

    int mm = mi + mj + mk + ml;
    calc_R_coef_eri(zarg, wPx, wPy, wPz, wPpx, wPpy, wPpz, mm, Fjs, 1.0, Rrs);
      

    // -- determine 4 primitive GTOs for integration (ij|kl) --
	  
    dcomplex cumsum(0);
    for(int Nx  = 0; Nx  <= nxi + nxj; Nx++)
    for(int Nxp = 0; Nxp <= nxk + nxl; Nxp++)
    for(int Ny  = 0; Ny  <= nyi + nyj; Ny++)
    for(int Nyp = 0; Nyp <= nyk + nyl; Nyp++)
    for(int Nz  = 0; Nz  <= nzi + nzj; Nz++)
    for(int Nzp = 0; Nzp <= nzk + nzl; Nzp++) {
      dcomplex r0;
      r0 = Rrs(Nx+Nxp, Ny+Nyp, Nz+Nzp);
      cumsum += (dxmap(nxi, nxj, Nx) * dxmap_p(nxk, nxl, Nxp) *
		 dymap(nyi, nyj, Ny) * dymap_p(nyk, nyl, Nyp) *
		 dzmap(nzi, nzj, Nz) * dzmap_p(nzk, nzl, Nzp) *
		 r0 * pow(-1.0, Nxp+Nyp+Nzp));
    }
    return eij * ekl * lambda * cumsum;
  }
  int one_dim(int ip, int ni, int jp, int nj, int kp, int nk, int lp) {
    return ip + jp * ni + kp * ni * nj + lp * ni * nj * nk;
  }
  void CheckEqERI(pSymmetryGroup sym_group, 
		  SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		  int ip, int jp, int kp, int lp,
		  int ni, int nj, int nk,
		  int *mark_I, bool *is_zero, bool *is_youngest) {
    *is_zero = false;
    *is_youngest = true;;
    mark_I[0] = 1;
    for(int I = 1; I < sym_group->num_class(); I++) {
      int ipt = isub->ip_jg_kp(I, ip);
      int jpt = jsub->ip_jg_kp(I, jp);
      int kpt = ksub->ip_jg_kp(I, kp);
      int lpt = lsub->ip_jg_kp(I, lp);
      int sig_ipt = isub->sign_ip_jg_kp(I, ip);
      int sig_jpt = jsub->sign_ip_jg_kp(I, jp);
      int sig_kpt = ksub->sign_ip_jg_kp(I, kp);
      int sig_lpt = lsub->sign_ip_jg_kp(I, lp);
      int sig = sig_ipt * sig_jpt * sig_kpt * sig_lpt;

      if(sig == 0) {
	mark_I[I] = 0;
	
      } else if(ipt == ip && jpt == jp && kp == kpt && lp == lpt) {

	if(sig == 1) {
	  mark_I[I] = 0;
	} else {
	  *is_zero = true;
	  return;
	}

      } else {
	if(one_dim(ip, ni, jp, nj, kp, nk, lp) >
	   one_dim(ipt,ni, jpt,nj, kpt,nk, lpt)) {
	  *is_youngest = false;
	}
	mark_I[I] = sig;
      }
    }
  }
  bool ExistNon0(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub) {
    pSymmetryGroup sym = isub->sym_group;
    bool non0(false);
    for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds)
    for(RdsIt jrds = jsub->rds.begin(); jrds != jsub->rds.end(); ++jrds)
    for(RdsIt krds = ksub->rds.begin(); krds != ksub->rds.end(); ++krds)
    for(RdsIt lrds = lsub->rds.begin(); lrds != lsub->rds.end(); ++lrds) {
      if(sym->Non0_4(irds->irrep, jrds->irrep, krds->irrep, lrds->irrep))
	non0 = true;
    }
    return non0;
  }
  
  struct ERI_buf {
    A3dc dx, dy, dz, dxp, dyp, dzp;
    A1dc Fjs;
    A3dc Rrs;
    //    dcomplex eij, ekl, lambda;
    dcomplex lambda;
    ERI_buf(int n) :
      dx(n), dy(n), dz(n), dxp(n), dyp(n), dzp(n), Fjs(n), Rrs(n) {}
  };
  void CalcCoef(dcomplex xi, dcomplex yi, dcomplex zi,
	       int mi, dcomplex zetai,
	       dcomplex xj, dcomplex yj, dcomplex zj,
	       int mj, dcomplex& zetaj,
	       dcomplex xk, dcomplex yk, dcomplex zk,
	       int mk, dcomplex& zetak,
	       dcomplex xl, dcomplex yl, dcomplex zl,
	       int ml, dcomplex zetal, ERI_buf& buf) {
    dcomplex zetaP = zetai + zetaj;
    dcomplex zetaPp= zetak + zetal;
    dcomplex wPx((zetai*xi + zetaj*xj)/zetaP);
    dcomplex wPy((zetai*yi + zetaj*yj)/zetaP);
    dcomplex wPz((zetai*zi + zetaj*zj)/zetaP);
    
    calc_d_coef(mi, mj, mi+mj, zetaP,  wPx,  xi, xj, buf.dx);
    calc_d_coef(mi, mj, mi+mj, zetaP,  wPy,  yi, yj, buf.dy);
    calc_d_coef(mi, mj, mi+mj, zetaP,  wPz,  zi, zj, buf.dz);

    dcomplex wPpx((zetak*xk + zetal*xl)/zetaPp);
    dcomplex wPpy((zetak*yk + zetal*yl)/zetaPp);
    dcomplex wPpz((zetak*zk + zetal*zl)/zetaPp);
    
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpx, xk, xl, buf.dxp);
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpy, yk, yl, buf.dyp);
    calc_d_coef(mk, ml, mk+ml, zetaPp, wPpz, zk, zl, buf.dzp);

    dcomplex zarg(zetaP * zetaPp / (zetaP + zetaPp));
    dcomplex argIncGamma(zarg * dist2(wPx-wPpx, wPy-wPpy, wPz-wPpz));
    double delta(0.0000000000001);
    if(real(argIncGamma)+delta > 0.0) {
      //      cout << "re: " << argIncGamma << endl;
      IncompleteGamma(mi+mj+mk+ml, argIncGamma, &buf.Fjs(0));      
      int mm = mi + mj + mk + ml;
      dcomplex eij = exp(-zetai * zetaj / zetaP *  dist2(xi-xj, yi-yj, zi-zj));
      dcomplex ekl = (exp(-zetak * zetal / zetaPp * dist2(xk-xl, yk-yl, zk-zl)));
      calc_R_coef_eri(zarg, wPx, wPy, wPz, wPpx, wPpy, wPpz, mm,
		      &buf.Fjs(0), eij * ekl, buf.Rrs);
    } else {      
      int mm = mi + mj + mk + ml;
      // treat F as G[-z] = Exp[z]F[z]
      ExpIncompleteGamma(mm, -argIncGamma, &buf.Fjs(0)); 
      dcomplex arg_other = 
	-zetai * zetaj / zetaP *  dist2(xi-xj, yi-yj, zi-zj)
	-zetak * zetal / zetaPp * dist2(xk-xl, yk-yl, zk-zl)
	-argIncGamma;
      calc_R_coef_eri(zarg, wPx, wPy, wPz, wPpx, wPpy, wPpz, mm,
		      &buf.Fjs(0), exp(arg_other), buf.Rrs);
    }

  }
  dcomplex CalcPrimOne(int nxi, int nyi, int nzi,
		   int nxj, int nyj, int nzj,
		   int nxk, int nyk, int nzk,
		   int nxl, int nyl, int nzl, ERI_buf& buf) {
    dcomplex cumsum(0);
    for(int Nx  = 0; Nx  <= nxi + nxj; Nx++)
      for(int Nxp = 0; Nxp <= nxk + nxl; Nxp++)
	for(int Ny  = 0; Ny  <= nyi + nyj; Ny++)
	  for(int Nyp = 0; Nyp <= nyk + nyl; Nyp++)
	    for(int Nz  = 0; Nz  <= nzi + nzj; Nz++)
	      for(int Nzp = 0; Nzp <= nzk + nzl; Nzp++) {
		dcomplex r0;
		r0 = buf.Rrs(Nx+Nxp, Ny+Nyp, Nz+Nzp);
		cumsum += (buf.dx(nxi, nxj, Nx) * buf.dxp(nxk, nxl, Nxp) *
			   buf.dy(nyi, nyj, Ny) * buf.dyp(nyk, nyl, Nyp) *
			   buf.dz(nzi, nzj, Nz) * buf.dzp(nzk, nzl, Nzp) *
			   r0 * pow(-1.0, Nxp+Nyp+Nzp));
	      }
    return buf.lambda * cumsum;;
	
  }

  void CalcPrimERI0(SymGTOs gtos,
		    SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		    int iz, int jz, int kz, int lz,
		    A4dc& prim) {
    dcomplex zetai, zetaj, zetak, zetal, zetaP, zetaPp;
    zetai = isub->zeta_iz[iz]; zetaj = jsub->zeta_iz[jz];
    zetak = ksub->zeta_iz[kz]; zetal = lsub->zeta_iz[lz];

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();

    prim.SetRange(0, nati*npni, 0, natj*npnj, 0, natk*npnk, 0, natl*npnl);

    int numI(gtos.sym_group->num_class());
    if(numI == 1) {
      string msg; SUB_LOCATION(msg); 
      throw runtime_error(msg);
    }

    int mark_I[10];
    for(int iat = 0; iat < nati; iat++) 
    for(int jat = 0; jat < natj; jat++)       
    for(int kat = 0; kat < natk; kat++) 
    for(int lat = 0; lat < natl; lat++) 
    for(int ipn = 0; ipn < npni; ipn++) 
    for(int jpn = 0; jpn < npnj; jpn++) 
    for(int kpn = 0; kpn < npnk; kpn++) 
    for(int lpn = 0; lpn < npnl; lpn++) {
      int ip = isub->ip_iat_ipn(iat, ipn);
      int jp = jsub->ip_iat_ipn(jat, jpn);
      int kp = ksub->ip_iat_ipn(kat, kpn);
      int lp = lsub->ip_iat_ipn(lat, lpn);
      bool is_youngest(true), is_zero(false);
      
      mark_I[0] = 1;
      for(int I = 1; I < numI; I++) {
	int ipt = isub->ip_jg_kp(I, ip);
	int jpt = jsub->ip_jg_kp(I, jp);
	int kpt = ksub->ip_jg_kp(I, kp);
	int lpt = lsub->ip_jg_kp(I, lp);
	int sig_ipt = isub->sign_ip_jg_kp(I, ip);
	int sig_jpt = jsub->sign_ip_jg_kp(I, jp);
	int sig_kpt = ksub->sign_ip_jg_kp(I, kp);
	int sig_lpt = lsub->sign_ip_jg_kp(I, lp);
	int sig = sig_ipt * sig_jpt * sig_kpt * sig_lpt;
	if(sig == 0) {
	  mark_I[I] = 0;
	  break;
	}

	if(ipt == ip && jpt == jp && kp == kpt && lp == lpt) {
	  if(sig == 1) {
	    mark_I[I] = 0;
	  } else {
	    is_zero = true;
	    break;
	  }
	} else {
	  if(one_dim(ip, nati*npni, jp, natj*npnj, kp, natk*npnk, lp) >
	     one_dim(ipt,nati*npni, jpt,natj*npnj, kpt,natk*npnk, lpt)) {
	    is_youngest = false;
	  }
	  mark_I[I] = sig;
	}
      }
      dcomplex v;
      if(is_zero) {
	v = 0.0;
      } else {
	if(is_youngest) {
	  v = OneERI(isub->x(iat), isub->y(iat), isub->z(iat),
		   isub->nx(ipn), isub->ny(ipn), isub->nz(ipn), zetai,
		   jsub->x(jat), jsub->y(jat), jsub->z(jat),
		   jsub->nx(jpn), jsub->ny(jpn), jsub->nz(jpn), zetaj, 
		   ksub->x(kat), ksub->y(kat), ksub->z(kat),
		   ksub->nx(kpn), ksub->ny(kpn), ksub->nz(kpn), zetak, 
		   lsub->x(lat), lsub->y(lat), lsub->z(lat),
		   lsub->nx(lpn), lsub->ny(lpn), lsub->nz(lpn), zetal);
	  for(int I = 0; I < numI; I++) {
	    if(mark_I[I] != 0) {
	      int ipt = isub->ip_jg_kp(I, ip);
	      int jpt = jsub->ip_jg_kp(I, jp);
	      int kpt = ksub->ip_jg_kp(I, kp);
	      int lpt = lsub->ip_jg_kp(I, lp);
	      prim(ipt, jpt, kpt, lpt) = dcomplex(mark_I[I]) * v;
	    }
	  }
	}
      }
    }
  }
  // -- very simple --
  void CalcPrimERI0(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		    dcomplex zetai, dcomplex zetaj, dcomplex zetak, dcomplex zetal,
		    A4dc& prim) {

    static ERI_buf buf(1000);
    dcomplex zetaP = zetai + zetaj; dcomplex zetaPp = zetak + zetal;

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();

    prim.SetRange(0, nati*npni, 0, natj*npnj, 0, natk*npnk, 0, natl*npnl);

    int mi, mj, mk, ml;
    mi = isub->maxn; mj = jsub->maxn; mk = ksub->maxn; ml = lsub->maxn;
    buf.lambda = 2.0*pow(M_PI, 2.5)/(zetaP * zetaPp * sqrt(zetaP + zetaPp));    

    prim.SetValue(0.0);

    for(int iat = 0; iat < nati; iat++) 
    for(int jat = 0; jat < natj; jat++)       
    for(int kat = 0; kat < natk; kat++) 
    for(int lat = 0; lat < natl; lat++) {
      CalcCoef(isub->x(iat), isub->y(iat), isub->z(iat), mi, zetai,
	       jsub->x(jat), jsub->y(jat), jsub->z(jat), mj, zetaj, 
	       ksub->x(kat), ksub->y(kat), ksub->z(kat), mk, zetak, 
	       lsub->x(lat), lsub->y(lat), lsub->z(lat), ml, zetal, buf);
      
      for(int ipn = 0; ipn < npni; ipn++) 
      for(int jpn = 0; jpn < npnj; jpn++) 
      for(int kpn = 0; kpn < npnk; kpn++) 
      for(int lpn = 0; lpn < npnl; lpn++) {
	int ip = isub->ip_iat_ipn(iat, ipn); int jp = jsub->ip_iat_ipn(jat, jpn);
	int kp = ksub->ip_iat_ipn(kat, kpn); int lp = lsub->ip_iat_ipn(lat, lpn);
	dcomplex v;
	v = CalcPrimOne(isub->nx(ipn), isub->ny(ipn), isub->nz(ipn),
			jsub->nx(jpn), jsub->ny(jpn), jsub->nz(jpn),
			ksub->nx(kpn), ksub->ny(kpn), ksub->nz(kpn),
			lsub->nx(lpn), lsub->ny(lpn), lsub->nz(lpn), buf);
	prim(ip, jp, kp, lp) = v;
      }
    }
  }
  // -- Symmetry considerration --
  void CalcPrimERI1(SymGTOs gtos, SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		    dcomplex zetai, dcomplex zetaj, dcomplex zetak, dcomplex zetal,
		    A4dc& prim) {

    static ERI_buf buf(1000);
    dcomplex zetaP = zetai + zetaj; dcomplex zetaPp = zetak + zetal;

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();

    prim.SetRange(0, nati*npni, 0, natj*npnj, 0, natk*npnk, 0, natl*npnl);

    int mi, mj, mk, ml;
    mi = isub->maxn; mj = jsub->maxn; mk = ksub->maxn; ml = lsub->maxn;
    buf.lambda = 2.0*pow(M_PI, 2.5)/(zetaP * zetaPp * sqrt(zetaP + zetaPp));    

    int numI(gtos.sym_group->order());

    prim.SetValue(0.0);

    for(int iat = 0; iat < nati; iat++) 
    for(int jat = 0; jat < natj; jat++)       
    for(int kat = 0; kat < natk; kat++) 
    for(int lat = 0; lat < natl; lat++) {

      // -- check 0 or non 0 --
      bool find_non0(false);
      for(int ipn = 0; ipn < npni; ipn++) 
      for(int jpn = 0; jpn < npnj; jpn++) 
      for(int kpn = 0; kpn < npnk; kpn++) 
      for(int lpn = 0; lpn < npnl; lpn++) {
	int ip = isub->ip_iat_ipn(iat, ipn); int jp = jsub->ip_iat_ipn(jat, jpn);
	int kp = ksub->ip_iat_ipn(kat, kpn); int lp = lsub->ip_iat_ipn(lat, lpn);
	int mark_I[10]; bool is_zero, is_youngest;
	CheckEqERI(isub->sym_group, isub, jsub, ksub, lsub, ip, jp, kp, lp,
		   nati*npni, natj*npnj, natk*npnk, mark_I, &is_zero, &is_youngest);
	if(!is_zero)
	  find_non0 = true;
      }

      // -- compute if found non0 --
      if(find_non0) {
      
	CalcCoef(isub->x(iat), isub->y(iat), isub->z(iat), mi, zetai,
		 jsub->x(jat), jsub->y(jat), jsub->z(jat), mj, zetaj, 
		 ksub->x(kat), ksub->y(kat), ksub->z(kat), mk, zetak, 
		 lsub->x(lat), lsub->y(lat), lsub->z(lat), ml, zetal, buf);

	for(int ipn = 0; ipn < npni; ipn++) 
	for(int jpn = 0; jpn < npnj; jpn++) 
	for(int kpn = 0; kpn < npnk; kpn++) 
	for(int lpn = 0; lpn < npnl; lpn++) {
	  int ip = isub->ip_iat_ipn(iat, ipn); int jp = jsub->ip_iat_ipn(jat, jpn);
	  int kp = ksub->ip_iat_ipn(kat, kpn); int lp = lsub->ip_iat_ipn(lat, lpn);

	  int mark_I[10]; bool is_zero, is_youngest;
	  CheckEqERI(isub->sym_group, isub, jsub, ksub, lsub, ip, jp, kp, lp,
		     nati*npni, natj*npnj, natk*npnk, mark_I, &is_zero, &is_youngest);
	  if(!is_zero && is_youngest) {
	    dcomplex v;
	    v = CalcPrimOne(isub->nx(ipn), isub->ny(ipn), isub->nz(ipn),
			    jsub->nx(jpn), jsub->ny(jpn), jsub->nz(jpn),
			    ksub->nx(kpn), ksub->ny(kpn), ksub->nz(kpn),
			    lsub->nx(lpn), lsub->ny(lpn), lsub->nz(lpn), buf);
	    for(int I = 0; I < numI; I++) {
	      if(mark_I[I] != 0) {
		int ipt = isub->ip_jg_kp(I, ip);
		int jpt = jsub->ip_jg_kp(I, jp);
		int kpt = ksub->ip_jg_kp(I, kp);
		int lpt = lsub->ip_jg_kp(I, lp);
		prim(ipt, jpt, kpt, lpt) = dcomplex(mark_I[I]) * v;
	      }
	    }
	  }
	}
      }
    }

  }
  // -- Simple --
  void CalcPrimERI2(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		    int iz, int jz, int kz, int lz,
		    A4dc& prim) {
    static A3dc dxmap(1000);
    static A3dc dymap(1000);
    static A3dc dzmap(1000);
    static A3dc dxmap_p(1000);
    static A3dc dymap_p(1000);
    static A3dc dzmap_p(1000);

    dcomplex zetai, zetaj, zetak, zetal, zetaP, zetaPp;
    zetai = isub->zeta_iz[iz]; zetaj = jsub->zeta_iz[jz];
    zetak = ksub->zeta_iz[kz]; zetal = lsub->zeta_iz[lz];
    zetaP = zetai + zetaj;     zetaPp= zetak + zetal;

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();

    int mi, mj, mk, ml;
    mi = isub->maxn; mj = jsub->maxn; mk = ksub->maxn; ml = lsub->maxn;
    dcomplex lambda = 2.0*pow(M_PI, 2.5)/(zetaP * zetaPp * sqrt(zetaP + zetaPp));    

    prim.SetRange(0, nati*npni, 0, natj*npnj, 0, natk*npnk, 0, natl*npnl);

    static dcomplex Fjs[100];
    static A3dc Rrs(100);
    int idx(0);
    for(int iat = 0; iat < nati; iat++) {
    for(int jat = 0; jat < natj; jat++) {      
    for(int kat = 0; kat < natk; kat++) {
    for(int lat = 0; lat < natl; lat++) {      
      dcomplex xi(isub->x(iat)), yi(isub->y(iat)), zi(isub->z(iat));
      dcomplex xj(jsub->x(jat)), yj(jsub->y(jat)), zj(jsub->z(jat));
      dcomplex wPx((zetai*xi + zetaj*xj)/zetaP);
      dcomplex wPy((zetai*yi + zetaj*yj)/zetaP);
      dcomplex wPz((zetai*zi + zetaj*zj)/zetaP);
      dcomplex eij(exp(-zetai * zetaj / zetaP *  dist2(xi-xj, yi-yj, zi-zj)));
      calc_d_coef(mi, mj, mi+mj, zetaP,  wPx,  xi, xj, dxmap);
      calc_d_coef(mi, mj, mi+mj, zetaP,  wPy,  yi, yj, dymap);
      calc_d_coef(mi, mj, mi+mj, zetaP,  wPz,  zi, zj, dzmap);

      dcomplex xk(ksub->x(kat)), yk(ksub->y(kat)), zk(ksub->z(kat));
      dcomplex xl(lsub->x(lat)), yl(lsub->y(lat)), zl(lsub->z(lat));
      dcomplex wPpx((zetak*xk + zetal*xl)/zetaPp);
      dcomplex wPpy((zetak*yk + zetal*yl)/zetaPp);
      dcomplex wPpz((zetak*zk + zetal*zl)/zetaPp);
      dcomplex ekl(exp(-zetak * zetal / zetaPp * dist2(xk-xl, yk-yl, zk-zl)));
	calc_d_coef(mk, ml, mk+ml, zetaPp, wPpx, xk, xl, dxmap_p);
	calc_d_coef(mk, ml, mk+ml, zetaPp, wPpy, yk, yl, dymap_p);
	calc_d_coef(mk, ml, mk+ml, zetaPp, wPpz, zk, zl, dzmap_p);

	dcomplex zarg(zetaP * zetaPp / (zetaP + zetaPp));
	dcomplex argIncGamma(zarg * dist2(wPx-wPpx, wPy-wPpy, wPz-wPpz));
	IncompleteGamma(mi+mj+mk+ml, argIncGamma, Fjs);      

	int mm = mi + mj + mk + ml;
	calc_R_coef_eri(zarg, wPx, wPy, wPz, wPpx, wPpy, wPpz, mm, Fjs, 1.0, Rrs);

	for(int ipn = 0; ipn < npni; ipn++) 
	for(int jpn = 0; jpn < npnj; jpn++) 
	for(int kpn = 0; kpn < npnk; kpn++) 
	for(int lpn = 0; lpn < npnl; lpn++) {      
	  int ip = isub->ip_iat_ipn(iat, ipn);
	  int jp = jsub->ip_iat_ipn(jat, jpn);
	  int kp = ksub->ip_iat_ipn(kat, kpn);
	  int lp = lsub->ip_iat_ipn(lat, lpn);

	  // -- determine 4 primitive GTOs for integration (ij|kl) --
	  int nxi, nxj, nxk, nxl, nyi, nyj, nyk, nyl, nzi, nzj, nzk, nzl;
	  nxi = isub->nx(ipn); nyi = isub->ny(ipn); nzi = isub->nz(ipn);
	  nxj = jsub->nx(jpn); nyj = jsub->ny(jpn); nzj = jsub->nz(jpn);
	  nxk = ksub->nx(kpn); nyk = ksub->ny(kpn); nzk = ksub->nz(kpn);
	  nxl = lsub->nx(lpn); nyl = lsub->ny(lpn); nzl = lsub->nz(lpn);
	  
	  dcomplex cumsum(0);
	  for(int Nx  = 0; Nx  <= nxi + nxj; Nx++)
	  for(int Nxp = 0; Nxp <= nxk + nxl; Nxp++)
	  for(int Ny  = 0; Ny  <= nyi + nyj; Ny++)
	  for(int Nyp = 0; Nyp <= nyk + nyl; Nyp++)
	  for(int Nz  = 0; Nz  <= nzi + nzj; Nz++)
	  for(int Nzp = 0; Nzp <= nzk + nzl; Nzp++) {
	    dcomplex r0;
	    r0 = Rrs(Nx+Nxp, Ny+Nyp, Nz+Nzp);
	    cumsum += (dxmap(nxi, nxj, Nx) * dxmap_p(nxk, nxl, Nxp) *
		       dymap(nyi, nyj, Ny) * dymap_p(nyk, nyl, Nyp) *
		       dzmap(nzi, nzj, Nz) * dzmap_p(nzk, nzl, Nzp) *
		       r0 * pow(-1.0, Nxp+Nyp+Nzp));
		   
	  }
	  prim(ip, jp, kp, lp) = eij * ekl * lambda * cumsum;
	  ++idx;
	}
      }
    }}}
  }  

  void CalcTransERI(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		   int iz, int jz, int kz, int lz,
		    A4dc& prim, IB2EInt* eri, bool use_perm=false) {

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();
    

    for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds)
    for(RdsIt jrds = jsub->rds.begin(); jrds != jsub->rds.end(); ++jrds)
    for(RdsIt krds = ksub->rds.begin(); krds != ksub->rds.end(); ++krds)
    for(RdsIt lrds = lsub->rds.begin(); lrds != lsub->rds.end(); ++lrds) {

      if(isub->sym_group->Non0_4(irds->irrep, jrds->irrep, krds->irrep, lrds->irrep)) {
	dcomplex cz(irds->coef_iz(iz) * 
		    jrds->coef_iz(jz) * 
		    krds->coef_iz(kz) * 
		    lrds->coef_iz(lz));
	dcomplex cumsum(0);
	int idx(0);
	for(int iat = 0; iat < nati; iat++) {
	for(int ipn = 0; ipn < npni; ipn++) {
        for(int jat = 0; jat < natj; jat++) {
	for(int jpn = 0; jpn < npnj; jpn++) {
        for(int kat = 0; kat < natk; kat++) {
	for(int kpn = 0; kpn < npnk; kpn++) {
        for(int lat = 0; lat < natl; lat++) {
        for(int lpn = 0; lpn < npnl; lpn++) {
	  int ip = isub->ip_iat_ipn(iat, ipn);
	  dcomplex ci(irds->coef_iat_ipn(iat, ipn));
	  int jp = jsub->ip_iat_ipn(jat, jpn);
	  dcomplex cj(jrds->coef_iat_ipn(jat, jpn));
	  int kp = ksub->ip_iat_ipn(kat, kpn);
	  dcomplex ck(krds->coef_iat_ipn(kat, kpn));
	  int lp = lsub->ip_iat_ipn(lat, lpn);
	  dcomplex cc = ci * cj * ck * lrds->coef_iat_ipn(lat, lpn) * cz;
	  cumsum += cc * prim(ip, jp, kp, lp); ++idx;
	}}}}}}}}
	eri->Set(irds->irrep, jrds->irrep, krds->irrep, lrds->irrep,
		 irds->offset+iz, jrds->offset+jz, krds->offset+kz, lrds->offset+lz,
		 cumsum);
	if(use_perm)
	  eri->Set(krds->irrep, lrds->irrep, irds->irrep, jrds->irrep,
		   krds->offset+kz, lrds->offset+lz, irds->offset+iz, jrds->offset+jz,
		   cumsum);
      }
    }

  }
  void CalcTransERI0(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		   int iz, int jz, int kz, int lz,
		     A4dc& prim, IB2EInt* eri) {

    int nati, natj, natk, natl;
    nati = isub->size_at(); natj = jsub->size_at();
    natk = ksub->size_at(); natl = lsub->size_at();

    int npni, npnj, npnk, npnl;
    npni = isub->size_pn(); npnj = jsub->size_pn();
    npnk = ksub->size_pn(); npnl = lsub->size_pn();
    

    for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds)
    for(RdsIt jrds = jsub->rds.begin(); jrds != jsub->rds.end(); ++jrds)
    for(RdsIt krds = ksub->rds.begin(); krds != ksub->rds.end(); ++krds)
    for(RdsIt lrds = lsub->rds.begin(); lrds != lsub->rds.end(); ++lrds) {
      
      dcomplex cz(irds->coef_iz(iz) * 
		  jrds->coef_iz(jz) * 
		  krds->coef_iz(kz) * 
		  lrds->coef_iz(lz));
      dcomplex cumsum(0);
      int idx(0);
      for(int iat = 0; iat < nati; iat++) {
      for(int ipn = 0; ipn < npni; ipn++) {
      for(int jat = 0; jat < natj; jat++) {
      for(int jpn = 0; jpn < npnj; jpn++) {
      for(int kat = 0; kat < natk; kat++) {
      for(int kpn = 0; kpn < npnk; kpn++) {
      for(int lat = 0; lat < natl; lat++) {
      for(int lpn = 0; lpn < npnl; lpn++) {
	int ip = isub->ip_iat_ipn(iat, ipn);
	dcomplex ci(irds->coef_iat_ipn(iat, ipn));
	int jp = jsub->ip_iat_ipn(jat, jpn);
	dcomplex cj(jrds->coef_iat_ipn(jat, jpn));
	int kp = ksub->ip_iat_ipn(kat, kpn);
	dcomplex ck(krds->coef_iat_ipn(kat, kpn));
	int lp = lsub->ip_iat_ipn(lat, lpn);
	dcomplex cc = ci * cj * ck * lrds->coef_iat_ipn(lat, lpn) * cz;
	cumsum += cc * prim(ip, jp, kp, lp); ++idx;
      }}}}}}}}
      eri->Set(irds->irrep, jrds->irrep, krds->irrep, lrds->irrep,
	       irds->offset+iz, jrds->offset+jz, krds->offset+kz, lrds->offset+lz,
	       cumsum);
    }
  }

  // ---- summary for ERI ----
  // simple
  void CalcERI2(SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		A4dc& prim, IB2EInt* eri) {
    for(int iz = 0; iz < isub->size_zeta(); ++iz)
    for(int jz = 0; jz < jsub->size_zeta(); ++jz)
    for(int kz = 0; kz < ksub->size_zeta(); ++kz)
    for(int lz = 0; lz < lsub->size_zeta(); ++lz) {
      CalcPrimERI0(isub, jsub, ksub, lsub,
		   isub->zeta_iz[iz], jsub->zeta_iz[jz],
		   ksub->zeta_iz[kz], lsub->zeta_iz[lz], prim);
      CalcTransERI0(isub, jsub, ksub, lsub, iz, jz, kz, lz, prim, eri);
    }
  }
  void CalcERI1(SymGTOs& gi, SymGTOs& gj,SymGTOs& gk,SymGTOs& gl,
		SubIt isub, SubIt jsub, SubIt ksub, SubIt lsub,
		A4dc& prim, IB2EInt* eri) {
    if(!ExistNon0(isub, jsub, ksub, lsub))
      return;

    int n_ij = (distance(gi.subs.begin(), isub) +
		distance(gj.subs.begin(), jsub) * gi.subs.size());
    int n_kl = (distance(gk.subs.begin(), ksub) +
		distance(gl.subs.begin(), lsub) * gk.subs.size());    

    if(isub == ksub && jsub == lsub) {
    for(int iz = 0; iz < isub->size_zeta(); ++iz)
      for(int jz = 0; jz < jsub->size_zeta(); ++jz)
      for(int kz = 0; kz < ksub->size_zeta(); ++kz)
      for(int lz = 0; lz < lsub->size_zeta(); ++lz) {
	int nnnij = iz + isub->size_zeta() * jz;
	int nnnkl = kz + ksub->size_zeta() * lz;
	if(nnnij >= nnnkl) {
	  CalcPrimERI1(gi, isub, jsub, ksub, lsub,
		       isub->zeta_iz[iz], jsub->zeta_iz[jz],
		       ksub->zeta_iz[kz], lsub->zeta_iz[lz],
		       prim);
	  if(nnnij == nnnkl)
	    CalcTransERI(isub, jsub, ksub, lsub, kz, lz, iz, jz, prim, eri, false);
	  else
	    CalcTransERI(isub, jsub, ksub, lsub, kz, lz, iz, jz, prim, eri, true);
	}
      }
    } else if(n_ij > n_kl){
      for(int iz = 0; iz < isub->size_zeta(); ++iz)
      for(int jz = 0; jz < jsub->size_zeta(); ++jz)
      for(int kz = 0; kz < ksub->size_zeta(); ++kz)
      for(int lz = 0; lz < lsub->size_zeta(); ++lz) {
	CalcPrimERI1(gi, isub, jsub, ksub, lsub,
		     isub->zeta_iz[iz], jsub->zeta_iz[jz],
		     ksub->zeta_iz[kz], lsub->zeta_iz[lz],
		     prim);
	CalcTransERI(isub, jsub, ksub, lsub, iz, jz, kz, lz, prim, eri, true);
      }      
    }
  }
  void CalcERI(SymGTOs& gi, SymGTOs& gj,SymGTOs& gk,SymGTOs& gl, IB2EInt* eri) {

    eri->Init(gi.size_basis() * gj.size_basis() * gk.size_basis() * gl.size_basis());
    A4dc prim(gi.max_num_prim() * gj.max_num_prim() *
	      gk.max_num_prim() * gl.max_num_prim());
    for(SubIt isub = gi.subs.begin(); isub != gi.subs.end(); ++isub) {
      for(SubIt jsub = gj.subs.begin(); jsub != gj.subs.end(); ++jsub) {
	for(SubIt ksub = gk.subs.begin(); ksub != gk.subs.end(); ++ksub) {
	  for(SubIt lsub = gl.subs.begin(); lsub != gl.subs.end(); ++lsub) {
	    CalcERI2(isub, jsub, ksub, lsub, prim, eri);
	  }    
	}
      }
    }
  }

  void SymGTOs::loop() {

    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      for(SubIt jsub = subs.begin(); jsub != subs.end(); ++jsub) {

	// -- loop over each zeta --
	for(int iz = 0; iz < isub->size_zeta(); iz++) {
	  for(int jz = 0; jz < jsub->size_zeta(); jz++) {
	    dcomplex zetai, zetaj;
	    zetai = isub->zeta_iz[iz]; zetaj = jsub->zeta_iz[jz];
	    int niat(isub->size_at()); int njat(jsub->size_at());
	    int nipn(isub->size_pn()); int njpn(jsub->size_pn());

	    // -- primitive basis --
	    for(int iat = 0; iat < niat; iat++) {
	      for(int jat = 0; jat < njat; jat++) { 
		for(int ipn = 0; ipn < nipn; ipn++) {
		  for(int jpn = 0; jpn < njpn; jpn++) {

		  }}}}

	    // -- contractions --
	    for(RdsIt irds = isub->rds.begin();irds != isub->rds.end();++irds) {
	      for(RdsIt jrds = jsub->rds.begin(); jrds != jsub->rds.end();++jrds) {
	      }}

	  }}
      }}
    }
  int SymGTOs::max_num_prim() const {
    
    int max_num_prim(0);
    for(cSubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      max_num_prim = std::max(max_num_prim, isub->size_prim());
    }
    return max_num_prim;

  }
  void SymGTOs::CalcMatOther(SymGTOs& o, bool calc_coulomb, BMatSet* res) {

    if(not setupq)
      this->SetUp();

    if(not o.setupq)
      o.SetUp();

    int num_sym(this->sym_group->num_class());

    BMatSet mat_map(num_sym);
    for(Irrep isym = 0; isym < num_sym; isym++) {
      for(Irrep jsym = 0; jsym < num_sym; jsym++) {
	int numi = this->size_basis_isym(isym);
	int numj = o.size_basis_isym(jsym);
	MatrixXcd s = MatrixXcd::Zero(numi, numj);
	mat_map.SetMatrix("s", isym, jsym, s);
	MatrixXcd t = MatrixXcd::Zero(numi, numj); 
	mat_map.SetMatrix("t", isym, jsym, t);
	MatrixXcd v = MatrixXcd::Zero(numi, numj); 
	mat_map.SetMatrix("v", isym, jsym, v);

	MatrixXcd x = MatrixXcd::Zero(numi, numj); 
	mat_map.SetMatrix("x", isym, jsym, x);
	MatrixXcd y = MatrixXcd::Zero(numi, numj); 
	mat_map.SetMatrix("y", isym, jsym, y);
	MatrixXcd z = MatrixXcd::Zero(numi, numj); 
	mat_map.SetMatrix("z", isym, jsym, z);
      }
    }

    PrimBasis prim(100);
    A4dc rmap(100);
    A3dc dxmap(100),  dymap(100),  dzmap(100);
    A2dc Fjs_iat(100);
    
    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      for(SubIt jsub = o.subs.begin(); jsub != o.subs.end(); ++jsub) {
	for(int iz = 0; iz < isub->size_zeta(); iz++) {
	  for(int jz = 0; jz < jsub->size_zeta(); jz++) {
	    CalcPrim(*this, isub, jsub, iz, jz, dxmap, dymap, dzmap, rmap,
		     Fjs_iat, prim, calc_coulomb);
	    CalcTrans(isub, jsub, iz, jz, prim, mat_map);
	  }
	}
      }
    }
    *res = mat_map;    

  }
  void SymGTOs::CalcMat(BMatSet* res) {
    this->CalcMatOther(*this, true, res);
  }
  void SymGTOs::CalcERI(IB2EInt* eri, int method) {
    //    VectorXcd res = VectorXcd::Zero(n*n*n*n);

    eri->Init(pow(this->size_basis(), 4));
    int max_num_prim(0);
    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      max_num_prim = std::max(max_num_prim, isub->size_prim());
    }
    A4dc prim(pow(max_num_prim, 4));

    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) 
    for(SubIt jsub = subs.begin(); jsub != subs.end(); ++jsub) 
    for(SubIt ksub = subs.begin(); ksub != subs.end(); ++ksub) 
    for(SubIt lsub = subs.begin(); lsub != subs.end(); ++lsub) {
      
      if(method == 0) {
	throw runtime_error("not implemented");
      }
      if(method == 1) {
	CalcERI1(*this, *this,*this,*this,isub, jsub, ksub, lsub, prim, eri);
      }
      if(method == 2) {
	CalcERI2(isub, jsub, ksub, lsub, prim, eri);
      }
    }
  }
  
  // ---- AtR ----
  bool IsCenter(SubIt isub, int iat, double eps) {
    dcomplex x  = isub->x(iat);
    dcomplex y  = isub->y(iat);
    dcomplex z  = isub->z(iat);
    dcomplex a2 = x*x+y*y+z*z;
    dcomplex a = sqrt(a2);
    return (abs(a) < eps);
  }
  void AtR_Ylm_cen(SubIt isub, int iz,
		   int ipn, dcomplex r, int L, int M, dcomplex* v, dcomplex* dv) {

    int nx = isub->nx(ipn);
    int ny = isub->ny(ipn);
    int nz = isub->nz(ipn);
    int nn = nx + ny + nz;
    dcomplex zeta = isub->zeta_iz(iz);
    if(L == 0) {
      // -- s-GTO --
      if(nn == 0) {
	dcomplex c(sqrt(4.0*M_PI));
	*v = c*r*exp(-zeta*r*r);
	*dv= c*(1.0 -2.0*zeta*r*r) * exp(-zeta*r*r);
      } else {
	*v = 0.0;
	*dv= 0.0;
      }
    } else if(L == 1) {
      // -- p-GTO --
      if(nx == 0 && ny == 0 && nz == 1 && M == 0) {
	dcomplex c(sqrt(4.0*M_PI/3.0));
	*v = c*r*r*exp(-zeta*r*r);
	*dv= c*(2.0*r - 2.0*zeta*r*r*r )*exp(-zeta*r*r);
      }	  
      else if(nx == 0 && ny == 1 && nz == 0 && M == 1)
	throw runtime_error("0101 is not implemented");
      else if(nx == 1 && ny == 0 && nz == 0 && M ==-1)
	throw runtime_error("0101 is not implemented");	
      else {
	*v = 0.0;
	*dv= 0.0;
      }
    } else {
      string msg; SUB_LOCATION(msg);
      msg += "L>1 is not implemented";
      throw runtime_error(msg);
    }
    
  }
  void AtR_Ylm_noncen(SubIt isub, int iz, int iat, int ipn,
		      dcomplex r, int L, int M, dcomplex* v, dcomplex *dv) {

    dcomplex* il_ipl  = new dcomplex[2*L+2];
    dcomplex* il = &il_ipl[0];
    dcomplex* ipl= &il_ipl[L+1];
    dcomplex* ylm = new dcomplex[num_lm_pair(L)];

    dcomplex res;

    int nn = (isub->nx(ipn) +
	      isub->ny(ipn) +
	      isub->nz(ipn));
    dcomplex zeta = isub->zeta_iz(iz);
    dcomplex x  = isub->x(iat);
    dcomplex y  = isub->y(iat);
    dcomplex z  = isub->z(iat);
    dcomplex xxyy = x*x+y*y;
    dcomplex a2 = xxyy+z*z;
    dcomplex a = sqrt(a2);

    if(abs(a) < 0.000001) {
      string msg; SUB_LOCATION(msg);
      msg += "(x,y,z) is not centered on origin."; 
      throw runtime_error(msg);
    }

    dcomplex expz = exp(-zeta*(r*r+a2));
    dcomplex theta = acos(z / a);
    dcomplex phi   = (abs(xxyy) < 0.00001) ? 0.0 : acos(x / sqrt(xxyy));
    ModSphericalBessel(2.0*zeta*a*r, L, il_ipl);
    RealSphericalHarmonics(theta, phi, L, ylm);
    if(nn == 0) {
      
      dcomplex c((4.0*M_PI) * pow(-1.0, M) * ylm[lm_index(L, -M)]);
      *v = c * r * il[L] * expz;
      *dv = (c*   il[L]*expz +
	     c*r*ipl[L]*expz*(+2.0*zeta*a) +
	     c*r* il[L]*expz*(-2.0*zeta*r));
    } else {
      string msg; SUB_LOCATION(msg);
      msg += ": not implemented yet for p or higher orbital";
      throw runtime_error(msg);
    }		  

    delete[] il_ipl;
    delete[] ylm;
    
  }      
  void SymGTOs::AtR_Ylm(int L, int M, int irrep, const VectorXcd& cs_ibasis,
			const VectorXcd& rs,
			VectorXcd* res_vs, VectorXcd* res_dvs ) {

    if(not setupq) 
      this->SetUp();

    if(!is_lm_pair(L, M)) {
      string msg; SUB_LOCATION(msg);
      msg += ": invalid L,M pair";
      throw runtime_error(msg);
    }

    try{
      sym_group->CheckIrrep(irrep);
    } catch(const runtime_error& e) {
      string msg; SUB_LOCATION(msg);
      msg += ": Invalid irreq.\n";
      msg += e.what(); throw runtime_error(msg);
    }

    if(cs_ibasis.size() != this->size_basis_isym(irrep)) {
      string msg; SUB_LOCATION(msg);
      ostringstream oss;
      oss << msg << endl << ": size of cs must be equal to basis size" << endl;
      oss << "(L, M, irrep)     = " << L << M << irrep << endl;
      oss << "csibasis.size()   = " << cs_ibasis.size() << endl;
      oss << "size_basis_isym() = " << this->size_basis_isym(irrep) << endl;
      oss << endl;
      oss << "print SymGTOs:" << endl;
      oss << this->str();
      
      throw runtime_error(oss.str());
    }

    VectorXcd vs  = VectorXcd::Zero(rs.size());   // copy
    VectorXcd dvs = VectorXcd::Zero(rs.size());   // copy
    dcomplex* ylm = new dcomplex[num_lm_pair(L)];
    dcomplex* il  = new dcomplex[2*L+2];
    double eps(0.0000001);

    // Y00   = 1/sqrt(4pi)
    // r Y10 = sqrt(3/4pi) z
    for(SubIt isub = subs.begin(); isub != subs.end(); ++isub) {
      for(RdsIt irds = isub->rds.begin(); irds != isub->rds.end(); ++irds) {
	if(irds->irrep != irrep) 
	  continue;
	
	for(int iz = 0; iz < isub->size_zeta(); iz++) {
	  for(int iat = 0; iat < isub->size_at(); iat++) {
	    for(int ipn = 0; ipn < isub->size_pn(); ipn++) {
	      for(int ir = 0; ir < rs.size(); ir++) {
		int ibasis = irds->offset + iz;
		dcomplex c = (cs_ibasis(ibasis) *
			      irds->coef_iat_ipn(iat, ipn) *
			      irds->coef_iz(iz));
		dcomplex v, dv;
		if(IsCenter(isub, iat, eps)) {
		  AtR_Ylm_cen(isub, iz, ipn, rs[ir], L, M, &v, &dv);
		} else {
		  AtR_Ylm_noncen(isub, iz, iat, ipn, rs[ir], L, M, &v, &dv);
		}
		vs[ir] += c * v; dvs[ir]+= c * dv;
	      }
	    }
	  }
	}
      }
    }
    delete[] ylm;
    delete[] il;
    res_vs->swap(vs);
    res_dvs->swap(dvs);
  }

  // ---- Correct Sign ----
  void SymGTOs::CorrectSign(int L, int M, int irrep, Eigen::VectorXcd& cs) {
			    

    if(not setupq) {
      string msg; SUB_LOCATION(msg);
      msg += ": call SetUp() before calculation.";
      throw runtime_error(msg);
    }

    if(!is_lm_pair(L, M)) {
      string msg; SUB_LOCATION(msg);
      msg += ": invalid L,M pair";
      throw runtime_error(msg);
    }

    try{
      sym_group->CheckIrrep(irrep);
    } catch(const runtime_error& e) {
      string msg; SUB_LOCATION(msg);
      msg += ": Invalid irreq.\n";
      msg += e.what(); throw runtime_error(msg);
    }

    if(cs.size() != this->size_basis_isym(irrep)) {
      string msg; SUB_LOCATION(msg);
      msg += ": size of cs must be equal to basis size";
      throw runtime_error(msg);
    }

    VectorXcd rs(1); rs << 0.0;
    VectorXcd vs, ds;
    this->AtR_Ylm(L, M, irrep, cs, rs, &vs, &ds);

    if(ds(0).real() < 0) {
      cs = -cs;
    }
  }
  
}

