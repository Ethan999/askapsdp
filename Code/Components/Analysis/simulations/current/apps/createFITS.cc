///
/// @file : Create a FITS file with fake sources and random noise
///
/// Control parameters are passed in from a LOFAR ParameterSet file.
///
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
/// @author Matthew Whiting <matthew.whiting@csiro.au>

// Package level header file
#include <askap_simulations.h>

// System includes
#include <iostream>
#include <cstdlib>
#include <ctime>

// ASKAPsoft includes
#include <askap/Application.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askap/StatReporter.h>
#include <FITS/FITSparallel.h>
#include <FITS/FITSfile.h>
#include <Common/ParameterSet.h>

using namespace askap;
using namespace askap::simulations;
using namespace askap::simulations::FITS;

ASKAP_LOGGER(logger, "createFITS.log");

class CreateFitsApp : public askap::Application
{
    public:
        virtual int run(int argc, char* argv[])
        {
            // This class must have scope outside the main try/catch block
            askap::askapparallel::AskapParallel comms(argc, const_cast<const char**>(argv));

            try {
                StatReporter stats;

                srandom(time(0));
                LOFAR::ParameterSet subset(config().makeSubset("createFITS."));
                if (comms.isMaster()) {
                    ASKAPLOG_INFO_STR(logger, "Parset file contents:\n" << config());
                }

                bool doNoise = subset.getBool("addNoise", false);
                bool noiseBeforeConvolve = subset.getBool("noiseBeforeConvolve", true);
                bool doConvolution = subset.getBool("doConvolution", true);
                FITSparallel file(comms, subset);

                if (comms.isMaster()) ASKAPLOG_INFO_STR(logger, "In MASTER node!");

                if (comms.isWorker()) ASKAPLOG_INFO_STR(logger, "In WORKER node #" << comms.rank());

                file.processSources();

                if (doNoise && (noiseBeforeConvolve || !doConvolution))
                    file.addNoise(true);

                file.toMaster();

                if (doConvolution)
                    file.convolveWithBeam();

                if (doNoise && (!noiseBeforeConvolve && doConvolution))
                    file.addNoise(false);

                file.output();

                stats.logSummary();

            } catch (const askap::AskapError& x) {
                ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
                std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
                exit(1);
            } catch (const std::exception& x) {
                ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
                std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
                exit(1);
            }

            return 0;
        }
};

// Main function
int main(int argc, char *argv[])
{
    CreateFitsApp app;
    return app.main(argc, argv);
}