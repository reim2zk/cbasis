#include <iostream>
#include <gtest/gtest.h>
#include "gtest_plus.hpp"
#include "b2eint.hpp"
#include "symmolint.hpp"

using namespace std;
using namespace Eigen;
using namespace l2func;

TEST(first, first) {
  EXPECT_EQ(2, 1+1);
}
class TestB2EInt :public ::testing::Test {
public:
  IB2EInt *eri;
public:
  TestB2EInt() {
    eri = new B2EIntMem(100);
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
    delete eri;
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
VectorXcd OneVec(dcomplex z) {
  VectorXcd zs(1); zs << z;
  return zs;
}
TEST(SymGTOs, CalcERI) {
  Irrep Ap = Cs_Ap();
  SymGTOs gtos(SymmetryGroup_Cs());

  // -- A' --
  gtos.AddSub(Sub_s(Ap, Vector3cd(0.0, 0.0,  0.4), OneVec(1.2)));
  gtos.AddSub(Sub_s(Ap, Vector3cd(0.0, 0.0,  0.0), OneVec(1.4)));
  gtos.AddSub(Sub_s(Ap, Vector3cd(0.0, -0.2, 0.0), OneVec(1.1)));
  gtos.AddSub(Sub_s(Ap, Vector3cd(0.2, 0.0,  0.1), OneVec(1.0)));
  

  // -- potential --
  MatrixXcd xyzq(4, 1); xyzq << 0.0, 0.0, 0.0, 1.0;
  gtos.SetAtoms(xyzq);
  gtos.SetUp();

  // -- Calculation --
  IB2EInt *eri = new B2EIntMem(pow(4, 4));
  gtos.CalcERI(eri);

  // -- size check --
  EXPECT_EQ(pow(4, 4), eri->size());

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
  
  delete eri;
}
SubSymGTOs SubC1(Vector3i ns, Vector3cd xyz, dcomplex z) {
  MatrixXcd xyz_in(3, 1); xyz_in << xyz[0], xyz[1], xyz[2];
  MatrixXi  ns_in(3, 1);  ns_in << ns[0], ns[1], ns[2];
  MatrixXcd cs = MatrixXcd::Ones(1, 1);
  Reduction rds(0, cs);
  vector<Reduction> rds_list; rds_list.push_back(rds);
  VectorXcd zs(1); zs << z;
  SubSymGTOs sub(xyz_in, ns, rds_list, zs);
  return sub;
}
TEST(SymGTOs, CalcERI2) {

  SymGTOs gtos(SymmetryGroup_Cs());
  cout << 1 << endl;
  gtos.AddSub(SubC1(Vector3i(0, 1, 0), Vector3cd(0.0, 0.0, 0.4), 1.2));
  gtos.AddSub(SubC1(Vector3i(1, 1, 0), Vector3cd(0.0, 0.0, 0.0), 1.4));
  gtos.AddSub(SubC1(Vector3i(1, 1, 1), Vector3cd(0.0,-0.2, 0.0), 1.1));
  gtos.AddSub(SubC1(Vector3i(0, 3, 0), Vector3cd(0.2, 0.0, 0.1), 1.0));

  // -- potential --
  cout << 1 << endl;
  MatrixXcd xyzq(4, 1); xyzq << 0.0, 0.0, 0.0, 1.0;
  gtos.SetAtoms(xyzq);
  gtos.SetUp();

  // -- Calculation --
  cout << 1 << endl;
  IB2EInt *eri = new B2EIntMem(pow(4, 4));
  cout << 0.5 << endl;
  gtos.CalcERI(eri);

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

  delete eri;
}

int main (int argc, char **args) {
  ::testing::InitGoogleTest(&argc, args);
  return RUN_ALL_TESTS();
}
