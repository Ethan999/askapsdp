#include <gridding/TableVisGridder.h>

using namespace conrad::scimath;

#include <stdexcept>

namespace conrad
{
namespace synthesis
{

TableVisGridder::TableVisGridder()
{
}

TableVisGridder::~TableVisGridder()
{
}

void TableVisGridder::initConvolutionFunction(IDataSharedIter& idi, const casa::Vector<double>& cellsize,
        const casa::IPosition& shape) {
}

void TableVisGridder::reverse(IDataSharedIter& idi,
        const scimath::Axes& axes,
        casa::Cube<casa::Complex>& grid,
        casa::Vector<float>& weights)
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericReverse(idi->uvw(), idi->visibility(), visweight, idi->frequency(),
        cellsize, grid, weights);
}
        
void TableVisGridder::reverse(IDataSharedIter& idi,
        const scimath::Axes& axes,
        casa::Array<casa::Complex>& grid,
        casa::Matrix<float>& weights)
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericReverse(idi->uvw(), idi->visibility(), visweight, idi->frequency(),
        cellsize, grid, weights);
}
        
void TableVisGridder::reverseWeights(IDataSharedIter& idi,
        const scimath::Axes& axes,
        casa::Cube<casa::Complex>& grid)
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericReverseWeights(idi->uvw(), visweight, idi->frequency(),
        cellsize, grid);
}
        
void TableVisGridder::reverseWeights(IDataSharedIter& idi,
        const scimath::Axes& axes,
        casa::Array<casa::Complex>& grid)
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericReverseWeights(idi->uvw(), visweight, idi->frequency(),
        cellsize, grid);
}
        
void TableVisGridder::forward(IDataSharedIter& idi,
    const scimath::Axes& axes,
    const casa::Cube<casa::Complex>& grid)
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericForward(idi->uvw(), idi->rwVisibility(), visweight, idi->frequency(),
        cellsize, grid);
}

void TableVisGridder::forward(IDataSharedIter& idi,
    const scimath::Axes& axes,
    const casa::Array<casa::Complex>& grid) 
{
    casa::Cube<float> visweight(grid.shape()); visweight.set(1.0);
    casa::Vector<double> cellsize;
    findCellsize(cellsize, grid.shape(), axes);
    initConvolutionFunction(idi, cellsize, grid.shape());
    genericForward(idi->uvw(), idi->rwVisibility(), visweight, idi->frequency(),
        cellsize, grid);
}

void TableVisGridder::genericReverse(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    const casa::Cube<casa::Complex>& visibility,
                    const casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    casa::Array<casa::Complex>& grid,
                    casa::Matrix<float>& sumwt)
{
}

void TableVisGridder::genericReverse(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    const casa::Cube<casa::Complex>& visibility,
                    const casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    casa::Cube<casa::Complex>& grid,
                    casa::Vector<float>& sumwt)
{

    const int gSize = grid.ncolumn();
    const int nSamples = uvw.size();
    const int nChan = freq.size();
    const int nPol = visibility.shape()(2);

    sumwt.set(0.0);
    
    // Loop over all samples adding them to the grid
    // First scale to the correct pixel location
    // Then find the fraction of a pixel to the nearest pixel
    // Loop over the entire itsSupport, calculating weights from
    // the convolution function and adding the scaled
    // visibility to the grid.
    for (int i=0;i<nSamples;i++) {
        for (int chan=0;chan<nChan;chan++) {
            for (int pol=0;pol<nPol;pol++) {

                int coff=cOffset(i,chan);
            
                double uScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(0));
                int iu=int(uScaled);
                int fracu=int(itsOverSample*(uScaled-double(iu)));
                iu+=gSize/2;

                double vScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(1));
                int iv=int(vScaled);
                int fracv=int(itsOverSample*(vScaled-double(iv)));
                iv+=gSize/2;

                for (int suppu=-itsSupport;suppu<+itsSupport;suppu++) {
                    for (int suppv=-itsSupport;suppv<+itsSupport;suppv++) {
                        float wt=itsC(iu+fracu+itsCCenter,iu+fracu+itsCCenter,coff)*visweight(i,chan,pol);
                        grid(iu+suppu,iv+suppv,pol)+=wt*visibility(i,chan,pol);
                        sumwt(pol)+=wt;
                    }
                }
            }
        }
    }
}

void TableVisGridder::genericReverseWeights(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    const casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    casa::Array<casa::Complex>& grid)
{
}

