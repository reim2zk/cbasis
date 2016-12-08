#include <iostream>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <gtest/gtest.h>
#include "../utils/fact.hpp"
#include "../utils/gtest_plus.hpp"
#include "b2eint.hpp"
#include "one_int.hpp"
#include "two_int.hpp"
#include "symmolint.hpp"
#include "timer.hpp"

using namespace std;
using namespace Eigen;
using namespace cbasis;
using namespace boost::assign;
/*
TEST(Many, Many) {
  
  pSymmetryGroup D2h = SymmetryGroup::D2h();  

  VectorXcd zeta(20);
  zeta << 0.01,0.0177160054,0.0313856847, 0.0556028960, 0.0985061205, 0.174513496,
    0.309168204, 0.547722558, 0.970345578, 1.71906475, 3.04549604, 5.39540243,
    9.55849785, 16.9338400, 30.0, 
    dcomplex(0.0812755955262, -0.0613237786222),
    dcomplex(0.00497147387796, -0.0113737972763),
    dcomplex(0.0323712622673, -0.0451300037076),
    dcomplex(0.00317417887792, -0.022582254987),
    dcomplex(0.0118391719646, -0.0327847352576);

  SymGTOs gtos(new _SymGTOs); gtos->SetSym(D2h);

  SubSymGTOs sub_x;
  sub_x.AddXyz(Vector3cd(0, 0, 0));
  sub_x.AddNs( Vector3i( 1, 0, 0));
  sub_x.AddRds(Reduction(D2h->irrep_x, MatrixXcd::Ones(1, 1)));
  sub_x.AddZeta(zeta);
  gtos->AddSub(sub_x);

  SubSymGTOs sub_y;
  sub_y.AddXyz(Vector3cd(0, 0, 0));
  sub_y.AddNs( Vector3i( 0, 1, 0));
  sub_y.AddRds(Reduction(D2h->irrep_y, MatrixXcd::Ones(1, 1)));
  sub_y.AddZeta(zeta);
  gtos->AddSub(sub_y);
    
  SubSymGTOs sub_z;
  sub_z.AddXyz(Vector3cd(0, 0, 0));
  sub_z.AddNs( Vector3i( 0, 0, 1));
  sub_z.AddRds(Reduction(D2h->irrep_z, MatrixXcd::Ones(1, 1)));
  sub_z.AddZeta(zeta);
  gtos->AddSub(sub_z);  
  
  gtos->SetUp();

  ERIMethod m; m.symmetry=1; m.coef_R_memo=2; 
  B2EInt eri = CalcERI_Complex(gtos, m);
}
*/







