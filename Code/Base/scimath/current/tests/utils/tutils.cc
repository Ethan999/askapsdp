/// @copyright (c) 2007 CSIRO
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

// ASKAPsoft includes
#include <AskapTestRunner.h>

// Test includes
#include <MultiDimArrayPlaneIterTest.h>
#include <FixedSizeCacheTest.h>
#include <CasaProjectionTest.h>
#include <ChangeMonitorTest.h>
#include <PaddingUtilsTest.h>
#include <PolConverterTest.h>
#include <SpheroidalFunctionTest.h>
#include <EigenDecomposeTest.h>
#include <ComplexGaussianNoiseTest.h>
#include <DelayEstimatorTest.h>
#include <MultiDimPosIterTest.h>
#include <SharedGSLTypesTest.h>

int main(int argc, char *argv[])
{
    askapdev::testutils::AskapTestRunner runner(argv[0]);
    runner.addTest(askap::scimath::MultiDimArrayPlaneIterTest::suite());
    runner.addTest(askap::scimath::FixedSizeCacheTest::suite());
    runner.addTest(askap::scimath::ChangeMonitorTest::suite());
    runner.addTest(askap::scimath::PaddingUtilsTest::suite());
    runner.addTest(askap::scimath::PolConverterTest::suite());
    runner.addTest(askap::scimath::SpheroidalFunctionTest::suite());
    runner.addTest(askap::scimath::EigenDecomposeTest::suite());
    runner.addTest(askap::scimath::ComplexGaussianNoiseTest::suite());
    runner.addTest(askap::scimath::DelayEstimatorTest::suite());
    runner.addTest(askap::scimath::MultiDimPosIterTest::suite());
    runner.addTest(askap::utility::SharedGSLTypesTest::suite());

    bool wasSucessful = runner.run();

    return wasSucessful ? 0 : 1;
}
