/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef COMPUTE_CLASS
// clang-format off
ComputeStyle(sna/grid/local/kk,ComputeSNAGridLocalKokkosDevice<LMPDeviceType>);
ComputeStyle(sna/grid/local/kk/device,ComputeSNAGridLocalKokkosDevice<LMPDeviceType>);
#ifdef LMP_KOKKOS_GPU
ComputeStyle(sna/grid/local/kk/host,ComputeSNAGridLocalKokkosHost<LMPHostType>);
#else
ComputeStyle(sna/grid/local/kk/host,ComputeSNAGridLocalKokkosDevice<LMPHostType>);
#endif
// clang-format on
#else

// clang-format off
#ifndef LMP_COMPUTE_SNA_GRID_LOCAL_KOKKOS_H
#define LMP_COMPUTE_SNA_GRID_LOCAL_KOKKOS_H

#include "compute_sna_grid_local.h"
#include "kokkos_type.h"
//#include "pair_snap.h"
//#include "kokkos_type.h"
//#include "neigh_list_kokkos.h"
#include "sna_kokkos.h"
//#include "pair_kokkos.h"

namespace LAMMPS_NS {

// Routines for both the CPU and GPU backend
//template<int NEIGHFLAG, int EVFLAG>
//struct TagPairSNAPComputeForce{};


// GPU backend only
/*
struct TagPairSNAPComputeNeigh{};
struct TagPairSNAPComputeCayleyKlein{};
struct TagPairSNAPPreUi{};
struct TagPairSNAPComputeUiSmall{}; // more parallelism, more divergence
struct TagPairSNAPComputeUiLarge{}; // less parallelism, no divergence
struct TagPairSNAPTransformUi{}; // re-order ulisttot from SoA to AoSoA, zero ylist
struct TagPairSNAPComputeZi{};
struct TagPairSNAPBeta{};
struct TagPairSNAPComputeBi{};
struct TagPairSNAPTransformBi{}; // re-order blist from AoSoA to AoS
struct TagPairSNAPComputeYi{};
struct TagPairSNAPComputeYiWithZlist{};
template<int dir>
struct TagPairSNAPComputeFusedDeidrjSmall{}; // more parallelism, more divergence
template<int dir>
struct TagPairSNAPComputeFusedDeidrjLarge{}; // less parallelism, no divergence
*/
//struct TagPairSNAPPreUi{};
struct TagCSNAGridLocalComputeNeigh{};
struct TagCSNAGridLocalComputeCayleyKlein{};
struct TagCSNAGridLocalPreUi{};
struct TagCSNAGridLocalComputeUiSmall{}; // more parallelism, more divergence
struct TagCSNAGridLocalComputeUiLarge{}; // less parallelism, no divergence
struct TagCSNAGridLocalTransformUi{}; // re-order ulisttot from SoA to AoSoA, zero ylist
struct TagCSNAGridLocalComputeZi{};
struct TagCSNAGridLocalComputeBi{};
struct TagCSNAGridLocalTransformBi{}; // re-order blist from AoSoA to AoS
struct TagCSNAGridLocal2Fill{}; // fill the gridlocal array
//struct TagCSNAGridLocalFill2{}; // fill the gridlocal array using same kinda loop as ComputeForce

struct TagComputeSNAGridLocalLoop{};
struct TagComputeSNAGridLocal3D{};

// CPU backend only
/*
struct TagPairSNAPComputeNeighCPU{};
struct TagPairSNAPPreUiCPU{};
struct TagPairSNAPComputeUiCPU{};
struct TagPairSNAPTransformUiCPU{};
struct TagPairSNAPComputeZiCPU{};
struct TagPairSNAPBetaCPU{};
struct TagPairSNAPComputeBiCPU{};
struct TagPairSNAPZeroYiCPU{};
struct TagPairSNAPComputeYiCPU{};
struct TagPairSNAPComputeDuidrjCPU{};
struct TagPairSNAPComputeDeidrjCPU{};
*/
struct TagComputeSNAGridLocalLoopCPU{};

//template<class DeviceType>
template<class DeviceType, typename real_type_, int vector_length_>
class ComputeSNAGridLocalKokkos : public ComputeSNAGridLocal {
 public:
  typedef DeviceType device_type;
  typedef ArrayTypes<DeviceType> AT;

  static constexpr int vector_length = vector_length_;
  using real_type = real_type_;
  using complex = SNAComplex<real_type>;

