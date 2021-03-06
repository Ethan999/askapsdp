//==============================================================
/// @file : testing ways to do fitting of a list of sources by
/// a range of worker nodes
//==============================================================
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

#include <askapparallel/AskapParallel.h>

#include <Common/ParameterSet.h>
#include <Common/LofarTypedefs.h>
using namespace LOFAR::TYPES;
#include <Blob/BlobString.h>
#include <Blob/BlobIBufString.h>
#include <Blob/BlobOBufString.h>
#include <Blob/BlobIStream.h>
#include <Blob/BlobOStream.h>

#include <casa/aipstype.h>
#include <ms/MeasurementSets/MeasurementSet.h>
#include <images/Images/ImageOpener.h>
#include <lattices/Lattices/LatticeLocker.h>
#include <images/Images/FITSImage.h>
#include <images/Images/SubImage.h>
#include <coordinates/Coordinates/CoordinateSystem.h>
#include <casa/Arrays/IPosition.h>
#include <casa/Arrays/Slicer.h>
#include <casa/Containers/RecordInterface.h>
#include <tables/Tables/Table.h>
#include <tables/Tables/TableDesc.h>
#include <lattices/Lattices/LatticeBase.h>
#include <images/Images/ImageOpener.h>
#include <casa/Utilities/Assert.h>

#include <string>
#include <iostream>

#include <sourcefitting/RadioSource.h>

#include <duchamp/duchamp.hh>
#include <duchamp/Cubes/cubes.hh>
#include <duchamp/Utils/Section.hh>

#include <wcslib/wcs.h>

using namespace casa;
using namespace askap;
using namespace askap::askapparallel;
using namespace askap::analysis;

ASKAP_LOGGER(logger, "tDistribFitter.log");

int main(int argc, const char *argv[])
{
    try {

        AskapParallel parl(argc, argv);

        if (!parl.isParallel()) {
            ASKAPLOG_ERROR_STR(logger, "This needs to be run in parallel!");
            exit(1);
        }

        if (parl.isMaster()) {
            const int size = 20;
            std::vector<int> mylist(size);
            for (int i = 0; i < size; i++) mylist[i] = i;

            int16 rank;
            LOFAR::BlobString bs;
            for (int i = 0; i < size; i++) {

                rank = i % (parl.nProcs() - 1);
                ASKAPLOG_INFO_STR(logger, "Master about to send number " << mylist[i] <<
                                  " to worker #" << rank + 1);
                bs.resize(0);
                LOFAR::BlobOBufString bob(bs);
                LOFAR::BlobOStream out(bob);
                out.putStart("fitsrc", 1);
                out << true << mylist[i];
                out.putEnd();
                parl.sendBlob(bs, rank + 1);
                ASKAPLOG_INFO_STR(logger, "Done");
            }
            // now notify all workers that we're finished.
            bs.resize(0);
            LOFAR::BlobOBufString bob(bs);
            LOFAR::BlobOStream out(bob);
            out.putStart("fitsrc", 1);
            // sourcefitting::RadioSource dudSrc;
            // out << false << dudSrc;
            // out.putEnd();
            //for (int i = 1; i < parl.nProcs(); ++i) {
            //    parl.sendBlob(bs, i);
            //}
            out << false << -1;
            out.putEnd();
            for (int i = 1; i < parl.nProcs(); ++i) {
                parl.sendBlob(bs, i);
            }

            // read data back from workers
            std::vector<int> newlist;
            for (int n = 0; n < parl.nProcs() - 1; n++) {
                int size, num;
                ASKAPLOG_INFO_STR(logger, "Master about to read from worker #" << n);
                parl.receiveBlob(bs, n + 1);
                LOFAR::BlobIBufString bib(bs);
                LOFAR::BlobIStream in(bib);
                int version = in.getStart("final");
                ASKAPASSERT(version == 1);
                in >> size;
                ASKAPLOG_INFO_STR(logger, "The list from worker #" << n <<
                                  " is of size " << size);
                for (int i = 0; i < size; i++) {
                    in >> num;
                    newlist.push_back(num);
                }
                in.getEnd();
            }
            std::stringstream ss;
            for (size_t i = 0; i < newlist.size(); i++) ss << newlist[i] << " ";
            ASKAPLOG_INFO_STR(logger, "Master has : " << ss.str());

        } else if (parl.isWorker()) {
            LOFAR::BlobString bs;
            bool isOK = true;
            int num;
            std::vector<int> numbers;

            while (isOK) {
                // bs.resize(0);
                // LOFAR::BlobOBufString bob(bs);
                // LOFAR::BlobOStream out(bob);
                // out.putStart("fitready", 1);
                // out << true;
                // out.putEnd();
                // parl.sendBlob(bs, 0);

                // const unsigned long size = 0;
                // const int dest=0;
                // send(&size, sizeof(long), dest, 0);

                parl.receiveBlob(bs, 0);
                LOFAR::BlobIBufString bib(bs);
                LOFAR::BlobIStream in(bib);
                int version = in.getStart("fitsrc");
                ASKAPASSERT(version == 1);
                in >> isOK >> num;
                in.getEnd();
                if (isOK) {
                    ASKAPLOG_INFO_STR(logger, "Worker #" << parl.rank() <<
                                      " has number " << num);
                    numbers.push_back(num);
                }
            }

            std::stringstream ss;
            for (size_t i = 0; i < numbers.size(); i++) ss << numbers[i] << " ";
            ASKAPLOG_INFO_STR(logger, "Worker #" << parl.rank() << " has : " << ss.str());
            for (size_t i = 0; i < numbers.size(); i++) numbers[i] += 100;
            // send numbers back to master
            bs.resize(0);
            LOFAR::BlobOBufString bob(bs);
            LOFAR::BlobOStream out(bob);
            out.putStart("final", 1);
            out << int(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) out << numbers[i];
            out.putEnd();
            parl.sendBlob(bs, 0);


        }

    } catch (askap::AskapError& x) {
        ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
        std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    } catch (const duchamp::DuchampError& x) {
        ASKAPLOG_FATAL_STR(logger, "Duchamp error in " << argv[0] << ": " << x.what());
        std::cerr << "Duchamp error in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    } catch (std::exception& x) {
        ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
        std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    }

    exit(0);

}
