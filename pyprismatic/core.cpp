// Copyright Alan (AJ) Pryor, Jr. 2017
// Transcribed from MATLAB code by Colin Ophus
// Prismatic is distributed under the GNU General Public License (GPL)
// If you use Prismatic, we kindly ask that you cite the following papers:

// 1. Ophus, C.: A fast image simulation algorithm for scanning
//    transmission electron microscopy. Advanced Structural and
//    Chemical Imaging 3(1), 13 (2017)

// 2. Pryor, Jr., A., Ophus, C., and Miao, J.: A Streaming Multi-GPU
//    Implementation of Image Simulation Algorithms for Scanning
//	  Transmission Electron Microscopy. arXiv:1706.08563 (2017)

#include <Python.h>
#include "params.h"
#include "configure.h"
#include "parseInput.h"
#include "go.h"
#include <iostream>
#include <stdio.h>
#ifdef PRISMATIC_ENABLE_GPU
#include "cuprismatic.h"
#endif //PRISMATIC_ENABLE_GPU

static PyObject *pyprismatic_core_go(PyObject *self, PyObject *args)
{
	Prismatic::Metadata<PRISMATIC_FLOAT_PRECISION> meta;
	int interpolationFactorY = 1;
	int interpolationFactorX = 1;
	int randomSeed;
	int numFP, batchSizeTargetCPU, batchSizeTargetGPU,
		tileX, tileY, tileZ,
		numGPUs, numStreamsPerGPU, numThreads, includeThermalEffects, alsoDoCPUWork, save2DOutput,
		save3DOutput, save4DOutput, saveDPC_CoM, savePotentialSlices, nyquistSampling, numSlices;
	char *filenameAtoms, *filenameOutput, *algorithm, *transferMode;
	double realspacePixelSizeX, realspacePixelSizeY, potBound,
		sliceThickness, probeStepX, probeStepY,
		cellDimX, cellDimY, cellDimZ, earlyCPUStopCount, E0, alphaBeamMax,
		detectorAngleStep, probeDefocus, C3,
		C5, probeSemiangle, probeXtilt,
		probeYtilt, scanWindowXMin, scanWindowXMax,
		scanWindowYMin, scanWindowYMax,
		integrationAngleMin, integrationAngleMax, zStart, scanWindowXMin_r, scanWindowXMax_r,
		scanWindowYMin_r, scanWindowYMax_r;
#ifdef PRISMATIC_ENABLE_GPU
	std::cout << "COMPILED FOR GPU" << std::endl;
#endif //PRISMATIC_ENABLE_GPU

	if (!PyArg_ParseTuple(
			args, "iissdddiddddiiiddiiiiiddddddddddddddispppppddsiiiiddddd",
			&interpolationFactorX,
			&interpolationFactorY,
			&filenameAtoms,
			&filenameOutput,
			&realspacePixelSizeX,
			&realspacePixelSizeY,
			&potBound,
			&numFP,
			&sliceThickness,
			&cellDimX,
			&cellDimY,
			&cellDimZ,
			&tileX,
			&tileY,
			&tileZ,
			&E0,
			&alphaBeamMax,
			&numGPUs,
			&numStreamsPerGPU,
			&numThreads,
			&batchSizeTargetCPU,
			&batchSizeTargetGPU,
			&earlyCPUStopCount,
			&probeStepX,
			&probeStepY,
			&probeDefocus,
			&C3,
			&C5,
			&probeSemiangle,
			&detectorAngleStep,
			&probeXtilt,
			&probeYtilt,
			&scanWindowXMin,
			&scanWindowXMax,
			&scanWindowYMin,
			&scanWindowYMax,
			&randomSeed,
			&algorithm,
			&includeThermalEffects,
			&alsoDoCPUWork,
			&save2DOutput,
			&save3DOutput,
			&save4DOutput,
			&integrationAngleMin,
			&integrationAngleMax,
			&transferMode,
			&saveDPC_CoM,
			&savePotentialSlices,
			&nyquistSampling,
			&numSlices,
			&zStart,
			&scanWindowXMin_r,
			&scanWindowXMax_r,
			&scanWindowYMin_r,
			&scanWindowYMax_r))
	{
		return NULL;
	}
	meta.interpolationFactorX = interpolationFactorX;
	meta.interpolationFactorY = interpolationFactorY;
	meta.filenameAtoms = filenameAtoms;
	meta.filenameOutput = filenameOutput;
	meta.realspacePixelSize[0] = realspacePixelSizeY;
	meta.realspacePixelSize[1] = realspacePixelSizeX;
	meta.potBound = potBound;
	meta.numFP = numFP;
	meta.sliceThickness = sliceThickness;
	meta.numSlices = numSlices;
	meta.zStart = zStart;
	meta.cellDim[2] = cellDimX;
	meta.cellDim[1] = cellDimY;
	meta.cellDim[0] = cellDimZ;
	meta.tileX = tileX;
	meta.tileY = tileY;
	meta.tileZ = tileZ;
	meta.E0 = E0 * 1000;
	meta.alphaBeamMax = alphaBeamMax / 1000;
	meta.numGPUs = numGPUs;
	meta.numStreamsPerGPU = numStreamsPerGPU;
	meta.numThreads = numThreads;
	meta.batchSizeTargetCPU = batchSizeTargetCPU;
	meta.batchSizeTargetGPU = batchSizeTargetGPU;
	meta.earlyCPUStopCount = earlyCPUStopCount;
	meta.probeStepX = probeStepX;
	meta.probeStepY = probeStepY;
	meta.probeDefocus = probeDefocus;
	meta.C3 = C3;
	meta.C5 = C5;
	meta.probeSemiangle = probeSemiangle /1000;
	meta.detectorAngleStep = detectorAngleStep /1000;
	meta.probeXtilt = probeXtilt /1000;
	meta.probeYtilt = probeYtilt /1000;
	meta.scanWindowXMin = scanWindowXMin;
	meta.scanWindowXMax = scanWindowXMax;
	meta.scanWindowYMin = scanWindowYMin;
	meta.scanWindowYMax = scanWindowYMax;
	meta.scanWindowXMin_r = scanWindowXMin_r;
	meta.scanWindowXMax_r = scanWindowXMax_r;
	meta.scanWindowYMin_r = scanWindowYMin_r;
	meta.scanWindowYMax_r = scanWindowYMax_r;
	meta.randomSeed = randomSeed;
	if (std::string(algorithm) == "multislice")
	{
		meta.algorithm = Prismatic::Algorithm::Multislice;
	}
	else
	{
		meta.algorithm = Prismatic::Algorithm::PRISM;
	}
	meta.includeThermalEffects = includeThermalEffects;
	meta.alsoDoCPUWork = alsoDoCPUWork;
	meta.save2DOutput = save2DOutput;
	meta.save3DOutput = save3DOutput;
	meta.save4DOutput = save4DOutput;
	meta.savePotentialSlices = savePotentialSlices;
	meta.saveDPC_CoM = saveDPC_CoM;
	meta.integrationAngleMin = integrationAngleMin / 1000;
	meta.integrationAngleMax = integrationAngleMax / 1000;
	meta.nyquistSampling = nyquistSampling;

	if (std::string(transferMode) == "singlexfer")
	{
		meta.transferMode = Prismatic::StreamingMode::SingleXfer;
	}
	else if (std::string(transferMode) == "streaming")
	{
		meta.transferMode = Prismatic::StreamingMode::Stream;
	}
	else
	{
		meta.transferMode = Prismatic::StreamingMode::Auto;
	}

	// print metadata
	//meta.toString();
	int scratch = Prismatic::writeParamFile(meta,"scratch_param.txt");

	Prismatic::Metadata<PRISMATIC_FLOAT_PRECISION> tmp_meta;
	if(Prismatic::parseParamFile(tmp_meta,"scratch_param.txt")) 
	{
		Prismatic::go(meta);
	}else{
		std::cout << "Invalid parameters detected. Cancelling calculation, please check inputs." << std::endl;
	}
	// configure simulation behavior
	//	Prismatic::configure(meta);
	//std::remove("scratch_param.txt");
	// execute simulation
	//	Prismatic::execute_plan(meta);

	Py_RETURN_NONE;
}

static PyMethodDef pyprismatic_core_methods[] = {
	{"go", (PyCFunction)pyprismatic_core_go, METH_VARARGS, "Execute Prismatic calculation"},
	{NULL, NULL, 0, NULL}};

static struct PyModuleDef module_def = {
	PyModuleDef_HEAD_INIT, "pypristmatic.core", "Python wrapper for Prismatic\
	 package for fast image simulation using the PRISM and multislice\
	 algorithms in Scanning Transmission Electron Microscopy (STEM)",
	-1, pyprismatic_core_methods};

PyMODINIT_FUNC PyInit_core()
{
	return PyModule_Create(&module_def);
}