  // Static team/tile sizes for device offload

#ifdef KOKKOS_ENABLE_HIP
  static constexpr int team_size_compute_neigh = 2;
  static constexpr int tile_size_compute_ck = 2;
  static constexpr int tile_size_pre_ui = 2;
  static constexpr int team_size_compute_ui = 2;
  static constexpr int tile_size_transform_ui = 2;
  static constexpr int tile_size_compute_zi = 2;
  static constexpr int tile_size_compute_bi = 2;
  static constexpr int tile_size_transform_bi = 2;
  static constexpr int tile_size_compute_yi = 2;
  static constexpr int team_size_compute_fused_deidrj = 2;
#else
  static constexpr int team_size_compute_neigh = 4;
  static constexpr int tile_size_compute_ck = 4;
  static constexpr int tile_size_pre_ui = 4;
  static constexpr int team_size_compute_ui = sizeof(real_type) == 4 ? 8 : 4;
  static constexpr int tile_size_transform_ui = 4;
  static constexpr int tile_size_compute_zi = 8;
  static constexpr int tile_size_compute_bi = 4;
  static constexpr int tile_size_transform_bi = 4;
  static constexpr int tile_size_compute_yi = 8;
  static constexpr int team_size_compute_fused_deidrj = sizeof(real_type) == 4 ? 4 : 2;
#endif

  // Custom MDRangePolicy, Rank3, to reduce verbosity of kernel launches
  // This hides the Kokkos::IndexType<int> and Kokkos::Rank<3...>
  // and reduces the verbosity of the LaunchBound by hiding the explicit
  // multiplication by vector_length
  template <class Device, int num_tiles, class TagComputeSNAP>
  using Snap3DRangePolicy = typename Kokkos::MDRangePolicy<Device, Kokkos::IndexType<int>, Kokkos::Rank<3, Kokkos::Iterate::Left, Kokkos::Iterate::Left>, Kokkos::LaunchBounds<vector_length * num_tiles>, TagComputeSNAP>;

  // MDRangePolicy for the 3D grid loop:
  template <class Device, class TagComputeSNAP>
  using CSNAGridLocal3DPolicy = typename Kokkos::MDRangePolicy<Device, Kokkos::IndexType<int>, Kokkos::Rank<3, Kokkos::Iterate::Left, Kokkos::Iterate::Left>>;

  // Testing out team policies
  template <class Device, int num_teams,  class TagComputeSNAP>
  using CSNAGridLocalTeamPolicy = typename Kokkos::TeamPolicy<Device, Kokkos::LaunchBounds<vector_length * num_teams>, TagComputeSNAP>;

  // Custom SnapAoSoATeamPolicy to reduce the verbosity of kernel launches
  // This hides the LaunchBounds abstraction by hiding the explicit
  // multiplication by vector length
  template <class Device, int num_teams, class TagComputeSNAP>
  using SnapAoSoATeamPolicy = typename Kokkos::TeamPolicy<Device, Kokkos::LaunchBounds<vector_length * num_teams>, TagComputeSNAP>;

  ComputeSNAGridLocalKokkos(class LAMMPS *, int, char **);
  ~ComputeSNAGridLocalKokkos() override;

  void init() override;
  void setup() override;
  void compute_local() override;

  // Utility functions for teams

  template<class TagStyle>
  void check_team_size_for(int, int&);

  template<class TagStyle>
  void check_team_size_reduce(int, int&);

