/*
 * The MIT License
 *
 * Copyright (c) 1997-2019 The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
 
 #ifndef UINTAH_GRID_VARIABLES_KOKKOSVIEWS_H
#define UINTAH_GRID_VARIABLES_KOKKOSVIEWS_H

#if defined( UINTAH_ENABLE_KOKKOS )
#include <Kokkos_Core.hpp>
#include <Core/Parallel/LoopExecution.hpp>
#include <Core/Grid/Patch.h>
namespace Uintah {


template <typename T, typename MemSpace>
using KokkosData = Kokkos::View<T***, Kokkos::LayoutLeft, MemSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;


//For the default memory space
template <typename T, typename MemSpace>
struct KokkosView3
{
  using view_type = Kokkos::View<T***, Kokkos::LayoutStride, MemSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged> >;

  using reference_type = typename view_type::reference_type;

  template< typename IType, typename JType, typename KType >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type operator()(const IType & i, const JType & j, const KType & k ) const
  { return m_view( i - m_i, j - m_j, k - m_k ); }

  template< typename IType >
  KOKKOS_FORCEINLINE_FUNCTION
  reference_type operator()(const IType & i ) const
  { return m_view( i, 0, 0 ); }

  KokkosView3( const view_type & v, int i, int j, int k )
    : m_view(v)
    , m_i(i)
    , m_j(j)
    , m_k(k)
  {}

  KokkosView3() = default;

  template <typename U, typename MemSpaceSource,
            typename = std::enable_if< std::is_same<U,T>::value || std::is_same<const U,T>::value >,
            typename = std::enable_if< std::is_same<MemSpaceSource, MemSpace>::value > >
  KokkosView3( const KokkosView3<U, MemSpaceSource> & v)
    : m_view(v.m_view)
    , m_i(v.m_i)
    , m_j(v.m_j)
    , m_k(v.m_k)
  {}

  template <typename U, typename MemSpaceSource,
            typename = std::enable_if< std::is_same<U,T>::value || std::is_same<const U,T>::value >,
            typename = std::enable_if< std::is_same<MemSpaceSource, MemSpace>::value > >
  KokkosView3 & operator=( const KokkosView3<U, MemSpaceSource> & v)
  {
    m_view = v.m_view;
    m_i = v.m_i;
    m_j = v.m_j;
    m_k = v.m_k;
    return *this;
  }

  view_type m_view;
  int       m_i{0};
  int       m_j{0};
  int       m_k{0};

    template <typename ExecSpace>
    inline  void
    initialize( T init_val){
      Uintah::parallel_for<ExecSpace>(*this,init_val );
    }

};

template <typename T>
struct KokkosView3<const Uintah::Patch *, T>
{

};

} // End namespace Uintah
#endif //UINTAH_ENABLE_KOKKOS
#endif //UINTAH_GRID_VARIABLES_KOKKOSVIEWS_H
