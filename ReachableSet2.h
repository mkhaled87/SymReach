/*
 * ReachableSet2.h
 *
 *  Created on: Apr 15, 2018
 *      Author: MahendraSinghTomar
 */

#ifndef REACHABLESET2_H_
#define REACHABLESET2_H_

#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <numeric>
#include <algorithm>
#include "fadiff.h"
#include "badiff.h"
#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>
#include "UsingGnuplot.h"

#include "TicToc.hh"

#include <ginac/ginac.h>
using namespace GiNaC;
#include <boost/numeric/interval.hpp>
namespace Eigen {
  namespace internal {
    template<typename X, typename S, typename P>
    struct is_convertible<X,boost::numeric::interval<S,P> > {
      enum { value = is_convertible<X,S>::value };
    };

    template<typename S, typename P1, typename P2>
    struct is_convertible<boost::numeric::interval<S,P1>,boost::numeric::interval<S,P2> > {
      enum { value = true };
    };
  }
}
namespace vnodelp{
	typedef boost::numeric::interval<double> interval;
	
	double sup(interval& I){
		return I.upper();
	}
	
	double inf(interval& I){
		return I.lower();
	}
	
	double mag(interval& I){
		return boost::numeric::norm(I);
	}
	
	typedef Eigen::Matrix<interval,Eigen::Dynamic,Eigen::Dynamic> iMatrix; 
	typedef Eigen::MatrixXd pMatrix;
	typedef Eigen::VectorXd pVector;
	
	double midpoint(interval& I){
		return boost::numeric::median(I);
	}
	
	void midpoint(pMatrix& M, iMatrix& iM){
		unsigned int r = iM.rows();
		unsigned int c = iM.cols();
		for(unsigned int i=0;i<r;i++)
			for(unsigned int j=0;j<c;j++)
				M(i,j) = midpoint(iM(i,j));
	}
	
	double rad(interval& I){
		return boost::numeric::width(I);
	}
	
	void rad(pVector& M, iMatrix iM){
		unsigned int r = iM.rows();
		for(unsigned int i=0;i<r;i++)
			M(i) = rad(iM(0,i));
	}
	
}

extern int ZorDeltaZ;
extern const int gdim;	// global dim
extern std::vector<symbol> xs;	// x_symbol
extern std::vector<ex> f;
ex *JacobianB = new ex[gdim*gdim];
ex *HessianB = new ex[gdim*gdim*gdim];

// extern Eigen::VectorXd L_hat_previous;  // abstraction.hh: compute_gb2
extern int PERFINDS;
extern int ExampleMst;

typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic> MatrixXld;
typedef Eigen::Matrix<vnodelp::interval,Eigen::Dynamic,Eigen::Dynamic> MatrixXint;

Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, " ", "\n", "", " ", "[", "]");
Eigen::IOFormat LightFmt(Eigen::StreamPrecision, 0, " ", "\n", "", " ", "[", "]");

namespace mstom {

	Eigen::MatrixXd unitNorm(Eigen::MatrixXd& Min, int m){
		// returns unitNorm columnwise(m=1) or rowwise(m=2)
		Eigen::MatrixXd M;
		if(m==1)
			M = Min.cwiseAbs().colwise().sum();
		else
			M = Min.cwiseAbs().rowwise().sum();
		return M;
	}

	Eigen::MatrixXd infNorm(Eigen::MatrixXd& Min, int m){
		// returns infinity norm columnwise(m=1) or  rowwise(m=2)
		Eigen::MatrixXd M;
		if(m==1)
			M = Min.cwiseAbs().colwise().maxCoeff();
		else
			M = Min.cwiseAbs().rowwise().maxCoeff();
		return M;
	}

    template<class T>
    void VXdToV(T& v, const Eigen::VectorXd& vxd){
        for(int i=0;i<vxd.size();i++)
            v[i] = vxd(i);
        return;
    }

    std::vector<double> VXdToV(Eigen::VectorXd vxd){    //Eigen to C++ vector
		unsigned int dim = vxd.size();
        std::vector<double> v(dim);
        for(unsigned int i=0;i<dim;i++)
            v[i] = vxd(i);
        return v;
    }

    class zonotope {
    public:
        //int n;
        Eigen::MatrixXd generators; //Each column represents a generator
        Eigen::VectorXd centre;
        zonotope (){};
        zonotope (const Eigen::VectorXd& c, const Eigen::VectorXd& eta){
            centre = c;
            unsigned int dim = eta.rows();
            generators = Eigen::MatrixXd::Zero(dim,dim);
            for (unsigned int i=0; i<dim; i++){
                generators(i,i) = eta(i)/2;
            }
        };

        template<class state_type>
        zonotope (state_type ct, state_type etat, int dim){
            Eigen::VectorXd c(dim), eta(dim);
            for(unsigned int i=0;i<dim;i++)
            {
                c(i) = ct[i];
                eta(i) = etat[i];
            }
            centre = c;
            generators = Eigen::MatrixXd::Zero(dim,dim);
            for (unsigned int i=0; i<dim; i++){
                generators(i,i) = eta(i)/2;
            }
        };


        //Minkowskii sum
        zonotope operator + (const zonotope& Zb){
            zonotope Ztemp;
            Ztemp.centre = centre + Zb.centre;
            Ztemp.generators = Eigen::MatrixXd::Zero(generators.rows(), generators.cols() + Zb.generators.cols());
            Ztemp.generators.block(0,0,generators.rows(),generators.cols()) = generators;
            Ztemp.generators.block(0,generators.cols(),Zb.generators.rows(),Zb.generators.cols()) = Zb.generators;
            return Ztemp;
        }

        zonotope operator + (Eigen::VectorXd& V){
            // zonotope + vector
            zonotope Ztemp;
            Ztemp.centre = centre + V;
            Ztemp.generators = generators;
            return Ztemp;
        }

        zonotope operator - (Eigen::VectorXd& V){
            // zonotope - vector
            zonotope Ztemp;
            Ztemp.centre = centre - V;
            Ztemp.generators = generators;
            return Ztemp;
        }

        zonotope operator - (const zonotope& Zb){
            zonotope Ztemp;
            Ztemp.centre = centre - Zb.centre;
            Ztemp.generators = Eigen::MatrixXd::Zero(generators.rows(), generators.cols() + Zb.generators.cols());
            Ztemp.generators.block(0,0,generators.rows(),generators.cols()) = generators;
            Ztemp.generators.block(0,generators.cols(),Zb.generators.rows(),Zb.generators.cols()) = (-1 * Zb.generators);
            return Ztemp;
        }

        zonotope operator * (double a){
            // zonotope * scalar
            zonotope Ztemp;
            Ztemp.centre = centre * a;
            Ztemp.generators = generators * a;
            return Ztemp;
        }

        void display(){
            unsigned int r = generators.rows();
            unsigned int gc = generators.cols();
            Eigen::MatrixXd M(r,1+gc);
            M.block(0,0,r,1) = centre;
            M.block(0,1,r,gc) = generators;
            std::cout<< "no. of gen = " << gc << "\n"<< M.format(HeavyFmt) << std::endl;        }
    };

    // product of matrix with Zonotope
    zonotope operator * (const Eigen::MatrixXd& M, const zonotope& Z){
        zonotope Ztemp;
        Ztemp.centre = M * Z.centre;
        Ztemp.generators = M * Z.generators;
        return Ztemp;
    }

    // vector<interval> to zonotope
    mstom::zonotope vecIntToZono(std::vector<vnodelp::interval> Kprime){
        unsigned int dim = Kprime.size();
        Eigen::VectorXd c(dim), lb(dim), ub(dim);
        for(unsigned int i=0;i<dim;i++)
        {
            lb(i) = vnodelp::inf(Kprime[i]);
            ub(i) = vnodelp::sup(Kprime[i]);
        }
        c = (lb + ub) * 0.5;
        mstom:: zonotope Z(c, ub-lb);
        return Z;
    }

    class intervalMatrix{
    public:
        MatrixXld lb;
        MatrixXld ub;
        intervalMatrix(){};
        zonotope operator * ( const zonotope& Z){    //interval matrix map for zonotope
            unsigned int row =  Z.generators.rows();
            unsigned int col =  Z.generators.cols();
            zonotope Ztemp;
            MatrixXld M_tilde = (lb + ub)/2;
            MatrixXld M_hat = ub - M_tilde;
            Ztemp.centre = M_tilde * Z.centre;
            Ztemp.generators = Eigen::MatrixXd::Zero(row, col+row);
            Ztemp.generators.block(0,0,row,col) = M_tilde * Z.generators;
            MatrixXld v = MatrixXld::Zero(row,row);
            for (unsigned int i=0; i<row; i++){
                v(i,i) = M_hat.row(i) * (Z.centre.cwiseAbs() + Z.generators.cwiseAbs().rowwise().sum() ) ;
            }
            Ztemp.generators.block(0,col,row,row) = v;
            return Ztemp;
        }

        intervalMatrix operator * (double a){
            intervalMatrix Mitemp;
            MatrixXld temp1 = lb*a;
            MatrixXld temp2 = ub*a;
            Mitemp.lb = temp1.array().min(temp2.array()).matrix();  //converted to array for min, then reconverted to matrix
            Mitemp.ub = temp1.array().max(temp2.array()).matrix();
            return Mitemp;
        }

        intervalMatrix operator + (const MatrixXld& M){
            intervalMatrix Mitemp;
            Mitemp.lb = lb + M;
            Mitemp.ub = ub + M;
            return Mitemp;
        }

        intervalMatrix operator + (const intervalMatrix& Mi){
            intervalMatrix Mitemp;
            Mitemp.lb = lb + Mi.lb;
            Mitemp.ub = ub + Mi.ub;
            return Mitemp;
        }

    };

    zonotope convexHull(const zonotope& Z1, const Eigen::MatrixXd& eAr){
        // requires initial zonotope and exponential matirx (state-transition matrix)
        unsigned int r1 =  Z1.generators.rows();
        unsigned int c1 =  Z1.generators.cols();
        zonotope Ztemp;
        Ztemp.centre = (Z1.centre + eAr * Z1.centre)/2;
        Ztemp.generators = Eigen::MatrixXd::Zero(r1, 1+2*c1);
        Ztemp.generators.block(0,0,r1,c1) = (Z1.generators + eAr * Z1.generators)/2;
        Ztemp.generators.block(0,c1,r1,1) = (Z1.centre - eAr * Z1.centre)/2;
        Ztemp.generators.block(0,1+c1,r1,c1) = (Z1.generators - eAr * Z1.generators)/2;
        return Ztemp;
    }

