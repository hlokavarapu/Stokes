#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
namespace po = boost::program_options;

#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <vector>
#include "geometry.h"
#include "dataWindow.h"
#include "hdf5.h"

using namespace std;
using namespace Eigen;

// A helper function to simplify the main part.
template<class T>
ostream& operator<<(ostream& os, const vector<T>& v);
void computeNextTimeStep(Geometry *toGrid, string FDM, double deltaT, double h, double a); 
void stepInTime(SparseMatrix<double>* fd, SparseMatrix<double>* cd, Geometry* grid, double k, double h, double a);
//void writeH5(std::ofstream xdm5, string outputDir, Geometry* toGrid, int timeStep);

//This program models the advection equation for various conservative finite difference methods
int main(int ac, char *av[]) {
    double cellCount = 0;
    double h = 0.0;
    double a = 0.0;
    double startT, endT, dt, ts;
    string icType, bndryType;
    string FDM;
    string config_file;
    string outputDir;

  try {
    //Set of options for Command Line
    po::options_description generic("Generic Options");
    generic.add_options() 
      ("help", "produce a help message")
      ("pF", po::value<string>(&config_file)->default_value("exampleParameters.cfg"), "name of a Parameter file")
    ;
    //Set of options for Config file
    po::options_description tokens("Parameters for stokes solver");
    tokens.add_options() 
      ("M", po::value<double>(&cellCount), "number of cells within the domain, must be a power of 2")
      ("startTime", po::value<double>(&startT), "Initial Time")
      ("endTime", po::value<double>(&endT), "End Time")
      ("timeWidth", po::value<double>(&dt), "Width of each time step")
      ("finiteDifferenceMethod", po::value<string>(&FDM), "Possibilities are Upwind, Lax-Friedrichs, Lax-Wendroff")
      ("initialConditions", po::value<string>(&icType), "Type of initial condition, (Square Wave, Semicircle, Gaussian Pulse")
      ("boundaryConditions", po::value<string>(&bndryType), "Type of Boundary Condition, (Periodic")
      ("advectionConstant", po::value<double>(&a), "speed")
      ("outputDirectory", po::value<string>(&outputDir), "Output Directory")
    ;
    
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(tokens);
    
    po::options_description config_file_options;
    config_file_options.add(tokens);
    
    po::positional_options_description pCF;
    pCF.add("pF", 1);
    
    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).
              options(cmdline_options).positional(pCF).run(), vm);
    po::notify(vm);

    ifstream ifs(config_file.c_str());
    if(!ifs) {
      cout << "Could not open parameter file: " << config_file << "\n";
      return 0;
    }
    else
    {
      po::store(po::parse_config_file(ifs, config_file_options), vm);
      notify(vm);
    }

    if(vm.count("help")) {
      cout << cmdline_options << "\n";
      return 0;
    }
    else if(vm.count("pF")) {
      cout << "Parameter File has been set to: " << config_file << "\n";
    }
  }
  catch(exception& e) {
    cerr << "error: " << e.what() << "\n";
    return 1;
  }	
  catch(...) {
    cerr << "exception of unknown type";
    return 1;
  }

  //Initializing the Problem
  h = 1/cellCount;
  ts = (endT-startT)/dt;
  Geometry grid(cellCount, h, icType); 
  Geometry* toGrid = &grid;
  int n = (int) toGrid->getM();
  int i,j;
  cout << "Spacing of adjacent cells is: " << h << "\n";
  cout << "Number of Time Steps is set to: " << ts << "\n";
  DataWindow<double> uWindow (grid.getU(), grid.getM(), 1);
  cout << "At time step 0:" << endl;
  cout << "Initial conditions have been set to a " << icType << endl;
  cout << uWindow.displayMatrix() << endl;
  for( int i=0; i<ts; i++) {
    computeNextTimeStep(toGrid, FDM, dt, h, a); 
    DataWindow<double> uNextWindow (toGrid->getU(), toGrid->getM(), 1);
    cout << uNextWindow.displayMatrix() << endl << endl; 
  }

  return 0;
}
// ----------------------------
//First Order Difference
/*
  SparseMatrix<double,ColMajor> fda(grid.getM()+1, grid.getM()+1);  
//Second Order Difference
  SparseMatrix<double,ColMajor> identity(grid.getM()+1, grid.getM()+1);  
  //vector<Triplet<double> > fdTmp, cdTmp;
  //fdTmp.reserve(2);
  //cdTmp.reserve(2);
  //fdTmp.push_back(Triplet<double>(1,3,2));
  //cdTmp.push_back(Triplet<double>(1,3,2));
  //fd1.setFromTriplets(fdTmp.begin(),fdTmp.end());
  //cd1.setFromTriplets(cdTmp.begin(), cdTmp.end()); 
  
  fda.reserve(VectorXi::Constant(n+1,2));
  identity.reserve(VectorXi::Constant(n+1,2));
  
  for(j = 0; j < n+1; j++) {
    for(i= 0; i < n+1; i++) {  
      if(i==j) { 
        fda.insert(i,j) = -1;
        identity.insert(i,j) = 1;
      }
      else if((i-1)==j) {
        fda.insert(i,j) = 1;
      }
    }
  }
  
  fda.makeCompressed();
  identity.makeCompressed();
 //Initializing output
  //std::ofstream xdmfFile;
  DataWindow<double> uWindow (grid.getU(), grid.getM(), 1);
  cout << "original" << endl;
  cout << tWindow.displayMatrix() << endl; 

  cout << "Twindow" << endl;
  for( i=0; i<ts; i++) { 
    stepInTime(&fda, &identity, toGrid, dt, h, a_c);
    DataWindow<double> tWindow (grid.getT(), grid.getM(), 1);
    cout << tWindow.displayMatrix() << endl << endl; 
   // writeH5(xdmfFile, outputDir, toGrid, i);
  }
*/

