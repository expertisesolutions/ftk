///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Felipe Magno de Almeida.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// See http://www.boost.org/libs/foreach for documentation
//

#ifndef FASTDRAWCXX_OBJECTS_LINE_HH
#define FASTDRAWCXX_OBJECTS_LINE_HH

#include <fastdraw/point.hpp>

namespace fastdraw { namespace object {

template <typename Coord, typename Color>
struct line
{
  typedef point<Coord> point_type;
  point_type cur_p1, cur_p2;
  Color color;
};
  
} }

#endif