    zonotope convexHull(const zonotope& Z1, const zonotope& Z2){
        // when zonotopes may have different number of generators
        zonotope Ztemp;
        unsigned int r = (int)Z1.generators.rows();
        unsigned int c1 = (int)Z1.generators.cols();
        unsigned int c2 = (int)Z2.generators.cols();
        unsigned int c = (c1 > c2 ? c1 : c2);
        Eigen::MatrixXd M = Eigen::MatrixXd::Zero(r,c);
        Ztemp.generators = Eigen::MatrixXd::Zero(r,1+2*c);
        if(c1<c2)
        {
            M.block(0,0,r,c1) = Z1.generators;
            Ztemp.generators.block(0,0,r,c) = 0.5 * (M + Z2.generators);
            Ztemp.generators.block(0,c,r,1) = 0.5 * (Z1.centre - Z2.centre);
            Ztemp.generators.block(0,c+1,r,c) = 0.5 * (M - Z2.generators);
        }
        else
        {
            M.block(0,0,r,c2) = Z2.generators;
            Ztemp.generators.block(0,0,r,c) = 0.5 * (M + Z1.generators);
            Ztemp.generators.block(0,c,r,1) = 0.5 * (Z2.centre - Z1.centre);
            Ztemp.generators.block(0,c+1,r,c) = 0.5 * (M - Z1.generators);
        }
        Ztemp.centre = 0.5*(Z1.centre + Z2.centre);
        return Ztemp;
    }


    zonotope convexHull(std::vector<zonotope>& stora){
        zonotope Z = stora[0];
        for(unsigned int i=1;i<stora.size();i++)
        {
            Z = convexHull(Z, stora[i]);
        }

        return Z;
    }



    double factorial(double n){
        return (n==1 || n==0) ? 1 : factorial(n-1) * n;
    }

    double compute_epsilon(const Eigen::MatrixXd& A, const double& r, int& p){
        double norm_rA = (r*A).cwiseAbs().rowwise().sum().maxCoeff();
        double epsilone = norm_rA/(p+2);
        while (epsilone >= 1){
            p += 1;
            epsilone = norm_rA/(p+2);
         }


        return epsilone;
    }

    double p_adjust_Er_bound(Eigen::MatrixXd& A, double& r, int& p, double& epsilone){
        // adjusts p, epsilone
        Eigen::MatrixXd rA = r * A;
        double norm_rA = rA.cwiseAbs().rowwise().sum().maxCoeff();
        double temp = std::pow(norm_rA,p+1) / factorial(p+1);
        double bound = temp / (1-epsilone);
        while(bound > std::pow(10,-12))
        {
            p++;
            epsilone = norm_rA/(p+2);
            temp = temp * norm_rA / (p+1);
            bound = temp / (1-epsilone);
        }
        return bound;
    }

    void matrix_product(double* M1, double* M2, double* Mresult, unsigned int m, unsigned int n, unsigned int q){
        // size M1 = mxn; size M2 = nxq; M1xM2
        double temp;
        for(unsigned int i=0;i<m;i++)
            for(unsigned int j=0;j<q;j++)
            {
                temp = 0;
                for(unsigned int k=0;k<n;k++)
                {
                    temp += M1[i*n+k] * M2[k*q+j];
                }
                Mresult[i*q+j] = temp;
            }
    }

    void sum_matrix(double M1[], double M2[], unsigned int m, unsigned int n){
        // result stored in M1; size = m x n
        for(unsigned int i=0;i<m;i++)
            for(unsigned int j=0;j<n;j++)
                M1[i*n+j] = M1[i*n+j] + M2[i*n+j];
    }

    void matrix_exponential(MatrixXld& A, double r, int& p, intervalMatrix& Er, std::vector<MatrixXld>& Apower){
        // Apower updated
        unsigned int state_dim = A.rows();
		std::vector<MatrixXld> Apower_abs(p+1);
		MatrixXld M = MatrixXld::Identity(state_dim,state_dim);
		Apower_abs[0] = A.cwiseAbs();
		Apower[0] = A;
		//int fac = 1;
		for(int i=0;i<p;i++)
		{
			Apower[i+1] = Apower[i] * A;
			Apower_abs[i+1] = Apower_abs[i] * Apower_abs[0];
			M = M + Apower_abs[i] * std::pow(r,i+1) / factorial(i+1);
			//fac = fac*(i+2);
		}
		MatrixXld W = (Apower_abs[0]*r).exp() - M;
		W = W.cwiseAbs();
		Er.lb = -W;
		Er.ub = W;

		//std::cout<<"M:\n" << M << std::endl;
		//std::cout <<"exp(Aabs*r):\n" << (Apower_abs[0]*r).exp() << std::endl;
        return ;
    }

    intervalMatrix compute_F(const int& p, const double& r, const MatrixXld& A, intervalMatrix& Er, std::vector<MatrixXld>& Apower){
        unsigned int state_dim = A.rows();
        intervalMatrix Asum;
        Asum.ub = MatrixXld::Zero(state_dim, state_dim);
        Asum.lb = MatrixXld::Zero(state_dim, state_dim);
		//int fac = 2;
		for (double i=2; i<=p; i++)
		{
             double data = (std::pow(i,-i/(i-1)) - std::pow(i,-1/(i-1))) * std::pow(r,i) / factorial(i);
			 //fac = fac * (i+1);
			 for(unsigned int j=0;j<state_dim;j++)
				 for(unsigned int k=0;k<state_dim;k++)
					 if(Apower[i-1](j,k)<0)
						 Asum.ub(j,k) = Asum.ub(j,k) + data * Apower[i-1](j,k);
					 else
						 Asum.lb(j,k) = Asum.lb(j,k) + data * Apower[i-1](j,k);
		}
		Asum = Er + Asum;
        // Eigen::MatrixXd temp(state_dim,state_dim);
        // for (int i=2; i<=p; i++){
              // double data = (pow(i,-i/(i-1)) - pow(i,-1/(i-1)));
            // for(int i2=0;i2<state_dim;i2++)
                // for(int j=0;j<state_dim;j++)
                    // temp(i2,j) = data * Ar_powers_fac[i*state_dim*state_dim + i2*state_dim + j];

            // for(int i1=0; i1<A.rows(); i1++){
                // for(int i2=0; i2<A.rows(); i2++){
                    // if (temp(i1,i2) < 0)
                        // Ftemp.lb(i1,i2) += temp(i1,i2);
                    // else
                        // Ftemp.ub(i1,i2) += temp(i1,i2);
                // }
            // }
        // }
        // Ftemp = Ftemp + Er;
        return Asum;
    }

    intervalMatrix compute_F_tilde(const int& p, const double& r, const MatrixXld& A, intervalMatrix& Er, int isOriginContained, std::vector<MatrixXld> Apower){
        unsigned int state_dim = A.rows();
        intervalMatrix Asum;
        Asum.ub = MatrixXld::Zero(state_dim,state_dim);
        Asum.lb = MatrixXld::Zero(state_dim,state_dim);
		for(double i=2;i<= p+1;i++)
		{
			double data = (std::pow(i,-i/(i-1)) - std::pow(i,-1/(i-1))) * std::pow(r,i)/factorial(i);
			for(unsigned int j=0;j<state_dim;j++)
				for(unsigned int k=0;k<state_dim;k++)
					if(Apower[i-1-1](j,k)<0)
						Asum.ub(j,k) += data * Apower[i-1-1](j,k);
					else
						Asum.lb(j,k) += data * Apower[i-1-1](j,k);
		}
		Asum = Er * (r / A.cwiseAbs().rowwise().sum().maxCoeff()) + Asum;

        // if(!isOriginContained)
        // {
            // Eigen::MatrixXd temp(state_dim,state_dim);
            // for (int i=2; i<=p; i++)
			// {
                   // double data = (pow(i,-i/(i-1)) - pow(i,-1/(i-1)));
                // for(int i2=0;i2<state_dim;i2++)
                    // for(int j=0;j<state_dim;j++)
                        // temp(i2,j) = data * Ar_powers_fac[(i-1)*state_dim*state_dim + i2*state_dim + j] * r / i;

                // for(int i1=0; i1<A.rows(); i1++){
                    // for(int i2=0; i2<A.rows(); i2++){
                        // if (temp(i1,i2) < 0)
                            // Ftemp.lb(i1,i2) += temp(i1,i2);
                        // else
                            // Ftemp.ub(i1,i2) += temp(i1,i2);
                    // }
                // }
            // }
            // Eigen::VectorXd temp2 = A.cwiseAbs().rowwise().sum();
            // Ftemp = Ftemp + Er * pow(temp2.maxCoeff(),-1);
        // }
         return Asum;
    }

    intervalMatrix compute_Data_interm(intervalMatrix& Er, double r, int p, const MatrixXld& A, std::vector<MatrixXld>& Apower){
        unsigned int state_dim = A.rows();
		MatrixXld Asum = r * MatrixXld::Identity(state_dim,state_dim);
		//int fac = 2;
		for(int i=1;i<=p;i++)
		{
			Asum = Asum + Apower[i-1] * std::pow(r,i+1) / factorial(i+1);
			//fac = fac * (i+2);
		}
		intervalMatrix eAtInt = (Er * r) + Asum;
		return eAtInt;
    }

    intervalMatrix IntervalHull(const zonotope& Z)
    {
        intervalMatrix iM;
        iM.lb = Z.centre - Z.generators.cwiseAbs().rowwise().sum();
        iM.ub = Z.centre + Z.generators.cwiseAbs().rowwise().sum();
        return iM;
    }

    zonotope project(zonotope& Z, int a, int b){
        // project on to the dimensions a and b
        zonotope Zp;
        Eigen::Vector2d c;
        Eigen::MatrixXd M(2,Z.generators.cols());
        c(0) = Z.centre(a-1);
        c(1) = Z.centre(b-1);
        M.row(0) = Z.generators.row(a-1);
        M.row(1) = Z.generators.row(b-1);
        Zp.centre = c;
        Zp.generators = M;
        return Zp;
    }