template<class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
    copy(v.begin(), v.end(), ostream_iterator<T>(os, " "));
    return os;
}

void stepInTime(SparseMatrix<double>* fda, SparseMatrix<double>* identity, Geometry* grid, double k, double h, double a) {
  //cout << *lfda << endl;
  //cout << *lfds << endl;
  int n = (int) grid->getM();
  Eigen::Matrix<double, Dynamic, 1> u_j, u_j_1;
  u_j.resize(n+1,1);
  u_j_1.resize(n+1,1);
  for(int i = 0; i < n; i++) {
    u_j(i) = *(grid->getU()+i);
  }
  u_j(n) = 0;
  //Forward Difference Approximation Method
  double sigma = (k*a)/h;
  //u_j_1 = 1/2 * (*lfda) * u_j - c * (*lfds) * u_j;
  //u_j_1 = (*identity) * u_j + sigma * (*fda) * u_j;
  u_j_1 = (*identity) * u_j + (*fda) * u_j;
  if(u_j_1(n) != 0) {
    u_j_1(0) = u_j_1(n);
  }
  //Updating Temperature data 
  double* T = grid->getU();
  for(int i = 0; i < n; i++) {
    *(T+i) = u_j_1(i);
  }     
}

/*void writeH5(ofstream xdmfFile, string outputDir, Geometry* toGrid, int timeStep) {
  hid_t file_id;
  herr_t status;
  string fileName = "timestep";
  stringstream stmp;
  stmp << fileName << "-" << timeStep;  
  file_id = H5Fcreate((outputDir + "/" + stmp.str()  + ".h5").c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
  cout << "<Outputting current data to \"" << outputDir << "/" << fileName  << "-" << timestep << ".h5\">" << endl;
  status = H5Fclose(file_id);
         
}*/

//void stepInTime(double* td, double n, double ts) {
/*void stepInTime(Geometry* grid) {
  int n = grid->getM();
  //MatrixXd A = MatrixXd::Constant(n,n,0);
  SparseMatrix<double, ColMajor> A(n,n);

  A.setFromTriplets(T.begin(),T.end());
  SparseLU<SparseMatrix<double, ColMajor> > solver;
  solver.analyzePattern(A);
  solver.factorize(A);

    
  cout << A << endl;
  return;
}*/
