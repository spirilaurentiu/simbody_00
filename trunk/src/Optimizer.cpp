
/* Portions copyright (c) 2006 Stanford University and Jack Middleton.
 * Contributors:
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "SimTKmath.h"
#include "Optimizer.h"
#include "LBFGSOptimizer.h"
#include "LBFGSBOptimizer.h"
#include "InteriorPointOptimizer.h"
#include "CFSQPOptimizer.h"
#include <string>

namespace SimTK {

   Optimizer::~Optimizer() {
         delete( (OptimizerRep *)rep );
      }

    bool Optimizer::isAlgorithmAvailable(OptimizerAlgorithm algorithm) {
        switch(algorithm) {
           case InteriorPoint: return InteriorPointOptimizer::isAvailable();
           case LBFGS: return LBFGSOptimizer::isAvailable();
           case LBFGSB: return LBFGSBOptimizer::isAvailable();
           case CFSQP: return CFSQPOptimizer::isAvailable();
           default: return false;
        }
    }

void Optimizer::librarySideOptimizerConstructor( OptimizerSystem& sys, OptimizerAlgorithm algorithm ) {
 
   rep = 0;

   // if constructor specifies which algorithm, use it else select base on problem paramters 
   if ( algorithm == InteriorPoint ) {
           rep = (OptimizerRep *) new InteriorPointOptimizer( sys  );
   } else if( algorithm == LBFGSB ) {
           rep = (OptimizerRep *) new LBFGSBOptimizer( sys  );
   } else if( algorithm == LBFGS ) {
           rep = (OptimizerRep *) new LBFGSOptimizer( sys  );
   } else if( algorithm == CFSQP ) {
       try {
           rep = (OptimizerRep *) new CFSQPOptimizer( sys  );
       } catch (const SimTK::Exception::Base &ex) {
           std::cout << ex.getMessage() << std::endl;
           std::cout << "Failed to load CFSQP library.  Will fall back to default optimizer choice." << std::endl;
           rep = 0;
        }
   }

   if(!rep) { 
       if( sys.getNumConstraints() > 0)   {
           rep = (OptimizerRep *) new InteriorPointOptimizer( sys  );
       } else if( sys.getHasLimits() ) {
           rep = (OptimizerRep *) new LBFGSBOptimizer( sys  );
       } else {
           rep = (OptimizerRep *) new LBFGSOptimizer( sys  );
       }
   } 
      rep->setMyHandle(*this);
      updRep().sysp = &sys;
}

void Optimizer::useNumericalGradient( const bool flag ) {

      ((OptimizerRep *)rep)->useNumericalGradient(flag);
      return;
}
void Optimizer::useNumericalJacobian( const bool flag )  {

      ((OptimizerRep *)rep)->useNumericalJacobian(flag);
      return;
}

void Optimizer::setConvergenceTolerance( const Real tolerance ){

      ((OptimizerRep *)rep)->setConvergenceTolerance(tolerance);
      return;
}

void Optimizer::setMaxIterations( const int iter ){

      ((OptimizerRep *)rep)->setMaxIterations(iter);
      return;
}

void Optimizer::setLimitedMemoryHistory( const int history ){

      ((OptimizerRep *)rep)->setLimitedMemoryHistory(history);
      return;
}

void Optimizer::setDiagnosticsLevel( const int  level ){

      ((OptimizerRep *)rep)->setDiagnosticsLevel(level);
      return;
}

bool Optimizer::setAdvancedStrOption( const char *option, const char *value ){

      return( ((OptimizerRep *)rep)->setAdvancedStrOption( option, value) );
}

bool Optimizer::setAdvancedRealOption( const char *option, const Real value ){

      return( ((OptimizerRep *)rep)->setAdvancedRealOption( option, value) );
}

bool Optimizer::setAdvancedIntOption( const char *option, const int value ){

      return( ((OptimizerRep *)rep)->setAdvancedIntOption( option, value) );
}

bool Optimizer::setAdvancedBoolOption( const char *option, const bool value ){

      return( ((OptimizerRep *)rep)->setAdvancedBoolOption( option, value) );
}

Real Optimizer::optimize(SimTK::Vector   &results) {
      return( ((OptimizerRep *)rep)->optimize(results) );
}
   
int objectiveFuncWrapper( int n, Real *x, int new_x,  Real *f, void* user_data) {
      Vector parameters( n, x, true);
      Real& frep = *f;
      bool new_param;
      if( new_x == 1 )
          new_param = true;
      else
          new_param = false;
      const OptimizerRep& rep = *reinterpret_cast<const OptimizerRep*>(user_data);
      if( 0 == rep.objectiveFunc( rep.getOptimizerSystem(), parameters, new_param, frep )) {
          return(1);
      } else {
          return(0);
      }
}
int gradientFuncWrapper( int n, Real *x, int new_x, Real *gradient, void* user_data) {

      Real fy0;
      Real& sfy0 = fy0;
      Vector params( n, x, true);
      Vector grad_vec(n,gradient,true);
      const OptimizerRep& rep = *reinterpret_cast<const OptimizerRep*>(user_data);
      bool new_param;
      if( new_x == 1 )
          new_param = true;
      else
          new_param = false;

      if( rep.getNumericalGradient() ) {
          rep.getOptimizerSystem().objectiveFunc( params, true, sfy0 );
          rep.gradDiff->calcGradient( params, sfy0, grad_vec);
          return(1);
      } else {
          if( 0 == rep.gradientFunc( rep.getOptimizerSystem(), params, new_param, grad_vec )) {
            return(1);
          } else {
            return(0);
          }
      }

}
int constraintFuncWrapper( int n, Real *x, int new_x, int m, Real *g,  void*user_data) {
      Vector parameters( n, x, true);
      Vector constraints(m, g, true);
      const OptimizerRep& rep = *reinterpret_cast<const OptimizerRep*>(user_data);
      bool new_param;

      if( new_x == 1 )
          new_param = true;
      else
          new_param = false;
      if( 0 == rep.constraintFunc( rep.getOptimizerSystem(), parameters, new_param, constraints )) {
         return 1;
      } else { 
         return 0;
      }
}
int constraintJacobianWrapper(int n, Real *x, int new_x, int m, Index nele_jac,
                int *iRow, int *jCol, Real *values, void *user_data)
{
  if(m==0) return 1; // m==0 case occurs if you run IPOPT with no constraints

  int i,j,index;
  bool new_param;
  if( new_x == 1 )
      new_param = true;
  else
      new_param = false;

  if (values == NULL) {

    /* always assume  the jacobian is dense */
    index = 0;
    for(j=0;j<m;j++) {
      for(i=0;i<n;i++) {
          iRow[index] = j;
          jCol[index++] = i;
//printf("IROW=%d JCol=%d \n",iRow[index-1],jCol[index-1]);
       }
    }
  } else {
    /* jacobian of the constraints */
    
    Vector params(n,x,true); 
    const OptimizerRep& rep = *reinterpret_cast<const OptimizerRep*>(user_data);



    Matrix jac(m,n);      

    if( rep.getNumericalJacobian() ) {
        Vector sfy0(m);            
        rep.getOptimizerSystem().constraintFunc( params, true, sfy0 );
        rep.jacDiff->calcJacobian( params, sfy0, jac);

    } else {

        rep.constraintJacobian( rep.getOptimizerSystem(), params, new_param, jac );

    }
    /* transpose the jacobian because Ipopt indexes in Row major format */
    Real *ptr = values;
    for(j=0;j<m;j++) {
        for(i=0;i<n;i++,ptr++) {
            *ptr = jac(j,i);
        }
    }
//   std::cout << std::setprecision(20);
//   std::cout << jac << std::endl << std::endl;

  } 
  return( 1 );
}
// TODO finish hessianWrapper
int hessianWrapper(int n, Real *x, int new_x, Real obj_factor,
            int m, Number *lambda, int new_lambda,
            int nele_hess, int *iRow, int *jCol,
            Real *values, void *user_data) {


    Vector coeff(n,x,true); 
    Vector hess(n*n,values,true); 
    const OptimizerRep& rep = *reinterpret_cast<const OptimizerRep*>(user_data);
    bool new_param;
    if( new_x == 1 )
       new_param = true;
    else
       new_param = false;

    return( rep.hessian( rep.getOptimizerSystem(), coeff, new_param, hess ));
}


void Optimizer::registerObjectiveFunc(Optimizer::ObjectiveFunc f) {
    updRep().objectiveFunc = f;
}
void Optimizer::registerGradientFunc(Optimizer::GradientFunc f) {
    updRep().gradientFunc = f;
}
void Optimizer::registerConstraintFunc(Optimizer::ConstraintFunc f) {
    updRep().constraintFunc = f;
}
void Optimizer::registerConstraintJacobian(Optimizer::ConstraintJacobian f) {
    updRep().constraintJacobian = f;
}
void Optimizer::registerHessian(Optimizer::Hessian f) {
    updRep().hessian = f;
}

} // namespace SimTK