	std::vector<zonotope> project(std::vector<zonotope>& Zv, int a, int b){
		unsigned int num = Zv.size();
		std::vector<zonotope> Z(num);
		for(unsigned int i=0;i<num;i++)
			Z[i] = project(Zv[i], a, b);
		return Z;
	}

	template <typename T>
    std::vector<size_t> sort_indexes(const T &v) {
// https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes?rq=1
        // sorts std::vector and Eigen::VectorXd
		// initialize original index locations
        std::vector<size_t> idx(v.size());
        std::iota(idx.begin(), idx.end(), 0);

        // sort indexes based on comparing values in v
        std::sort(idx.begin(), idx.end(),
             [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

        return idx;
    }

    zonotope deletezeros(zonotope Z){
        // delete zero generators
        zonotope Zd;
        Eigen::MatrixXd vtemp = Z.generators.cwiseAbs().colwise().sum();
        int ncol = (vtemp.array() == 0).select(vtemp, 1).sum();
        Eigen::MatrixXd M(Z.generators.rows(),ncol);
        int index = 0;
        for(int i=0;i<vtemp.cols();i++)
        {
            if(vtemp(i) != 0)
            {
                M.col(index) = Z.generators.col(i);
                index++;
            }
        }
        Zd.centre = Z.centre;
        Zd.generators = M;
        return Zd;
    }

    void vertices(zonotope& Z, Eigen::MatrixXd& p2){
        std::vector<std::pair<double, double>> v;
        Eigen::VectorXd c = Z.centre;
        Eigen::MatrixXd g = Z.generators;
        int n = g.cols();   // number of generators
        double xmax = g.row(0).cwiseAbs().sum();
        double ymax = g.row(1).cwiseAbs().sum();
        Eigen::MatrixXd gup = g;
        for(int i=0;i<g.cols();i++)
        {
            if(g(1,i)<0)    // if 2nd column of g is negative, then reverse the vector to make it point up
                gup.col(i) = -1 * g.col(i);
        }
        std::vector<double> angles;
        for(int i =0;i<n;i++)
        {
            double angletemp = atan2(gup(1,i), gup(0,i));
            if(angletemp<0)
                angletemp += 2 * M_PI;
            angles.push_back(angletemp);
        }
        std::vector<size_t> sortIndexes = sort_indexes(angles);
        Eigen::MatrixXd p = Eigen::MatrixXd::Zero(2, n+1);
        for(int i=0;i<n;i++)
        {
            p.col(i+1) = p.col(i) + 2 * gup.col(sortIndexes[i]);
        }
        p.row(0) = p.row(0) + Eigen::MatrixXd::Constant(1,p.cols(), (xmax - p.row(0).maxCoeff()));
        p.row(1) = p.row(1) - Eigen::MatrixXd::Constant(1,p.cols(), ymax);
        //Eigen::MatrixXd p2(2, (p.cols()*2));
        p2.row(0).head(p.cols()) = p.row(0);
        p2.row(0).tail(p.cols()) = Eigen::MatrixXd::Constant(1,p.cols(), (p.row(0).tail(1) + p.row(0).head(1))(0)) - p.row(0);
        p2.row(1).head(p.cols()) = p.row(1);
        p2.row(1).tail(p.cols()) = Eigen::MatrixXd::Constant(1,p.cols(), (p.row(1).tail(1)+p.row(1).head(1))(0)) - p.row(1);
        p2.row(0) += Eigen::MatrixXd::Constant(1,p2.cols(), c(0));
        p2.row(1) += Eigen::MatrixXd::Constant(1,p2.cols(), c(1));
        return ;
    }

	std::vector<std::pair<double, double>> vertices_pair(zonotope Z){
		int n = Z.generators.cols();   // number of generators
		int p2cols = (n+1)*2;
		Eigen::MatrixXd p2(2, p2cols);
		vertices(Z, p2);
		std::vector<std::pair<double, double>> v(p2cols);
		for(int i=0;i<p2cols;i++)
            v[i] = (std::make_pair(p2(0,i),p2(1,i)));
		return v;
	}

	Eigen::MatrixXd verticesH(zonotope Z){
        // vertices for H-representation; same as vertices(), difference is only in the return type
        Eigen::VectorXd c = Z.centre;
        Eigen::MatrixXd g = Z.generators;
        int n = g.cols();   // number of generators
        double xmax = g.row(0).cwiseAbs().sum();
        double ymax = g.row(1).cwiseAbs().sum();
        Eigen::MatrixXd gup = g;
        for(int i=0;i<g.cols();i++)
        {
            if(g(1,i)<0)    // if 2nd column of g is negative, then reverse the vector to make it point up
                gup.col(i) = -1 * g.col(i);
        }
        std::vector<double> angles;
        for(int i =0;i<n;i++)
        {
            double angletemp = atan2(gup(1,i), gup(0,i));
            if(angletemp<0)
                angletemp += 2 * M_PI;
            angles.push_back(angletemp);
        }
        std::vector<size_t> sortIndexes = sort_indexes(angles);
        Eigen::MatrixXd p = Eigen::MatrixXd::Zero(2, n+1);
        for(int i=0;i<n;i++)
        {
            p.col(i+1) = p.col(i) + 2 * gup.col(sortIndexes[i]);
        }
        p.row(0) = p.row(0) + Eigen::MatrixXd::Constant(1,p.cols(), (xmax - p.row(0).maxCoeff()));
        p.row(1) = p.row(1) - Eigen::MatrixXd::Constant(1,p.cols(), ymax);
        Eigen::MatrixXd p2(2, (p.cols()*2));
        p2.row(0).head(p.cols()) = p.row(0);
        p2.row(0).tail(p.cols()) = Eigen::MatrixXd::Constant(1,p.cols(), (p.row(0).tail(1) + p.row(0).head(1))(0)) - p.row(0);
        p2.row(1).head(p.cols()) = p.row(1);
        p2.row(1).tail(p.cols()) = Eigen::MatrixXd::Constant(1,p.cols(), (p.row(1).tail(1)+p.row(1).head(1))(0)) - p.row(1);
        p2.row(0) += Eigen::MatrixXd::Constant(1,p2.cols(), c(0));
        p2.row(1) += Eigen::MatrixXd::Constant(1,p2.cols(), c(1));
        // vertex in column
        return p2;
    }

    void H_rep(zonotope& Z, Eigen::VectorXd& M){
        // H representation for 2D zonotope
        if(Z.centre.rows() > 2)
            std::cout << "Please use a 2D zonotope\n";
        Eigen::MatrixXd v = verticesH(Z);
        int n = v.cols();   // number of vertices
        Eigen::MatrixXd temp(v.rows(),v.cols());
        temp.block(0,0,v.rows(),v.cols()-1) = v.block(0,1,v.rows(),v.cols()-1);
        temp.col(v.cols()-1) = v.col(0);
        temp = temp - v;    // column as edge vector
        Eigen::Matrix2d temp2;
        temp2 << 0, 1, -1, 0;
        temp = temp2 * temp;    // perpendicular vectors
        Eigen::VectorXd Ma(n);
        for(int i=0;i<n;i++)
        {
            double dd = temp.col(i).transpose() * v.col(i);
            int j = ((i+n/2)< n)? (i+n/2) : (i+n/2-n);
            if(temp.col(i).transpose() * v.col(j) < dd)
            {
                  Ma(i) = dd;
            }
            else
            {
                  Ma(i) = -dd;
            }
        }
        M = Ma;
    }

    bool isOriginInZonotope(zonotope& Z){
        // only for 2D zonotopes
        Eigen::VectorXd M;
        H_rep(Z,M); // H representation; last row of M stores d; h.x < d inside the Z
        // for origin only sign of d need to be checked
        bool chk = (M.minCoeff()>=0);
        return chk;
    }

    void plot(zonotope& Z, int a1, int a2){
        Gnuplot gp;
        //gp << "set terminal lua\n";
        std::vector<std::pair<double, double>> v = vertices_pair(project(Z, a1, a2));
        //gp << "set output 'my_graph_1.png'\n";
        //gp << "set xrange [-2:2]\nset yrange [-2:2]\n";
        gp << "plot '-' with lines title 'cubic'\n";
        gp.send1d(v);
    }

	void plot(std::vector<zonotope> Zv, int a1, int a2){
        // a1, a2 : dimensions to plot
        Gnuplot gp;
        gp << "set grid\n";
        //gp << "set output 'my_graph_1.png'\n";
        gp << "unset key\n";
		int numbe = Zv.size();
        for(int i=0;i<numbe;i++)
        {
            if(i==0)
                gp << "plot '-' with lines lc rgb \"blue\"";
            // else if(i==numbe-1)
				// gp << "'-' with filledcurves closed lc rgb \"white\"";
			else
                gp << "'-' with lines lc rgb \"blue\"";
            if(i==numbe-1)
                // gp << "\n";
			gp << std::endl;
            else
                gp << ", ";
        }
        for(int i=0;i<numbe;i++)
        {
            std::vector<std::pair<double, double>> v = vertices_pair(project(Zv[i], a1, a2));
            gp.send1d(v);
        }
    }

    void plotfilled(std::vector<zonotope> Zv, int a1, int a2){
        // a1, a2 : dimensions to plot
        Gnuplot gp;
        gp << "set grid\n";
        //gp << "set output 'my_graph_1.png'\n";
        gp << "unset key\n";

		gp <<"set style fill noborder\n";
		gp << "set style function filledcurves closed\n";
		gp << "set style fs solid\n";
		int numbe = Zv.size();
        for(int i=0;i<numbe;i++)
        {
            if(i==0)
                gp << "plot '-' with filledcurves closed lc rgb \"grey\"";
            else if(i==numbe-1)
				gp << "'-' with filledcurves closed lc rgb \"white\"";
			else
                gp << "'-' with filledcurves closed lc rgb \"grey\"";
            if(i==numbe-1)
                gp << "\n";
            else
                gp << ", ";
        }
        for(int i=0;i<numbe;i++)
        {
            std::vector<std::pair<double, double>> v = vertices_pair(project(Zv[i], a1, a2));
            gp.send1d(v);
        }
    }

	std::vector<double> project(std::vector<mstom::zonotope> Zv, int a){
        // project on to the dimension a(begins from 1); returns the end points of the line segment
        //int dim = Zv[0].centre.rows();
		std::vector<double> v(2), vs(2);
		double c;
		for(unsigned int i=0;i<Zv.size();i++)
		{
			Eigen::VectorXd M(Zv[i].generators.cols());
			M = Zv[i].generators.row(a-1).transpose();
			//M.row(1) = Z.generators.row(b-1);
			c = Zv[i].centre(a-1);
			vs[0] = c + M.cwiseAbs().sum();	// ub
			vs[1] = c - M.cwiseAbs().sum();	// lb
			if(i==0)
			{
				v[0] = vs[0];
				v[1] = vs[1];
			}
			else
			{
				v[0] = (vs[0] > v[0]) ? (vs[0]) : (v[0]);
				v[1] = (vs[1] < v[1]) ? (vs[1]) : (v[1]);
			}
		}
        return v;
    }

	std::vector<std::pair<double, double>> vertices(std::vector<double> ve, double tau, double k){
		// for plot w.r.t. time. Returns vertices of a rectangle of time width tau
		//k = the time instant
		// double v1 = Z.centre + Z.generators.cwiseAbs().sum();
		// double v2 = Z.centre - Z.generators.cwiseAbs().sum();
		std::vector<std::pair<double, double>> v(4);
		v[0] = std::make_pair(k*tau, ve[0]);
		v[1] = std::make_pair((k+1)*tau, ve[0]);
		v[2] = std::make_pair((k+1)*tau, ve[1]);
		v[3] = std::make_pair(k*tau, ve[1]);
		return v;
	}

	void plotfilled(std::vector<std::vector<mstom::zonotope>> Ztp, int a1, double tau){
        // a1 : dimension to plot w.r.t. time
        Gnuplot gp;
        gp << "set grid\n";
        //gp << "set output 'my_graph_1.png'\n";
        gp << "unset key\n";

		gp <<"set style fill noborder\n";
		gp << "set style function filledcurves closed\n";
		gp << "set style fs solid\n";
		int steps = Ztp.size();
        for(int i=0;i<steps;i++)
        {
            if(i==0)
                gp << "plot '-' with filledcurves closed lc rgb \"grey\"";
            // else if(i==steps-1)
				// gp << "'-' with filledcurves closed lc rgb \"white\"";
			else
                gp << "'-' with filledcurves closed lc rgb \"grey\"";
            if(i==steps-1)
                gp << "\n";
            else
                gp << ", ";
        }
        for(double i=0;i<steps;i++)
        {
            std::vector<std::pair<double, double>> v = vertices(project(Ztp[i], a1), tau, i);
            gp.send1d(v);
        }
    }

    void plot(std::vector<zonotope> Zv, int a1, int a2, bool tb){
        // a1, a2 : dimensions to plot
        Gnuplot gp;
        gp << "set grid\n";

        //gp << "set output 'my_graph_1.png'\n";
        for(unsigned int i=0;i<Zv.size();i++)
        {
            if(i==0)
                gp << "plot '-' with lines title " << ((tb)? "'True'":"'False'");
            else
                gp << "'-' with lines";
            if(i==Zv.size()-1)
                gp << "\n";
            else
                gp << ", ";
        }
        for(unsigned int i=0;i<Zv.size();i++)
        {
            std::vector<std::pair<double, double>> v = vertices_pair(project(Zv[i], a1, a2));
            gp.send1d(v);
        }
    }

    void plot(std::vector<double> L){
        Gnuplot gp;
        gp << "set output 'my_graph.png'\n";
        gp << "plot '-' with points\n";
        gp.send1d(L);
    }

    void plotstore(std::vector<zonotope>& PlotStorage, zonotope Z){
        PlotStorage.push_back(Z);
    }

    void plotstore(std::vector<zonotope>& PlotStorage, std::vector<zonotope> Zv){
        for(unsigned int i=0;i<Zv.size();i++)
        {
            PlotStorage.push_back(Zv[i]);
        }
    }

    void printVector(std::vector<double> v){
        std::cout<< "The vector is: \n";
        for(unsigned int i=0;i<v.size();i++)
            std::cout << v[i] << ", ";
        std::cout << std::endl;
    }

std::vector<zonotope> PlotStorage2;

	void reduce(zonotope& Z, int morder){
		// reduces Z to order = morder, if it is greater than that
		int dim = Z.centre.rows();
		if(Z.generators.cols() <= (morder * dim))
			return;
		Eigen::MatrixXd Gred(dim, dim * morder);
		Eigen::VectorXd h = (unitNorm(Z.generators,1) - infNorm(Z.generators,1)).transpose();
		std::vector<size_t> idx = sort_indexes(h);
		int nUnreduced = dim * (morder-1);
		int nReduced = Z.generators.cols() - nUnreduced;
		Eigen::VectorXd d_ihr(dim);
		d_ihr = Z.generators.col(idx[0]).cwiseAbs();
		for(int i=1;i<nReduced;i++)
			d_ihr += Z.generators.col(idx[i]).cwiseAbs();
		for(unsigned int i=nReduced; i<idx.size(); i++)
			Gred.col(i-nReduced) = Z.generators.col(idx[i]);	//
		Gred.block(0,nUnreduced,dim,dim) = d_ihr.asDiagonal();
		Z.generators = Gred;
		return;
	}

	void reduce(std::vector<zonotope>& Zv, int morder){
		for(unsigned int i=0;i<Zv.size();i++)
		{
			reduce(Zv[i], morder);
		}
	}

	void wfile(zonotope& Z, std::string str1, int flag){
		// Eigen::IOFormat HeavyFmt(Eigen::FullPrecision, 0, " ", "\n", "", " ", "[", "]");
		//Eigen::IOFormat LightFmt(Eigen::StreamPrecision, 0, " ", "\n", "", " ", "[", "]");
		//flag: 1(write), 2(append)
		std::ofstream myfile;
		if(flag==1)
			myfile.open("datastorage.txt");
		else
			myfile.open("datastorage.txt",std::ios::app);
		int r = Z.generators.rows();
        int gc = Z.generators.cols();
        Eigen::MatrixXd M(r,1+gc);
        M.block(0,0,r,1) = Z.centre;
        M.block(0,1,r,gc) = Z.generators;
		myfile << str1 << "=";
        myfile << "zonotope("<< M.format(LightFmt) << ");" << std::endl;
		// myfile << "no_of_gen = " << gc << ";" << std::endl;
		myfile.close();
		return;
	}

	void wfile(std::vector<std::vector<mstom::zonotope>>& Zti){
		// to plot with MATLAB
		int flag;	// 1(write), 2(append)
		for(unsigned int i=0;i<Zti.size();i++)
		{
			for(unsigned int j=0;j<Zti[i].size();j++)
			{
				if(i==0 && j==0)
					flag = 1;
				else
					flag = 2;
				std::string stri = "Zti{" + std::to_string(i+1) + "}{" + std::to_string(j+1) + "}";
				wfile(Zti[i][j], stri, flag);
			}
		}
	}

	void wfile_gnuplot(std::vector<std::vector<mstom::zonotope>>& Zti){
		Eigen::IOFormat GnuplotFmt(Eigen::StreamPrecision, 0, " ", "\n", "", "", "", "\n");
		// precision,flags,coeffSeparator,rowSeparator,rowPrefix,rowSuffix,matPrefix,matSuffix
		std::ofstream myfile;
		myfile.open("datastorage_gnuplot.txt");
		myfile << "# x y\n";
		for(unsigned int i=0;i<Zti.size();i++)
		{
			for(unsigned int j=0;j<Zti[i].size();j++)
			{
				unsigned int n = Zti[i][j].generators.cols();   // number of generators
				unsigned int p2cols = (n+1)*2;
				Eigen::MatrixXd p2(2, p2cols);
				vertices(Zti[i][j], p2);
				myfile << p2.transpose().format(GnuplotFmt) << "\n";
			}
		}
		myfile.close();
	}

	void wfile_time(std::vector<std::vector<mstom::zonotope>>& Zti, int a1, double tau){
		// to plot one dimension vs time with MATLAB
		int flag;	// 1(write), 2(append)
		for(double i=0;i<Zti.size();i++)
		{
			if(i==0)
				flag = 1;
			else
				flag = 2;
			std::string stri = "Zti1D{" + std::to_string(i+1) + "}" ;
			std::vector<double> v = project(Zti[i], a1);
			//vertices(project(Zti[i], a1), tau, i);
			Eigen::Vector2d c;
			c(0) = (i+1)*tau - 0.5*tau;
			c(1) = 0.5*(v[0]+v[1]);
			Eigen::Vector2d g;
			g(0) = 0.5*tau;
			g(1) = 0.5*(v[0]-v[1]);
			zonotope Z;
			Z.centre = c;
			Z.generators = g.asDiagonal();
			wfile(Z, stri, flag);
		}

	}

} // end namespace mstom

//#############################################################################
// Derivative Hessian

std::vector<mstom::zonotope> PlotStorage;
int ZorDeltaZ;  // 1 if Z, 0 if deltaZ

template<typename Tx, typename Tu>
Tx funcLj_system(Tx x, Tu u, Tx xx);


void computeJacobian(double A[], const Eigen::VectorXd& x_bar, Eigen::VectorXd uin)
{
	
    int dim = x_bar.rows();
    B<double> x[dim], xx[dim], *f;
    for(int i=0;i<dim;i++)
        x[i]=x_bar(i);
    f = funcLj_system(x, uin, xx);
    for(int i=0;i<dim;i++)
        f[i].diff(i,dim);
    for(int j=0;j<dim;j++)
        for(int i=0;i<dim;i++)
            A[j*dim+i] = x[i].d(j);
}


template<typename Fu>
void computeJacobian_Lu_array(vnodelp::interval xin[], Fu u[], Eigen::MatrixXd& L,const int dim){
    // from function in main file; using arrays
    // jacobian for L(u) for growth bound
    B<vnodelp::interval>* f;
    B<vnodelp::interval> x[dim];
    B<vnodelp::interval> xx[dim];
    for(int i=0;i<dim;i++)
        x[i] = xin[i];
    f = funcLj_system(x, u, xx);  // evaluate function and derivatives
    for(int i=0;i<dim;i++)
        f[i].diff(i,dim);
    for(int j=0;j<dim;j++)
        for(int i =0;i<dim;i++)
        {
            if(j==i)
                L(j,i) = vnodelp::sup(x[i].d(j));
            else
                L(j,i) = vnodelp::mag(x[i].d(j));
        }
}

template<typename Fu>
void computeJacobian_Lu_array2(vnodelp::interval xin[], Fu u[], const int dim, int jin, double* LuStore){
    // without eigen::matrix
    // from function in main file; using arrays
    // jacobian for L(u) for growth bound
    B<vnodelp::interval>* f;
    B<vnodelp::interval> x[dim];
    B<vnodelp::interval> xx[dim];
    for(int i=0;i<dim;i++)
        x[i] = xin[i];
    f = funcLj_system(x, u, xx);  // evaluate function and derivatives
    for(int i=0;i<dim;i++)
        f[i].diff(i,dim);
    for(int j=0;j<dim;j++)
        for(int i =0;i<dim;i++)
        {
            if(j==i)
                LuStore[jin*dim*dim+j*dim+i] = vnodelp::sup(x[i].d(j));
            else
                LuStore[jin*dim*dim+j*dim+i] = vnodelp::mag(x[i].d(j));
        }
}

template<typename T2>
void compute_J_abs_max(const mstom::intervalMatrix& iM, Eigen::MatrixXd J_abs_max[], T2 u)
{
    int nm = iM.lb.rows();
    vnodelp::interval d2f;
    B<F<vnodelp::interval>>* f;
    B<F<vnodelp::interval>> x[nm], xx[nm];
    for(int j=0;j<nm;j++)
    {
        x[j] = vnodelp::interval(iM.lb(j,0), iM.ub(j,0));
        x[j].x().diff(j,nm);
    }
    f = funcLj_system(x, u, xx);
    for(int i=0;i<nm;i++)
        f[i].diff(i,nm);

    Eigen::MatrixXd M(nm,nm);
    for(int i=0;i<nm;i++)
    {
        for(int j=0;j<nm;j++)
            for(int k=0;k<nm;k++)
            {
                d2f = x[j].d(i).d(k);
                M(j,k) = vnodelp::mag(d2f);
              }
        J_abs_max[i] = M;
    }
}

template<class TM>
void compute_H(const mstom::intervalMatrix& iM, std::vector<TM>& H, Eigen::VectorXd uin)
{
    int nm = iM.lb.rows(); // state_dimension
    vnodelp::interval d2f;
    B<F<vnodelp::interval>>* f;
    B<F<vnodelp::interval>> x[nm], xx[nm];
     for(int j=0;j<nm;j++)
    {
        x[j] = vnodelp::interval(iM.lb(j,0), iM.ub(j,0));
        x[j].x().diff(j,nm);
    }
    f = funcLj_system(x, uin, xx);
    for(int i=0;i<nm;i++)
        f[i].diff(i,nm);

    vnodelp::iMatrix M(nm, nm);
    // vnodelp::sizeM(M,nm);
    for(int i=0;i<nm;i++)
    {
        for(int j=0;j<nm;j++)
            for(int k=0;k<nm;k++)
            {
                d2f = x[j].d(i).d(k);
                M(j,k) = (d2f);
             }
        //H.push_back(M);
		H[i] = M;
    }
}

Eigen::MatrixXd pMatrix_to_MatrixXd(vnodelp::pMatrix pM)
{
    int n = pM.size();
    Eigen::MatrixXd M(n,n);
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            M(i,j) = pM(i,j);
        }
    }
    return M;
}