TEST(SymGTOsMatrix, OneIntNewOld) {

  // ==== Symmetry ====
  SymmetryGroup D2h = SymmetryGroup_D2h();

  // ==== Molecule ====
  Molecule mole = NewMolecule(D2h);
  mole
    ->Add(NewAtom("H", 1.0)->Add(0,0,0.7))
    ->Add(NewAtom("Cen", 0.0)->Add(0,0,0))
    ->SetSymPos();
  EXPECT_EQ(3, mole->size());

  // ==== Sub ====
// ==== GTOs ====
  SymGTOs gtos = NewSymGTOs(mole);
  VectorXcd z1(4); z1 << 2.013, 0.1233, 0.0411, 0.0137;
  MatrixXcd c1_1(2, 1); c1_1 <<+1.0,+1.0;
  MatrixXcd c1_2(2, 1); c1_2 <<+1.0,-1.0;  
  gtos->NewSub("H")
    .AddNs( 0, 0, 0)
    .AddRds(Reduction(D2h->irrep_s(), c1_1))
    .AddRds(Reduction(D2h->irrep_z(), c1_2))
    .AddConts_Mono(z1);
  VectorXcd z2(1); z2 << 1.0;
  MatrixXcd C2_1(2, 1); C2_1 << +1,-1;
  MatrixXcd C2_2(2, 1); C2_2 << +1,+1;
  gtos->NewSub("H")
    .AddNs( 0, 0, 1)
    .AddRds(Reduction(D2h->irrep_s(), C2_1))
    .AddRds(Reduction(D2h->irrep_z(), C2_2))
    .AddConts_Mono(z2);
  VectorXcd z3(1); z3 << dcomplex(0.011389, -0.002197);
  gtos->NewSub("Cen")
    .SolidSH_M(0, 0, z3);
  VectorXcd z4(1); z4 << dcomplex(5.063464, -0.024632);
  MatrixXcd C4_1(1, 3); C4_1 << -1,-1,+2; 
  gtos->NewSub("Cen")
    .AddNs(2, 0, 0)
    .AddNs(0, 2, 0)
    .AddNs(0, 0, 2)
    .AddRds(Reduction(D2h->irrep_s(), C4_1))
    .AddConts_Mono(z4);
  gtos->SetUp();

  BMatSet mat = CalcMat(gtos, gtos, true);
  BMat S,T,V,X,Y,Z,DX,DY,DZ;
  CalcSTVMat(gtos, gtos, &S, &T, &V);
  CalcDipMat(gtos, gtos, &X, &Y, &Z, &DX, &DY, &DZ);
  EXPECT_MATXCD_EQ(S(0,0), mat->GetMatrix("s", 0, 0));
  EXPECT_MATXCD_EQ(Z(0,D2h->irrep_z()),
		   mat->GetMatrix("z", 0, D2h->irrep_z()));

}
void test_SymGTOsOneInt(CartGTO a, Vector3cd at, CartGTO b) {
  /*  
  SymmetryGroup sym = SymmetryGroup_C1();

  Atom h = NewAtom("H", 1.0); h->Add(at);
  Molecule mole(new _Molecule(sym));
  mole->Add(h);

  SymGTOs gtos(new _SymGTOs(mole));

  SubSymGTOs sub_a(sym, h);
  sub_a.AddNs( Vector3i( a.nx,a.ny,a.nz));
  sub_a.AddRds(Reduction(sym->irrep_s(), MatrixXcd::Ones(1, 1)));
  VectorXcd zeta_a(1); zeta_a << a.zeta;
  sub_a.AddConts_Mono(zeta_a);
  gtos->AddSub(sub_a);
  
  SubSymGTOs sub_b(sym, h);
  sub_b.AddNs( b.nx,b.ny,b.nz);
  sub_b.AddRds(Reduction(sym->irrep_s(), MatrixXcd::Ones(1, 1)));
  VectorXcd zeta_b(1); zeta_b << b.zeta;
  sub_b.AddConts_Mono(zeta_b);
  gtos->AddSub(sub_b);

  gtos->SetUp();
  BMatSet mat = CalcMat_Complex(gtos, true);

  const MatrixXcd& S_sym  = mat->GetMatrix("s", 0, 0);
  MatrixXcd S_cart(2, 2);
  S_cart(0, 0) = SMatEle(a, a); S_cart(0, 1) = SMatEle(a, b);
  S_cart(1, 0) = SMatEle(b, a); S_cart(1, 1) = SMatEle(b, b);
  dcomplex c_sym = 1.0/sqrt(S_sym(0, 0) *S_sym( 1, 1));
  dcomplex c_cart= 1.0/sqrt(S_cart(0, 0)*S_cart(1, 1));
  dcomplex s_sym  = S_sym(0, 1)  * c_sym;
  dcomplex s_cart = S_cart(0, 1) * c_cart;
  EXPECT_C_EQ(s_cart, s_sym) << endl
			     << "S matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;
  dcomplex T_sym = mat->GetMatrix("t", 0, 0)(0, 1)*c_sym;
  dcomplex T_cart= TMatEle(a, b)*c_cart;
  EXPECT_C_EQ(T_cart, T_sym) << endl
			     << "T matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;
  dcomplex V_sym = mat->GetMatrix("v", 0, 0)(0, 1)*c_sym;
  dcomplex V_cart= VMatEle(a, at, b)*c_cart;
  EXPECT_C_EQ(V_cart, V_sym) << endl
			     << "V matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;

  dcomplex DX_sym = mat->GetMatrix("dx", 0, 0)(0, 1)*c_sym;
  dcomplex DX_cart= DXMatEle(a, b)*c_cart;
  EXPECT_C_EQ(DX_cart, DX_sym) << endl
			     << "Dx matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;
  dcomplex DY_sym = mat->GetMatrix("dy", 0, 0)(0, 1)*c_sym;
  dcomplex DY_cart= DYMatEle(a, b)*c_cart;
  EXPECT_C_EQ(DY_cart, DY_sym) << endl
			     << "Dy matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;
  dcomplex DZ_sym = mat->GetMatrix("dz", 0, 0)(0, 1)*c_sym;
  dcomplex DZ_cart= DZMatEle(a, b)*c_cart;
  EXPECT_C_EQ(DZ_cart, DZ_sym) << endl
			     << "Dz matrix" << endl
			     << "a: " << a.str() << endl
			     << "b: " << b.str() << endl;
  */
  
}
TEST(SymGTOsMatrix, OneInt) {
  
  /*
  CartGTO s0(0, 0, 0, 0.0, 0.0, +0.0, 1.336);
  CartGTO s1(0, 0, 0, 0.0, 0.0, -0.7, 1.336);
  CartGTO p0(0, 0, 1, 0.0, 0.0, +0.7, 1.0);
  CartGTO p1(0, 0, 1, 0.0, 0.0, -0.7, 1.0);
  dcomplex zeta_d(0.00256226, -0.01559939);
  CartGTO dx(2, 0, 0, 0.0, 0.0, 0.0, zeta_d);
  CartGTO dy(0, 2, 0, 0.0, 0.0, 0.0, zeta_d);
  CartGTO dz(0, 0, 2, 0.0, 0.0, 0.0, zeta_d);
  */
  
  /*
  try {
    test_SymGTOsOneInt(s0, Vector3cd(0, 0, 0), s0);
  } catch(runtime_error& e) {
    cout << "s0,s0" << endl;
    cout << e.what() << endl;
    throw runtime_error("exception");
  }    
  
  try {
    test_SymGTOsOneInt(s0, Vector3cd(0, 0, 0.35), s1);
  } catch(runtime_error& e) {
    cout << "s,s" << endl;
    cout << e.what() << endl;
    throw runtime_error("exception");
  }  
  try {
    test_SymGTOsOneInt(s0, Vector3cd(0, 0, 0.35), dz);
  } catch(runtime_error& e) {
    cout << "s,zz" << endl;
    cout << e.what() << endl;
    throw runtime_error("exception");
  }
  try {
    test_SymGTOsOneInt(p0, Vector3cd(0, 0, 0.35), dz);
  } catch(runtime_error& e) {
    cout << "z,zz" << endl;
    cout << e.what() << endl;
  }
  test_SymGTOsOneInt(CartGTO(2, 1, 3, 0.1, 0.2, 0.3, dcomplex(1.0, -0.4)),
		     Vector3cd(-0.1, 0, 0.35),
		     CartGTO(0, 2, 2, 0.4, 0.3, 0.0, dcomplex(0.1, -0.1)));
  test_SymGTOsOneInt(p0, Vector3cd(0, 0, 0.7), dz);
 
 */
  
}

TEST_F(TestValue, TwoInt) {
  
  int ib,jb,kb,lb,i,j,k,l,t;
  dcomplex v;
  eri->Reset();
  while(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v)) {
    dcomplex cc = N[i]*N[j]*N[k]*N[l];
    dcomplex cr = 1.0/sqrt(S_ref(i, i) * S_ref(j, j) * S_ref(k, k) * S_ref(l, l));
    dcomplex eri_c = ERIEle(cart_gtos[i], cart_gtos[j], cart_gtos[k], cart_gtos[l]);
    EXPECT_C_EQ(cr*v, cc*eri_c) << i << j << k << l;
  }
    
}

