/// @file
///
/// XXX Notes on program XXX
///
/// @copyright (c) 2010 CSIRO
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
/// @author XXX XXX <XXX.XXX@csiro.au>
///
#include <askap_analysis.h>
#include <parallelanalysis/Weighter.h>

#include <casainterface/CasaInterface.h>

#include <askapparallel/AskapParallel.h>
#include <askap/AskapLogging.h>
#include <askap/AskapError.h>

#include <casa/aipstype.h>
#include <casa/Arrays/Vector.h>
#define AIPS_ARRAY_INDEX_CHECK

#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

#include <vector>
#include <string>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".weighter");

namespace askap {

namespace analysis {

using namespace analysisutilities;

Weighter::Weighter(askap::askapparallel::AskapParallel& comms,
                   const LOFAR::ParameterSet &parset):
    itsComms(&comms)
{
    itsImage = parset.getString("weightsImage", "");
    if (itsComms->isMaster()) {
        ASKAPLOG_INFO_STR(logger, "Using weights image: " << itsImage);
    }
    itsFlagDoScaling = parset.getBool("scaleByWeights", false);
    itsWeightCutoff = parset.getFloat("weightsCutoff", -1.);
    itsCutoffType = parset.getString("weightsCutoffType", "zero");
    if (itsCutoffType != "zero" && itsCutoffType != "blank") {
        ASKAPLOG_WARN_STR(logger,
                          "'weightsCutoffType' can only be 'zero' or 'blank' " <<
                          "- setting to 'zero'");
        itsCutoffType = "zero";
    }
}

void Weighter::initialise(duchamp::Cube &cube, bool doAllocation)
{
    itsCube = &cube;
    if (doAllocation) {
        this->readWeights();
    }

    if (itsFlagDoScaling || (itsWeightCutoff > 0.)) {
        this->findNorm();
    }
}

void Weighter::readWeights()
{
    ASKAPCHECK(itsImage != "", "Weights image not defined");
    ASKAPLOG_INFO_STR(logger, "Reading weights from " << itsImage <<
                      ", section " << itsCube->pars().section().getSection());

    itsWeights = getPixelsInBox(itsImage,
                                subsectionToSlicer(itsCube->pars().section()),
                                false);
}

void Weighter::findNorm()
{
    if (itsComms->isParallel()) {
        LOFAR::BlobString bs;
        if (itsComms->isWorker()) {
            if (itsWeights.size() == 0)
                ASKAPLOG_ERROR_STR(logger, "Weights array not initialised!");
            // find maximum of weights and send to master
            float maxW = max(itsWeights);
            ASKAPLOG_DEBUG_STR(logger, "Local maximum weight = " << maxW);
            bs.resize(0);
            LOFAR::BlobOBufString bob(bs);
            LOFAR::BlobOStream out(bob);
            out.putStart("localmax", 1);
            out << maxW;
            out.putEnd();
            itsComms->sendBlob(bs, 0);

            // now read actual maximum from master
            itsComms->broadcastBlob(bs, 0);
            LOFAR::BlobIBufString bib(bs);
            LOFAR::BlobIStream in(bib);
            int version = in.getStart("maxweight");
            ASKAPASSERT(version == 1);
            in >> itsNorm;
            in.getEnd();

        } else if (itsComms->isMaster()) {
            // read local maxima from workers and find the maximum of them
            for (int n = 0; n < itsComms->nProcs() - 1; n++) {
                float localmax;
                itsComms->receiveBlob(bs, n + 1);
                LOFAR::BlobIBufString bib(bs);
                LOFAR::BlobIStream in(bib);
                int version = in.getStart("localmax");
                ASKAPASSERT(version == 1);
                in >> localmax;
                itsNorm = (n == 0) ? localmax : std::max(localmax, itsNorm);
                in.getEnd();
            }
            // send the actual maximum to all workers
            bs.resize(0);
            LOFAR::BlobOBufString bob(bs);
            LOFAR::BlobOStream out(bob);
            out.putStart("maxweight", 1);
            out << itsNorm;
            out.putEnd();
            itsComms->broadcastBlob(bs, 0);
        }
    } else {
        // serial mode - read entire weights image, so can just measure maximum directly
        itsNorm = max(itsWeights);
    }

    ASKAPLOG_INFO_STR(logger, "Normalising weights image to maximum " << itsNorm);

}

float Weighter::weight(size_t i)
{
    ASKAPCHECK(i < itsWeights.size(),
               "Index out of bounds for weights array : index=" << i <<
               ", weights array is size " << itsWeights.size());

    return sqrt(itsWeights.data()[i] / itsNorm);
}

bool Weighter::isValid(size_t i)
{
    if (this->doApplyCutoff()) {
        return (itsWeights.data()[i] / itsNorm > itsWeightCutoff);
    } else {
        return true;
    }
}

void Weighter::applyCutoff()
{
    if (this->doApplyCutoff()) {

        ASKAPASSERT(itsCube->getSize() == itsWeights.size());

        float blankValue;
        if (itsCutoffType == "zero") blankValue = 0.;
        else if (itsCutoffType == "blank") {
            blankValue = itsCube->pars().getBzeroKeyword() +
                         itsCube->pars().getBlankKeyword() * itsCube->pars().getBscaleKeyword();
            ASKAPASSERT(itsCube->pars().isBlank(blankValue));
        } else {
            ASKAPLOG_WARN_STR(logger, "Bad value for \"weightsCutoffType\" - using 0.");
            blankValue = 0.;
        }

        for (size_t i = 0; i < itsCube->getSize(); i++) {
            if (! this->isValid(i)) itsCube->getArray()[i] = blankValue;
        }

    }
}

void Weighter::search()
{

    if (itsFlagDoScaling) {

        ASKAPASSERT(itsCube->getSize() == itsWeights.size());
        ASKAPASSERT(itsCube->getRecon() > 0);
        for (size_t i = 0; i < itsCube->getSize(); i++) {
            itsCube->getRecon()[i] = itsCube->getPixValue(i) * this->weight(i);
        }
        itsCube->setReconFlag(true);

        ASKAPLOG_DEBUG_STR(logger, "Searching weighted image to threshold " <<
                           itsCube->stats().getThreshold());
        itsCube->ObjectList() = searchReconArray(itsCube->getDimArray(),
                                itsCube->getArray(),
                                itsCube->getRecon(),
                                itsCube->pars(),
                                itsCube->stats());
    } else {
        ASKAPLOG_DEBUG_STR(logger, "Searching image to threshold " <<
                           itsCube->stats().getThreshold());
        itsCube->ObjectList() = search3DArray(itsCube->getDimArray(),
                                              itsCube->getArray(),
                                              itsCube->pars(),
                                              itsCube->stats());
    }

    itsCube->updateDetectMap();
    if (itsCube->pars().getFlagLog())
        itsCube->logDetectionList();

}


}

}