Eigen::VectorXd pV_to_V(vnodelp::pVector pV)
{
    int n = pV.size();
    Eigen::VectorXd V(n);
    for(int i=0;i<n;i++)
        V(i) = pV[i];
    return V;
}

mstom::zonotope compute_quad(mstom::zonotope Z, std::vector<Eigen::MatrixXd> H_mid)
{
    int row, gcol; // gcol = number of generators
    row = Z.centre.rows();
    gcol = Z.generators.cols();
    mstom::zonotope Zq;
    Eigen::MatrixXd Zmat(row, 1+gcol);
    Zmat.block(0,0,row,1) = Z.centre;
    Zmat.block(0,1,row,gcol) = Z.generators;
    Eigen::MatrixXd quadMat, centre(row,1), gen(row,int((gcol*gcol + 3*gcol)*0.5));
    for(unsigned int i =0;i<H_mid.size();i++)
    {
        quadMat = Zmat.transpose() * H_mid[i] * Zmat; // (gcol+1) x (gcol+1)
        centre(i,0) = quadMat(0,0) + 0.5 * quadMat.diagonal().tail(gcol).sum();
        gen.row(i).head(gcol) = 0.5 * quadMat.diagonal().tail(gcol);
        int count =0;
        for(int j=0;j<gcol+1;j++)
        {
            gen.row(i).segment(gcol+count,gcol-j) = quadMat.row(j).segment(j+1,gcol-j)  + quadMat.col(j).segment(j+1,gcol-j).transpose();
            count += gcol - j;
        }
    }
    Zq.centre = centre;
    Zq.generators = gen;
    return mstom::deletezeros(Zq);
}