void test_SymGTOsTwoInt(CartGTO a, CartGTO b, CartGTO c, CartGTO d) {

  // == Cart GTO ==
  CartGTO *cart_gtos[4];
  cart_gtos[0] = &a;
  cart_gtos[1] = &b;
  cart_gtos[2] = &c;
  cart_gtos[3] = &d;

  dcomplex c_cart(1);
  for(int i = 0; i < 4; i++) {
    CartGTO *o = cart_gtos[i];
    c_cart *= 1.0/sqrt(SMatEle(*o, *o));
  }
  dcomplex eri_cart = ERIEle(a, b, c, d) * c_cart;
  
  
  // == SymGTOs ==
  SymmetryGroup sym = SymmetryGroup_C1();
  Molecule mole(new _Molecule(sym));
  string names[4] = {"A", "B", "C", "D"};
  for(int i = 0; i < 4; i++) {
    CartGTO *o = cart_gtos[i];
    mole->Add(NewAtom(names[i], 0.0, Vector3cd(o->x, o->y, o->z)));
  }
  
  SymGTOs gtos(new _SymGTOs(mole));
  for(int i = 0; i < 4; i++) {
    SubSymGTOs sub(sym, mole->atom(names[i]));
    CartGTO *o = cart_gtos[i];
    sub.AddNs( Vector3i( o->nx,o->ny,o->nz));
    sub.AddRds(Reduction(sym->irrep_s(), MatrixXcd::Ones(1, 1)));
    VectorXcd zeta(1); zeta << o->zeta;
    sub.AddConts_Mono(zeta);
    gtos->AddSub(sub);
  }
  
  gtos->SetUp();
  BMatSet mat = CalcMat_Complex(gtos, false);
  const MatrixXcd& S_sym  = mat->GetMatrix("s", 0, 0);
  dcomplex c_sym(1);
  for(int i = 0; i < 4; i++) {
    c_sym *= 1.0/sqrt(S_sym(i, i));
  }

  ERIMethod m; m.symmetry = 1;
  B2EInt eri = CalcERI_Complex(gtos, m);
  dcomplex eri_sym  = eri->At(0, 0, 0, 0, 0, 1, 2, 3) * c_sym;  

  // == Compare ==
  EXPECT_C_EQ(eri_cart, eri_sym)
    << "a: " << a.str() << endl
    << "b: " << b.str() << endl
    << "c: " << c.str() << endl
    << "d: " << d.str() << endl;

}
TEST(SymGTOsMatrix, TwoInt) {

  CartGTO s0(0, 0, 0, 0.0, 0.0, +0.7, 1.336);
  CartGTO s1(0, 0, 0, 0.0, 0.0, -0.7, 1.336);
  CartGTO p0(0, 0, 1, 0.0, 0.0, +0.7, 1.0);
  CartGTO p1(0, 0, 1, 0.0, 0.0, -0.7, 1.0);
  dcomplex zeta_d(0.00256226, -0.01559939);
  CartGTO dx(2, 0, 0, 0.0, 0.0, 0.0, zeta_d);
  CartGTO dy(0, 2, 0, 0.0, 0.0, 0.0, zeta_d);
  CartGTO dz(0, 0, 2, 0.0, 0.0, 0.0, zeta_d);
  CartGTO fff(1, 1, 2, 0.1, 0.2, 0.3, dcomplex(1.0, -0.4));

  test_SymGTOsTwoInt(s0, s0, s0, s0);
  test_SymGTOsTwoInt(s0, s1, p0, p1);
  test_SymGTOsTwoInt(s0, dz, dx, p1);
  test_SymGTOsTwoInt(s0, dz, fff, p1);
  
}

TEST(CompareCColumbus, small_he) {

  dcomplex z1(0.09154356, -0.24865707);
  
  SymmetryGroup sym = SymmetryGroup_Cs();
  Molecule mole = NewMolecule(sym);
  Atom he = NewAtom("He", 2.0); he->Add(0,0,0);
  mole->Add(he);

  // sub set (S orbital)
  SubSymGTOs sub_s(sym, he);
  sub_s.AddNs(Vector3i(0, 0, 0));
  VectorXcd zeta_s(2); zeta_s << 0.107951, 3293.694;
  sub_s.AddConts_Mono(zeta_s);
  sub_s.AddRds(Reduction(sym->irrep_s(), MatrixXcd::Ones(1, 1)));

  // sub set (P orbital)
  SubSymGTOs sub_z(sym, he);
  sub_z.AddNs(Vector3i(0, 0, 1));
  VectorXcd zeta_z(1); zeta_z << z1;
  sub_z.AddConts_Mono(zeta_z);
  sub_z.AddRds(Reduction(sym->irrep_z(), MatrixXcd::Ones(1, 1)));

  // GTO set
  SymGTOs gtos(new _SymGTOs(mole));
  gtos->AddSub(sub_s);
  gtos->AddSub(sub_z);
  gtos->SetUp();

  // compute basic matrix
  BMatSet mat_set = CalcMat_Complex(gtos, true);
  ERIMethod m; m.symmetry = 1;
  B2EInt eri = CalcERI_Complex(gtos, m);
  
  const MatrixXcd& S00 = mat_set->GetMatrix("s", 0, 0);
  EXPECT_C_EQ(dcomplex(1.00000000000000,0.000000000000000), S00(0, 0));
  EXPECT_C_EQ(dcomplex(0.001225127274895041, 0.000000000000000), S00(0, 1));
  EXPECT_C_EQ(dcomplex(0.001225127274895041, 0.000000000000000), S00(1, 0));
  EXPECT_C_EQ(1.0, S00(1, 1));
  const MatrixXcd& S11 = mat_set->GetMatrix("s", 1, 1);
  EXPECT_C_EQ(1.0, S11(0, 0));
  
  const MatrixXcd& T00 = mat_set->GetMatrix("t", 0, 0);
  EXPECT_C_EQ(0.1619265, T00(0, 0));
  EXPECT_C_EQ(0.0003967481399147181, T00(0, 1));
  EXPECT_C_EQ(0.0003967481399147181, T00(1, 0));
  EXPECT_C_EQ(4940.54100000000, T00(1, 1));
  const MatrixXcd& T11 = mat_set->GetMatrix("t", 1, 1);  
  EXPECT_C_EQ(dcomplex(0.2288589, -0.621642675), T11(0, 0));

  const MatrixXcd& V00 = mat_set->GetMatrix("v", 0, 0);
  EXPECT_C_EQ(-1.04860853360520,  V00(0, 0));
  EXPECT_C_EQ(-0.158677374090625, V00(0, 1));
  EXPECT_C_EQ(0.0003967481399147181, T00(1, 0));
  EXPECT_C_EQ(-183.164657050577, V00(1, 1));
  const MatrixXcd& V11 = mat_set->GetMatrix("v", 1, 1);  
  EXPECT_C_EQ(dcomplex(-0.898325031872102,0.626548799637203), V11(0, 0));

  EXPECT_C_EQ(eri->At(1,  1,  1,  1,  0,  0,  0,  0),
	      dcomplex(0.389067179569661,  -0.271360104292752));
  EXPECT_C_EQ(eri->At(1, 1, 0, 0,  0, 0, 0, 0),
	      dcomplex(0.431669280913818,  -0.113052122000587));
  EXPECT_C_EQ(eri->At(1, 0, 1, 0,  0, 0, 0, 0),
	      dcomplex(0.148545492311628,  0.012837791008275));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0,  0, 0, 0, 0),
	      dcomplex(0.370739102461159,   0.000000000000000));
  EXPECT_C_EQ(eri->At(1, 1, 0, 0, 0, 0, 1, 0),
	      dcomplex(0.000550281255615,  -0.000383801011115));
  EXPECT_C_EQ(eri->At(1, 0, 1, 0,  0, 1, 0, 0),
	      dcomplex(-0.000000049115720,  -0.000000075073399));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0, 1, 0, 0, 0),
	      dcomplex(0.000642318406618,   0.000000000000000));
  EXPECT_C_EQ(eri->At(1, 1, 0, 0, 0, 0, 1, 1),
	      dcomplex( 0.449162517258858,  -0.313274399690404));
  EXPECT_C_EQ(eri->At(1, 0, 1, 0, 0, 1, 0, 1),
	      dcomplex(-0.000000007054518,  -0.000000000684658));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0, 1, 1, 0, 0),
	      dcomplex( 0.524295674963366,   0.000000000000000));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0, 1, 0, 1, 0),
	      dcomplex(0.000068730771674,   0.000000000000000));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0, 1, 1, 1, 0),
	      dcomplex( 0.064779412850629,   0.000000000000000));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0, 1, 1, 1, 1),
	      dcomplex(64.758485537085718,   0.000000000000000));
	   
}

