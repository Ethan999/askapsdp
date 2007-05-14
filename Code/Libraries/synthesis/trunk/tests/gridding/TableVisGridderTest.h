#include <gridding/TableVisGridder.h>
#include <fitting/Params.h>
#include <measurementequation/ComponentEquation.h>
#include <dataaccess/DataAccessorStub.h>
#include <casa/aips.h>
#include <casa/Arrays/Matrix.h>
#include <measures/Measures/MPosition.h>
#include <casa/Quanta/Quantum.h>
#include <casa/Quanta/MVPosition.h>
#include <casa/BasicSL/Constants.h>

#include <cppunit/extensions/HelperMacros.h>

#include <stdexcept>

using namespace conrad::scimath;

namespace conrad {
namespace synthesis {
	
class TableVisGridderTest : public CppUnit::TestFixture  {

    CPPUNIT_TEST_SUITE(TableVisGridderTest);
    CPPUNIT_TEST(testForward);
    CPPUNIT_TEST(testReverse);
    CPPUNIT_TEST_SUITE_END();
	
  private:
    TableVisGridder *p1, *p2, *p3, *pempty;
    boost::shared_ptr<IDataAccessor> ida;
	casa::Vector<double>* cellSize;
	casa::Cube<casa::Complex>* grid;
	casa::Vector<float>* weights;
		

  public:
    void setUp()
    {
      ida = boost::shared_ptr<IDataAccessor>(new DataAccessorStub(true));
            
	  Params ip;
	  ip.add("flux.i.cena", 100.0);
	  ip.add("direction.ra.cena", 0.5);
	  ip.add("direction.dec.cena", -0.3);
	  
	  ComponentEquation ce(ip, ida);
	  ce.predict();

      p1 = new TableVisGridder(ida);

      p2 = new TableVisGridder(ida);
      
      p3 = new TableVisGridder(ida);
      
      pempty = new TableVisGridder(ida);

      cellSize=new casa::Vector<double>(2);

      (*cellSize)(0)=10.0*casa::C::arcsec;
      (*cellSize)(1)=10.0*casa::C::arcsec;
      
      grid=new casa::Cube<casa::Complex>(512,512,1);
      grid->set(0.0);
      
      weights=new casa::Vector<float>(1);
      weights->set(0.0);
      
    }
    
    void tearDown() 
    {
      delete p1;
      delete p2;
      delete p3;
      delete pempty;
      delete cellSize;
      delete grid;
      delete weights;
    }

	void testForward()
	{
		p1->forward(*cellSize, *grid, *weights);
	}    
	void testReverse()
	{
		p1->reverse(*grid, *cellSize);
	}    
  };
  
}
}
