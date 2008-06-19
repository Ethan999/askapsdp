/// @file 
/// @brief ATCA L-band illumination model
/// @details This class represents a ATCA L-band illumination model.
/// It includes a disk with Jamesian illumination and optionally feed leg shadows.
/// The glish scripts written by Tim Cornwell were used as a guide.
/// Optionally a phase slope can be applied to simulate offset pointing.
///
/// @copyright (c) 2008 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#include <casa/BasicSL/Constants.h>
#include <casa/Arrays/ArrayMath.h>


#include <gridding/ATCAIllumination.h>
#include <askap/AskapError.h>


using namespace askap;
using namespace askap::synthesis;

/// @brief construct the model
/// @param[in] diam disk diameter in metres
/// @param[in] blockage a diameter of the central hole in metres
ATCAIllumination::ATCAIllumination(double diam, double blockage) :
   itsDiameter(diam), itsBlockage(blockage) 
{
  ASKAPDEBUGASSERT(diam>0);
  ASKAPDEBUGASSERT(blockage>=0);  
  ASKAPDEBUGASSERT(diam > blockage);
}

/// @brief obtain illumination pattern
/// @details This is the main method which populates the 
/// supplied uv-pattern with the values corresponding to the model
/// represented by this object. It has to be overridden in the 
/// derived classes. An optional phase slope can be applied to
/// simulate offset pointing.
/// @param[in] freq frequency in Hz for which an illumination pattern is required
/// @param[in] pattern a UVPattern object to fill
/// @param[in] l angular offset in the u-direction (in radians)
/// @param[in] m angular offset in the v-direction (in radians)
/// @param[in] pa parallactic angle, or strictly speaking the angle between 
/// uv-coordinate system and the system where the pattern is defined (unused)
void ATCAIllumination::getPattern(double freq, UVPattern &pattern, double l, 
                          double m, double) const
{
    const casa::uInt oversample = pattern.overSample();
    const double cellU = pattern.uCellSize()/oversample;
    const double cellV = pattern.vCellSize()/oversample;
    
    // scaled l and m to take the calculations out of the loop
    // these quantities are effectively dimensionless 
    const double lScaled = 2.*casa::C::pi*cellU *l;
    const double mScaled = 2.*casa::C::pi*cellV *m;
    
    // zero value of the pattern by default
    pattern.pattern().set(0.);
    
    // currently don't work with rectangular cells.
    ASKAPCHECK(std::abs(std::abs(cellU/cellV)-1.)<1e-7, 
               "Rectangular cells are not supported at the moment");
    
    const double cell = std::abs(cellU*(casa::C::c/freq));
    
    const double dishRadiusInCells = itsDiameter/(2.0*cell);  
    
    // squares of the disk and blockage area radii
	const double rMaxSquared = casa::square(dishRadiusInCells);
	const double rMinSquared = casa::square(itsBlockage/(2.0*cell));     
	
	// sizes of the grid to fill with pattern values
	const casa::uInt nU = pattern.uSize();
	const casa::uInt nV = pattern.vSize();
	
	ASKAPCHECK((casa::square(double(nU)) > rMaxSquared) || 
	           (casa::square(double(nV)) > rMaxSquared),
	           "The pattern buffer passed to DiskIllumination::getPattern is too small for the given model. "
	           "Sizes should be greater than "<<sqrt(rMaxSquared)<<" on each axis, you have "
	            <<nU<<" x "<<nV);
	
	// maximum possible support for this class corresponds to the dish size
	pattern.setMaxSupport(1+2*casa::uInt(dishRadiusInCells)/oversample);
	           
	double sum=0.; // normalisation factor
	for (casa::uInt iU=0; iU<nU; ++iU) {
	     const double offsetU = double(iU)-double(nU)/2.;
		 const double offsetUSquared = casa::square(offsetU);
		 for (casa::uInt iV=0; iV<nV; ++iV) {
		      const double offsetV = double(iV)-double(nV)/2.;
		      const double offsetVSquared = casa::square(offsetV);
		   	  const double radiusSquared = offsetUSquared + offsetVSquared;
			  if ( (radiusSquared >= rMinSquared) && (radiusSquared <= rMaxSquared)) {
			      // don't need to multiply by wavelength here because we
			      // divided the radius (i.e. the illumination pattern is given
			      // in a relative coordinates in frequency
				  const double phase = lScaled*offsetU + mScaled*offsetV;
				  pattern(iU, iV) = casa::Complex(cos(phase), -sin(phase));
				  sum += 1.;
			   }
		 }
	}
	
    ASKAPCHECK(sum > 0., "Integral of the disk should be non-zero");
    pattern.pattern() *= casa::Complex(float(nU)*float(nV)/float(sum),0.);
}

/// @brief check whether the pattern is symmetric
/// @details Some illumination patterns like this one are trivial and known a priori to
/// be symmetric. This method always returns true to reflect this
/// @return always true 
bool ATCAIllumination::isSymmetric() const
{
  return true;
}