Eigen::VectorXd maxAbs(mstom::intervalMatrix IH){
    Eigen::MatrixXd Mtemp2(IH.lb.rows(),2);
    Mtemp2.col(0) = IH.lb;
    Mtemp2.col(1) = IH.ub;
    return Mtemp2.cwiseAbs().rowwise().maxCoeff();
}

Eigen::VectorXd compute_L_Hat1(mstom::zonotope Rtotal1, Eigen::VectorXd x_bar, int state_dim, Eigen::VectorXd uin){
	mstom::intervalMatrix RIH = mstom::IntervalHull(Rtotal1);
	mstom::intervalMatrix totalInt = RIH + x_bar;
    Eigen::VectorXd L_hat(state_dim);
    {
        Eigen::VectorXd Gamma;
        if (ZorDeltaZ)  //1 if Z, 0 if deltaZ;
		{
            Gamma = maxAbs(RIH);
			//std::cout<<"gamma:\n" << Gamma << std::endl;
			//Gamma = (Rtotal1.centre - x_bar).cwiseAbs() + Rtotal1.generators.cwiseAbs().rowwise().sum();
        }
		else
            Gamma = (Rtotal1.centre).cwiseAbs() + Rtotal1.generators.cwiseAbs().rowwise().sum();
       // Eigen::MatrixXd J_abs_max[state_dim];
        //compute_J_abs_max(totalInt, J_abs_max, uin);
		std::vector<MatrixXint> H(state_dim);
		MatrixXint Htemp(state_dim, state_dim);
		std::vector<vnodelp::iMatrix> Hvno(state_dim);
		compute_H(totalInt, Hvno, uin);
		for(int i=0;i<state_dim;i++)
		{
			for(int j=0;j<state_dim;j++)
				for(int k=0;k<state_dim;k++)
				{
					Htemp(j,k) = Hvno[i](j,k);
				}
			H[i] = Htemp;
		}
		MatrixXint error(state_dim,1), Gammaint(state_dim,1);
		for(int i=0;i<state_dim;i++)
			Gammaint(i,0) = vnodelp::interval(Gamma(i));
        for(int i=0; i<state_dim;i++)
        {
            //L_hat(i) = 0.5 * Gamma.transpose() * J_abs_max[i] * Gamma;
			MatrixXint ertemp;
			vnodelp::interval sctemp(0,0);
			ertemp = (H[i] * Gammaint);
			MatrixXint ertemptemp = Gammaint.transpose();
			// ertemp = (ertemptemp * ertemp);
			for(int i=0;i<ertemp.rows();i++)
				sctemp += ertemptemp(0,i) * ertemp(i,0);
			sctemp = 0.5 * sctemp;
			error(i,0) = sctemp;
			//std::cout <<"error: " << ertemp(0,0) << std::endl;
		}
		for(int i=0;i<state_dim;i++)
			L_hat(i) = vnodelp::mag(error(i,0));
    }
    return L_hat;
}

Eigen::VectorXd compute_L_Hat3(std::vector<vnodelp::interval> Kprime, std::vector<vnodelp::interval> uin){
    // global L_hat computation
     int dim = Kprime.size();
    Eigen::VectorXd L_hat(dim);
    {

        Eigen::VectorXd c(dim), lb(dim), ub(dim);
        for(int i=0;i<dim;i++)
        {
            lb(i) = vnodelp::inf(Kprime[i]);
            ub(i) = vnodelp::sup(Kprime[i]);
        }
        c = (lb + ub) * 0.5;
        mstom:: zonotope Z(c, ub-lb);
        mstom::intervalMatrix RIH = mstom::IntervalHull(Z);
        std::cout << "Z \n";
        Z.display();
        std::cout << "(RIH.ub-RIH.lb)*0.5\n "<< (RIH.ub-RIH.lb)*0.5 << std::endl;
        Eigen::VectorXd Gamma;
        Gamma = (0.5 * (ub-lb)) + Z.generators.cwiseAbs().rowwise().sum();
        Eigen::MatrixXd J_abs_max[dim]; //<-----------
        compute_J_abs_max(RIH, J_abs_max, uin);  //<----------
        for(int i=0; i<dim;i++)
        {
            L_hat(i) = 0.5 * Gamma.transpose() * J_abs_max[i] * Gamma;
        }

    }
    return L_hat;
}

mstom::zonotope compute_L_Hat2(mstom::zonotope Rtotal1, Eigen::VectorXd x_bar, int state_dim, Eigen::VectorXd u){
    // 2nd L_hat computation method (less interval arithmatic)
    mstom::intervalMatrix RIH = mstom::IntervalHull(Rtotal1);
    Eigen::VectorXd L_hat(state_dim);
         std::vector<vnodelp::iMatrix> H;
        compute_H(RIH, H, u);
        int dim = H[1].size();
        std::vector<Eigen::MatrixXd> H_mid, H_rad;
        Eigen::MatrixXd Mtemp(dim,dim);
        vnodelp::pMatrix pMtemp(dim,dim);
        // vnodelp::sizeM(pMtemp,dim);
        vnodelp::pVector pV(dim);
        // vnodelp::sizeV(pV,dim);
        for(unsigned int i=0;i<H.size();i++)
        {
            vnodelp::midpoint(pMtemp, H[i]);
            H_mid.push_back(pMatrix_to_MatrixXd(pMtemp));
            for(int ii=0;ii<dim;ii++)
            {
                vnodelp::rad(pV, H[i].row(ii));
                Mtemp.block(ii,0,1,dim) = pV_to_V(pV).transpose();
            }
            H_rad.push_back(Mtemp);
        }
        mstom::zonotope Zq = compute_quad(Rtotal1-x_bar, H_mid);
        mstom::zonotope error_mid;
        error_mid.centre = 0.5 * Zq.centre;
        error_mid.generators = 0.5 * Zq.generators;
        Eigen::VectorXd dz_abs = maxAbs(mstom::IntervalHull(Rtotal1-x_bar));   
        Eigen::VectorXd error_rad(H.size());
        for(unsigned int i=0;i<H.size();i++)
            error_rad(i) = 0.5 * dz_abs.transpose() * H_rad[i] * dz_abs;
        mstom::zonotope error  = error_mid + mstom::zonotope(Eigen::VectorXd::Zero(dim), 2*error_rad);
    return error;
}

