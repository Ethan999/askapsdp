#include <gridding/WProjectVisGridder.h>

#include <conrad/ConradError.h>

#include <casa/Arrays/Array.h>
#include <casa/Arrays/ArrayMath.h>

#include <casa/BasicSL/Constants.h>
#include <scimath/Mathematics/FFTServer.h>

using namespace conrad;

#include <cmath>

namespace conrad
{
	namespace synthesis
	{

		WProjectVisGridder::WProjectVisGridder(const double wmax,
		    const int nwplanes, const double cutoff, const int overSample,
						       const int maxSupport, const std::string& name)
		{
			CONRADCHECK(wmax>0.0, "Baseline length must be greater than zero");
			CONRADCHECK(nwplanes>0, "Number of w planes must be greater than zero");
			CONRADCHECK(overSample>0, "Oversampling must be greater than 0");
			CONRADCHECK(cutoff>0.0, "Cutoff must be positive");
			CONRADCHECK(cutoff<1.0, "Cutoff must be less than 1.0");
			CONRADCHECK(maxSupport>0, "Maximum support must be greater than 0")
			itsSupport=0;
			itsNWPlanes=2*nwplanes+1;
			itsWScale=wmax/double(nwplanes);
			itsOverSample=overSample;
			itsCutoff=cutoff;
			itsMaxSupport=maxSupport;
			itsName=name;
		}

		WProjectVisGridder::~WProjectVisGridder()
		{
		}

