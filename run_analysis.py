import subprocess
import argparse
import os
################################### WARNING
### Currently the threshold 2000 is used just for the Proteus conversion

parser = argparse.ArgumentParser()
parser.add_argument("-r", "--run"         , help="Run number"      , type=int           , required =True)
parser.add_argument("-s", "--save"        , help="Save name"       , type = str         , required = True)
parser.add_argument("-d", "--digitize"    , help="Digitize the raw data"                , action="store_true")
parser.add_argument("-dmp", "--digitizemultiplane"    , help="Digitize the raw data of multiple planes", action="store_true")
parser.add_argument("-t", "--tracking"    , help="Track matching"                       , action="store_true")
parser.add_argument("-c", "--clustering"  , help="Cluster the hits"                     , action="store_true")
parser.add_argument("-C", "--calorimetry" , help="Calorimetry analysis"                 , action="store_true")
parser.add_argument("-f", "--fifo"        , help="FIFO Processing"                      , action="store_true")
parser.add_argument("-a", "--analysis"    , help="Analyze and generate plots"           , action="store_true")
parser.add_argument("-p", "--proteus"     , help="Create to Proteus Merged file"        , action="store_true")
parser.add_argument("-thr", "--threshold" , help="Threshold list"  , type= str          , required = False)
parser.add_argument("-i", "--input"       , help="Input configuration .cfg", type = str , required = True)
args=parser.parse_args()

runNumber  = args.run
saveName   = args.save

# Pass the path for the config input as env variable
config_name = args.input
os.environ["ANALYSIS_CONFIG"] = f"configs/{config_name}"
command = f"echo $ANALYSIS_CONFIG"
subprocess.run(command, shell = True)

if not args.proteus:
    thresholds = args.threshold.split(",")
    thresholds = [float(i) for i in thresholds]

    for thr in thresholds:
        if args.digitize:
            command = f"root -l -b -q 'analysis/DigitalProcessing.cc({thr},{runNumber},\"{saveName}\", false)'"
            subprocess.run(command, shell=True, check=True)
        
        if args.digitizemultiplane:
            command = f"root -l -b -q 'analysis/DigitalProcessing_multiPlane.cc({thr},{runNumber},\"{saveName}\", false)'"
            subprocess.run(command, shell=True, check=True)

        if args.tracking:
            command = f"root -l -b -q 'analysis/Tracking.cc({thr},{runNumber},\"{saveName}\")'"
            subprocess.run(command, shell=True, check=True)

        if args.clustering:
            command = f"root -l -b -q 'analysis/Clustering.cc({thr},{runNumber},\"{saveName}\")'"
            subprocess.run(command, shell=True, check=True)

        if args.analysis:
            command = f"root -l -b -q 'analysis/Analysis.cc({thr},{runNumber},\"{saveName}\")'"
            subprocess.run(command, shell=True, check=True)


        if args.fifo:
            command = f"root -l -b -q 'analysis/FIFOProcessing.cc({thr},{runNumber},\"{saveName}\")'"
            subprocess.run(command, shell=True, check=True)


        if args.calorimetry:
            command = f"root -l -b -q 'analysis/Calorimetry.cc({thr},{runNumber},\"{saveName}\")'"
            subprocess.run(command, shell=True, check=True)
else:
    command = f"root -l -b -q 'analysis/DigitalProcessing.cc(2000,{runNumber},\"{saveName}\", true)'"
    subprocess.run(command, shell=True, check=True)

    command = f"root -l -b -q 'analysis/SimulationToProteus.cc({runNumber})'"
    subprocess.run(command, shell=True, check=True)



savePath = f"Plots/local_{runNumber:04d}/{saveName}"
command = f"cp configs/{config_name} {savePath}"
print(f"Copying configuration file to: {savePath}")
subprocess.run(command, shell = True, check = True)