mstom::zonotope compute_Rerr_bar(int state_dim, mstom::intervalMatrix& Data_interm, mstom::zonotope& Rhomt, Eigen::VectorXd x_bar,
		Eigen::VectorXd f_bar, Eigen::VectorXd u, Eigen::VectorXd& L_hat, int LinErrorMethod, mstom::intervalMatrix& F_tilde,
		Eigen::VectorXd& L_max, int& nr, double& perfInd){
	// nr tells if split needed
	// updates: nr, perfInd, Rhomt
	mstom::zonotope F_tilde_f_bar = F_tilde * mstom::zonotope(f_bar, Eigen::VectorXd::Zero(state_dim));
	mstom::zonotope Rhom;
    Eigen::VectorXd appliedError;

//    if(L_hat_previous.rows() == 0)
       appliedError  = Eigen::MatrixXd::Constant(state_dim,1,0);
//    else
//        appliedError = L_hat_previous;

    mstom::zonotope Verror, RerrAE, Rtotal1, ErrorZonotope;
    mstom::intervalMatrix IMtemp;
    Eigen::VectorXd trueError;
    Verror.centre = Eigen::VectorXd::Zero(state_dim);
    double perfIndCurr = 2;
    while(perfIndCurr > 1)
    {
        Rhom = Rhomt;
        //if((f_bar-appliedError).maxCoeff() > 0 || (f_bar+appliedError).minCoeff() < 0)
            // Rhom = Rhom + F_tilde_f_bar;    // when f_bar + [-L,L] does not contain origin
        Verror.generators = Eigen::MatrixXd(appliedError.asDiagonal());
        RerrAE = Data_interm * Verror;
        Rtotal1 = (RerrAE + Rhom);
        if(LinErrorMethod == 1)
            trueError = compute_L_Hat1(Rtotal1, x_bar, state_dim,u);
        else
        {
        	 ErrorZonotope = compute_L_Hat2(Rtotal1, x_bar, state_dim,u);// maxAbs(IH(error zonotope))
        	 IMtemp = mstom::IntervalHull(ErrorZonotope);
        	 trueError = maxAbs(IMtemp);
        }
           // trueError = compute_L_Hat2(Rtotal1, x_bar, state_dim,u);
        perfIndCurr = (trueError.cwiseProduct(appliedError.cwiseInverse())).maxCoeff(); // max(trueError./appliedError)
        perfInd = (trueError.cwiseProduct(L_max.cwiseInverse())).maxCoeff(); // max(trueError./L_max)
        if(perfInd > 1)
        {
        	nr = -2;
        	break;
        }
        appliedError = 1.02 * trueError;
    }
    L_hat = trueError;
   // L_hat_previous = trueError;
    Verror.generators = Eigen::MatrixXd(trueError.asDiagonal());
    mstom::zonotope Rerror = Data_interm * Verror;

    Rhom = Rhomt;
        if(LinErrorMethod == 1)
        {
            //if((f_bar-trueError).maxCoeff() > 0 || (f_bar+trueError).minCoeff() < 0)
            //    Rhom = Rhom + F_tilde_f_bar;    // when f_bar + [-L,L] does not contain origin
            Rhomt = (Rerror + Rhom) + mstom::zonotope(x_bar, Eigen::VectorXd::Zero(state_dim));    // R[0,r] returned in Rhomt
        }
        else
        {
            mstom::zonotope Ztemp = ErrorZonotope + f_bar;
            mstom::zonotope F_tilde_u_tilde = F_tilde * mstom::zonotope(Ztemp.centre, Eigen::VectorXd::Zero(state_dim)); 
            Ztemp.centre = Eigen::VectorXd::Zero(state_dim);
    //        if((f_bar+IMtemp.lb).maxCoeff() > 0 || (f_bar+IMtemp.ub).minCoeff() < 0)
                Rhom = Rhom + F_tilde_u_tilde;    // when f_bar + [-L,L] does not contain origin
            Rerror = Data_interm * Ztemp;
            Rhomt = Rhom + Rerror;
            Rerror = Data_interm * ErrorZonotope;
        }
        return Rerror;
}

mstom::zonotope compute_L_hatB(int state_dim, Eigen::VectorXd& x_bar, mstom::zonotope& Z0, mstom::zonotope& exprAX0, double r, Eigen::VectorXd& fAx_bar,double Datab, double Datac, double Datad, int LinErrorMethod, Eigen::VectorXd& L_hat, Eigen::VectorXd u){
    // Guernic Girard
    Eigen::VectorXd appliedError = Eigen::MatrixXd::Constant(state_dim,1,0);
    mstom::zonotope AE; // zonotope(appliedError)
    AE.centre = Eigen::VectorXd::Zero(state_dim);
    Eigen::MatrixXd Mtemp(state_dim,2);
    mstom::zonotope B;  // unit ball in infinity norm: unit hyperrectangle
    B.centre = Eigen::VectorXd::Zero(state_dim);
    B.generators = Eigen::MatrixXd((Eigen::MatrixXd::Constant(state_dim,1,1)).asDiagonal());
    Eigen::VectorXd trueError;
    double Rv;
    mstom::zonotope V;
    double perfIndCurr = 2;
    while(perfIndCurr > 1)
    {
        AE.generators = Eigen::MatrixXd(appliedError.asDiagonal());
        V = AE + fAx_bar;
        Mtemp.col(0) = fAx_bar-appliedError;
        Mtemp.col(1) = fAx_bar+appliedError;
        Rv = Mtemp.cwiseAbs().maxCoeff();
        double alpha = Datac + Datad * Rv;
        mstom::zonotope omega0 = mstom::convexHull(Z0, (exprAX0+(V*r)+(B*alpha)));
        if(LinErrorMethod == 1)
            trueError = compute_L_Hat1(omega0, x_bar, state_dim,u);
//        else
//            trueError = compute_L_Hat2(omega0, x_bar, state_dim,u);
         perfIndCurr = (trueError.cwiseProduct(appliedError.cwiseInverse())).maxCoeff(); // max(trueError./appliedError)
        appliedError = 1.1 * trueError;
     }
    L_hat = trueError;

    AE.generators = Eigen::MatrixXd(trueError.asDiagonal());
    V = AE + fAx_bar;
    Mtemp.col(0) = fAx_bar-trueError;
    Mtemp.col(1) = fAx_bar+trueError;
    Rv = Mtemp.cwiseAbs().maxCoeff();

    double beta = Datad * Rv;
    mstom::zonotope Rtau = exprAX0 + (V*r) + (B*beta);
    return Rtau;
}

//----------------------------------------------------------------------------------------
//########################################################################################
// Reachable set



void splitz(mstom::zonotope& Z0in, mstom::zonotope& Z01, mstom::zonotope& Z02, int mIndex)
{
	// splitted sets returned in Z01 and Z02; split along dimension mIndex by first taking interval hull
	mstom::intervalMatrix Z0ih = IntervalHull(Z0in);
	mstom::zonotope Z0;	// Z0 = hyperinterval
	Z0.centre = 0.5*(Z0ih.lb + Z0ih.ub);
	Z0.generators = (0.5*(Z0ih.ub - Z0ih.lb)).asDiagonal();

    Z01.centre = Z0.centre - 0.5 * Z0.generators.col(mIndex);
    Z02.centre = Z0.centre + 0.5 * Z0.generators.col(mIndex);
    Z01.generators = Z0.generators;
    Z02.generators = Z0.generators;
    Z01.generators.col(mIndex) = 0.5 * Z0.generators.col(mIndex);
    Z02.generators.col(mIndex) = 0.5 * Z0.generators.col(mIndex);
}

void splitz2(const mstom::zonotope& Z0in, mstom::zonotope& Z01, mstom::zonotope& Z02,
				int mIndex){
	Z01.centre = Z0in.centre - 0.5 * Z0in.generators.col(mIndex);
	Z01.generators = Z0in.generators;
	Z01.generators.col(mIndex) = 0.5 * Z0in.generators.col(mIndex);
	Z02.centre = Z0in.centre + 0.5 * Z0in.generators.col(mIndex);
	Z02.generators = Z0in.generators;
	Z02.generators.col(mIndex) = 0.5 * Z0in.generators.col(mIndex);
	}

template<class state_type>
Eigen::VectorXd computeM(double tau, state_type lower_left, state_type upper_right, state_type inp_lower_left, state_type inp_upper_right){
    int state_dim = 2;
    int input_dim = 1;
    Eigen::VectorXd M(state_dim), Mb(state_dim);
    std::vector<vnodelp::interval> xx(state_dim), x(state_dim), u(input_dim), Kprime(state_dim), Kbprime(state_dim);
    for(int i=0;i<state_dim;i++)
    {
        x[i] = vnodelp::interval(lower_left[i], upper_right[i]);
    }
    for(int i=0; i<input_dim;i++)
        u[i] = vnodelp::interval(inp_lower_left[i], inp_upper_right[i]);
    xx = funcLj_system(x, u, xx);
    for(int i=0;i<state_dim;i++)
    {
        M(i) = vnodelp::mag(xx[i]);
        Kprime[i] = vnodelp::interval(lower_left[i]-M(i)*tau, upper_right[i]+M(i)*tau);
    }
    x = Kprime;
    std::cout << "M\n"<< M << std::endl;
    int checkval = 0;
    while(checkval < state_dim)
    {
        checkval = 0;   //0 if further iteration needed else >0
         xx = funcLj_system(x, u, xx);
        for(int i=0;i<state_dim;i++)
        {
            Mb(i) = vnodelp::mag(xx[i]);
            Kbprime[i] = vnodelp::interval(lower_left[i]-Mb(i)*tau, upper_right[i]+Mb(i)*tau);
             std::cout<< "bool: " << (Mb(i) <= M(i)) << std::endl;
            std::cout << "(Mb(i)-M(i)) = " << (Mb(i) - M(i))<< std::endl;
            if(Mb(i) <= M(i))
            {
                  checkval++;
            }
        }
        M = Mb; // Mb will never be less than M
        std::cout << "Mb\n"<< Mb <<"\ncheckval= "<< checkval << std::endl ;
        x=Kbprime;
    }
    Eigen::VectorXd gL_hat = compute_L_Hat3(Kbprime, u);
    return gL_hat;
}


