import subprocess
import argparse
import os

parser = argparse.ArgumentParser()
parser.add_argument("-r", "--run"         , help="Run number"      , type=str           , required =True)
parser.add_argument("-s", "--save"        , help="Save name"       , type = str         , required = True)
parser.add_argument("-d", "--digitize"    , help="Digitize the raw data"                , action="store_true")
parser.add_argument("-fHWC", "--fifoHWC"    , help="FIFO HWC MALTA3 "                   , action="store_true")
parser.add_argument("-t", "--tracking"    , help="Track matching"                       , action="store_true")
parser.add_argument("-c", "--clustering"  , help="Cluster the hits"                     , action="store_true")
parser.add_argument("-C", "--calorimetry" , help="Calorimetry analysis"                 , action="store_true")
parser.add_argument("-a", "--analysis"    , help="Analyze and generate plots"           , action="store_true")
parser.add_argument("-p", "--proteus"     , help="Create to Proteus Merged file"        , action="store_true")
parser.add_argument("-thr", "--threshold" , help="Threshold list"  , type= str          , required = False)
parser.add_argument("-i", "--input"       , help="Input configuration .cfg", type = str , required = True)
parser.add_argument("--build", help="Path to build directory", type=str, default="build")
args=parser.parse_args()

#runNumber  = args.run
runNumbers = args.run.split(",")
runNumbers = [int(i) for i in runNumbers]
saveName   = args.save
build_dir  = args.build

# Pass the path for the config input as env variable
config_name = args.input
env = os.environ.copy()
env["ANALYSIS_CONFIG"] = f"configs/{config_name}"
print(f"ANALYSIS_CONFIG={env['ANALYSIS_CONFIG']}")

if not args.proteus:
    thresholds = [float(t) for t in args.threshold.split(",")]

    for runNumber in runNumbers:
        for thr in thresholds:
            if args.digitize:
                command = [f"{build_dir}/run_DigitalProcessing", str(thr), str(runNumber), saveName, "0"]
                subprocess.run(command, check=True, env=env)

            if args.fifoHWC:
                command = [f"{build_dir}/run_PRIOFIFOHWCProcessing", str(thr), str(runNumber), saveName]
                subprocess.run(command, check=True, env=env)

            if args.tracking:
                command = [f"{build_dir}/run_Tracking", str(thr), str(runNumber), saveName]
                subprocess.run(command, check=True, env=env)

            if args.clustering:
                command = [f"{build_dir}/run_Clustering", str(thr), str(runNumber), saveName]
                subprocess.run(command, check=True, env=env)

            if args.analysis:
                command = [f"{build_dir}/run_Analysis", str(thr), str(runNumber), saveName]
                subprocess.run(command, check=True, env=env)

            if args.calorimetry:
                command = [f"{build_dir}/run_Calorimetry", str(thr), str(runNumber), saveName]
                subprocess.run(command, check=True, env=env)

savePath = f"Plots/local_{runNumber:04d}/{saveName}"
command = f"cp configs/{config_name} {savePath}"
print(f"Copying configuration file to: {savePath}")
subprocess.run(command, shell = True, check = True)
