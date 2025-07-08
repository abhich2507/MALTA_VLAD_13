#!/bin/bash

source ../config.sh
NAF_HOST="naf-atlas.desy.de"
NAF_DIR="/afs/desy.de/user/"${NAF_USER:0:1}"/$NAF_USER/$NAF_PATH"
SOCKET="$HOME/.ssh/naf-socket"
RUN_MODE=$1




if [[ "$RUN_MODE" == "local" ]]; then
    echo "[LOCAL] Running locally..."
    #TODO: consider also handling directory janitorial services.
    #LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu ./sim 
    source run_script_local.sh

elif [[ "$RUN_MODE" == "naf" ]]; then

    if ! ssh -S "$SOCKET" -O check "$NAF_USER@${NAF_HOST}" 2>/dev/null; then
        echo "Starting SSH master session (you will need to enter password and OTP)..."
        ssh -M -S "$SOCKET" -fnNT "$NAF_USER@${NAF_HOST}" 
    fi

    echo "[NAF] Mode selected"


    #First things first we do a dry run to make sure the config is not bad
    source run_script_local_test.sh  
    EXIT_CODE=$?
    #TODO: ensure jobfile is correct
    # Check the exit code of the above script.
    if [[ $EXIT_CODE -ne 0 ]]; then
        echo "🛑 Preflight check failed. Aborting submission."
        return 1

    elif ! grep -q -E "Simulation completed. Good day! *" sim_preflight.log; then
        echo "🛑 GEANT4 did not compile sucesfully. Aborting submission."
        return 1

    fi
    echo "✅ Preflight check passed."

    # Check if there is another job already running
    CUR_JOB=$(ssh -S "$SOCKET" ${NAF_USER}@${NAF_HOST} "condor_q $NAF_USER")
    echo "$CUR_JOB" | grep -q "$NAF_USER" > /dev/null 2>&1

    if [[ $? -eq 0 ]]; then
        echo "🛑 There is already a Condor job running. Exiting... "
        return 1
    fi
    echo "✅ No condor jobs currently running."  

    
    # SYNC the flag.cfg between local and naf.
    rsync -avz --inplace -e "ssh -S $SOCKET" $LOCAL_PATH/flags.cfg $NAF_USER@$NAF_HOST:$NAF_DIR/flags.cfg > /dev/null 2>&1
    if [[ $? -ne 0 ]]; then
        echo "🛑 Could not SYNC the configuration between local and naf."
        return 1
    fi
    echo "✅ Configuration SYNCED between local and naf."
    # SSH and submit Condor job
    
    ssh -S "$SOCKET" ${NAF_USER}@${NAF_HOST} "
        cd $NAF_DIR
        cd build
        cmake .. > /dev/null 2>&1
        make > /dev/null 2>&1
        source ../config.sh
        if [[ \$? -ne 0 ]]; then
            echo "🛑 Source compilation failed."
            exit 1
        fi
            echo "✅ Source compilation success."        
        "
        
    if [[ $? -ne 0 ]]; then
        return 1
    fi

    JOB_ID=$(ssh -S "$SOCKET" ${NAF_USER}@${NAF_HOST} "
            cd $NAF_DIR
            sed -i \"s|__BASE_PATH__|$NAF_DIR|g\" build/job.submit 
            condor_submit build/job.submit | awk '/submitted to cluster/ {print \$6}' | tr -d '.'
        ")



    echo "[NAF] Submitting job to NAF... with JOB_ID: $JOB_ID"
    #persistent daemon run that transfers the job files to the current run directory
    nohup ssh -S "$SOCKET" ${NAF_USER}@${NAF_HOST} "
        NAF_DIR=\"$NAF_DIR\"
        start_time=\$(date +%s)
        start_time_human=\$(date -d \"@\${start_time}\" \"+%Y-%m-%d %H:%M:%S\")
        job_started=0
        echo \"📤 Job submitted at: \$start_time_human\"
        while [ ! -f \"\$NAF_DIR/condor_log/job.out\" ]; do
            now=\$(date +%s)
            now_human=\$(date -d \"@\${now}\" \"+%Y-%m-%d %H:%M:%S\")
            elapsed=\$((now - start_time))
            echo -ne \"⏳ Elapsed: \${elapsed}s\r\"
            sleep 5
        done
        echo
        echo \"✅ Job completed at: \$now_human\"
        echo \"⏱️ Job ran for: \${elapsed}s\"
        latest_dir=\$(ls -d \$NAF_DIR/Results/naf_* | sort -V | tail -n 1)
        mv \$NAF_DIR/condor_log/job.* \"\$latest_dir/\"
    " > monitor.log 2>&1 &

    echo "ℹ️  Monitoring started in the background. Check progress with: tail -f monitor.log"

else
    echo "🛑 Invalid run_mode: $RUN_MODE"
    return 1
fi