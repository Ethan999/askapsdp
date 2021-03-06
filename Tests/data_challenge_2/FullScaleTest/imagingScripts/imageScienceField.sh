#!/bin/bash -l


POINTING=0
while [ $POINTING -lt 9 ]; do
    . ${imScripts}/imageScienceFieldBeam.sh
    imdepend="${imdepend}:${latestID}"
    POINTING=`expr $POINTING + 1`
done

range="BEAM0..8"

if [ $doMFS == true ]; then
    nterms="linmos.nterms     = 2"
else
    nterms="# no nterms parameter since not MFS"
fi

if [ ${IMAGING_GRIDDER} == "AWProject" ]; then
    weightingPars="# Use the weight images directly:
linmos.weighttype = FromWeightImages
"
else
    if [ ${model} == "SKADS" ]; then
	weightingPars="# Use primary beam models at specific positions:
linmos.weighttype    = FromPrimaryBeamModel
linmos.weightstate   = Inherent
linmos.feeds.centre  = [12h30m00.00, -45.00.00.00]
linmos.feeds.spacing = 1deg
linmos.feeds.BEAM0   = [-1.0, -1.0]
linmos.feeds.BEAM1   = [-1.0,  0.0]
linmos.feeds.BEAM2   = [-1.0,  1.0]
linmos.feeds.BEAM3   = [ 0.0, -1.0]
linmos.feeds.BEAM4   = [ 0.0,  0.0]
linmos.feeds.BEAM5   = [ 0.0,  1.0]
linmos.feeds.BEAM6   = [ 1.0, -1.0]
linmos.feeds.BEAM7   = [ 1.0,  0.0]
linmos.feeds.BEAM8   = [ 1.0,  1.0]
"
    else
	weightingPars="# Use primary beam models at specific positions:
linmos.weighttype    = FromPrimaryBeamModel
linmos.weightstate   = Inherent
linmos.feeds.centre  = [12h30m00.00, -45.00.00.00]
linmos.feeds.spacing = 1deg
linmos.feeds.BEAM0   = [0,0]
linmos.feeds.BEAM1   = [-0.572425, 0.947258]
linmos.feeds.BEAM2   = [-1.14485, 1.89452]
linmos.feeds.BEAM3   = [0.572425, -0.947258]
linmos.feeds.BEAM4   = [-1.23347, -0.0987957]
linmos.feeds.BEAM5   = [-1.8059, 0.848462]
linmos.feeds.BEAM6   = [0.661046, 1.04605]
linmos.feeds.BEAM7   = [0.0886209, 1.99331]
linmos.feeds.BEAM8   = [1.23347, 0.0987957]
"
    fi
fi

linmossbatch=linmosFull.sbatch
cat > $linmossbatch <<EOF
#!/bin/bash -l
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name linmos
#SBATCH --export=ASKAP_ROOT,AIPSPATH

linmos=\${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/linmos.sh

parset=parsets/linmos_${type}_\${SLURM_JOB_ID}.in
cat > \${parset} <<EOF_INNER
linmos.names       = [${range}]
linmos.findmosaics = true
linmos.psfref      = 4
${weightingPars}
${nterms}
EOF_INNER
log=logs/linmos_${type}_\${SLURM_JOB_ID}.log

aprun \${linmos} -c \${parset} > \${log}


EOF

if [ $doSubmit == true ]; then 
    ID=`sbatch ${imdepend} ${linmossbatch} | awk '{print $4}'`
    echo "Have submitted a linmos job with ID=${ID}, via 'sbatch ${imdepend} ${linmossbatch}'"
fi