		/// Initialize the convolution function into the cube. If necessary this
		/// could be optimized by using symmetries.
		void WProjectVisGridder::initConvolutionFunction(IDataSharedIter& idi,
		    const conrad::scimath::Axes& axes,
		    casa::Vector<casa::RigidVector<double, 3> >& uvw,
		    const casa::Vector<double>& cellSize, const casa::IPosition& shape)
		{
			/// We have to calculate the lookup function converting from
			/// row and channel to plane of the w-dependent convolution
			/// function
			const int nSamples = uvw.size();
			const int nChan = idi->frequency().size();
			itsCMap.resize(nSamples, nChan);
			itsCMap.set(0);

			int cenw=(itsNWPlanes-1)/2;
			for (int i=0; i<nSamples; i++)
			{
				double w=(uvw(i)(2))/(casa::C::c);
				for (int chan=0; chan<nChan; chan++)
				{
					double freq=idi->frequency()[chan];
					itsCMap(i, chan)=cenw+nint(w*freq/itsWScale);
					if(itsCMap(i, chan)<0) {
						std::cout << w << " " << freq << " " << itsWScale << " " << itsCMap(i, chan) << std::endl; 
					}
					CONRADCHECK(itsCMap(i,chan)<itsNWPlanes, "W scaling error: recommend allowing larger range of w");
					CONRADCHECK(itsCMap(i,chan)>-1, "W scaling error: recommend allowing larger range of w");
				}
			}
			if (itsSupport!=0) {
				return;
			}

			casa::FFTServer<casa::Float,casa::Complex> ffts;
			itsSupport=0;

			/// These are the actual cell sizes used
			float cellx=1.0/(float(shape(0))*cellSize(0));
			float celly=1.0/(float(shape(1))*cellSize(1));

			/// Limit the size of the convolution function since
			/// we don't need it finely sampled in image space. This
			/// will reduce the time taken to calculate it.
			int nx=std::min(itsMaxSupport, shape(0));
			int ny=std::min(itsMaxSupport, shape(1));
			/// We want nx * ccellx = overSample * shape(0) * cellx
			
			// Find the actual cellsizes in x and y (radians) after over
			// oversampling (in uv space)
			float ccellx=float(itsOverSample)*float(shape(0))*cellx/float(nx);
			float ccelly=float(itsOverSample)*float(shape(1))*celly/float(ny);

			int qnx=nx/itsOverSample;
			int qny=ny/itsOverSample;
			
			casa::Vector<float> ccfx(qnx);
			casa::Vector<float> ccfy(qny);
			for (int ix=0; ix<qnx; ix++)
			{
				float nux=std::abs(float(ix-qnx/2))/float(qnx/2);
				ccfx(ix)=grdsf(nux)/float(qnx);
			}
			for (int iy=0; iy<qny; iy++)
			{
				float nuy=std::abs(float(iy-qny/2))/float(qny/2);
				ccfy(iy)=grdsf(nuy)/float(qny);
			}

			// Now we step through the w planes, starting the furthest
			// out. We calculate the support for that plane and use it
			// for all the others.

			// We pad here to do sinc interpolation of the convolution
			// function in uv space
			casa::Matrix<casa::Complex> thisPlane(nx, ny);

			for (int iw=0; iw<=cenw; iw++)
			{
				thisPlane.set(0.0);

				float w=2.0f*casa::C::pi*float(iw-cenw)*itsWScale;
				// Loop over the central nx, ny region, setting it to the product
				// of the phase screen and the spheroidal function
				for (int iy=0; iy<qny; iy++)
				{
					float y2=float(iy-qny/2)*ccelly;
					y2*=y2;
					for (int ix=0; ix<qnx; ix++)
					{
						float x2=float(ix-qnx/2)*ccellx;
						x2*=x2;
						float r2=x2+y2;
						float phase=w*(1.0-sqrt(1.0-r2));
						float wt=ccfx(iy)*ccfy(ix);
						thisPlane(ix-qnx/2+nx/2, iy-qny/2+ny/2)=casa::Complex(wt*cos(phase), -wt*sin(phase));
					}
				}
			  // At this point, we have the phase screen multiplied by the spheroidal
				// function, sampled on larger cellsize (itsOverSample larger) in image
				// space. Only the inner qnx, qny pixels have a non-zero value
				
				// Now we have to calculate the Fourier transform to get the
				// convolution function in uv space
				for (int iy=0; iy<ny; iy++)
				{
					casa::Array<casa::Complex> vec(thisPlane.column(iy));
					ffts.fft(vec, true);
				}
				for (int ix=0; ix<nx; ix++)
				{
					casa::Array<casa::Complex> vec(thisPlane.row(ix));
					ffts.fft(vec, true);
				}
				// Now thisPlane is filled with convolution function
				// sampled on a finer grid in u,v
				//
				// If the support is not yet set, find it and size the
				// convolution function appropriately
				float maxPlane=abs(thisPlane(nx/2,ny/2));
				if (itsSupport==0)
				{
					// Find the support by starting from the edge and
					// working in
					for (int ix=0; ix<nx/2; ix++)
					  {
						if (casa::abs(thisPlane(ix, ny/2))>itsCutoff*maxPlane)
						{
							itsSupport=abs(ix-nx/2)/itsOverSample;
							break;
						}
					}
					itsSupport=(itsSupport<nx/2) ? itsSupport : nx/2;
					itsCSize=2*(itsSupport+1)*itsOverSample;
					std::cout << "Convolution function support = "<< itsSupport
						  << " pixels, convolution function size = "<< itsCSize
						  << " pixels"<< std::endl;
					itsCCenter=itsCSize/2-1;
					itsC.resize(itsCSize, itsCSize, itsNWPlanes);
					itsC.set(0.0);
				}
				// Now cut out the inner part of the convolution function and
				// insert it into the convolution function
				for (int iy=-itsOverSample*itsSupport; iy<+itsOverSample*itsSupport; iy++)
				{
					for (int ix=-itsOverSample*itsSupport; ix<+itsOverSample*itsSupport; ix++)
					{
						itsC(ix+itsCCenter, iy+itsCCenter, iw)=thisPlane(ix+nx/2, iy+ny/2);
					}
				}
				itsC.xyPlane(itsNWPlanes-1-iw)=casa::conj(itsC.xyPlane(iw));
			}
			std::cout << "Shape of convolution function = "<< itsC.shape()
			    << std::endl;
			if(itsName!="") save(itsName);
		}

		int WProjectVisGridder::cOffset(int row, int chan)
		{
			return itsCMap(row, chan);
		}

	}
}
