///
/// @file : Code to do all analysis and evaluation. Aimed at continuum data
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
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <casa/Logging/LogIO.h>
#include <askap/Log4cxxLogSink.h>

#include <askapparallel/AskapParallel.h>
#include <parallelanalysis/DuchampParallel.h>
#include <patternmatching/CatalogueMatcher.h>

#include <duchamp/duchamp.hh>

#include <Common/ParameterSet.h>

#include <stdexcept>
#include <iostream>

#include <casa/OS/Timer.h>

using std::cout;
using std::endl;

using namespace askap;
using namespace askap::analysis;
using namespace askap::analysis::matching;

ASKAP_LOGGER(logger, "continuumAnalysis.log");

// Move to Askap Util
std::string getInputs(const std::string& key, const std::string& def, int argc,
                      const char** argv)
{
    if (argc > 2) {
        for (int arg = 0; arg < (argc - 1); arg++) {
            std::string argument = std::string(argv[arg]);

            if (argument == key) {
                return std::string(argv[arg + 1]);
            }
        }
    }

    return def;
}

// Main function
int main(int argc, const char** argv)
{
    // This class must have scope outside the main try/catch block
    askap::askapparallel::AskapParallel comms(argc, argv);
    try {
        // Ensure that CASA log messages are captured
        casa::LogSinkInterface* globalSink = new Log4cxxLogSink();
        casa::LogSink::globalSink(globalSink);

        casa::Timer timer;
        timer.mark();
        std::string parsetFile(getInputs("-inputs", "continuumAnalysis.in", argc, argv));
        LOFAR::ParameterSet parset(parsetFile);
        LOFAR::ParameterSet subsetCduchamp(parset.makeSubset("Cduchamp."));
        DuchampParallel image(comms, subsetCduchamp);
        ASKAPLOG_INFO_STR(logger,  "parset file " << parsetFile);
        image.readData();
        image.setupLogfile(argc, argv);
        image.preprocess();
        image.gatherStats();
        image.setThreshold();
        image.findSources();
        image.fitSources();
        image.sendObjects();
        image.receiveObjects();
        image.cleanup();
        image.printResults();

        if (comms.isMaster()) { // only do the cross matching on the master node.
            LOFAR::ParameterSet subsetCrossmatch(parset.makeSubset("Crossmatch."));
            CatalogueMatcher matcher(subsetCrossmatch);
            if (matcher.read()) {
                matcher.findMatches();
                matcher.findOffsets();
                matcher.addNewMatches();
                matcher.findOffsets();
                matcher.outputLists();
                matcher.outputSummary();
            } else {
                if (matcher.srcListSize() == 0) {
                    ASKAPLOG_WARN_STR(logger, "Source list has zero length - no matching done.");
                }
                if (matcher.refListSize() == 0) {
                    ASKAPLOG_WARN_STR(logger,
                                      "Reference list has zero length - no matching done.");
                }
            }
        }

        ASKAPLOG_INFO_STR(logger, "Time for execution of contAnalysis = " <<
                          timer.real() << " sec");
        ///==============================================================================
    } catch (const askap::AskapError& x) {
        ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
        std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    } catch (const duchamp::DuchampError& x) {
        ASKAPLOG_FATAL_STR(logger, "Duchamp error in " << argv[0] << ": " << x.what());
        std::cerr << "Duchamp error in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    } catch (const std::exception& x) {
        ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
        std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    }

    return 0;
}

