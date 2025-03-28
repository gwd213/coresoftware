#ifndef PHOOL_SPHENIX_CONSTANTS_H
#define PHOOL_SPHENIX_CONSTANTS_H
/*!
 * \file sphenix_constants.h
 * \author Hugo Pereira Da Costa <hugo.pereira-da-costa@cea.fr>
 * \brief convenient namespace to store a number of sphenix specific constants and avoid magic numbers multiplication
 */

namespace sphenix_constants
{

  //! time between RHIC crossings (ns)
  static constexpr double time_between_crossings = 106.65237;
  static constexpr double CF4_density = 3.86;  // mg / cm3 Tom Hemmick
}  // namespace sphenix_constants
#endif