int computeKprime(double tau, vnodelp::interval* xin,  vnodelp::interval u[], int dim, vnodelp::interval* Kbprime, int KprimeLimit, vnodelp::interval* x_initial){
    // double M[dim], Mb[dim];
    // vnodelp::interval xx[dim], Kprime[dim], *x;
    // x = xin;
    // funcLj_system(x, u, xx);
    // for(int i=0;i<dim;i++)
    // {
        // M[i] = vnodelp::mag(xx[i]);
        // Kprime[i] = xin[i] + vnodelp::interval(-M[i]*tau, M[i]*tau);
    // }
    // x = Kprime;

    // int checkval = 0;
    // int countchk = 0;
    // while(checkval < dim)
    // {
        // countchk++;
        // checkval = 0;   //0 if further iteration needed else >0
        // funcLj_system(x, u, xx);
        // for(int i=0;i<dim;i++)
        // {
            // Mb[i] = vnodelp::mag(xx[i]);
            // Kbprime[i] = xin[i] + vnodelp::interval(-Mb[i]*tau, Mb[i]*tau);

            // if(std::abs(vnodelp::sup(Kbprime[i])) > (KprimeLimit*std::abs(vnodelp::sup(x_initial[i]))))
            // {
                 // return 1;
            // }
              // if(Mb[i] - M[i] <= std::pow(10,-4))
            // {
                // checkval++;
            // }
        // }
        // // Mb will never be less than M
        // for(int i=0;i<dim;i++)
        // {
            // M[i] = Mb[i];
        // }
         // x = Kbprime;
    // }
     return 0;
}

template<class Lugbx, class Lugbu>
void computeLu(Lugbx xin, Lugbu uin, Lugbx rin, double tau_in, int dim, int dimInput, double* Lu){
    // if M keep rising, then split tau
    vnodelp::interval x[dim], u[dimInput], Kprime[dim], *xp;
    int KprimeLimit = std::pow(10,4);
    for(int i=0;i<dim;i++)
    {
        x[i] = vnodelp::interval(xin[i]-rin[i],xin[i]+rin[i]);  // m_z already included in rin
    }
    for(int i=0;i<dimInput;i++)
        u[i] = vnodelp::interval(uin[i]);
    double tau = tau_in;
    int checkval = 0;
    int chk = 1;
    while(chk == 1)
    {
        xp = x;
        checkval = 0;
        int tauDivision = tau_in/tau;
        for(int i = 0;i<tauDivision;i++)
        {
            chk = computeKprime(tau,xp,u,dim,Kprime,KprimeLimit, x);
            xp = Kprime;
            if(chk == 1)
                break;
        }
        tau = tau/2;
    }
    computeJacobian_Lu_array2(Kprime, u, dim, 0, Lu); // returns data in Lu
}

template<class Lugbx, class Lugbu, class F4>
Eigen::MatrixXd LuOverSS(Lugbx& lower_left, Lugbx& upper_right, Lugbu& uin, int dim, int dimInput){
    //    std::vector<vnodelp::interval> x(dim), u(dimInput);
    vnodelp::interval x[dim], u[dimInput];
    for(int i=0;i<dim;i++)
        x[i] = vnodelp::interval(lower_left[i], upper_right[i]);
    for(int i=0;i<dimInput;i++)
        u[i] = vnodelp::interval(uin[i]);
    Eigen::MatrixXd L(dim,dim);
    computeJacobian_Lu_array(x,u,L,dim);
    return L;
}

template<class Lugbx, class Lugbu>
Eigen::MatrixXd LuOverSS_array(Lugbx& lower_left, Lugbx& upper_right, Lugbu& uin, int& dim, int& dimInput){
    // using arrays
    vnodelp::interval x[dim], u[dimInput];
    for(int i=0;i<dim;i++)
        x[i] = vnodelp::interval(lower_left[i], upper_right[i]);
    for(int i=0;i<dimInput;i++)
        u[i] = uin[i];
    Eigen::MatrixXd L(dim,dim);
    computeJacobian_Lu_array(x,u,L, dim);
    return L;
}

template<class Lugbx, class Lugbu>
void LuOverSS_array2(Lugbx& lower_left, Lugbx& upper_right, Lugbu& uin, int& dim, int& dimInput, int jin, double* LuStore){
    // without eigen::matrix
    // using arrays
    vnodelp::interval x[dim], u[dimInput];
    for(int i=0;i<dim;i++)
        x[i] = vnodelp::interval(lower_left[i], upper_right[i]);
    for(int i=0;i<dimInput;i++)
        u[i] = uin[i];
    computeJacobian_Lu_array2(x,u, dim, jin, LuStore);
}

template<class CL>
double one_iteration(mstom::zonotope Z0, Eigen::VectorXd u, int state_dim, double r, int& p, Eigen::VectorXd L_max,
		std::vector<mstom::zonotope>& stora, std::vector<mstom::zonotope>& Zti_stora, int& count1, int LinErrorMethod, CL& L_hat_storage, const Eigen::VectorXd& ss_eta,
		int recur, int morder)
{
    TicToc timehat;
    count1++;
    Eigen::VectorXd c = Z0.centre;
    std::vector<double> cv(state_dim) ;  // to pass c as c++ vector to func
    mstom::VXdToV(cv, c);
    Eigen::VectorXd x_bar(state_dim);
    x_bar = c + 0.5 * r * funcLj_system(c, u, x_bar);

    MatrixXld A(state_dim,state_dim);

    // double A_array[state_dim*state_dim];

    // computeJacobian(A_array, x_bar, u);
    // for(int i=0;i<state_dim;i++)
        // for(int j=0;j<state_dim;j++)
            // A(i,j) = A_array[i*state_dim+j];
	
	// computing Jacobian at x_bar
	exmap mb;
	for(int i=0;i<gdim;i++)
		mb[xs[i]] = x_bar(i);
	for(int i=0;i<gdim;i++)
		for(int j=0;j<gdim;j++)
			A(i,j) = ex_to<numeric>(JacobianB[i*gdim+j].subs(mb)).to_double();	//unsafe cast; first check
	
    Eigen::VectorXd f_bar(state_dim);
    f_bar = funcLj_system(x_bar, u, f_bar);
    mstom::zonotope Rtotal_tp;
    Eigen::VectorXd L_hat;

	// std::cout << "x_bar = ";
	// for(int i=0;i<state_dim;i++)
		// std::cout << x_bar(i) << ", ";
	// std::cout << std::endl;

	//std::cout << "Jacobian, A: \n" << A.format(HeavyFmt)  << std::endl;

    mstom::intervalMatrix Er;   //E(r)
    mstom::intervalMatrix Data_interm;  //(A�-1)*(exp(Ar) - I)
    //double epsilone = mstom::compute_epsilon(A,r,p);
    int isOriginContained = 0;
    //    Ar_powers_fac; // pow(A*r,i)/factorial(i)

    //double bound = mstom::p_adjust_Er_bound(A,r,p,epsilone);
    //double A_powers_arr[state_dim*state_dim*(p+1)];
	std::vector<MatrixXld> Apower(p+1);

    mstom::matrix_exponential(A, r, p, Er, Apower);  //updates Er, Apower
    mstom::intervalMatrix F = mstom::compute_F(p, r, A, Er, Apower);
    mstom::intervalMatrix F_tilde = mstom::compute_F_tilde(p,r,A,Er,isOriginContained, Apower);
    mstom::zonotope F_tilde_f_bar = F_tilde * mstom::zonotope(f_bar, Eigen::VectorXd::Zero(state_dim));

    // Data_interm = (A�-1)*(exp(Ar) - I)
    Data_interm = compute_Data_interm(Er, r, p, A, Apower);

    mstom::zonotope Z0delta = Z0 + mstom::zonotope(-x_bar,Eigen::VectorXd::Zero(A.rows()));
    mstom::zonotope Rtrans = Data_interm * mstom::zonotope(f_bar, Eigen::VectorXd::Zero(state_dim));
    mstom::zonotope Rhom_tp = (A*r).exp() * Z0delta + Rtrans ;

    mstom::zonotope Rtotal;
	mstom::zonotope Rhom = mstom::convexHull(Z0delta, Rhom_tp) + F * Z0delta + F_tilde * mstom::zonotope(f_bar, Eigen::VectorXd::Zero(state_dim));

	mstom::reduce(Rhom, morder);
	mstom::reduce(Rhom_tp, morder);

    ZorDeltaZ = 1;  //1 if Z, 0 if deltaZ
    mstom::zonotope RV;

    int nr = -1;	// nr = -1 (empty), -2 (split needed)
	double perfInd;

    RV = compute_Rerr_bar(state_dim, Data_interm, Rhom, x_bar, f_bar, u, L_hat,LinErrorMethod, F_tilde, L_max, nr, perfInd);  // Rerr_bar; L_hat, Rhom updated in the call

    Rtotal_tp = Rhom_tp + RV  + x_bar;
    Rtotal = Rhom;

    if(nr == -1)	// no split needed
    {
		mstom::reduce(Rtotal_tp, morder);
		mstom::reduce(Rtotal, morder);
        stora.push_back(Rtotal_tp);
		Zti_stora.push_back(Rtotal);
		// mstom::plotstore(PlotStorage, Rtotal);
		// std::cout << "stora.size()=" << stora.size() << "\n";
		// std::cout << "Zti_stora.size()=" << Zti_stora.size() << "\n";
    }
	if(recur == 1)
	{
		std::cout << "L_hat = " ;
		for(int i=0;i<state_dim;i++)
			std::cout << L_hat(i) << ", ";
		std::cout << std::endl;
	}
    mstom::zonotope Z01, Z02;
    if(nr == -2 && recur == 1) // split needed
    {
		std::cout << "###########  Split  #############";

		// int number_split = state_dim;
		int number_split = Z0.generators.cols();

		std::vector<std::vector<mstom::zonotope>> split_stora1(number_split);
		std::vector<std::vector<mstom::zonotope>> split_Zti1(number_split);
		std::vector<std::vector<mstom::zonotope>> split_Zti2(number_split);
		//if(PERFINDS==2)
			std::vector<std::vector<mstom::zonotope>> split_stora2(number_split);

		std::vector<double> perfInd_split(number_split);
		std::vector<double> perftemp2(number_split);
		std::vector<mstom::zonotope> Zsplit1(number_split);	// only one set from each split
		std::vector<mstom::zonotope> Zsplit2(number_split);	// only one set from each split
		for(int i=0;i<number_split;i++)
		{
			// splitz(Z0, Zsplit1[i], Zsplit2[i], i);	// split along state_dim
			splitz2(Z0, Zsplit1[i], Zsplit2[i], i); // split along ith generator
			double perftemp;
			perftemp = one_iteration(Zsplit1[i], u, state_dim, r, p, L_max, split_stora1[i], split_Zti1[i],
									count1, LinErrorMethod, L_hat_storage, ss_eta, 0, morder);
			if(PERFINDS==2)
			{
				perftemp2[i] = one_iteration(Zsplit2[i], u, state_dim, r, p, L_max, split_stora2[i], split_Zti2[i],
											count1, LinErrorMethod, L_hat_storage, ss_eta, 0, morder);
				perftemp = perftemp * perftemp2[i];
			}
			perfInd_split[i] = perftemp;
		}
		auto r_iterat = std::min_element(perfInd_split.begin(), perfInd_split.end());
		int sel_index = r_iterat - perfInd_split.begin();

		std::cout << " psel= " << perfInd_split[sel_index]
			<<", p1= " <<perfInd_split[sel_index]/perftemp2[sel_index]<<", p2= "<<perftemp2[sel_index]<<std::endl;

        Z01 = Zsplit1[sel_index];
		Z02 = Zsplit2[sel_index];
		//plot(Z01,1,2);
		//plot(Z02,1,2);

		if(perfInd_split[sel_index]/perftemp2[sel_index] < 1 && perftemp2[sel_index] < 1)
		{
			if(stora.size()==0)
			{
				stora = split_stora1[sel_index];
				Zti_stora = split_Zti1[sel_index];
				if(PERFINDS==2)
				{
					stora.insert(stora.end(), split_stora2[sel_index].begin(), split_stora2[sel_index].end());
					Zti_stora.insert(Zti_stora.end(), split_Zti2[sel_index].begin(), split_Zti2[sel_index].end());
				}

			}
			else
			{
				stora.insert(stora.end(), split_stora1[sel_index].begin(), split_stora1[sel_index].end());
				Zti_stora.insert(Zti_stora.end(), split_Zti1[sel_index].begin(), split_Zti1[sel_index].end());
				if(PERFINDS==2)
				{
					stora.insert(stora.end(), split_stora2[sel_index].begin(), split_stora2[sel_index].end());
					Zti_stora.insert(Zti_stora.end(), split_Zti2[sel_index].begin(), split_Zti2[sel_index].end());
				}

			}
			if(PERFINDS==1)
			{
			std::vector<mstom::zonotope> split_sto2, split_Ztio2;
			one_iteration(Z02, u, state_dim, r, p, L_max, split_sto2, split_Ztio2, count1, LinErrorMethod, L_hat_storage, ss_eta, 1, morder);
			stora.insert(stora.end(), split_sto2.begin(), split_sto2.end());
			Zti_stora.insert(Zti_stora.end(), split_Ztio2.begin(), split_Ztio2.end());
			}
		}
		else
		{
			one_iteration(Z01, u, state_dim, r, p, L_max, stora, Zti_stora, count1, LinErrorMethod, L_hat_storage, ss_eta, 1, morder);
			one_iteration(Z02, u, state_dim, r, p, L_max, stora, Zti_stora, count1, LinErrorMethod, L_hat_storage, ss_eta, 1, morder);
		}
    }

    return perfInd;
}

