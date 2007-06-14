#include <fitting/LinearSolver.h>

#include <stdexcept>

#include <casa/aips.h>
#include <casa/Arrays/Array.h>
#include <casa/Arrays/Matrix.h>
#include <casa/Arrays/Vector.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

#include <iostream>

#include <string>
#include <map>

#include <cmath>
using std::abs;
using std::map;
using std::string;

namespace conrad
{
  namespace scimath
  {

    void LinearSolver::init()
    {
      itsNormalEquations.reset();
    }

// Fully general solver for the normal equations for any shape
// parameters.
    bool LinearSolver::solveNormalEquations(Quality& quality)
    {

// Solving A^T Q^-1 V = (A^T Q^-1 A) P
      uint nParameters=0;

// Find all the free parameters
      const vector<string> names(itsParams.freeNames());
      if(names.size()<1)
      {
        throw(std::domain_error("No free parameters"));
      }
      vector<string>::const_iterator it;
      map<string, uint> indices;
      for (it=names.begin();it!=names.end();it++)
      {
        indices[*it]=nParameters;
        nParameters+=itsParams.value(*it).nelements();
      }
      if(nParameters<1)
      {
        throw(std::domain_error("No free parameters"));
      }

// Convert the normal equations to gsl format
      gsl_matrix * A = gsl_matrix_alloc (nParameters, nParameters);
      gsl_vector * B = gsl_vector_alloc (nParameters);
      gsl_vector * X = gsl_vector_alloc (nParameters);

      map<string, uint>::iterator indit1;
      map<string, uint>::iterator indit2;
      for (indit2=indices.begin();indit2!=indices.end();indit2++)
      {
        for (indit1=indices.begin();indit1!=indices.end();indit1++)
        {
// Axes are dof, dof for each parameter
          const casa::Matrix<double>& nm(itsNormalEquations.normalMatrix()[indit1->first][indit2->first]);
          for (uint row=0;row<nm.nrow();row++)
          {
            for (uint col=0;col<nm.ncolumn();col++)
            {
//                    std::cout << indit1->first << " " << indit2->first << " " << row << " " << col << " " << nm(row,col) << std::endl;
              gsl_matrix_set(A, row+(indit1->second), col+(indit2->second), nm(row,col));
            }
          }
        }
      }
      for (indit1=indices.begin();indit1!=indices.end();indit1++)
      {
        const casa::Vector<double>& dv(itsNormalEquations.dataVector()[indit1->first]);
        for (uint row=0;row<dv.nelements();row++)
        {
//            std::cout << indit1->first << " " << row << " " << dv(row) << std::endl;
          gsl_vector_set(B, row+(indit1->second), dv(row));
        }
      }

      if(algorithm()=="SVD")
      {
        gsl_matrix * V = gsl_matrix_alloc (nParameters, nParameters);
        gsl_vector * S = gsl_vector_alloc (nParameters);
        gsl_vector * work = gsl_vector_alloc (nParameters);
        gsl_linalg_SV_decomp (A, V, S, work);

        gsl_vector * X = gsl_vector_alloc(nParameters);
        gsl_linalg_SV_solve (A, V, S, B, X);
// Now find the statistics for the decomposition
        uint rank=0;
        double smin=1e50;
        double smax=0.0;
        for (uint i=0;i<nParameters;i++)
        {
          double sValue=std::abs(gsl_vector_get(S, i));
          if(sValue>0.0)
          {
            rank++;
            if(sValue>smax) smax=sValue;
            if(sValue<smin) smin=sValue;
          }
        }
        quality.setDOF(nParameters);
        quality.setRank(rank);
        quality.setCond(smax/smin);
        if(rank==nParameters)
        {
          quality.setInfo("SVD decomposition rank complete");
        }
        else
        {
          quality.setInfo("SVD decomposition rank deficient");
        }
// Update the parameters for the calculated changes. Exploit reference
// semantics of casa::Array.
        map<string, uint>::iterator indit;
        for (indit=indices.begin();indit!=indices.end();indit++)
        {
          casa::IPosition vecShape(1, itsParams.value(indit->first).nelements());
          casa::Vector<double> value(itsParams.value(indit->first).reform(vecShape));
          for (uint i=0;i<value.nelements();i++)
          {
            value(i)+=gsl_vector_get(X, indit->second+i);
          }
        }
        gsl_vector_free(S);
        gsl_vector_free(work);
        gsl_matrix_free(V);
      }
      else
      {
        quality.setInfo("Cholesky decomposition");
        gsl_linalg_cholesky_decomp(A);
        gsl_linalg_cholesky_solve(A, B, X);
// Update the parameters for the calculated changes
        map<string, uint>::iterator indit;
        for (indit=indices.begin();indit!=indices.end();indit++)
        {
          casa::IPosition vecShape(1, itsParams.value(indit->first).nelements());
          casa::Vector<double> value(itsParams.value(indit->first).reform(vecShape));
          for (uint i=0;i<value.nelements();i++)
          {
            value(i)=gsl_vector_get(X, indit->second+i);
          }
        }
      }

// Free up gsl storage
      gsl_vector_free(B);
      gsl_matrix_free(A);
      gsl_vector_free(X);

      return true;
    };

  }
}
