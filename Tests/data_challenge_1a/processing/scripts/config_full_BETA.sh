#
# ASKAP Data Challenge 1A Processing Pipeline
#

##############################################################################
# Specific Configuration
##############################################################################
#
##############
#
# This file defines various constants used in the scripts.
#
# This version is for a full BETA simulation, with the original BETA
# specs of 300MHz bandwidth, and full field-of-view of 36 beams

echo Running pipeline for original BETA specs: 300MHz BW, 36 beams

##################
# THIS IS FOR FULL BANDWIDTH

# final channel, used by create-coarse-ms.sh
END_CHANNEL_CREATECOARSE=16416
# number of workers used by create-coarse-ms.sh
NUM_WORKERS_CREATECOARSE=304
SBATCH_RANGE_CREATECOARSE="0-303"

# number of worker nodes needed for gains-calibration.sh - work with 2 worker cpus per node
GAINS_CAL_MPPWIDTH=305
GAINS_CAL_MPPNPPN=4
NUM_BEAMS_GAINSCAL=36

# image size -- number of pixels and cellsize
IMAGING_NUM_PIXELS=3328
IMAGING_CELLSIZE=10arcsec
IMAGING_DIRECTION="[12h30m00.00, -45.00.00.00, J2000]"
IMAGING_WTOL=800
IMAGING_WMAX=800
IMAGING_MAXSUP=512
IMAGING_GAUSSTAPER="[30arcsec, 30arcsec, 0deg]"
IMAGING_EQUALISE=True

# number of worker nodes needed for imager-cont-clean.sh - work with 2 worker cpus per node (but with nworkergroups=3)
CONT_CLEAN_MPPWIDTH=913
CONT_CLEAN_MPPNPPN=16
CONT_CLEAN_FREQ=1.270e9

# number of worker nodes needed for imager-cont-dirty.sh - work with 6 worker cpus per node, plus extra on the master's node
CONT_DIRTY_MPPWIDTH=305
CONT_DIRTY_MPPNPPN=16
CONT_DIRTY_FREQ=1.270e9

# base frequency for continuum cubes
CONT_CUBE_FREQ_ZERO_CHAN=1.421e9
# number of workers used for continuum cubes, and qsub range
NUM_WORKERS_CONT_CUBE=304
SBATCH_RANGE_CONT_CUBE="0-303"
# final channel used for the make-spectral-cube call for continuum cubes
CONT_CUBE_FINALCH=303

# base frequency for spectral-line cubes
SPECTRAL_CUBE_FREQ_ZERO_CHAN=1.421e9
# number of workers used for spectral-line cubes, and qsub range
NUM_WORKERS_SPECTRAL_CUBE=16416
SBATCH_RANGE_SPECTRAL_CUBE_1="0-8207"
SBATCH_RANGE_SPECTRAL_CUBE_2="8208-16415"
SBATCH_RANGE_SPECTRAL_CUBE_FULL="0-16415"
# final channel used for the make-spectral-cube call for continuum cubes
SPECTRAL_CUBE_FINALCH=16415

# reference frequency for sky model
SKYMODEL_REFFREQ="1.270GHz"
SKYMODEL_FREQ="1.270GHz"
SKYMODEL_ALT_FREQ="1.270e9"

# subsection of image used for analysis
ANALYSIS_SUBSECTION_BLC=600
ANALYSIS_SUBSECTION_TRC=2699
ANALYSIS_SUBSECTION=`echo ${ANALYSIS_SUBSECTION_BLC} ${ANALYSIS_SUBSECTION_TRC} | awk '{printf "[%d:%d,%d:%d,*,*]",\$1+1,\$2+1,\$1+1,\$2+1}'`
THRESHIMAGE=detectionThreshold.i.clean
NOISEIMAGE=noiseMap.i.clean
SNRIMAGE=snr.i.clean
AVERAGEIMAGE=meanMap.i.clean