  KOKKOS_INLINE_FUNCTION
  void operator() (TagComputeSNAGridLocalLoop, const int& ) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagComputeSNAGridLocalLoopCPU, const int&) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeNeigh,const typename Kokkos::TeamPolicy<DeviceType, TagCSNAGridLocalComputeNeigh>::member_type& team) const;

  // PrintNeigh
  //void operator() (TagPrintNeigh,const typename Kokkos::TeamPolicy<DeviceType, TagPrintNeigh>::member_type& team) const;

  // 3D case - used by parallel_for
  KOKKOS_INLINE_FUNCTION
  void operator()(TagComputeSNAGridLocal3D, const int& iz, const int& iy, const int& ix) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeCayleyKlein, const int iatom_mod, const int jnbor, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalPreUi,const int iatom_mod, const int j, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeUiSmall,const typename Kokkos::TeamPolicy<DeviceType, TagCSNAGridLocalComputeUiSmall>::member_type& team) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeUiLarge,const typename Kokkos::TeamPolicy<DeviceType, TagCSNAGridLocalComputeUiLarge>::member_type& team) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalTransformUi,const int iatom_mod, const int j, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeZi,const int iatom_mod, const int idxz, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalComputeBi,const int iatom_mod, const int idxb, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocalTransformBi,const int iatom_mod, const int idxb, const int iatom_div) const;

  KOKKOS_INLINE_FUNCTION
  void operator() (TagCSNAGridLocal2Fill,const int& ii) const;

 protected:

  SNAKokkos<DeviceType, real_type, vector_length> snaKK;

  int max_neighs, chunk_size, chunk_offset;
  int host_flag;
  int ntotal;
  int total_range; // total number of loop iterations in grid
  int zlen; //= nzhi-nzlo+1;
  int ylen; //= nyhi-nylo+1;
  int xlen; //= nxhi-nxlo+1;

  double cutsq_tmp; // temporary cutsq until we get a view

  Kokkos::View<real_type*, DeviceType> d_radelem;              // element radii
  Kokkos::View<real_type*, DeviceType> d_wjelem;               // elements weights
  //Kokkos::View<real_type**, Kokkos::LayoutRight, DeviceType> d_coeffelem;           // element bispectrum coefficients
  Kokkos::View<real_type*, DeviceType> d_sinnerelem;           // element inner cutoff midpoint
  Kokkos::View<real_type*, DeviceType> d_dinnerelem;           // element inner cutoff half-width
  Kokkos::View<T_INT*, DeviceType> d_ninside;                // ninside for all atoms in list
  Kokkos::View<T_INT*, DeviceType> d_map;                    // mapping from atom types to elements
  Kokkos::View<real_type*, DeviceType> d_test;              // test view

  typedef Kokkos::DualView<F_FLOAT**, DeviceType> tdual_fparams;
  tdual_fparams k_cutsq;
  typedef Kokkos::View<const F_FLOAT**, DeviceType,
      Kokkos::MemoryTraits<Kokkos::RandomAccess> > t_fparams_rnd;
  t_fparams_rnd rnd_cutsq;

  typename AT::t_x_array_randomread x;
  typename AT::t_int_1d_randomread type;
  
  DAT::tdual_float_2d k_alocal;
  typename AT::t_float_2d d_alocal;

  /*
  DAT::tdual_float_2d k_grid;
  DAT::tdual_float_2d k_gridall;
  typename AT::t_float_2d d_grid;
  typename AT::t_float_2d d_gridall;

  DAT::tdual_float_4d k_gridlocal;
  typename AT::t_float_4d d_gridlocal;
  */


  // Utility routine which wraps computing per-team scratch size requirements for
  // ComputeNeigh, ComputeUi, and ComputeFusedDeidrj
  template <typename scratch_type>
  int scratch_size_helper(int values_per_team);

  class DomainKokkos *domainKK;

  // triclinic vars
  /*
  xgrid[0] = domain->h[0]*xgrid[0] + domain->h[5]*xgrid[1] + domain->h[4]*xgrid[2] + domain->boxlo[0];
  xgrid[1] = domain->h[1]*xgrid[1] + domain->h[3]*xgrid[2] + domain->boxlo[1];
  xgrid[2] = domain->h[2]*xgrid[2] + domain->boxlo[2];
  */
  double h0, h1, h2, h3, h4, h5;
  double lo0, lo1, lo2;

};

// These wrapper classes exist to make the compute style factory happy/avoid having
// to extend the compute  style factory to support Compute classes w/an arbitrary number
// of extra template parameters

template <class DeviceType>
class ComputeSNAGridLocalKokkosDevice : public ComputeSNAGridLocalKokkos<DeviceType, SNAP_KOKKOS_REAL, SNAP_KOKKOS_DEVICE_VECLEN> {

 private:
  using Base = ComputeSNAGridLocalKokkos<DeviceType, SNAP_KOKKOS_REAL, SNAP_KOKKOS_DEVICE_VECLEN>;

 public:

  ComputeSNAGridLocalKokkosDevice(class LAMMPS *, int, char **);

  void init() override;
  void compute_local() override;
  //void setup() override;

};

#ifdef LMP_KOKKOS_GPU
template <class DeviceType>
class ComputeSNAGridLocalKokkosHost : public ComputeSNAGridLocalKokkos<DeviceType, SNAP_KOKKOS_REAL, SNAP_KOKKOS_HOST_VECLEN> {

 private:
  using Base = ComputeSNAGridLocalKokkos<DeviceType, SNAP_KOKKOS_REAL, SNAP_KOKKOS_HOST_VECLEN>;

 public:

  ComputeSNAGridLocalKokkosHost(class LAMMPS *, int, char **);

  void init() override;
  void compute_local() override;

};
#endif

}

#endif
#endif