void TableVisGridder::genericReverseWeights(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    const casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    casa::Cube<casa::Complex>& grid)
{

    const int gSize = grid.ncolumn();
    const int nSamples = uvw.size();
    const int nChan = freq.size();
    const int nPol = 1;

    // Loop over all samples adding them to the grid
    // First scale to the correct pixel location
    // Then find the fraction of a pixel to the nearest pixel
    // Loop over the entire itsSupport, calculating weights from
    // the convolution function and adding the scaled
    // visibility to the grid.
    for (int i=0;i<nSamples;i++) {
        for (int chan=0;chan<nChan;chan++) {
            for (int pol=0;pol<nPol;pol++) {

                int coff=cOffset(i,chan);
            
                double uScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(0));
                int iu=int(uScaled);
                int fracu=int(itsOverSample*(uScaled-double(iu)));
                iu+=gSize/2;

                double vScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(1));
                int iv=int(vScaled);
                int fracv=int(itsOverSample*(vScaled-double(iv)));
                iv+=gSize/2;

                for (int suppu=-itsSupport;suppu<+itsSupport;suppu++) {
                    for (int suppv=-itsSupport;suppv<+itsSupport;suppv++) {
                        float wt=itsC(iu+fracu+itsCCenter,iu+fracu+itsCCenter,coff)*visweight(i,chan,pol);
                        grid(suppu,suppv,pol)+=wt;
                    }
                }
            }
        }
    }
}

void TableVisGridder::genericForward(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    casa::Cube<casa::Complex>& visibility,
                    casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    const casa::Array<casa::Complex>& grid)
{
}

void TableVisGridder::genericForward(const casa::Vector<casa::RigidVector<double, 3> >& uvw,
                    casa::Cube<casa::Complex>& visibility,
                    casa::Cube<float>& visweight,
                    const casa::Vector<double>& freq,
                    const casa::Vector<double>& cellsize,
                    const casa::Cube<casa::Complex>& grid)
{

    const int gSize = grid.ncolumn();
    const int nSamples = uvw.size();
    const int nChan = freq.size();
    const int nPol = visibility.shape()(2);

    // Loop over all samples adding them to the grid
    // First scale to the correct pixel location
    // Then find the fraction of a pixel to the nearest pixel
    // Loop over the entire itsSupport, calculating weights from
    // the convolution function and adding the scaled
    // visibility to the grid.
    for (int i=0;i<nSamples;i++) {
        for (int chan=0;chan<nChan;chan++) {
            for (int pol=0;pol<nPol;pol++) {

                int coff=cOffset(i,chan);
            
                double uScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(0));
                int iu=int(uScaled);
                int fracu=int(itsOverSample*(uScaled-double(iu)));
                iu+=gSize/2;

                double vScaled=freq[chan]*uvw(i)(0)/(casa::C::c*cellsize(1));
                int iv=int(vScaled);
                int fracv=int(itsOverSample*(vScaled-double(iv)));
                iv+=gSize/2;

                double sumviswt=0.0;
                for (int suppu=-itsSupport;suppu<+itsSupport;suppu++) {
                    for (int suppv=-itsSupport;suppv<+itsSupport;suppv++) {
                        float wt=itsC(iu+fracu+itsCCenter,iu+fracu+itsCCenter,coff);
                        visibility(i,chan,pol)+=wt*grid(iu+suppu,iv+suppv,pol);
                        sumviswt+=wt;
                    }
                }
                visibility(i,chan,pol)=visibility(i,chan,pol)/casa::Complex(sumviswt);
                visweight(i,chan,pol)=sumviswt;
            }
        }
    }
}

void TableVisGridder::findCellsize(casa::Vector<double>& cellsize, const casa::IPosition& imageShape, 
    const Axes& axes) {
    
    if(!axes.has("RA")||!axes.has("DEC")) {
        throw(std::invalid_argument("RA and DEC specification not present in axes"));
    }
    double raStart=axes.start("RA");
    double raEnd=axes.end("RA");
    int raCells=imageShape(axes.order("RA"));

    double decStart=axes.start("DEC");
    double decEnd=axes.end("DEC");
    int decCells=imageShape(axes.order("DEC"));
    
    cellsize.resize(2);
    cellsize(0)=double(raCells)/(raStart-raEnd);
    cellsize(1)=double(decCells)/(decStart-decEnd);
}
            
}
}