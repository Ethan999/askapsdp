/// @file VisSource.h
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

#ifndef ASKAP_CP_INGEST_VISSOURCE_H
#define ASKAP_CP_INGEST_VISSOURCE_H

// ASKAPsoft includes
#include "boost/shared_ptr.hpp"
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/asio.hpp"
#include "cpcommon/VisDatagram.h"

// Local package includes
#include "ingestpipeline/sourcetask/IVisSource.h"
#include "ingestpipeline/sourcetask/CircularBuffer.h"

namespace askap {
namespace cp {
namespace ingest {

class VisSource : public IVisSource {
    public:
        /// Constructor
        VisSource(const unsigned int port, const unsigned int bufSize);
        
        /// Destructor
        ~VisSource();

        /// @see IVisSource::next
        boost::shared_ptr<VisDatagram> next(const long timeout = -1);

    private:
        void start_receive(void);

        void handle_receive(const boost::system::error_code& error,
                            std::size_t bytes);

        void run(void);

        // Circular buffer of VisDatagram objects
        askap::cp::ingest::CircularBuffer< VisDatagram > itsBuffer;

        // Service thread
        boost::shared_ptr<boost::thread> itsThread;

        // Used to request the service thread to stop
        bool itsStopRequested;

        // Boost io_service
        boost::asio::io_service itsIOService;

        // UDP socket
        boost::scoped_ptr<boost::asio::ip::udp::socket> itsSocket;

        boost::asio::ip::udp::endpoint itsRemoteEndpoint;

        boost::shared_ptr<VisDatagram> itsRecvBuffer;

        // No support for assignment
        VisSource& operator=(const VisSource& rhs);

        // No support for copy constructor
        VisSource(const VisSource& src);

};

}
}
}

#endif