class TestB2EInt :public ::testing::Test {
public:
  B2EInt eri;
public:
  TestB2EInt() {
    eri = B2EInt(new B2EIntMem(100));
    eri->Set(1, 2, 3, 4,
	     5, 6, 7, 8, 1.1);
    eri->Set(0, 0, 0, 1,
	     0, 0, 3, 0, 1.2);
    eri->Set(0, 2, 0, 0,
	     0, 0, 1, 0, 1.3);
    eri->Set(1, 0, 0, 0,
	     0, 1, 0, 0, 1.4);
  }
  ~TestB2EInt() {
  }
};
TEST_F(TestB2EInt, Get) {
  int ib,jb,kb,lb,i,j,k,l,t;
  dcomplex v;
  EXPECT_TRUE(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v));
  EXPECT_EQ(1, ib);
  EXPECT_EQ(2, jb);
  EXPECT_EQ(3, kb);
  EXPECT_EQ(4, lb);
  EXPECT_EQ(5, i);
  EXPECT_EQ(6, j);
  EXPECT_EQ(7, k);
  EXPECT_EQ(8, l);
  EXPECT_EQ(0, t);
  EXPECT_C_EQ(1.1, v);
  
  EXPECT_TRUE(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v));
  EXPECT_EQ(k, 3);
  EXPECT_TRUE(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v));
  EXPECT_EQ(jb, 2);
  EXPECT_TRUE(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v));
  EXPECT_EQ(v, 1.4);
  EXPECT_FALSE(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v));
  
  eri->Reset();
  while(eri->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v)) {
    cout << ib << jb << kb << lb << endl;
  }
}
TEST_F(TestB2EInt, At) {

  dcomplex v;
  v = eri->At(1, 2, 3, 4, 5, 6, 7, 8);
  EXPECT_C_EQ(1.1, v);

  EXPECT_ANY_THROW(eri->At(0, 0, 0, 0, 1, 1, 1, 1));
}
TEST_F(TestB2EInt, size) {
  EXPECT_EQ(4, eri->size());
  EXPECT_EQ(100, eri->capacity());
}
TEST_F(TestB2EInt, IO) {

  string fn("eri.bin");
  eri->Write(fn);

  B2EInt eri2 = ERIRead(fn);

  EXPECT_C_EQ(eri->At( 1, 2, 3, 4, 5, 6, 7, 8),
	      eri2->At(1, 2, 3, 4, 5, 6, 7, 8));
  EXPECT_C_EQ(eri->At( 0, 0, 0, 1, 0, 0, 3, 0),
	      eri2->At(0, 0, 0, 1, 0, 0, 3, 0));
  EXPECT_C_EQ(eri->At( 0, 2, 0, 0, 0, 0, 1, 0),
	      eri2->At(0, 2, 0, 0, 0, 0, 1, 0));
  EXPECT_C_EQ(eri->At( 1, 0, 0, 0, 0, 1, 0, 0),
	      eri2->At(1, 0, 0, 0, 0, 1, 0, 0));
  
}

