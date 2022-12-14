instanceName="TestInstance"
startDate="2022-03-02"
endDate="2022-03-08"
strategyName="ResistanceStrategy"
sharedObjectFileName="Resistance.so"
SYMBOLS="DIA|AAPL|TSLA"

strategyFolderPath="/vagrant/ie421/strategy"
sharedObjectOriginPath="$strategyFolderPath/$sharedObjectFileName"
sharedObjectDestinationPath="/home/vagrant/ss/bt/strategies_dlls"

rm /home/vagrant/ss/bt/backtesting-results/*
rm /home/vagrant/ss/bt/backtesting-cra-exports/*

# Run BackTesting Server in the background
(cd /home/vagrant/ss/bt && ./StrategyServerBacktesting&)
sleep 2

# Add test instance
cd /home/vagrant/ss/bt/utilities
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd terminate -all
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd create_instance $instanceName $strategyName UIUC SIM-1001-101 dlariviere 1000000 -symbols "$SYMBOLS"
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd strategy_instance_list

# Get the current number of lines in the main_log.txt file
logFileNumLines=$(wc -l < /home/vagrant/ss/bt/logs/main_log.txt)

# DEBUGGING OUTPUT
echo "Number of lines Currently in Log file:",$logFileNumLines

# Start the backtest
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd start_backtest $startDate $endDate $instanceName 1

# Get the line number which ends with finished.
foundFinishedLogFile=$(grep -nr "finished.$" /home/vagrant/ss/bt/logs/main_log.txt | gawk '{print $1}' FS=":"|tail -1)

# DEBUGGING OUTPUT
echo "Last line found:",$foundFinishedLogFile

# If the line ending with finished. is less than the previous length of the log file, then strategyBacktesting has not finished,
# once its greater than the previous, it means it has finished.
while ((logFileNumLines > foundFinishedLogFile))
do
    foundFinishedLogFile=$(grep -nr "finished.$" /home/vagrant/ss/bt/logs/main_log.txt | gawk '{print $1}' FS=":"|tail -1)
done

# Get the latest file in terms of modified time
latestCRA=$(ls -t /home/vagrant/ss/bt/backtesting-results | head -1)

# Run the export command to generate csv files
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd export_cra_file backtesting-results/$latestCRA backtesting-cra-exports

# Quit strategy studio working in the background
/home/vagrant/ss/bt/utilities/./StrategyCommandLine cmd quit


