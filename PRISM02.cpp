//
// Created by AJ Pryor on 2/24/17.
//

#include "PRISM02.h"
#include <iostream>
using namespace std;
namespace PRISM {
	template <class T>
	using Array3D = PRISM::ArrayND<3, std::vector<T> >;
	template <class T>
	using Array2D = PRISM::ArrayND<2, std::vector<T> >;
	template <class T>
	using Array1D = PRISM::ArrayND<1, std::vector<T> >;
	template <class T>
	Array1D<T> makeFourierCoords(const size_t& N, const T& pixel_size){
		Array1D<T> result = zeros_ND<1, T>({N});
		long long nc = (size_t)floor( (double)N/2 );

		T dp = 1/(N*pixel_size);
		for (auto i = 0; i < N; ++i){
			result[(nc + (size_t)i) % N] = (i-nc) * dp;
		}
		return result;
};

	template <class T>
	void PRISM02(emdSTEM<T>& pars){


		constexpr double m = 9.109383e-31;
		constexpr double e = 1.602177e-19;
		constexpr double c = 299792458;
		constexpr double h = 6.62607e-34;
        const double pi = acos(-1);
        const std::complex<double> i(0, 1);
		pars.imageSize[0] = pars.pot.get_nlayers();
		pars.imageSize[1] = pars.pot.get_ncols();
		Array1D<T> qx = makeFourierCoords(pars.imageSize[0], pars.pixelSize[0]);
		Array1D<T> qy = makeFourierCoords(pars.imageSize[1], pars.pixelSize[1]);

		pair< Array2D<T>, Array2D<T> > mesh = meshgrid(qx,qy);
		pars.qxa = mesh.first;
		pars.qya = mesh.second;
		Array2D<T> q2(pars.qya);
		transform(pars.qxa.begin(), pars.qxa.end(),
				  pars.qya.begin(), q2.begin(), [](const T& a, const T& b){
				return a*a + b*b;
				});

		// get qMax
		pars.qMax = 0;
		{
			T qx_max;
			T qy_max;
			for (auto i = 0; i < qx.size(); ++i) {
				qx_max = ( abs(qx[i]) > qx_max) ? abs(qx[i]) : qx_max;
				qy_max = ( abs(qy[i]) > qy_max) ? abs(qy[i]) : qy_max;
			}
			pars.qMax = min(qx_max, qy_max) / 2;
		}

		pars.qMask = zeros_ND<2, unsigned int>({pars.imageSize[1], pars.imageSize[0]});
		{
			long offset_x = pars.qMask.get_ncols()/4;		cout << "offset_x = " << offset_x << endl;
			long offset_y = pars.qMask.get_nrows()/4;
			cout << "(y-offset_y) % pars.qMask.get_nrows() = " << (0-offset_y) % pars.qMask.get_nrows() << endl;
			cout << "pars.qMask.get_nrows() = " << pars.qMask.get_nrows() << endl;
			cout << "-offset_y % pars.qMask.get_nrows() = " << (-offset_y) % (int)pars.qMask.get_nrows() << endl;
			cout << "-250 % 1000 = " << -250%1000 << endl;
			// Fix this
			long ndimy = (long)pars.qMask.get_nrows();
			long ndimx = (long)pars.qMask.get_nrows();

			for (long y = 0; y < pars.qMask.get_nrows() / 2; ++y) {
				for (long x = 0; x < pars.qMask.get_ncols() / 2; ++x) {
					pars.qMask.at( ((y-offset_y) % ndimy + ndimy) % ndimy,
								   ((x-offset_x) % ndimx + ndimx) % ndimx) = 1;
				}
			}
		}

        // build propagators
        pars.prop     = zeros_ND<2, std::complex<T> >({pars.imageSize[1], pars.imageSize[0]});
        pars.propBack = zeros_ND<2, std::complex<T> >({pars.imageSize[1], pars.imageSize[0]});
        cout << "exp(-i * pi) << endl =" << exp(-i * pi) << endl;
        for (auto y = 0; y < pars.qMask.get_nrows(); ++y) {
            for (auto x = 0; x < pars.qMask.get_ncols(); ++x) {
                if (pars.qMask.at(y,x)==1)
                {
                    pars.prop.at(y,x)     = exp(-i * pi * complex<T>(pars.lambda, 0) *
                                                          complex<T>(pars.sliceThickness, 0) *
                                                          complex<T>(q2.at(y, x), 0));
                    pars.propBack.at(y,x) = exp(i * pi * complex<T>(pars.lambda, 0) *
                                                         complex<T>(pars.cellDim[2], 0) *
                                                         complex<T>(q2.at(y, x), 0));
                }
            }
        }
        cout << "pars.propBack.at(3,4) = " << pars.propBack.at(3,4) << endl;
        cout << "pars.prop.at(33,44) = " << pars.prop.at(33,44) << endl;


        Array1D<T> xv = makeFourierCoords(pars.imageSize[1], (double)1/pars.imageSize[1]);
        Array1D<T> yv = makeFourierCoords(pars.imageSize[0], (double)1/pars.imageSize[0]);
        pair< Array2D<T>, Array2D<T> > mesh_a = meshgrid(xv, yv);

        // create beam mask and count beams
        PRISM::ArrayND<2, std::vector<unsigned int> > mask;
        mask = zeros_ND<2, unsigned int>({pars.imageSize[1], pars.imageSize[0]});
        pars.numberBeams = 0;
        long interp_f = (long)pars.interpolationFactor;
        for (auto y = 0; y < pars.qMask.get_nrows(); ++y) {
            for (auto x = 0; x < pars.qMask.get_ncols(); ++x) {
                if (q2.at(y,x) < pow(pars.alphaBeamMax / pars.lambda,2) &&
                    pars.qMask.at(y,x)==1 &&
                    (size_t)round(mesh_a.first.at(y,x))  % pars.interpolationFactor == 0 &&
                    (size_t)round(mesh_a.second.at(y,x)) % pars.interpolationFactor == 0){
                    mask.at(y,x)=1;
                    ++pars.numberBeams;
                }
            }
        }


        cout << " pow(pars.alphaBeamMax / pars.lambda,2) = " <<  pow(pars.alphaBeamMax / pars.lambda,2) << endl;
        pars.beams = zeros_ND<2, T>({{pars.imageSize[0], pars.imageSize[1]}});
        {
            int beam_count = 0;
            for (auto y = 0; y < pars.qMask.get_nrows(); ++y) {
                for (auto x = 0; x < pars.qMask.get_ncols(); ++x) {
                    if (mask.at(y,x)==1){
                        pars.beams.at(y,x) = beam_count++;
                    }
                }
            }
            cout << "beam_count = " << beam_count << endl;
            cout << "pars.numberBeams = " << pars.numberBeams << endl;
        }
        q2.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/q2.mrc");
        mesh_a.first.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/xa.mrc");
        mesh_a.second.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/ya.mrc");

        mask.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/mask.mrc");
        pars.beams.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/beams.mrc");



        //pars.qMask.toMRC_f("/mnt/spareA/clion/PRISM/MATLABdebug.mrc");
		pars.qMask.toMRC_f("/Users/ajpryor/Documents/MATLAB/multislice/PRISM/debug.mrc");
		cout << "pars.qMax = " << pars.qMax << endl;
		cout << "pars.qya.at(1,1) = " << pars.qya.at(1,1) << endl;
		cout << "pars.qya.at(0,1) = " << pars.qya.at(0,1) << endl;
		cout << "q2.at(3,4) = " << q2.at(3,4) << endl;
		cout << "q2.at(5,5) = " << q2.at(5,5) << endl;

		cout << "pars.pixelSize[0] = " << pars.pixelSize[0]<< endl;
		cout << "pars.pixelSize[1] = " << pars.pixelSize[1]<< endl;
		for (auto i = 0; i < 10; ++i){
			cout << "qx[" << i << "] = " << qx[i] << endl;
			cout << "qy[" << i << "] = " << qy[i] << endl;
		}
		cout << "qx[499] = " << qx[499] << endl;
		cout << "qy[499] = " << qy[499] << endl;
		cout << "qx[500] = " << qx[500] << endl;
		cout << "qy[500] = " << qy[500] << endl;

	}
}