TEST(coef_R, method1) {

  static const int nn(6);
  dcomplex Fjs[nn];
  Fjs[0] = 0.1;  Fjs[1] = 0.2; Fjs[2] = 0.3;
  Fjs[3] = 0.25; Fjs[4] = 0.7; Fjs[5] = 0.6;

  dcomplex zeta(0.1, 0.001);
  MultArray<dcomplex, 3> res0(1000); ERIMethod m0;
  MultArray<dcomplex, 3> res1(1000); ERIMethod m1; m1.coef_R_memo = 1;

  try {
    coef_R_eri_switch(zeta, 0.1, 0.2, 0.3, 0.11, 0.22, 0.33, 2, Fjs, 1, res0, m0);
  } catch(runtime_error& e) {
    cout << "exception on non speedy method" << endl;
    cout << e.what() << endl;
  }
  try {
    coef_R_eri_switch(zeta, 0.1, 0.2, 0.3, 0.11, 0.22, 0.33, 2, Fjs, 1, res1, m1);
  } catch(runtime_error& e) {
    cout << "exception on memorize method" << endl;
    cout << e.what() << endl;
  }

  
  for(int i = 0; i < 2; i++)
    for(int j = 0; j < 2; j++)
      EXPECT_C_EQ(res0(i, j, 0), res1(i, j, 0)) << i << j;

}
TEST(SymGTOs, CalcERI_differenct) {

  VectorXcd zeta = VectorXcd::Zero(2);
  zeta << dcomplex(0.4, 0.1), dcomplex(1.4, 0.6);
  
  SymmetryGroup Cs = SymmetryGroup_Cs();
  Molecule mole = NewMolecule(Cs);
  mole
    ->Add(NewAtom("CEN", 0.0)->Add(0,0,0))
    ->Add(NewAtom("H",   1.0)->Add(0,0,1)->Add(0,0,1));
  
  SymGTOs gtos_1 = NewSymGTOs(mole);
  gtos_1->NewSub("CEN").SolidSH_M(0, 0, zeta);
  gtos_1->SetUp();

  SymGTOs gtos_2 = NewSymGTOs(mole);
  gtos_2->NewSub("CEN").SolidSH_M(1, 0, zeta);
  gtos_2->SetUp();

  SymGTOs gtos_3 = NewSymGTOs(mole);
  gtos_3->NewSub("H")
    .AddNs(0,0,0)
    .AddRds(Reduction(0, MatrixXcd::Ones(2,1)))
    .AddZeta(zeta);
  gtos_3->SetUp();
  
  // -- Compute ERI --
  ERIMethod method; method.perm = 0; method.symmetry = 0;
  B2EInt eri_0 = CalcERI(gtos_1, gtos_3, gtos_2, gtos_2, method);
  ERIMethod method1; method1.perm = 0; method1.symmetry = 0; method1.coef_R_memo=1;
  B2EInt eri_1 = CalcERI(gtos_1, gtos_3, gtos_2, gtos_2, method1);

  for(int i = 0; i < 2; i++)
    for(int j = 0; j < 2; j++)
      for(int k = 0; k < 2; k++)
	for(int l = 0; l < 2; l++) {
	  EXPECT_C_EQ(eri_0->At(0, 0, 1, 1, i, j, k, l),
		      eri_1->At(0, 0, 1, 1, i, j, k, l)) << i<<j<<k<<l;
	}

}
VectorXcd OneVec(dcomplex z) {
  VectorXcd zs(1); zs << z;
  return zs;
}
TEST(SymGTOs, CalcERI) {

  SymmetryGroup C1 = SymmetryGroup_C1();
  Molecule mole = NewMolecule(C1);
  mole
    ->Add(NewAtom("A", 0.0, Vector3cd(0.0, 0.0,  0.4)))
    ->Add(NewAtom("B", 1.0, Vector3cd(0.0, 0.0,  0.0)))
    ->Add(NewAtom("C", 0.0, Vector3cd(0.0, -0.2, 0.4)))
    ->Add(NewAtom("D", 0.0, Vector3cd(0.2, 0.0,  0.1)));
  SymGTOs gtos = NewSymGTOs(mole);
  gtos->NewSub("A").SolidSH_M(0,0, OneVec(1.2));
  gtos->NewSub("B").SolidSH_M(0,0, OneVec(1.4));
  gtos->NewSub("C").SolidSH_M(0,0, OneVec(1.1));
  gtos->NewSub("D").SolidSH_M(0,0, OneVec(1.0));
  gtos->SetUp();

  // -- A --
  /*
  gtos->AddSub(Sub_s(0, Vector3cd(0.0, 0.0,  0.4), OneVec(1.2)));
  gtos->AddSub(Sub_s(0, Vector3cd(0.0, 0.0,  0.0), OneVec(1.4)));
  gtos->AddSub(Sub_s(0, Vector3cd(0.0, -0.2, 0.0), OneVec(1.1)));
  gtos->AddSub(Sub_s(0, Vector3cd(0.2, 0.0,  0.1), OneVec(1.0)));
  */

  // -- Calculation --
  ERIMethod method; 
  B2EInt eri = CalcERI_Complex(gtos, method);

  // -- size check --
  EXPECT_EQ(ipow(4, 4), eri->size());

  // -- Symmetry check --
  // eri is chemist's notation.
  EXPECT_C_EQ(eri->At(0, 0, 0, 0,  0, 1, 2, 3),
	      eri->At(0, 0, 0, 0,  1, 0, 2, 3));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0,  0, 1, 2, 3),
	      eri->At(0, 0, 0, 0,  0, 1, 3, 2));
  EXPECT_C_EQ(eri->At(0, 0, 0, 0,  0, 1, 2, 3),
	      eri->At(0, 0, 0, 0,  2, 3, 0, 1));

  // -- copied from neo_ccol/aoint/gto_utest.cpp --
  // (00|00): (1.23607744235395,0)
  // (01|23): (1.01774464000324,0)
  // (00|11): (1.03695966484363,0)
  // (00|01): (1.11023051353687,0)
  // (00|12): (0.992414842562312,0)

  double eps(pow(10.0, -8.0));
  dcomplex ref0000(1.23607744235395, 0);
  dcomplex ref0123(1.01774464000324, 0);
  dcomplex ref0011(1.03695966484363,0);
  dcomplex ref0001(1.11023051353687,0);
  dcomplex ref0012(0.992414842562312,0);
  
  EXPECT_C_NEAR(ref0000, eri->At(0, 0, 0, 0, 0, 0, 0, 0), eps);
  EXPECT_C_NEAR(ref0123, eri->At(0, 0, 0, 0, 0, 2, 1, 3), eps);
  EXPECT_C_NEAR(ref0011, eri->At(0, 0, 0, 0, 0, 1, 0, 1), eps);
  EXPECT_C_NEAR(ref0001, eri->At(0, 0, 0, 0, 0, 0, 0, 1), eps);
  EXPECT_C_NEAR(ref0012, eri->At(0, 0, 0, 0, 0, 1, 0, 2), eps);

}
TEST(SymGTOs, CalcERI2) {

  SymmetryGroup C1 = SymmetryGroup_C1();
  Molecule mole = NewMolecule(C1);
  mole
    ->Add(NewAtom("A", 0, Vector3cd(0.0, 0.0,  0.4)))
    ->Add(NewAtom("B", 0, Vector3cd(0.0, 0.0,  0.0)))
    ->Add(NewAtom("C", 0, Vector3cd(0.0, -0.2, 0.0)))
    ->Add(NewAtom("D", 0, Vector3cd(0.2, 0.0,  0.1)));
  SymGTOs gtos = NewSymGTOs(mole);
  gtos->NewSub("A").Mono(0, Vector3i(0,1,0), OneVec(1.2));
  gtos->NewSub("B").Mono(0, Vector3i(1,1,0), OneVec(1.4));
  gtos->NewSub("C").Mono(0, Vector3i(1,1,1), OneVec(1.1));
  gtos->NewSub("D").Mono(0, Vector3i(0,3,0), OneVec(1.0));
  gtos->SetUp();
  
  /*
    SubSymGTOs s1;
  s1.AddXyz(Vector3cd(0.0, 0.0, 0.4));
  s1.AddNs( Vector3i( 0,   1,   0));
  VectorXcd z1(1); z1 << 1.2; s1.AddZeta(z1);
  s1.AddRds(Reduction(0, MatrixXcd::Ones(1, 1)));

  gtos->AddSub(Sub_mono(0, Vector3cd(0.0, 0.0,  0.4),
		       Vector3i(0, 1, 0), OneVec(1.2)));
  gtos->AddSub(Sub_mono(0, Vector3cd(0.0, 0.0,  0.0),
		       Vector3i(1, 1, 0), OneVec(1.4)));
  gtos->AddSub(Sub_mono(0, Vector3cd(0.0, -0.2, 0.0),
		       Vector3i(1, 1, 1), OneVec(1.1)));
  gtos->AddSub(Sub_mono(0, Vector3cd(0.2, 0.0,  0.1),
		       Vector3i(0, 3, 0), OneVec(1.0)));

  */

  // -- Calculation --
  B2EInt eri = CalcERI_Complex(gtos, ERIMethod());

  // -- size check --
  //  cout << 1 << endl;
  //  EXPECT_EQ(pow(4, 4), eri->size());

  // -- reference --
  // (00|00): (1.00946324498146,0)
  // (00|11): (0.133478467219949,0)
  // (00|01): (0,0)
  // (00|12): (0.0481552620386253,3.14588102745493e-19)
  // (01|23): (0.0240232215271162,1.00002719131888e-18)

  double eps(pow(10.0, -8.0));
  EXPECT_C_NEAR(1.00946324498146,  eri->At(0, 0, 0, 0, 0, 0, 0, 0), eps);
  EXPECT_C_NEAR(0.133478467219949, eri->At(0, 0, 0, 0, 0, 1, 0, 1), eps);
  EXPECT_C_NEAR(0.0, eri->At(0, 0, 0, 0, 0, 0, 0, 1), eps);
  EXPECT_C_NEAR(0.0481552620386253, eri->At(0, 0, 0, 0, 0, 1, 0, 2), eps);
  EXPECT_C_NEAR(0.0240232215271162, eri->At(0, 0, 0, 0, 0, 2, 1, 3), eps);

}
TEST(SymGTOs, CalcERI_sym_p) {

  // ==== Symmetry ====
  SymmetryGroup D2h = SymmetryGroup_D2h();
  
  // ==== Molecule ====
  Molecule mole = NewMolecule(D2h);
  mole
    ->Add(NewAtom("H", 1.0)->Add(0,0,0.7))
    ->Add(NewAtom("Cen", 0.0)->Add(0,0,0))
    ->SetSymPos();

// ==== GTOs ====
  SymGTOs gtos = NewSymGTOs(mole);
  VectorXcd z1(4); z1 << 2.013, 0.1233, 0.0411, 0.0137;
  MatrixXcd c1_1(2, 1); c1_1 <<+1.0,+1.0;
  MatrixXcd c1_2(2, 1); c1_2 <<+1.0,-1.0;  
  gtos->NewSub("H")
    .AddNs( 0, 0, 0)
    .AddRds(Reduction(D2h->irrep_s(), c1_1))
    .AddRds(Reduction(D2h->irrep_z(), c1_2))
    .AddZeta(z1);
  VectorXcd z2(1); z2 << 1.0;
  MatrixXcd C2_1(2, 1); C2_1 << +1,-1;
  MatrixXcd C2_2(2, 1); C2_2 << +1,+1;
  gtos->NewSub("H")
    .AddNs( 0, 0, 1)
    .AddRds(Reduction(D2h->irrep_s(), C2_1))
    .AddRds(Reduction(D2h->irrep_z(), C2_2))
    .AddZeta(z2);
  VectorXcd z3(1); z3 << dcomplex(0.011389, -0.002197);
  gtos->NewSub("Cen")
    .SolidSH_M(0, 0, z3);
  VectorXcd z4(1); z4 << dcomplex(5.063464, -0.024632);
  MatrixXcd C4_1(1, 3); C4_1 << -1,-1,+2; 
  gtos->NewSub("Cen")
    .AddNs(2, 0, 0)
    .AddNs(0, 2, 0)
    .AddNs(0, 0, 2)
    .AddRds(Reduction(D2h->irrep_s(), C4_1))
    .AddZeta(z4);
  gtos->SetUp();

  ERIMethod method0; 
  ERIMethod method1; method1.symmetry = 1;

  B2EInt eri0 = CalcERI_Complex(gtos, method0);
  B2EInt eri1 = CalcERI_Complex(gtos, method1);

  int ib,jb,kb,lb,i,j,k,l,t;
  dcomplex v;
  eri1->Reset();
  while(eri0->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v)) {
    if(abs(v) > 0.000001)
      EXPECT_C_EQ(v, eri1->At(ib, jb, kb, lb, i, j, k, l)) <<
	ib << jb << kb << lb << i << j << k << l;
  }

}
TEST(SymGTOs, CalcERI_sym) {
  
  /*
  SymGTOs gtos(new _SymGTOs);
  pSymmetryGroup sym = SymmetryGroup::D2h();
  gtos->SetSym(sym);

  Vector3cd xyz0(0.0, 0.0, 0.0);

  // ---- s orbital ----
  SubSymGTOs sub_s;
  sub_s.AddXyz(Vector3cd(0, 0, 0));
  sub_s.AddNs( Vector3i( 0, 0, 0));
  int num_z(1);
  VectorXcd zs(num_z); zs << 1.1;
  sub_s.AddZeta(zs);
  sub_s.AddRds(Reduction(sym->irrep_s, MatrixXcd::Ones(1, 1)));
  gtos->AddSub(sub_s);

  // ---- p orbital ----
  SubSymGTOs sub_p;
  sub_p.AddXyz(Vector3cd(0, 0, 0));
  sub_p.AddNs( Vector3i( 1, 0, 0));
  sub_p.AddNs( Vector3i( 0, 1, 0));
  sub_p.AddNs( Vector3i( 0, 0, 1));
  VectorXcd zs_p(1); zs_p << 1.1; sub_p.AddZeta(zs_p);
  MatrixXcd c1(1, 3); c1 << 1.0, 0.0, 0.0; sub_p.AddRds(Reduction(sym->irrep_x, c1));
  MatrixXcd c2(1, 3); c2 << 0.0, 1.0, 0.0; sub_p.AddRds(Reduction(sym->irrep_y, c2));
  MatrixXcd c3(1, 3); c3 << 0.0, 0.0, 1.0; sub_p.AddRds(Reduction(sym->irrep_z, c3));
  gtos->AddSub(sub_p);

  // -- potential --
  MatrixXcd xyzq(4, 1); xyzq << 0.0, 0.0, 0.0, 1.0;
  gtos->SetAtoms(xyzq);
  gtos->SetUp();  

  BMatSet mat = CalcMat_Complex(gtos, true);

  //int n(gtos->size_basis());
  ERIMethod m0; 
  ERIMethod m1; m1.symmetry = 1;
  B2EInt eri0 = CalcERI_Complex(gtos, m0);
  B2EInt eri1 = CalcERI_Complex(gtos, m1);

  int ib,jb,kb,lb,i,j,k,l,t;
  dcomplex v;
  eri1->Reset();
  while(eri0->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v)) {
    if(eri1->Exist(ib, jb, kb, lb, i, j, k, l)) {
      EXPECT_C_EQ(v, eri1->At(ib, jb, kb, lb, i, j, k, l)) <<
	ib << jb << kb << lb << " : " << i << j << k << l;
    } else {
      EXPECT_TRUE(abs(v) < 0.000001) <<
	v << " : " <<
	ib << jb << kb << lb << " : " <<
	i << j << k << l;
    }

  }
  */
}
TEST(SymGTOs, method_time) {

  Timer timer;
  SymmetryGroup sym = SymmetryGroup_D2h();
  Molecule mole = NewMolecule(sym);
  mole->Add(NewAtom("H", 0.0, Vector3cd(0,0,0)));
  SymGTOs gtos = NewSymGTOs(mole);

  int num_z(10);
  VectorXcd zs(num_z); zs << 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0;
  gtos->NewSub("H").SolidSH_M(0,0, zs);

  // ---- p orbital ----
  int num_z_p(8);
  VectorXcd zs_p(num_z_p); zs_p << 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8;
  VectorXi Ms(3); Ms << -1,0,1;
  gtos->NewSub("H").SolidSH_Ms(0, Ms, zs_p);

  ERIMethod m00; 
  ERIMethod m01; m01.symmetry = 1;
  ERIMethod m10; m10.coef_R_memo = 1;
  ERIMethod m11; m11.coef_R_memo = 1; m11.symmetry = 1;
  B2EInt eri00, eri01, eri10, eri11;

  timer.Start("method00"); eri00 = CalcERI_Complex(gtos, m00); timer.End("method00");
  timer.Start("method01"); eri01 = CalcERI_Complex(gtos, m01); timer.End("method01");
  timer.Start("method10"); eri10 = CalcERI_Complex(gtos, m10); timer.End("method10");
  timer.Start("method11"); eri11 = CalcERI_Complex(gtos, m11); timer.End("method11");
    
  EXPECT_C_EQ(eri00->At(0, 0, 0, 0, 2, 1, 5, 0),
	      eri01->At(0, 0, 0, 0, 2, 1, 5, 0));
  EXPECT_C_EQ(eri00->At(0, 0, 0, 0, 2, 1, 5, 0),
	      eri11->At(0, 0, 0, 0, 2, 1, 5, 0));
  EXPECT_C_EQ(eri00->At(0, 0, 0, 0, 2, 1, 5, 0),
	      eri10->At(0, 0, 0, 0, 2, 1, 5, 0));

  timer.Display();
  cout << "size(eri00): " << eri00->size() << endl;
  cout << "size(eri10): " << eri10->size() << endl;
  cout << "size(eri01): " << eri01->size() << endl;
  cout << "size(eri11): " << eri11->size() << endl;

}
TEST(SymGTOs, method_check) {


    SymmetryGroup sym = SymmetryGroup_D2h();
  Molecule mole = NewMolecule(sym);
  mole->Add(NewAtom("H", 0.0, Vector3cd(0,0,0)));
  SymGTOs gtos = NewSymGTOs(mole);

  int num_z(2);
  VectorXcd zs(num_z); zs << 1.1, 1.2;
  gtos->NewSub("H").SolidSH_M(0,0, zs);

  int num_z_p(2);
  VectorXcd zs_p(num_z_p); zs_p << 1.7, 1.8;
  VectorXi Ms(3); Ms << -1,0,1;
  gtos->NewSub("H").SolidSH_Ms(0, Ms, zs_p);

  gtos->SetUp();
  
  // -- compute --
  ERIMethod m00;                                    
  ERIMethod m10; m10.symmetry = 1;                  
  ERIMethod m01; m01.coef_R_memo = 1;               
  ERIMethod m11; m11.symmetry=1; m11.coef_R_memo=2; 
  
  B2EInt eri00 = CalcERI_Hermite(gtos, m00);
  B2EInt eri10 = CalcERI_Hermite(gtos, m10);
  B2EInt eri01 = CalcERI_Hermite(gtos, m01);
  B2EInt eri11 = CalcERI_Hermite(gtos, m11);
  m11.coef_R_memo = 1; m11.symmetry = 1;

  int ib,jb,kb,lb,i,j,k,l,t;
  dcomplex v;
  eri00->Reset();
  while(eri00->Get(&ib,&jb,&kb,&lb,&i,&j,&k,&l, &t, &v)) {
    if(eri10->Exist(ib, jb, kb, lb, i, j, k, l)) {
      EXPECT_C_EQ(v, eri10->At(ib, jb, kb, lb, i, j, k, l)) <<
	ib << jb << kb << lb << " : " << i << j << k << l;
    } else {
      EXPECT_TRUE(abs(v) < 0.000001) <<
	v << " : " <<
	ib << jb << kb << lb << " : " <<
	i << j << k << l;
    }

    if(eri01->Exist(ib, jb, kb, lb, i, j, k, l)) {
      EXPECT_C_EQ(v, eri01->At(ib, jb, kb, lb, i, j, k, l)) <<
	ib << jb << kb << lb << " : " << i << j << k << l;
    } else {
      EXPECT_TRUE(abs(v) < 0.000001) <<
	v << " : " <<
	ib << jb << kb << lb << " : " <<
	i << j << k << l;
    }
    if(eri11->Exist(ib, jb, kb, lb, i, j, k, l)) {
      EXPECT_C_EQ(v, eri11->At(ib, jb, kb, lb, i, j, k, l)) <<
	ib << jb << kb << lb << " : " << i << j << k << l;
    } else {
      EXPECT_TRUE(abs(v) < 0.000001) <<
	v << " : " <<
	ib << jb << kb << lb << " : " <<
	i << j << k << l;
    }

  }

}
TEST(Time, MatrixAccess) {
  int n(1000);
  MatrixXcd a(n, n);
  dcomplex *b = new dcomplex[n*n];
  
  
  Timer timer;

  timer.Start("Eigen");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      a(i, j) = 1.2;
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      a(i, j);
  timer.End("Eigen");

  timer.Start("Array");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      b[i+n*j] = 1.2;
  dcomplex cumsum(0);
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      cumsum += b[i+n*j];
  timer.End("Array");

  timer.Start("Array2");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      b[j+n*i] = 1.2;
  cumsum = 0;
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++)
      cumsum += b[j+n*i];
  timer.End("Array2");
  timer.Display();

  Timer timer1;
  MatrixXcd xyz(3, 2);
  timer1.Start("MatrixXyz");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      xyz(0, 0) = 1.0;
      xyz(0, 1) = 1.2;
    }
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      dcomplex x= xyz(0, 0);
      x =  xyz(0, 1);
    }
  timer1.End("MatrixXyz");

  dcomplex xs[2];
  timer1.Start("ArrayXyz");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      xs[0]= 1.0*i*j;
      xs[1] = 1.2*i*j;
    }
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      dcomplex x= xs[0];
      x =  xs[1];
    }
  timer1.End("ArrayXyz");

  vector<dcomplex> cs(2);
  timer1.Start("VectorXyz");
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      cs[0]= 1.0*i*j;
      cs[1] = 1.2*i*j;
    }
  for(int i = 0; i < n; i++)
    for(int j = 0; j < n; j++) {
      dcomplex x= cs[0];
      x =  cs[1];
    }
  timer1.End("VectorXyz");  
  
  timer1.Display();

}
  /*
TEST(H2mole, element) {

  // ==== Symmetry ====
  pSymmetryGroup D2h = SymmetryGroup::D2h();

  // ==== Sub ====
  SubSymGTOs sub1(D2h);
  sub1.AddXyz(Vector3cd(0, 0, +0.7));
  sub1.AddXyz(Vector3cd(0, 0, -0.7));
  sub1.AddNs( Vector3i( 0, 0, 0));
  VectorXcd z1(4); z1 << 2.013, 0.1233, 0.0411, 0.0137; sub1.AddZeta(z1);
  // VectorXcd z1(2); z1 << 0.1233, 0.0411; sub1.AddZeta(z1);
  MatrixXcd c1_1(2, 1); c1_1 <<+1.0,+1.0; sub1.AddRds(Reduction(0, c1_1));
  MatrixXcd c1_2(2, 1); c1_2 <<+1.0,-1.0; sub1.AddRds(Reduction(1, c1_2));
  sub1.SetUp();

  SubSymGTOs sub2(D2h);
  sub2.AddXyz(Vector3cd(0, 0, +0.7));
  sub2.AddXyz(Vector3cd(0, 0, -0.7));
  sub2.AddNs( Vector3i( 0, 0, 1));
  VectorXcd z2(1); z2 << 1.0; sub2.AddZeta(z2);
  MatrixXcd C2_1(2, 1); C2_1 << +1,-1; sub2.AddRds(Reduction(0, C2_1));
  MatrixXcd C2_2(2, 1); C2_2 << +1,+1; sub2.AddRds(Reduction(1, C2_2));
  sub2.SetUp();

  SubSymGTOs sub3(D2h);
  sub3.AddXyz(Vector3cd(0, 0, 0));
  sub3.AddNs( Vector3i( 0, 0, 0));
  VectorXcd z3(1); z3 << dcomplex(0.011389, -0.002197); sub3.AddZeta(z3); 
  MatrixXcd C3_1(1, 1); C3_1 << 1; sub3.AddRds(Reduction(0, C3_1));
  sub3.SetUp();
  
  SubSymGTOs sub4(D2h);
  sub4.AddXyz(Vector3cd(0, 0, 0));
  sub4.AddNs( Vector3i( 2, 0, 0));
  sub4.AddNs( Vector3i( 0, 2, 0));
  sub4.AddNs( Vector3i( 0, 0, 2));
  VectorXcd z4(1); z4 << dcomplex(5.063464, -0.024632); sub4.AddZeta(z4);
  MatrixXcd C4_1(1, 3); C4_1 << -1,-1,+2; sub4.AddRds(Reduction(0, C4_1 ));
  sub4.SetUp();

  // ==== GTOs ====
  SymGTOs gtos(D2h);
  gtos.AddSub(sub1); gtos.AddSub(sub2); gtos.AddSub(sub3); gtos.AddSub(sub4);
  MatrixXcd xyzq(4, 2); xyzq <<
			  0,    0,
			  0,    0,
			  +0.7, -0.7,
			  1.0,  1.0;
  gtos.SetAtoms(xyzq);
  gtos.SetUp();

  // ==== matrix evaluation ====
  //  BMatSet mat;
  //  gtos.CalcMat(&mat);
  IB2EInt *eri = new B2EIntMem(pow(gtos.size_basis(), 4));
  gtos.CalcERI(eri, 1);

  // copied from ~/calc/cCOLUMBUS
  //1  1  1  1  6  2  3  1        0.18344       -0.02309
  //  EXPECT_C_EQ(dcomplex(0.18344, -0.02309),
  //	      eri->At(0, 0, 0, 0, 5, 1, 2, 0));
}
*/

int main (int argc, char **args) {
  ::testing::InitGoogleTest(&argc, args);
  return RUN_ALL_TESTS();
}