template<class CL>
std::vector<mstom::zonotope> one_iteration_s(std::vector<mstom::zonotope> Z0, Eigen::VectorXd u,
				int state_dim, double r, int& p, Eigen::VectorXd L_max, int& count1,
				int LinErrorMethod, CL& L_hat_storage, const Eigen::VectorXd& ss_eta,
				int morder, std::vector<mstom::zonotope>& Zti)
{
	std::vector<mstom::zonotope> Zoutput, Ztemp;
	for(unsigned int i=0;i<Z0.size();i++)
	{
		int recur = 1;	// 1 (keep on iterating for reachable set), 0 (return perfInd, no further splits)
		std::vector<mstom::zonotope> stora;
		std::vector<mstom::zonotope> Zti_stora;
		one_iteration(Z0[i],u,state_dim,r,p, L_max, stora, Zti_stora, count1, LinErrorMethod,
			L_hat_storage, ss_eta, recur, morder);	// reachable sets returned in stora
		if(i==0)
		{
			Zoutput = stora;
			Zti = Zti_stora;
		}
		else
		{
			Zoutput.insert(Zoutput.end(),stora.begin(),stora.end());
			Zti.insert(Zti.end(),Zti_stora.begin(),Zti_stora.end());
		}
	}
	std::cout << "Zoutput.size = " << Zoutput.size() << std::endl;
	return Zoutput;
}

template<class CL>
void ReachableSet(int dim, int dimInput, double tau, double rr[], double x[], double uu[],
 CL& L_hat_storage, double finaltime, int LinErrorMethod, double l_bar, int morder, int taylorTerms)

{
	for(int i=0;i<gdim;i++)
		for(int j=0;j<gdim;j++)
			JacobianB[i*gdim+j] = f[i].diff(xs[j]);
	for(int i=0;i<gdim;i++)
		for(int j=0;j<gdim;j++)
			for(int k=0;k<gdim;k++)
				HessianB[i*gdim*gdim + j*gdim + k] = JacobianB[i*gdim+j].diff(xs[k]);

    int p = taylorTerms; // matrix exponential terminated after p terms
    // appliedError = 1.02 * trueError for both Rerr_bar and L_hatB
    // appliedError begins with _0_ for Rerr_bar

    int state_dim = dim;
    int input_dim = dimInput;
    double r = tau;    //sampling period = r

    Eigen::VectorXd L_max(state_dim);
    for(int i=0;i<state_dim;i++)
    	L_max(i) = l_bar;
    Eigen::VectorXd c(state_dim);  // c = centre of the cell
    Eigen::VectorXd ss_eta(state_dim); // state space eta
    Eigen::VectorXd u(input_dim);  //the selected value of input
    for(int i=0;i<state_dim;i++)
    {
        c(i) = x[i];
        ss_eta(i) = 2 * rr[i];
    }
    for(int i=0;i<input_dim;i++)
        u(i) = uu[i];

    mstom::zonotope Z0(c,ss_eta);

    std::vector<mstom::zonotope> Zn;
    Zn.push_back(Z0);
	int no_of_steps = finaltime/tau;
	no_of_steps = (no_of_steps != finaltime/tau) ? ((finaltime/tau)+1) : (finaltime/tau);
	std::vector<std::vector<mstom::zonotope>> Zti(no_of_steps);
    std::vector<std::vector<mstom::zonotope>> Ztp(no_of_steps);
   // Ztp[0] = Zn;
    std::cout << "r, finaltime, no of steps = " << r << ","
    		<< finaltime << "," << no_of_steps << std::endl;

	TicToc reachtime;
	reachtime.tic();

    for(int i=0;i<no_of_steps;i++)
    {
        int count1 = 0;
        Zn = one_iteration_s(Zn, u, state_dim, r, p, L_max, count1,
							LinErrorMethod, L_hat_storage, ss_eta, morder, Zti[i]);
			Ztp[i] = Zn;

		std::cout << "step = " << i+1 << ", time = " << (i+1)*r << std::endl;
     }
	 reachtime.toc();

    std::cout << "Ztp.size() = " << Ztp.size() << std::endl;
    //for(int i=0;i<Ztp.size();i++)
    //	plot(Ztp[i],1,2);
//    char ask2 = 'y';
//    while(ask2 == 'y')
//    {
//    	int inda;
//    	std::cout << "\n index of Ztp to plot = ";
//    	std::cin >> inda;
//    	plot(Ztp[inda],1,2);
//    	std::cout << "\n want to plot more?(y/n) = ";
//    	std::cin >> ask2;
//    }

	if(ExampleMst == 3)	// Laubloomis
	{
		wfile_time(Zti, 4, tau); //dimension count starts from 1 (not 0)
		std::cout << "File write finished" << std::endl;
		plotfilled(Zti, 4, tau); //Don't store Z0 in Ztp; dimension count starts from 1 (not 0)
	}
	else
	{
		int plotorder = 3;
		PlotStorage.clear();
		mstom::reduce(Zti[0], plotorder);
		PlotStorage = Zti[0];
		for(unsigned int i=1;i<Zti.size();i++)
		{
			Zti[i] = project(Zti[i], 1, 2);	// now Zti has 2D zonotopes
			mstom::reduce(Zti[i], plotorder);
			PlotStorage.insert(PlotStorage.end(),Zti[i].begin(), Zti[i].end());
		}
		mstom::plotstore(PlotStorage, project(Z0, 1, 2));
		// wfile(Zti);
		//wfile_gnuplot(Zti);
		std::cout << "File write finished" << std::endl;
		plotfilled(PlotStorage, 1, 2);
		// plot(PlotStorage, 1, 2);
	}

	/* char ask3 = 'y';
	while(ask3 == 'y')
	{
		int ste;
		std::cout << "\nenter the step to plot = ";
		std::cin >> ste;
		plot(Ztp[ste],1,2);
		std::cout << "\nWant to plot more(y/n) = ";
		std::cin >> ask3;
	} */

	delete[] JacobianB;
	delete[] HessianB;
    return ;
}



#endif /* REACHABLESET2_H_ */

